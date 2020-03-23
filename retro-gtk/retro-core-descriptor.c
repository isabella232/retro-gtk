// This file is part of retro-gtk. License: GPL-3.0+.

#include "retro-core-descriptor.h"

#include "retro-error-private.h"

struct _RetroCoreDescriptor
{
  GObject parent_instance;
  gchar *filename;
  GKeyFile *key_file;
};

G_DEFINE_TYPE (RetroCoreDescriptor, retro_core_descriptor, G_TYPE_OBJECT)

#define RETRO_CORE_DESCRIPTOR_ERROR (retro_core_descriptor_error_quark ())

enum {
  RETRO_CORE_DESCRIPTOR_ERROR_REQUIRED_GROUP_NOT_FOUND,
  RETRO_CORE_DESCRIPTOR_ERROR_REQUIRED_KEY_NOT_FOUND,
  RETRO_CORE_DESCRIPTOR_ERROR_FIRMWARE_NOT_FOUND
};

G_DEFINE_QUARK (retro-core-descriptor-error, retro_core_descriptor_error)

#define RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP "Libretro"
#define RETRO_CORE_DESCRIPTOR_PLATFORM_GROUP_PREFIX "Platform:"
#define RETRO_CORE_DESCRIPTOR_FIRMWARE_GROUP_PREFIX "Firmware:"

#define RETRO_CORE_DESCRIPTOR_TYPE_KEY "Type"
#define RETRO_CORE_DESCRIPTOR_NAME_KEY "Name"
#define RETRO_CORE_DESCRIPTOR_ICON_KEY "Icon"
#define RETRO_CORE_DESCRIPTOR_MODULE_KEY "Module"
#define RETRO_CORE_DESCRIPTOR_LIBRETRO_VERSION_KEY "LibretroVersion"

#define RETRO_CORE_DESCRIPTOR_PLATFORM_MIME_TYPE_KEY "MimeType"
#define RETRO_CORE_DESCRIPTOR_PLATFORM_FIRMWARES_KEY "Firmwares"

#define RETRO_CORE_DESCRIPTOR_FIRMWARE_PATH_KEY "Path"
#define RETRO_CORE_DESCRIPTOR_FIRMWARE_MD5_KEY "MD5"
#define RETRO_CORE_DESCRIPTOR_FIRMWARE_SHA512_KEY "SHA-512"
#define RETRO_CORE_DESCRIPTOR_FIRMWARE_MANDATORY_KEY "Mandatory"

#define RETRO_CORE_DESCRIPTOR_TYPE_GAME "Game"
#define RETRO_CORE_DESCRIPTOR_TYPE_EMULATOR "Emulator"

/* Private */

static void
retro_core_descriptor_finalize (GObject *object)
{
  RetroCoreDescriptor *self = RETRO_CORE_DESCRIPTOR (object);

  g_free (self->filename);
  g_key_file_unref (self->key_file);

  G_OBJECT_CLASS (retro_core_descriptor_parent_class)->finalize (object);
}

static void
retro_core_descriptor_class_init (RetroCoreDescriptorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = retro_core_descriptor_finalize;
}

static void
retro_core_descriptor_init (RetroCoreDescriptor *self)
{
}

static gboolean
has_group_prefixed (RetroCoreDescriptor *self,
                    const gchar         *group_prefix,
                    const gchar         *group_suffix)
{
  g_autofree gchar *group = NULL;

  g_assert (group_prefix != NULL);
  g_assert (group_suffix != NULL);

  group = g_strconcat (group_prefix, group_suffix, NULL);

  return g_key_file_has_group (self->key_file, group);
}

