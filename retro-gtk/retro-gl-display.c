// This file is part of retro-gtk. License: GPL-3.0+.

/**
 * PRIVATE:retro-gl-display
 * @short_description: A widget rendering the video output from a core via OpenGL
 * @title: RetroGLDisplay
 * @stability: Private
 */

#include "retro-gl-display-private.h"

#include <epoxy/gl.h>
#include "retro-glsl-filter-private.h"
#include "retro-pixbuf.h"
#include "retro-pixdata.h"

#define RETRO_VIDEO_FILTER_COUNT (RETRO_VIDEO_FILTER_CRT + 1)

struct _RetroGLDisplay
{
  GtkGLArea parent_instance;
  RetroCore *core;
  RetroPixdata *pixdata;
  GdkPixbuf *pixbuf;
  RetroVideoFilter filter;
  gfloat aspect_ratio;
  gulong video_output_cb_id;

  RetroGLSLFilter *glsl_filter[RETRO_VIDEO_FILTER_COUNT];
  GLuint texture;
};

G_DEFINE_TYPE (RetroGLDisplay, retro_gl_display, GTK_TYPE_GL_AREA)

typedef struct {
  struct {
    float x, y;
  } position;
  struct {
    float x, y;
  } texture_coordinates;
} RetroVertex;

static float vertices[] = {
  -1.0f,  1.0f, 0.0f, 0.0f, // Top-left
   1.0f,  1.0f, 1.0f, 0.0f, // Top-right
   1.0f, -1.0f, 1.0f, 1.0f, // Bottom-right
  -1.0f, -1.0f, 0.0f, 1.0f, // Bottom-left
};

static GLuint elements[] = {
    0, 1, 2,
    2, 3, 0,
};

static const gchar *filter_uris[] = {
  "resource:///org/gnome/Retro/glsl-filters/bicubic.filter",
  "resource:///org/gnome/Retro/glsl-filters/sharp.filter",
  "resource:///org/gnome/Retro/glsl-filters/crt-simple.filter",
};

/* Private */

static void
clear_video (RetroGLDisplay *self)
{
  g_clear_object (&self->pixbuf);
  if (self->pixdata != NULL) {
    retro_pixdata_free (self->pixdata);
    self->pixdata = NULL;
  }
}

