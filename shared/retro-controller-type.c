// This file is part of retro-gtk. License: GPL-3.0+.

#include "retro-controller-type.h"

#include "retro-controller-codes.h"
#include "retro-keyboard-key-private.h"

GType
retro_controller_type_get_type (void)
{
  static volatile gsize retro_controller_type_type = 0;

  if (g_once_init_enter (&retro_controller_type_type)) {
    static const GEnumValue values[] = {
      { RETRO_CONTROLLER_TYPE_NONE, "RETRO_CONTROLLER_TYPE_NONE", "none" },
      { RETRO_CONTROLLER_TYPE_JOYPAD, "RETRO_CONTROLLER_TYPE_JOYPAD", "joypad" },
      { RETRO_CONTROLLER_TYPE_MOUSE, "RETRO_CONTROLLER_TYPE_MOUSE", "mouse" },
      { RETRO_CONTROLLER_TYPE_KEYBOARD, "RETRO_CONTROLLER_TYPE_KEYBOARD", "keyboard" },
      { RETRO_CONTROLLER_TYPE_LIGHTGUN, "RETRO_CONTROLLER_TYPE_LIGHTGUN", "lightgun" },
      { RETRO_CONTROLLER_TYPE_ANALOG, "RETRO_CONTROLLER_TYPE_ANALOG", "analog" },
      { RETRO_CONTROLLER_TYPE_POINTER, "RETRO_CONTROLLER_TYPE_POINTER", "pointer" },
      { RETRO_CONTROLLER_TYPE_COUNT, "RETRO_CONTROLLER_TYPE_COUNT", "count" },
      { RETRO_CONTROLLER_TYPE_TYPE_MASK, "RETRO_CONTROLLER_TYPE_TYPE_MASK", "type-mask" },
      { 0, NULL, NULL },
    };
    GType type;

    type = g_enum_register_static ("RetroControllerType", values);

    g_once_init_leave (&retro_controller_type_type, type);
  }

  return retro_controller_type_type;
}

int
retro_controller_type_get_id_count (RetroControllerType type)
{
  switch (type) {
  case RETRO_CONTROLLER_TYPE_JOYPAD:
    return RETRO_JOYPAD_ID_COUNT;

  case RETRO_CONTROLLER_TYPE_MOUSE:
    return RETRO_MOUSE_ID_COUNT;

  case RETRO_CONTROLLER_TYPE_KEYBOARD:
    return RETRO_KEYBOARD_KEY_LAST;

  case RETRO_CONTROLLER_TYPE_LIGHTGUN:
    return RETRO_LIGHTGUN_ID_COUNT;

  case RETRO_CONTROLLER_TYPE_ANALOG:
    return RETRO_ANALOG_ID_COUNT;

  case RETRO_CONTROLLER_TYPE_POINTER:
    return RETRO_POINTER_ID_COUNT;

  default:
    g_assert_not_reached ();
  }
}

int
retro_controller_type_get_index_count (RetroControllerType type)
{
  switch (type) {
  case RETRO_CONTROLLER_TYPE_ANALOG:
    return RETRO_ANALOG_INDEX_COUNT;

  case RETRO_CONTROLLER_TYPE_JOYPAD:
  case RETRO_CONTROLLER_TYPE_MOUSE:
  case RETRO_CONTROLLER_TYPE_KEYBOARD:
  case RETRO_CONTROLLER_TYPE_LIGHTGUN:
  case RETRO_CONTROLLER_TYPE_POINTER:
    return 1;

  default:
    g_assert_not_reached ();
  }
}
