// This file is part of retro-gtk. License: GPL-3.0+.

#include "retro-controller-codes.h"

GType
retro_joypad_id_get_type (void)
{
  static volatile gsize retro_joypad_id_type = 0;

  if (g_once_init_enter (&retro_joypad_id_type)) {
    static const GEnumValue values[] = {
      { RETRO_JOYPAD_ID_B, "RETRO_JOYPAD_ID_B", "b" },
      { RETRO_JOYPAD_ID_Y, "RETRO_JOYPAD_ID_Y", "y" },
      { RETRO_JOYPAD_ID_SELECT, "RETRO_JOYPAD_ID_SELECT", "select" },
      { RETRO_JOYPAD_ID_START, "RETRO_JOYPAD_ID_START", "start" },
      { RETRO_JOYPAD_ID_UP, "RETRO_JOYPAD_ID_UP", "up" },
      { RETRO_JOYPAD_ID_DOWN, "RETRO_JOYPAD_ID_DOWN", "down" },
      { RETRO_JOYPAD_ID_LEFT, "RETRO_JOYPAD_ID_LEFT", "left" },
      { RETRO_JOYPAD_ID_RIGHT, "RETRO_JOYPAD_ID_RIGHT", "right" },
      { RETRO_JOYPAD_ID_A, "RETRO_JOYPAD_ID_A", "a" },
      { RETRO_JOYPAD_ID_X, "RETRO_JOYPAD_ID_X", "x" },
      { RETRO_JOYPAD_ID_L, "RETRO_JOYPAD_ID_L", "l" },
      { RETRO_JOYPAD_ID_R, "RETRO_JOYPAD_ID_R", "r" },
      { RETRO_JOYPAD_ID_L2, "RETRO_JOYPAD_ID_L2", "l2" },
      { RETRO_JOYPAD_ID_R2, "RETRO_JOYPAD_ID_R2", "r2" },
      { RETRO_JOYPAD_ID_L3, "RETRO_JOYPAD_ID_L3", "l3" },
      { RETRO_JOYPAD_ID_R3, "RETRO_JOYPAD_ID_R3", "r3" },
      { RETRO_JOYPAD_ID_COUNT, "RETRO_JOYPAD_ID_COUNT", "count" },
      { 0, NULL, NULL },
    };
    GType type;

    type = g_enum_register_static ("RetroJoypadId", values);

    g_once_init_leave (&retro_joypad_id_type, type);
  }

  return retro_joypad_id_type;
}

GType
retro_mouse_id_get_type (void)
{
  static volatile gsize retro_mouse_id_type = 0;

  if (g_once_init_enter (&retro_mouse_id_type)) {
    static const GEnumValue values[] = {
      { RETRO_MOUSE_ID_X, "RETRO_MOUSE_ID_X", "x" },
      { RETRO_MOUSE_ID_Y, "RETRO_MOUSE_ID_Y", "y" },
      { RETRO_MOUSE_ID_LEFT, "RETRO_MOUSE_ID_LEFT", "left" },
      { RETRO_MOUSE_ID_RIGHT, "RETRO_MOUSE_ID_RIGHT", "right" },
      { RETRO_MOUSE_ID_WHEELUP, "RETRO_MOUSE_ID_WHEELUP", "wheelup" },
      { RETRO_MOUSE_ID_WHEELDOWN, "RETRO_MOUSE_ID_WHEELDOWN", "wheeldown" },
      { RETRO_MOUSE_ID_MIDDLE, "RETRO_MOUSE_ID_MIDDLE", "middle" },
      { RETRO_MOUSE_ID_HORIZ_WHEELUP, "RETRO_MOUSE_ID_HORIZ_WHEELUP", "horiz-wheelup" },
      { RETRO_MOUSE_ID_HORIZ_WHEELDOWN, "RETRO_MOUSE_ID_HORIZ_WHEELDOWN", "horiz-wheeldown" },
      { 0, NULL, NULL },
    };
    GType type;

    type = g_enum_register_static ("RetroMouseId", values);

    g_once_init_leave (&retro_mouse_id_type, type);
  }

  return retro_mouse_id_type;
}

GType
retro_lightgun_id_get_type (void)
{
  static volatile gsize retro_lightgun_id_type = 0;

  if (g_once_init_enter (&retro_lightgun_id_type)) {
    static const GEnumValue values[] = {
      { RETRO_LIGHTGUN_ID_X, "RETRO_LIGHTGUN_ID_X", "x" },
      { RETRO_LIGHTGUN_ID_Y, "RETRO_LIGHTGUN_ID_Y", "y" },
      { RETRO_LIGHTGUN_ID_TRIGGER, "RETRO_LIGHTGUN_ID_TRIGGER", "trigger" },
      { RETRO_LIGHTGUN_ID_CURSOR, "RETRO_LIGHTGUN_ID_CURSOR", "cursor" },
      { RETRO_LIGHTGUN_ID_TURBO, "RETRO_LIGHTGUN_ID_TURBO", "turbo" },
      { RETRO_LIGHTGUN_ID_PAUSE, "RETRO_LIGHTGUN_ID_PAUSE", "pause" },
      { RETRO_LIGHTGUN_ID_START, "RETRO_LIGHTGUN_ID_START", "start" },
      { 0, NULL, NULL },
    };
    GType type;

    type = g_enum_register_static ("RetroLightgunId", values);

    g_once_init_leave (&retro_lightgun_id_type, type);
  }

  return retro_lightgun_id_type;
}

GType
retro_analog_id_get_type (void)
{
  static volatile gsize retro_analog_id_type = 0;

  if (g_once_init_enter (&retro_analog_id_type)) {
    static const GEnumValue values[] = {
      { RETRO_ANALOG_ID_X, "RETRO_ANALOG_ID_X", "x" },
      { RETRO_ANALOG_ID_Y, "RETRO_ANALOG_ID_Y", "y" },
      { 0, NULL, NULL },
    };
    GType type;

    type = g_enum_register_static ("RetroAnalogId", values);

    g_once_init_leave (&retro_analog_id_type, type);
  }

  return retro_analog_id_type;
}

GType
retro_analog_index_get_type (void)
{
  static volatile gsize retro_analog_index_type = 0;

  if (g_once_init_enter (&retro_analog_index_type)) {
    static const GEnumValue values[] = {
      { RETRO_ANALOG_INDEX_LEFT, "RETRO_ANALOG_INDEX_LEFT", "left" },
      { RETRO_ANALOG_INDEX_RIGHT, "RETRO_ANALOG_INDEX_RIGHT", "right" },
      { 0, NULL, NULL },
    };
    GType type;

    type = g_enum_register_static ("RetroAnalogIndex", values);

    g_once_init_leave (&retro_analog_index_type, type);
  }

  return retro_analog_index_type;
}

GType
retro_pointer_id_get_type (void)
{
  static volatile gsize retro_pointer_id_type = 0;

  if (g_once_init_enter (&retro_pointer_id_type)) {
    static const GEnumValue values[] = {
      { RETRO_POINTER_ID_X, "RETRO_POINTER_ID_X", "x" },
      { RETRO_POINTER_ID_Y, "RETRO_POINTER_ID_Y", "y" },
      { RETRO_POINTER_ID_PRESSED, "RETRO_POINTER_ID_PRESSED", "pressed" },
      { 0, NULL, NULL },
    };
    GType type;

    type = g_enum_register_static ("RetroPointerId", values);

    g_once_init_leave (&retro_pointer_id_type, type);
  }

  return retro_pointer_id_type;
}
