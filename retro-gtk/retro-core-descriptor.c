// This file is part of retro-gtk. License: GPL-3.0+.

/**
 * SECTION:retro-core-descriptor
 * @short_description: An object describing the capabilities and requirements of a Libretro core
 * @title: RetroCoreDescriptor
 * @See_also: #RetroCore
 */

#include "retro-core-descriptor.h"

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

#define LIBRETRO_GROUP "Libretro"
#define PLATFORM_GROUP_PREFIX "Platform:"
#define FIRMWARE_GROUP_PREFIX "Firmware:"

#define TYPE_KEY "Type"
#define NAME_KEY "Name"
#define ICON_KEY "Icon"
#define MODULE_KEY "Module"
#define LIBRETRO_VERSION_KEY "LibretroVersion"

#define PLATFORM_MIME_TYPE_KEY "MimeType"
#define PLATFORM_FIRMWARES_KEY "Firmwares"

#define FIRMWARE_PATH_KEY "Path"
#define FIRMWARE_MD5_KEY "MD5"
#define FIRMWARE_SHA512_KEY "SHA-512"
#define FIRMWARE_MANDATORY_KEY "Mandatory"

#define TYPE_GAME "Game"
#define TYPE_EMULATOR "Emulator"

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
  gboolean has_group;

  g_assert (group_prefix != NULL);
  g_assert (group_suffix != NULL);

  group = g_strconcat (group_prefix, group_suffix, NULL);
  has_group = g_key_file_has_group (self->key_file, group);

  return has_group;
}

static gboolean
has_key_prefixed (RetroCoreDescriptor  *self,
                  const gchar          *group_prefix,
                  const gchar          *group_suffix,
                  const gchar          *key,
                  GError              **error)
{
  g_autofree gchar *group = NULL;
  gboolean has_key;
  GError *tmp_error = NULL;

  g_assert (group_prefix != NULL);
  g_assert (group_suffix != NULL);
  g_assert (key != NULL);

  group = g_strconcat (group_prefix, group_suffix, NULL);
  has_key = g_key_file_has_key (self->key_file,
                                group,
                                key,
                                &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return FALSE;
  }

  return has_key;
}

static gchar *
get_string_prefixed (RetroCoreDescriptor  *self,
                     const gchar          *group_prefix,
                     const gchar          *group_suffix,
                     const gchar          *key,
                     GError              **error)
{
  g_autofree gchar *group = NULL;
  g_autofree gchar *string = NULL;
  GError *tmp_error = NULL;

  g_assert (group_prefix != NULL);
  g_assert (group_suffix != NULL);
  g_assert (key != NULL);

  group = g_strconcat (group_prefix, group_suffix, NULL);

  string = g_key_file_get_string (self->key_file,
                                  group,
                                  key,
                                  &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL))
    return NULL;

  return g_steal_pointer (&string);
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
  g_auto (GStrv) list = NULL;
  GError *tmp_error = NULL;

  g_assert (group_prefix != NULL);
  g_assert (group_suffix != NULL);
  g_assert (key != NULL);
  g_assert (length != NULL);

  group = g_strconcat (group_prefix, group_suffix, NULL);
  list = g_key_file_get_string_list (self->key_file,
                                     group,
                                     key,
                                     length,
                                     &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return NULL;
  }

  return g_steal_pointer (&list);
}

static void
check_has_required_key (RetroCoreDescriptor  *self,
                        const gchar          *group,
                        const gchar          *key,
                        GError              **error)
{
  gboolean has_key;
  GError *tmp_error = NULL;

  g_assert (group != NULL);
  g_assert (key != NULL);

  has_key = g_key_file_has_key (self->key_file,
                                LIBRETRO_GROUP,
                                TYPE_KEY,
                                &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return;
  }

  if (!has_key)
    g_set_error (error,
                 RETRO_CORE_DESCRIPTOR_ERROR,
                 RETRO_CORE_DESCRIPTOR_ERROR_REQUIRED_KEY_NOT_FOUND,
                 "%s isn't a valid Libretro core descriptor: "
                 "required key %s not found in group [%s].",
                 self->filename,
                 key,
                 group);
}