static gboolean
has_key_prefixed (RetroCoreDescriptor  *self,
                  const gchar          *group_prefix,
                  const gchar          *group_suffix,
                  const gchar          *key,
                  GError              **error)
{
  g_autofree gchar *group = NULL;
  gboolean result;
  g_autoptr (RetroError) tmp_error = NULL;

  g_assert (group_prefix != NULL);
  g_assert (group_suffix != NULL);
  g_assert (key != NULL);

  group = g_strconcat (group_prefix, group_suffix, NULL);
  result = g_key_file_has_key (self->key_file,
                               group,
                               key,
                               &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, FALSE);

  return result;
}

static gchar *
get_string_prefixed (RetroCoreDescriptor  *self,
                     const gchar          *group_prefix,
                     const gchar          *group_suffix,
                     const gchar          *key,
                     GError              **error)
{
  g_autofree gchar *group = NULL;
  g_autofree gchar *result = NULL;
  g_autoptr (RetroError) tmp_error = NULL;

  g_assert (group_prefix != NULL);
  g_assert (group_suffix != NULL);
  g_assert (key != NULL);

  group = g_strconcat (group_prefix, group_suffix, NULL);

  result = g_key_file_get_string (self->key_file,
                                  group,
                                  key,
                                  &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, NULL);

  return g_steal_pointer (&result);
}

static gchar **
get_string_list_prefixed (RetroCoreDescriptor  *self,
                          const gchar          *group_prefix,
                          const gchar          *group_suffix,
                          const gchar          *key,
                          gsize                *length,
                          GError              **error)
{
  g_autofree gchar *group = NULL;
  g_auto (GStrv) result = NULL;
  g_autoptr (RetroError) tmp_error = NULL;

  g_assert (group_prefix != NULL);
  g_assert (group_suffix != NULL);
  g_assert (key != NULL);
  g_assert (length != NULL);

  group = g_strconcat (group_prefix, group_suffix, NULL);
  result = g_key_file_get_string_list (self->key_file,
                                       group,
                                       key,
                                       length,
                                       &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, NULL);

  return g_steal_pointer (&result);
}

static void
check_has_required_key (RetroCoreDescriptor  *self,
                        const gchar          *group,
                        const gchar          *key,
                        GError              **error)
{
  gboolean has_key;
  g_autoptr (RetroError) tmp_error = NULL;

  g_assert (group != NULL);
  g_assert (key != NULL);

  has_key = g_key_file_has_key (self->key_file,
                                RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                                RETRO_CORE_DESCRIPTOR_TYPE_KEY,
                                &tmp_error);
  retro_propagate_if_error (error, tmp_error);

  retro_throw_if_fail (error,
    has_key,
    RETRO_CORE_DESCRIPTOR_ERROR, RETRO_CORE_DESCRIPTOR_ERROR_REQUIRED_KEY_NOT_FOUND,
    "%s isn't a valid Libretro core descriptor: required key %s not found in group [%s].",
    self->filename, key, group);
}

static void
check_libretro_group (RetroCoreDescriptor  *self,
                      GError              **error)
{
  g_autoptr (RetroError) tmp_error = NULL;

  check_has_required_key (self,
                          RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                          RETRO_CORE_DESCRIPTOR_TYPE_KEY,
                          &tmp_error);
  retro_propagate_if_error (error, tmp_error);

  check_has_required_key (self,
                          RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                          RETRO_CORE_DESCRIPTOR_NAME_KEY,
                          &tmp_error);
  retro_propagate_if_error (error, tmp_error);

  check_has_required_key (self,
                          RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                          RETRO_CORE_DESCRIPTOR_MODULE_KEY,
                          &tmp_error);
  retro_propagate_if_error (error, tmp_error);

  check_has_required_key (self,
                          RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                          RETRO_CORE_DESCRIPTOR_LIBRETRO_VERSION_KEY,
                          &tmp_error);
  retro_propagate_if_error (error, tmp_error);
}

