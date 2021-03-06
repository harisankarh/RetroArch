/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2013 - Jason Fetters
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#import "RAInputResponder.h"

#include <unistd.h>
#include "input/input_common.h"
#include "general.h"
#include "driver.h"

#ifdef WIIMOTE
extern const rarch_joypad_driver_t ios_joypad;
static const rarch_joypad_driver_t* const g_joydriver = &ios_joypad;
#else
static const rarch_joypad_driver_t* const g_joydriver = 0;
#endif

static const struct rarch_key_map rarch_key_map_hidusage[];

static RAInputResponder* g_input_driver;

// Non-exported helpers
static bool ios_key_pressed(enum retro_key key)
{
   if ((int)key >= 0 && key < RETROK_LAST)
   {
      return [g_input_driver isKeyPressed:input_translate_rk_to_keysym(key)];
   }
   
   return false;
}

static bool ios_is_pressed(unsigned port_num, const struct retro_keybind *key)
{
   return ios_key_pressed(key->key) || input_joypad_pressed(g_joydriver, port_num, key);
}

// Exported input driver
static void *ios_input_init(void)
{
   input_init_keyboard_lut(rarch_key_map_hidusage);
   g_input_driver = [RAInputResponder new];
   return (void*)-1;
}

static void ios_input_poll(void *data)
{
   [g_input_driver poll];
   input_joypad_poll(g_joydriver);
}

static int16_t ios_input_state(void *data, const struct retro_keybind **binds, unsigned port, unsigned device, unsigned index, unsigned id)
{
   switch (device)
   {
      case RETRO_DEVICE_JOYPAD:
         return (id < RARCH_BIND_LIST_END) ? ios_is_pressed(port, &binds[port][id]) : false;

      case RARCH_DEVICE_POINTER_SCREEN:
      {
         const touch_data_t* touch = [g_input_driver getTouchDataAtIndex:index];

         switch (id)
         {
            case RETRO_DEVICE_ID_POINTER_X: return touch ? touch->full_x : 0;
            case RETRO_DEVICE_ID_POINTER_Y: return touch ? touch->full_y : 0;
            case RETRO_DEVICE_ID_POINTER_PRESSED: return touch ? 1 : 0;
         }
         
         return 0;
      }

      default:
         return 0;
   }
}

static bool ios_bind_button_pressed(void *data, int key)
{
   const struct retro_keybind *binds = g_settings.input.binds[0];
   return (key >= 0 && key < RARCH_BIND_LIST_END) ? ios_is_pressed(0, &binds[key]) : false;
}

static void ios_input_free_input(void *data)
{
   (void)data;
   g_input_driver = nil;
}

const input_driver_t input_ios = {
   ios_input_init,
   ios_input_poll,
   ios_input_state,
   ios_bind_button_pressed,
   ios_input_free_input,
   "ios_input",
};

// Key table
#include "keycode.h"
static const struct rarch_key_map rarch_key_map_hidusage[] = {
   { KEY_Left, RETROK_LEFT },
   { KEY_Right, RETROK_RIGHT },
   { KEY_Up, RETROK_UP },
   { KEY_Down, RETROK_DOWN },
   { KEY_Enter, RETROK_RETURN },
   { KEY_Tab, RETROK_TAB },
   { KEY_Insert, RETROK_INSERT },
   { KEY_Delete, RETROK_DELETE },
   { KEY_RightShift, RETROK_RSHIFT },
   { KEY_LeftShift, RETROK_LSHIFT },
   { KEY_RightControl, RETROK_RCTRL },
   { KEY_LeftControl, RETROK_LCTRL },
   { KEY_End, RETROK_END },
   { KEY_Home, RETROK_HOME },
   { KEY_PageDown, RETROK_PAGEDOWN },
   { KEY_PageUp, RETROK_PAGEUP },
   { KEY_RightAlt, RETROK_RALT },
   { KEY_LeftAlt, RETROK_LALT },
   { KEY_Space, RETROK_SPACE },
   { KEY_Escape, RETROK_ESCAPE },
   { KEY_Delete, RETROK_BACKSPACE },
   { KP_Enter, RETROK_KP_ENTER },
   { KP_Add, RETROK_KP_PLUS },
   { KP_Subtract, RETROK_KP_MINUS },
   { KP_Multiply, RETROK_KP_MULTIPLY },
   { KP_Divide, RETROK_KP_DIVIDE },
   { KEY_Grave, RETROK_BACKQUOTE },
   { KEY_Pause, RETROK_PAUSE },
   { KP_0, RETROK_KP0 },
   { KP_1, RETROK_KP1 },
   { KP_2, RETROK_KP2 },
   { KP_3, RETROK_KP3 },
   { KP_4, RETROK_KP4 },
   { KP_5, RETROK_KP5 },
   { KP_6, RETROK_KP6 },
   { KP_7, RETROK_KP7 },
   { KP_8, RETROK_KP8 },
   { KP_9, RETROK_KP9 },
   { KEY_0, RETROK_0 },
   { KEY_1, RETROK_1 },
   { KEY_2, RETROK_2 },
   { KEY_3, RETROK_3 },
   { KEY_4, RETROK_4 },
   { KEY_5, RETROK_5 },
   { KEY_6, RETROK_6 },
   { KEY_7, RETROK_7 },
   { KEY_8, RETROK_8 },
   { KEY_9, RETROK_9 },
   { KEY_F1, RETROK_F1 },
   { KEY_F2, RETROK_F2 },
   { KEY_F3, RETROK_F3 },
   { KEY_F4, RETROK_F4 },
   { KEY_F5, RETROK_F5 },
   { KEY_F6, RETROK_F6 },
   { KEY_F7, RETROK_F7 },
   { KEY_F8, RETROK_F8 },
   { KEY_F9, RETROK_F9 },
   { KEY_F10, RETROK_F10 },
   { KEY_F11, RETROK_F11 },
   { KEY_F12, RETROK_F12 },
   { KEY_A, RETROK_a },
   { KEY_B, RETROK_b },
   { KEY_C, RETROK_c },
   { KEY_D, RETROK_d },
   { KEY_E, RETROK_e },
   { KEY_F, RETROK_f },
   { KEY_G, RETROK_g },
   { KEY_H, RETROK_h },
   { KEY_I, RETROK_i },
   { KEY_J, RETROK_j },
   { KEY_K, RETROK_k },
   { KEY_L, RETROK_l },
   { KEY_M, RETROK_m },
   { KEY_N, RETROK_n },
   { KEY_O, RETROK_o },
   { KEY_P, RETROK_p },
   { KEY_Q, RETROK_q },
   { KEY_R, RETROK_r },
   { KEY_S, RETROK_s },
   { KEY_T, RETROK_t },
   { KEY_U, RETROK_u },
   { KEY_V, RETROK_v },
   { KEY_W, RETROK_w },
   { KEY_X, RETROK_x },
   { KEY_Y, RETROK_y },
   { KEY_Z, RETROK_z },
   { 0, RETROK_UNKNOWN }
};