static void
set_pixdata (RetroGLDisplay *self,
             RetroPixdata   *pixdata)
{
  if (self->pixdata == pixdata)
    return;

  clear_video (self);

  if (pixdata != NULL)
    self->pixdata = retro_pixdata_copy (pixdata);

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
get_video_box (RetroGLDisplay *self,
               gdouble        *width,
               gdouble        *height,
               gdouble        *x,
               gdouble        *y)
{
  gdouble w;
  gdouble h;
  gdouble display_ratio;
  gdouble allocated_ratio;
  gint scale;

  g_assert (width != NULL);
  g_assert (height != NULL);
  g_assert (x != NULL);
  g_assert (y != NULL);

  scale = gtk_widget_get_scale_factor (GTK_WIDGET (self));

  w = (gdouble) gtk_widget_get_allocated_width (GTK_WIDGET (self)) * scale;
  h = (gdouble) gtk_widget_get_allocated_height (GTK_WIDGET (self)) * scale;

  // Set the size of the display.
  display_ratio = (gdouble) self->aspect_ratio;
  allocated_ratio = w / h;

  // If the screen is wider than the video…
  if (allocated_ratio > display_ratio) {
    *height = h;
    *width = (gdouble) (h * display_ratio);
  }
  else {
    *width = w;
    *height = (gdouble) (w / display_ratio);
  }

  // Set the position of the display.
  *x = (w - *width) / 2;
  *y = (h - *height) / 2;
}

static gboolean
load_texture (RetroGLDisplay *self,
              gint           *texture_width,
              gint           *texture_height)
{
  glBindTexture (GL_TEXTURE_2D, self->texture);

  if (self->pixdata != NULL) {
    *texture_width = retro_pixdata_get_width (self->pixdata);
    *texture_height = retro_pixdata_get_height (self->pixdata);

    return retro_pixdata_load_gl_texture (self->pixdata);
  }

  if (retro_gl_display_get_pixbuf (self) == NULL)
    return FALSE;

  *texture_width = gdk_pixbuf_get_width (self->pixbuf),
  *texture_height = gdk_pixbuf_get_height (self->pixbuf),

  glTexImage2D (GL_TEXTURE_2D,
                0,
                GL_RGB,
                *texture_width,
                *texture_height,
                0,
                GL_RGBA, GL_UNSIGNED_BYTE,
                gdk_pixbuf_get_pixels (self->pixbuf));

  return TRUE;
}

static void
draw_texture (RetroGLDisplay  *self,
              RetroGLSLFilter *filter,
              gint             texture_width,
              gint             texture_height)
{
  GLfloat source_width, source_height;
  GLfloat target_width, target_height;
  GLfloat output_width, output_height;
  RetroGLSLShader *shader = retro_glsl_filter_get_shader (filter);

  retro_glsl_shader_use_program (shader);

  retro_glsl_shader_apply_texture_params (shader);

  retro_glsl_shader_set_uniform_1f (shader, "relative_aspect_ratio",
    (gfloat) gtk_widget_get_allocated_width (GTK_WIDGET (self)) /
    (gfloat) gtk_widget_get_allocated_height (GTK_WIDGET (self)) /
    self->aspect_ratio);

  source_width = (GLfloat) texture_width;
  source_height = (GLfloat) texture_height;
  target_width = (GLfloat) gtk_widget_get_allocated_width (GTK_WIDGET (self));
  target_height = (GLfloat) gtk_widget_get_allocated_height (GTK_WIDGET (self));
  output_width = (GLfloat) gtk_widget_get_allocated_width (GTK_WIDGET (self));
  output_height = (GLfloat) gtk_widget_get_allocated_height (GTK_WIDGET (self));

  retro_glsl_shader_set_uniform_4f (shader, "sourceSize[0]",
                                    source_width, source_height,
                                    1.0f / source_width, 1.0f / source_height);

  retro_glsl_shader_set_uniform_4f (shader, "targetSize",
                                    target_width, target_height,
                                    1.0f / target_width, 1.0f / target_height);

  retro_glsl_shader_set_uniform_4f (shader, "outputSize",
                                    output_width, output_height,
                                    1.0f / output_width, 1.0f / output_height);

  glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static void
realize (RetroGLDisplay *self)
{
  GLuint vertex_buffer_object;
  GLuint vertex_array_object;
  GLuint element_buffer_object;
  RetroVideoFilter current_filter;
  GError *inner_error = NULL;

  gtk_gl_area_make_current (GTK_GL_AREA (self));

  glGenBuffers (1, &vertex_buffer_object);
  glBindBuffer (GL_ARRAY_BUFFER, vertex_buffer_object);
  glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

  glGenVertexArrays (1, &vertex_array_object);
  glBindVertexArray (vertex_array_object);

  glGenBuffers (1, &element_buffer_object);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, element_buffer_object);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (elements), elements, GL_STATIC_DRAW);

  for (RetroVideoFilter filter = 0; filter < RETRO_VIDEO_FILTER_COUNT; filter++) {
    RetroGLSLShader *shader;

    self->glsl_filter[filter] = retro_glsl_filter_new (filter_uris[filter], &inner_error);
    if (G_UNLIKELY (inner_error != NULL)) {
      g_critical ("Shader program %s creation failed: %s",
                  filter_uris[filter],
                  inner_error->message);
      g_clear_object (&self->glsl_filter[filter]);
      g_clear_error (&inner_error);

      continue;
    }

    shader = retro_glsl_filter_get_shader (self->glsl_filter[filter]);

    retro_glsl_shader_set_attribute_pointer (shader,
                                             "position",
                                             sizeof (((RetroVertex *) NULL)->position) / sizeof (float),
                                             GL_FLOAT,
                                             GL_FALSE,
                                             sizeof (RetroVertex),
                                             (const GLvoid *) offsetof (RetroVertex, position));

    retro_glsl_shader_set_attribute_pointer (shader,
                                             "texCoord",
                                             sizeof (((RetroVertex *) NULL)->texture_coordinates) / sizeof (float),
                                             GL_FLOAT,
                                             GL_FALSE,
                                             sizeof (RetroVertex),
                                             (const GLvoid *) offsetof (RetroVertex, texture_coordinates));
  }

  glDeleteTextures (1, &self->texture);
  self->texture = 0;
  glGenTextures (1, &self->texture);
  glBindTexture (GL_TEXTURE_2D, self->texture);

  current_filter = self->filter >= RETRO_VIDEO_FILTER_COUNT ?
    RETRO_VIDEO_FILTER_SMOOTH :
    self->filter;

  if (self->glsl_filter[current_filter] != NULL) {
    RetroGLSLShader *shader = retro_glsl_filter_get_shader (self->glsl_filter[current_filter]);
    retro_glsl_shader_use_program (shader);
  }

  glClearColor (0, 0, 0, 1);
}