static void
check_libretro_group (RetroCoreDescriptor  *self,
                      GError              **error)
{
  GError *tmp_error = NULL;

  check_has_required_key (self,
                          LIBRETRO_GROUP,
                          TYPE_KEY,
                          &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return;
  }

  check_has_required_key (self,
                          LIBRETRO_GROUP,
                          NAME_KEY,
                          &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return;
  }

  check_has_required_key (self,
                          LIBRETRO_GROUP,
                          MODULE_KEY,
                          &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return;
  }

  check_has_required_key (self,
                          LIBRETRO_GROUP,
                          LIBRETRO_VERSION_KEY,
                          &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return;
  }
}

static void
check_platform_group (RetroCoreDescriptor  *self,
                      const gchar          *group,
                      GError              **error)
{
  gboolean has_key;
  g_auto (GStrv) firmwares = NULL;
  GError *tmp_error = NULL;

  g_assert (group != NULL);

  check_has_required_key (self,
                          group,
                          PLATFORM_MIME_TYPE_KEY,
                          &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return;
  }

  has_key = g_key_file_has_key (self->key_file,
                                group,
                                PLATFORM_FIRMWARES_KEY,
                                &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return;
  }

  if (!has_key)
    return;

  firmwares = g_key_file_get_string_list (self->key_file,
                                          group,
                                          PLATFORM_FIRMWARES_KEY,
                                          NULL,
                                          &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return;
  }

  for (GStrv firmware_p = firmwares; *firmware_p != NULL; firmware_p++) {
    g_autofree gchar *firmware_group = NULL;

    firmware_group = g_strconcat (FIRMWARE_GROUP_PREFIX, *firmware_p, NULL);
    if (!g_key_file_has_group (self->key_file, firmware_group)) {
      g_set_error (error,
                   RETRO_CORE_DESCRIPTOR_ERROR,
                   RETRO_CORE_DESCRIPTOR_ERROR_FIRMWARE_NOT_FOUND,
                   "%s isn't a valid Libretro core descriptor:"
                   "[%s] mentioned in [%s] not found.",
                   self->filename,
                   firmware_group,
                   group);

      return;
    }
  }
}