static void
check_platform_group (RetroCoreDescriptor  *self,
                      const gchar          *group,
                      GError              **error)
{
  gboolean has_key;
  g_auto (GStrv) firmwares = NULL;
  g_autoptr (RetroError) tmp_error = NULL;

  g_assert (group != NULL);

  check_has_required_key (self,
                          group,
                          RETRO_CORE_DESCRIPTOR_PLATFORM_MIME_TYPE_KEY,
                          &tmp_error);

  has_key = g_key_file_has_key (self->key_file,
                                group,
                                RETRO_CORE_DESCRIPTOR_PLATFORM_FIRMWARES_KEY,
                                &tmp_error);
  retro_propagate_if_error (error, tmp_error);

  if (!has_key)
    return;

  firmwares = g_key_file_get_string_list (self->key_file,
                                          group,
                                          RETRO_CORE_DESCRIPTOR_PLATFORM_FIRMWARES_KEY,
                                          NULL,
                                          &tmp_error);
  retro_propagate_if_error (error, tmp_error);

  for (GStrv firmware_p = firmwares; *firmware_p != NULL; firmware_p++) {
    g_autofree gchar *firmware_group = g_strconcat (RETRO_CORE_DESCRIPTOR_FIRMWARE_GROUP_PREFIX, *firmware_p, NULL);
    retro_throw_if_fail (error,
      g_key_file_has_group (self->key_file, firmware_group),
      RETRO_CORE_DESCRIPTOR_ERROR, RETRO_CORE_DESCRIPTOR_ERROR_FIRMWARE_NOT_FOUND,
      "%s isn't a valid Libretro core descriptor: [%s] mentioned in [%s] not found.",
      self->filename, firmware_group, group);
  }
}

static void
check_firmware_group (RetroCoreDescriptor  *self,
                      const gchar          *group,
                      GError              **error)
{
  g_autoptr (RetroError) tmp_error = NULL;

  g_assert (group != NULL);

  check_has_required_key (self,
                          group,
                          RETRO_CORE_DESCRIPTOR_FIRMWARE_PATH_KEY,
                          &tmp_error);
  retro_propagate_if_error (error, tmp_error);

  check_has_required_key (self,
                          group,
                          RETRO_CORE_DESCRIPTOR_FIRMWARE_MANDATORY_KEY,
                          &tmp_error);
  retro_propagate_if_error (error, tmp_error);
}

/* Public */

/**
 * retro_core_descriptor_has_icon:
 * @self: a #RetroCoreDescriptor
 * @error: return location for a #GError, or %NULL
 *
 * Gets whether the core has an icon.
 *
 * Returns: whether the core has an icon
 */
gboolean
retro_core_descriptor_has_icon (RetroCoreDescriptor  *self,
                                GError              **error)
{
  gboolean has_icon;
  g_autoptr (RetroError) tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  has_icon = g_key_file_has_key (self->key_file,
                                 RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                                 RETRO_CORE_DESCRIPTOR_ICON_KEY,
                                 &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, FALSE);

  return has_icon;
}

/**
 * retro_core_descriptor_get_uri
 * @self: a #RetroCoreDescriptor
 *
 * Gets the URI of the file of @self.
 *
 * Returns: the URI of the file of @self, free it with g_free()
 */
gchar *
retro_core_descriptor_get_uri (RetroCoreDescriptor *self)
{
  g_autoptr (GFile) file = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  file = g_file_new_for_path (self->filename);

  return g_file_get_uri (file);
}

/**
 * retro_core_descriptor_get_id
 * @self: a #RetroCoreDescriptor
 *
 * Gets the ID of @self.
 *
 * Returns: the ID of @self, free it with g_free()
 */
gchar *
retro_core_descriptor_get_id (RetroCoreDescriptor *self)
{
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  return g_path_get_basename (self->filename);
}

/**
 * retro_core_descriptor_get_is_game
 * @self: a #RetroCoreDescriptor
 * @error: return location for a #GError, or %NULL
 *
 * Gets whether the core is a game, and hence can't load games.
 *
 * Returns: whether the core is a game
 */