static void
unrealize (RetroGLDisplay *self)
{
  gtk_gl_area_make_current (GTK_GL_AREA (self));

  glDeleteTextures (1, &self->texture);
  self->texture = 0;
  for (RetroVideoFilter filter = 0; filter < RETRO_VIDEO_FILTER_COUNT; filter++)
    g_clear_object (&self->glsl_filter[filter]);
}

static gboolean
render (RetroGLDisplay *self)
{
  RetroVideoFilter filter;
  gint texture_width;
  gint texture_height;

  glClear (GL_COLOR_BUFFER_BIT);

  filter = self->filter >= RETRO_VIDEO_FILTER_COUNT ?
    RETRO_VIDEO_FILTER_SMOOTH :
    self->filter;

  g_assert (self->glsl_filter[filter] != NULL);

  if (!load_texture (self, &texture_width, &texture_height))
    return FALSE;

  draw_texture (self, self->glsl_filter[filter], texture_width, texture_height);

  return FALSE;
}

static void
retro_gl_display_finalize (GObject *object)
{
  RetroGLDisplay *self = (RetroGLDisplay *) object;

  glDeleteTextures (1, &self->texture);
  self->texture = 0;
  for (RetroVideoFilter filter = 0; filter < RETRO_VIDEO_FILTER_COUNT; filter++)
    g_clear_object (&self->glsl_filter[filter]);
  g_clear_object (&self->core);
  g_clear_object (&self->pixbuf);

  G_OBJECT_CLASS (retro_gl_display_parent_class)->finalize (object);
}

static void
retro_gl_display_class_init (RetroGLDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = retro_gl_display_finalize;

  gtk_widget_class_set_css_name (widget_class, "retrogldisplay");
}

static void
retro_gl_display_init (RetroGLDisplay *self)
{
  g_signal_connect_object (G_OBJECT (self),
                           "realize",
                           (GCallback) realize,
                           self,
                           0);

  g_signal_connect_object (G_OBJECT (self),
                           "unrealize",
                           (GCallback) unrealize,
                           self,
                           0);

  g_signal_connect_object (G_OBJECT (self),
                           "render",
                           (GCallback) render,
                           self,
                           0);

  self->filter = RETRO_VIDEO_FILTER_SMOOTH;

  g_signal_connect_object (G_OBJECT (self),
                           "notify::sensitive",
                           (GCallback) gtk_widget_queue_draw,
                           self,
                           G_CONNECT_SWAPPED);
}

static void
video_output_cb (RetroCore      *sender,
                 RetroPixdata   *pixdata,
                 RetroGLDisplay *self)
{
  if (pixdata == NULL)
    return;

  self->aspect_ratio = retro_pixdata_get_aspect_ratio (pixdata);
  set_pixdata (self, pixdata);
}

/* Public */

/**
 * retro_gl_display_set_core:
 * @self: a #RetroGLDisplay
 * @core: (nullable): a #RetroCore, or %NULL
 *
 * Sets @core as the #RetroCore displayed by @self.
 */
void
retro_gl_display_set_core (RetroGLDisplay *self,
                           RetroCore      *core)
{
  g_return_if_fail (RETRO_IS_GL_DISPLAY (self));

  if (self->core == core)
    return;

  if (self->core != NULL) {
    g_signal_handler_disconnect (G_OBJECT (self->core), self->video_output_cb_id);
    g_clear_object (&self->core);
  }

  if (core != NULL) {
    self->core = g_object_ref (core);
    self->video_output_cb_id = g_signal_connect_object (core, "video-output", (GCallback) video_output_cb, self, 0);
  }
}