static void
check_firmware_group (RetroCoreDescriptor  *self,
                      const gchar          *group,
                      GError              **error)
{
  GError *tmp_error = NULL;

  g_assert (group != NULL);

  check_has_required_key (self,
                          group,
                          FIRMWARE_PATH_KEY,
                          &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return;
  }

  check_has_required_key (self,
                          group,
                          FIRMWARE_MANDATORY_KEY,
                          &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return;
  }
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
  GError *tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  has_icon = g_key_file_has_key (self->key_file,
                                 LIBRETRO_GROUP,
                                 ICON_KEY,
                                 &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return FALSE;
  }

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
  g_autofree gchar *uri = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  file = g_file_new_for_path (self->filename);
  uri = g_file_get_uri (file);

  return g_steal_pointer (&uri);
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
  gboolean is_game;
  GError *tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  type = g_key_file_get_string (self->key_file,
                                LIBRETRO_GROUP,
                                TYPE_KEY,
                                &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return FALSE;
  }

  is_game = g_strcmp0 (type, TYPE_GAME) == 0;

  return is_game;
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
  gboolean is_emulator;
  GError *tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  type = g_key_file_get_string (self->key_file,
                                LIBRETRO_GROUP,
                                TYPE_KEY,
                                &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return FALSE;
  }

  is_emulator = g_strcmp0 (type, TYPE_EMULATOR) == 0;

  return is_emulator;
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
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  return g_key_file_get_string (self->key_file,
                                LIBRETRO_GROUP,
                                NAME_KEY,
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
  GIcon *icon;
  GError *tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  icon_name = g_key_file_get_string (self->key_file,
                                     LIBRETRO_GROUP,
                                     ICON_KEY,
                                     &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return NULL;
  }

  icon = G_ICON (g_themed_icon_new (icon_name));

  return icon;
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
                                LIBRETRO_GROUP,
                                MODULE_KEY,
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
  GError *tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  file = g_file_new_for_path (self->filename);
  dir = g_file_get_parent (file);
  if (dir == NULL)
    return NULL;

  module = retro_core_descriptor_get_module (self, &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return NULL;
  }

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
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  return has_group_prefixed (self,
                           PLATFORM_GROUP_PREFIX,
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
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  return has_key_prefixed (self,
                           PLATFORM_GROUP_PREFIX,
                           platform,
                           PLATFORM_FIRMWARES_KEY,
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
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  return has_key_prefixed (self,
                           FIRMWARE_GROUP_PREFIX,
                           firmware,
                           FIRMWARE_MD5_KEY,
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
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);

  return has_key_prefixed (self,
                           FIRMWARE_GROUP_PREFIX,
                           firmware,
                           FIRMWARE_SHA512_KEY,
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
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  return get_string_list_prefixed (self,
                                   PLATFORM_GROUP_PREFIX,
                                   platform,
                                   PLATFORM_MIME_TYPE_KEY,
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
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  return get_string_list_prefixed (self,
                                   PLATFORM_GROUP_PREFIX,
                                   platform,
                                   PLATFORM_FIRMWARES_KEY,
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
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  return get_string_prefixed (self,
                              FIRMWARE_GROUP_PREFIX,
                              firmware,
                              FIRMWARE_PATH_KEY,
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
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  return get_string_prefixed (self,
                              FIRMWARE_GROUP_PREFIX,
                              firmware,
                              FIRMWARE_MD5_KEY,
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
  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), NULL);

  return get_string_prefixed (self,
                              FIRMWARE_GROUP_PREFIX,
                              firmware,
                              FIRMWARE_SHA512_KEY,
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
  gboolean is_mandatory;
  GError *tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);
  g_return_val_if_fail (firmware != NULL, FALSE);

  group = g_strconcat (FIRMWARE_GROUP_PREFIX, firmware, NULL);
  is_mandatory = g_key_file_get_boolean (self->key_file,
                                         group,
                                         FIRMWARE_MANDATORY_KEY,
                                         &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return FALSE;
  }

  return is_mandatory;
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
  GError *tmp_error = NULL;

  g_return_val_if_fail (RETRO_IS_CORE_DESCRIPTOR (self), FALSE);
  g_return_val_if_fail (platform != NULL, FALSE);
  g_return_val_if_fail (mime_types != NULL, FALSE);

  supported_mime_types =
    retro_core_descriptor_get_mime_type (self,
                                         platform,
                                         &supported_mime_types_length,
                                         &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return FALSE;
  }

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
  GError *tmp_error = NULL;

  g_return_val_if_fail (filename != NULL, NULL);

  self =  g_object_new (RETRO_TYPE_CORE_DESCRIPTOR, NULL);
  self->filename = g_strdup (filename);
  self->key_file = g_key_file_new ();
  g_key_file_load_from_file (self->key_file,
                             filename,
                             G_KEY_FILE_NONE,
                             &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return NULL;
  }

  check_libretro_group (self, &tmp_error);
  if (G_UNLIKELY (tmp_error != NULL)) {
    g_propagate_error (error, tmp_error);

    return NULL;
  }

  groups = g_key_file_get_groups (self->key_file, &groups_length);
  for (gsize i = 0; i < groups_length; i++) {
    if (g_str_has_prefix (groups[i],
                          PLATFORM_GROUP_PREFIX)) {
      check_platform_group (self, groups[i], &tmp_error);
      if (G_UNLIKELY (tmp_error != NULL)) {
        g_propagate_error (error, tmp_error);

        return NULL;
      }
    }
    else if (g_str_has_prefix (groups[i],
                               FIRMWARE_GROUP_PREFIX)) {
      check_firmware_group (self, groups[i], &tmp_error);
      if (G_UNLIKELY (tmp_error != NULL)) {
        g_propagate_error (error, tmp_error);

        return NULL;
      }
    }
  }

  return g_steal_pointer (&self);
}