gboolean
retro_core_descriptor_get_is_game (RetroCoreDescriptor  *self,
                                   GError              **error)
{
  g_autofree gchar *type = NULL;
  g_autoptr (RetroError) tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  type = g_key_file_get_string (self->key_file,
                                  RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                                  RETRO_CORE_DESCRIPTOR_TYPE_KEY,
                                  &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, FALSE);

  return g_strcmp0 (type, RETRO_CORE_DESCRIPTOR_TYPE_GAME) == 0;
}

/**
 * retro_core_descriptor_get_is_emulator
 * @self: a #RetroCoreDescriptor
 * @error: return location for a #GError, or %NULL
 *
 * Gets whether the core is an emulator, and hence need games to be loaded.
 *
 * Returns: whether the core is an emulator
 */
gboolean
retro_core_descriptor_get_is_emulator (RetroCoreDescriptor  *self,
                                       GError              **error)
{
  g_autofree gchar *type = NULL;
  g_autoptr (RetroError) tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  type = g_key_file_get_string (self->key_file,
                                  RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                                  RETRO_CORE_DESCRIPTOR_TYPE_KEY,
                                  &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, FALSE);

  return g_strcmp0 (type, RETRO_CORE_DESCRIPTOR_TYPE_EMULATOR) == 0;
}

/**
 * retro_core_descriptor_get_name
 * @self: a #RetroCoreDescriptor
 * @error: return location for a #GError, or %NULL
 *
 * Gets the name, or %NULL if it doesn't exist.
 *
 * Returns: (nullable) (transfer full): a string or %NULL, free it with g_free()
 */
gchar *
retro_core_descriptor_get_name (RetroCoreDescriptor  *self,
                                GError              **error)
{
  return g_key_file_get_string (self->key_file,
                                RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                                RETRO_CORE_DESCRIPTOR_NAME_KEY,
                                error);
}

/**
 * retro_core_descriptor_get_icon
 * @self: a #RetroCoreDescriptor
 * @error: return location for a #GError, or %NULL
 *
 * Gets the icon, or %NULL if it doesn't exist.
 *
 * Returns: (nullable) (transfer full): a #GIcon or %NULL
 */
GIcon *
retro_core_descriptor_get_icon (RetroCoreDescriptor  *self,
                                GError              **error)
{
  g_autofree gchar *icon_name = NULL;
  g_autoptr (RetroError) tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  icon_name = g_key_file_get_string (self->key_file,
                                     RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                                     RETRO_CORE_DESCRIPTOR_ICON_KEY,
                                     &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, NULL);

  return G_ICON (g_themed_icon_new (icon_name));
}

/**
 * retro_core_descriptor_get_module
 * @self: a #RetroCoreDescriptor
 * @error: return location for a #GError, or %NULL
 *
 * Gets the module file name, or %NULL if it doesn't exist.
 *
 * Returns: (nullable) (transfer full): a string or %NULL, free it with g_free()
 */
char *
retro_core_descriptor_get_module (RetroCoreDescriptor  *self,
                                  GError              **error)
{
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  return g_key_file_get_string (self->key_file,
                                RETRO_CORE_DESCRIPTOR_LIBRETRO_GROUP,
                                RETRO_CORE_DESCRIPTOR_MODULE_KEY,
                                error);
}

/**
 * retro_core_descriptor_get_module_file
 * @self: a #RetroCoreDescriptor
 * @error: return location for a #GError, or %NULL
 *
 * Gets the module file, or %NULL if it doesn't exist.
 *
 * Returns: (nullable) (transfer full): a #GFile or %NULL
 */