/**
 * retro_gl_display_get_pixbuf:
 * @self: a #RetroGLDisplay
 *
 * Gets the currently displayed video frame.
 *
 * Returns: (transfer none): a #GdkPixbuf
 */
GdkPixbuf *
retro_gl_display_get_pixbuf (RetroGLDisplay *self)
{
  g_return_val_if_fail (RETRO_IS_GL_DISPLAY (self), NULL);

  if (self->pixbuf != NULL)
    return self->pixbuf;

  if (self->pixdata != NULL)
    self->pixbuf = retro_pixdata_to_pixbuf (self->pixdata);

  return self->pixbuf;
}

/**
 * retro_gl_display_set_pixbuf:
 * @self: a #RetroGLDisplay
 * @pixbuf: a #GdkPixbuf
 *
 * Sets @pixbuf as the currently displayed video frame.
 *
 * retro_pixbuf_set_aspect_ratio() can be used to specify the aspect ratio for
 * the pixbuf. Otherwise the core's aspect ratio will be used.
 */
void
retro_gl_display_set_pixbuf (RetroGLDisplay *self,
                             GdkPixbuf      *pixbuf)
{
  gfloat aspect_ratio;

  g_return_if_fail (RETRO_IS_GL_DISPLAY (self));

  if (self->pixbuf == pixbuf)
    return;

  clear_video (self);

  if (pixbuf != NULL)
    self->pixbuf = g_object_ref (pixbuf);

  aspect_ratio = retro_pixbuf_get_aspect_ratio (pixbuf);
  if (aspect_ratio != 0.f)
    self->aspect_ratio = aspect_ratio;

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * retro_gl_display_set_filter:
 * @self: a #RetroGLDisplay
 * @filter: a #RetroVideoFilter
 *
 * Sets the video filter to use to render the core's video on @self.
 */
void
retro_gl_display_set_filter (RetroGLDisplay   *self,
                             RetroVideoFilter  filter)
{
  g_return_if_fail (RETRO_IS_GL_DISPLAY (self));

  self->filter = filter;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * retro_gl_display_get_coordinates_on_display:
 * @self: a #RetroGLDisplay
 * @widget_x: the abscissa on @self
 * @widget_y: the ordinate on @self
 * @display_x: return location for a the abscissa on the core's video display
 * @display_y: return location for a the ordinate on the core's video display
 *
 * Gets coordinates on the core's video output from coordinates on @self, and
 * whether the point is inside the core's video display.
 *
 * Returns: whether the coordinates are on the core's video display
 */
gboolean
retro_gl_display_get_coordinates_on_display (RetroGLDisplay *self,
                                             gdouble         widget_x,
                                             gdouble         widget_y,
                                             gdouble        *display_x,
                                             gdouble        *display_y)
{
  gdouble w = 0.0;
  gdouble h = 0.0;
  gdouble x = 0.0;
  gdouble y = 0.0;
  gint scale_factor;

  g_return_val_if_fail (RETRO_IS_GL_DISPLAY (self), FALSE);
  g_return_val_if_fail (display_x != NULL, FALSE);
  g_return_val_if_fail (display_y != NULL, FALSE);

  get_video_box (self, &w, &h, &x, &y);

  scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (self));
  widget_x *= scale_factor;
  widget_y *= scale_factor;

  // Return coordinates as a [-1.0, 1.0] scale, (0.0, 0.0) is the center.
  *display_x = ((widget_x - x) * 2.0 - w) / w;
  *display_y = ((widget_y - y) * 2.0 - h) / h;

  return (-1.0 <= *display_x) && (*display_x <= 1.0) &&
         (-1.0 <= *display_y) && (*display_y <= 1.0);
}

/**
 * retro_gl_display_new:
 *
 * Creates a new #RetroGLDisplay.
 *
 * Returns: (transfer full): a new #RetroGLDisplay
 */
RetroGLDisplay *
retro_gl_display_new (void)
{
  return g_object_new (RETRO_TYPE_GL_DISPLAY, NULL);
}