GFile *
retro_core_descriptor_get_module_file (RetroCoreDescriptor  *self,
                                       GError              **error)
{
  g_autoptr (GFile) file = NULL;
  g_autoptr (GFile) dir = NULL;
  g_autofree gchar *module = NULL;
  g_autoptr (GFile) module_file = NULL;
  g_autoptr (RetroError) tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  file = g_file_new_for_path (self->filename);
  dir = g_file_get_parent (file);
  if (dir == NULL)
    return NULL;

  module = retro_core_descriptor_get_module (self, &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, NULL);

  module_file = g_file_get_child (dir, module);
  if (!g_file_query_exists (module_file, NULL))
    return NULL;

  return g_steal_pointer (&module_file);
}

/**
 * retro_core_descriptor_has_platform:
 * @self: a #RetroCoreDescriptor
 * @platform: a platform name
 *
 * Gets whether the core descriptor declares the given platform.
 *
 * Returns: whether the core descriptor declares the given platform
 */
gboolean
retro_core_descriptor_has_platform (RetroCoreDescriptor *self,
                                    const gchar         *platform)
{
  return has_group_prefixed (self,
                           RETRO_CORE_DESCRIPTOR_PLATFORM_GROUP_PREFIX,
                           platform);
}

/**
 * retro_core_descriptor_has_firmwares:
 * @self: a #RetroCoreDescriptor
 * @platform: a platform name
 * @error: return location for a #GError, or %NULL
 *
 * Gets whether the platform has associated firmwares.
 *
 * Returns: whether the platform has associated firmwares
 */
gboolean
retro_core_descriptor_has_firmwares (RetroCoreDescriptor  *self,
                                     const gchar          *platform,
                                     GError              **error)
{
  return has_key_prefixed (self,
                           RETRO_CORE_DESCRIPTOR_PLATFORM_GROUP_PREFIX,
                           platform,
                           RETRO_CORE_DESCRIPTOR_PLATFORM_FIRMWARES_KEY,
                           error);
}

/**
 * retro_core_descriptor_has_firmware_md5:
 * @self: a #RetroCoreDescriptor
 * @firmware: a firmware name
 * @error: return location for a #GError, or %NULL
 *
 * Gets whether the firmware declares its MD5 fingerprint.
 *
 * Returns: whether the firmware declares its MD5 fingerprint
 */
gboolean
retro_core_descriptor_has_firmware_md5 (RetroCoreDescriptor  *self,
                                        const gchar          *firmware,
                                        GError              **error)
{
  return has_key_prefixed (self,
                           RETRO_CORE_DESCRIPTOR_FIRMWARE_GROUP_PREFIX,
                           firmware,
                           RETRO_CORE_DESCRIPTOR_FIRMWARE_MD5_KEY,
                           error);
}

/**
 * retro_core_descriptor_has_firmware_sha512:
 * @self: a #RetroCoreDescriptor
 * @firmware: a firmware name
 * @error: return location for a #GError, or %NULL
 *
 * Gets whether the firmware declares its SHA512 fingerprint.
 *
 * Returns: whether the firmware declares its SHA512 fingerprint
 */
gboolean
retro_core_descriptor_has_firmware_sha512 (RetroCoreDescriptor  *self,
                                           const gchar          *firmware,
                                           GError              **error)
{
  return has_key_prefixed (self,
                           RETRO_CORE_DESCRIPTOR_FIRMWARE_GROUP_PREFIX,
                           firmware,
                           RETRO_CORE_DESCRIPTOR_FIRMWARE_SHA512_KEY,
                           error);
}

/**
 * retro_core_descriptor_get_mime_type
 * @self: a #RetroCoreDescriptor
 * @platform: a platform name
 * @length: (out) (optional): return location for the number of returned
 * strings, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Gets the list of MIME types accepted used by the core for this platform.
 *
 * Returns: (array zero-terminated=1 length=length) (element-type utf8)
 * (transfer full): a %NULL-terminated string array or %NULL, the array should
 * be freed with g_strfreev()
 */
gchar **
retro_core_descriptor_get_mime_type (RetroCoreDescriptor  *self,
                                     const gchar          *platform,
                                     gsize                *length,
                                     GError              **error)
{
  return get_string_list_prefixed (self,
                                   RETRO_CORE_DESCRIPTOR_PLATFORM_GROUP_PREFIX,
                                   platform,
                                   RETRO_CORE_DESCRIPTOR_PLATFORM_MIME_TYPE_KEY,
                                   length,
                                   error);
}

/**
 * retro_core_descriptor_get_firmwares
 * @self: a #RetroCoreDescriptor
 * @platform: a platform name
 * @length: (out) (optional): return location for the number of returned
 * strings, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Gets the list of firmwares used by the core for this platform.
 *
 * Returns: (array zero-terminated=1 length=length) (element-type utf8)
 * (transfer full): a %NULL-terminated string array or %NULL, the array should
 * be freed with g_strfreev()
 */
gchar **
retro_core_descriptor_get_firmwares (RetroCoreDescriptor  *self,
                                     const gchar          *platform,
                                     gsize                *length,
                                     GError              **error)
{
  return get_string_list_prefixed (self,
                                   RETRO_CORE_DESCRIPTOR_PLATFORM_GROUP_PREFIX,
                                   platform,
                                   RETRO_CORE_DESCRIPTOR_PLATFORM_FIRMWARES_KEY,
                                   length,
                                   error);
}

/**
 * retro_core_descriptor_get_firmware_path
 * @self: a #RetroCoreDescriptor
 * @firmware: a firmware name
 * @error: return location for a #GError, or %NULL
 *
 * Gets the demanded path to the firmware file, or %NULL.
 *
 * Returns: (nullable) (transfer full): a string or %NULL, free it with g_free()
 */
gchar *
retro_core_descriptor_get_firmware_path (RetroCoreDescriptor  *self,
                                         const gchar          *firmware,
                                         GError              **error)
{
  return get_string_prefixed (self,
                              RETRO_CORE_DESCRIPTOR_FIRMWARE_GROUP_PREFIX,
                              firmware,
                              RETRO_CORE_DESCRIPTOR_FIRMWARE_PATH_KEY,
                              error);
}

/**
 * retro_core_descriptor_get_firmware_md5
 * @self: a #RetroCoreDescriptor
 * @firmware: a firmware name
 * @error: return location for a #GError, or %NULL
 *
 * Gets the MD5 fingerprint of the firmware file, or %NULL.
 *
 * Returns: (nullable) (transfer full): a string or %NULL, free it with g_free()
 */
gchar *
retro_core_descriptor_get_firmware_md5 (RetroCoreDescriptor  *self,
                                        const gchar          *firmware,
                                        GError              **error)
{
  return get_string_prefixed (self,
                              RETRO_CORE_DESCRIPTOR_FIRMWARE_GROUP_PREFIX,
                              firmware,
                              RETRO_CORE_DESCRIPTOR_FIRMWARE_MD5_KEY,
                              error);
}

/**
 * retro_core_descriptor_get_firmware_sha512
 * @self: a #RetroCoreDescriptor
 * @firmware: a firmware name
 * @error: return location for a #GError, or %NULL
 *
 * Gets the SHA512 fingerprint of the firmware file, or %NULL.
 *
 * Returns: (nullable) (transfer full): a string or %NULL, free it with g_free()
 */
gchar *
retro_core_descriptor_get_firmware_sha512 (RetroCoreDescriptor  *self,
                                           const gchar          *firmware,
                                           GError              **error)
{
  return get_string_prefixed (self,
                              RETRO_CORE_DESCRIPTOR_FIRMWARE_GROUP_PREFIX,
                              firmware,
                              RETRO_CORE_DESCRIPTOR_FIRMWARE_SHA512_KEY,
                              error);
}

/**
 * retro_core_descriptor_get_is_firmware_mandatory:
 * @self: a #RetroCoreDescriptor
 * @firmware: a firmware name
 * @error: return location for a #GError, or %NULL
 *
 * Gets whether the firmware is mandatory for the core to function.
 *
 * Returns: whether the firmware is mandatory for the core to function
 */
gboolean
retro_core_descriptor_get_is_firmware_mandatory (RetroCoreDescriptor  *self,
                                                 const gchar          *firmware,
                                                 GError              **error)
{
  g_autofree gchar *group = NULL;
  gboolean result;
  g_autoptr (RetroError) tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);
  g_return_val_if_fail (firmware != NULL, FALSE);

  group = g_strconcat (RETRO_CORE_DESCRIPTOR_FIRMWARE_GROUP_PREFIX, firmware, NULL);
  result = g_key_file_get_boolean (self->key_file,
                                   group,
                                   RETRO_CORE_DESCRIPTOR_FIRMWARE_MANDATORY_KEY,
                                   &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, FALSE);

  return result;
}

/**
 * retro_core_descriptor_get_platform_supports_mime_types:
 * @self: a #RetroCoreDescriptor
 * @platform: a platform name
 * @mime_types: (array zero-terminated=1) (element-type utf8): the MIME types
 * @error: return location for a #GError, or %NULL
 *
 * Gets whether the platform supports all of the given MIME types.
 *
 * Returns: whether the platform supports all of the given MIME types
 */
gboolean
retro_core_descriptor_get_platform_supports_mime_types (RetroCoreDescriptor  *self,
                                                        const gchar          *platform,
                                                        const gchar * const  *mime_types,
                                                        GError              **error)
{
  g_auto (GStrv) supported_mime_types = NULL;
  gsize supported_mime_types_length;
  g_autoptr (RetroError) tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);
  g_return_val_if_fail (platform != NULL, FALSE);
  g_return_val_if_fail (mime_types != NULL, FALSE);

  supported_mime_types =
    retro_core_descriptor_get_mime_type (self,
                                         platform,
                                         &supported_mime_types_length,
                                         &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, FALSE);

  for (; *mime_types != NULL; mime_types++)
    if (!g_strv_contains ((const char * const *) supported_mime_types,
                          *mime_types))
      return FALSE;

  return TRUE;
}

/**
 * retro_core_descriptor_new:
 * @filename: the file name of the core descriptor
 * @error: return location for a #GError, or %NULL
 *
 * Creates a new #RetroCoreDescriptor.
 *
 * Returns: (transfer full): a new #RetroCoreDescriptor
 */
RetroCoreDescriptor *
retro_core_descriptor_new (const gchar  *filename,
                           GError      **error)
{
  g_autoptr (RetroCoreDescriptor) self = NULL;
  g_auto (GStrv) groups = NULL;
  gsize groups_length;
  g_autoptr (RetroError) tmp_error = NULL;

  g_return_val_if_fail (filename != NULL, NULL);

  self =  g_object_new (RETRO_TYPE_CORE_DESCRIPTOR, NULL);
  self->filename = g_strdup (filename);
  self->key_file = g_key_file_new ();
  g_key_file_load_from_file (self->key_file,
                             filename,
                             G_KEY_FILE_NONE,
                             &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, NULL);

  check_libretro_group (self, &tmp_error);
  retro_propagate_val_if_error (error, tmp_error, NULL);

  groups = g_key_file_get_groups (self->key_file, &groups_length);
  for (gsize i = 0; i < groups_length; i++) {
    if (g_str_has_prefix (groups[i],
                          RETRO_CORE_DESCRIPTOR_PLATFORM_GROUP_PREFIX)) {
      check_platform_group (self, groups[i], &tmp_error);
      retro_propagate_val_if_error (error, tmp_error, NULL);
    }
    else if (g_str_has_prefix (groups[i],
                               RETRO_CORE_DESCRIPTOR_FIRMWARE_GROUP_PREFIX)) {
      check_firmware_group (self, groups[i], &tmp_error);
      retro_propagate_val_if_error (error, tmp_error, NULL);
    }
  }

  return g_steal_pointer (&self);
}
