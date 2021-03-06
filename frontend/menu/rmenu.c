/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2013 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2013 - Daniel De Matteis
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

#include "rmenu.h"
#include "utils/file_browser.h"
#include "utils/menu_stack.h"

#if defined(__CELLOS_LV2__)
#include <sdk_version.h>

#if(CELL_SDK_VERSION > 0x340000)
#include <sysutil/sysutil_bgmplayback.h>
#endif

#endif

#include "../../console/rarch_console.h"
#include "menu_settings.h"

#include "../../gfx/image.h"

#include "../../gfx/gfx_common.h"
#include "../../gfx/gfx_context.h"

#include "../../file.h"
#include "../../driver.h"
#include "../../general.h"

#define EXT_IMAGES "png|PNG|jpg|JPG|JPEG|jpeg"
#define EXT_SHADERS "cg|CG"
#define EXT_CGP_PRESETS "cgp|CGP"
#define EXT_INPUT_PRESETS "cfg|CFG"

enum {
   MENU_ITEM_LOAD_STATE = 0,
   MENU_ITEM_SAVE_STATE,
   MENU_ITEM_KEEP_ASPECT_RATIO,
   MENU_ITEM_OVERSCAN_AMOUNT,
   MENU_ITEM_ORIENTATION,
#ifdef HAVE_FBO
   MENU_ITEM_SCALE_FACTOR,
#endif
   MENU_ITEM_RESIZE_MODE,
   MENU_ITEM_FRAME_ADVANCE,
   MENU_ITEM_SCREENSHOT_MODE,
   MENU_ITEM_RESET,
   MENU_ITEM_RETURN_TO_GAME,
   MENU_ITEM_RETURN_TO_MENU,
   MENU_ITEM_CHANGE_LIBRETRO,
#ifdef HAVE_MULTIMAN
   MENU_ITEM_RETURN_TO_MULTIMAN,
#endif
   MENU_ITEM_QUIT_RARCH,
   MENU_ITEM_LAST
};

static rmenu_state_t rmenu_state;

static bool set_libretro_core_as_launch;

filebrowser_t *browser;
filebrowser_t *tmpBrowser;
unsigned currently_selected_controller_menu = 0;

static const struct retro_keybind _rmenu_nav_binds[] = {
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_UP) | (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_UP), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN) | (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_DOWN), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT) | (1ULL << RARCH_ANALOG_LEFT_X_DPAD_LEFT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT) | (1ULL << RARCH_ANALOG_LEFT_X_DPAD_RIGHT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_UP), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_LEFT_Y_DPAD_DOWN), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_LEFT_X_DPAD_LEFT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_LEFT_X_DPAD_RIGHT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_UP), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_RIGHT_Y_DPAD_DOWN), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_LEFT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RARCH_ANALOG_RIGHT_X_DPAD_RIGHT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_B), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_A), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_X), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_Y), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_START), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_SELECT), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_L), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_R), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_L2), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_R2), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_L3), 0 },
   { 0, 0, NULL, (enum retro_key)0, (1ULL << RETRO_DEVICE_ID_JOYPAD_R3), 0 },
};

static const struct retro_keybind *rmenu_nav_binds[] = {
   _rmenu_nav_binds
};

enum
{
   RMENU_DEVICE_NAV_UP = 0,
   RMENU_DEVICE_NAV_DOWN,
   RMENU_DEVICE_NAV_LEFT,
   RMENU_DEVICE_NAV_RIGHT,
   RMENU_DEVICE_NAV_UP_ANALOG_L,
   RMENU_DEVICE_NAV_DOWN_ANALOG_L,
   RMENU_DEVICE_NAV_LEFT_ANALOG_L,
   RMENU_DEVICE_NAV_RIGHT_ANALOG_L,
   RMENU_DEVICE_NAV_UP_ANALOG_R,
   RMENU_DEVICE_NAV_DOWN_ANALOG_R,
   RMENU_DEVICE_NAV_LEFT_ANALOG_R,
   RMENU_DEVICE_NAV_RIGHT_ANALOG_R,
   RMENU_DEVICE_NAV_B,
   RMENU_DEVICE_NAV_A,
   RMENU_DEVICE_NAV_X,
   RMENU_DEVICE_NAV_Y,
   RMENU_DEVICE_NAV_START,
   RMENU_DEVICE_NAV_SELECT,
   RMENU_DEVICE_NAV_L1,
   RMENU_DEVICE_NAV_R1,
   RMENU_DEVICE_NAV_L2,
   RMENU_DEVICE_NAV_R2,
   RMENU_DEVICE_NAV_L3,
   RMENU_DEVICE_NAV_R3,
   RMENU_DEVICE_NAV_LAST
};

const char drive_mappings[][32] = {
#if defined(_XBOX1)
   "C:",
   "D:",
   "E:",
   "F:",
   "G:"
#elif defined(__CELLOS_LV2__)
   "/app_home/",
   "/dev_hdd0/",
   "/dev_hdd1/",
   "/host_root/"
#endif
};

#if defined__CELLOS_LV2__
unsigned char drive_mapping_idx = 1;
#elif defined(_XBOX1)
unsigned char drive_mapping_idx = 2;
#else
unsigned char drive_mapping_idx = 0;
#endif

static const char *menu_drive_mapping_previous(void)
{
   if(drive_mapping_idx > 0)
      drive_mapping_idx--;
   return drive_mappings[drive_mapping_idx];
}

static const char *menu_drive_mapping_next(void)
{
   size_t arr_size = sizeof(drive_mappings) / sizeof(drive_mappings[0]);
   if((drive_mapping_idx + 1) < arr_size)
      drive_mapping_idx++;
   return drive_mappings[drive_mapping_idx];
}

#if defined(_XBOX1)
#define HARDCODE_FONT_SIZE 21

#define POSITION_X m_menuMainRomListPos_x
#define POSITION_X_CENTER (m_menuMainRomListPos_x + 350)
#define POSITION_Y_START m_menuMainRomListPos_y
#define POSITION_Y_BEGIN (POSITION_Y_START + POSITION_Y_INCREMENT)
#define POSITION_Y_INCREMENT 20
#define COMMENT_Y_POSITION (ypos - ((POSITION_Y_INCREMENT/2) * 3))
#define COMMENT_TWO_Y_POSITION (ypos - ((POSITION_Y_INCREMENT/2) * 1))

#define MSG_QUEUE_X_POSITION POSITION_X
#define MSG_QUEUE_Y_POSITION (ypos - ((POSITION_Y_INCREMENT/2) * 7) + 5)
#define MSG_QUEUE_FONT_SIZE HARDCODE_FONT_SIZE

#define MSG_PREV_NEXT_Y_POSITION 24

#define CURRENT_PATH_Y_POSITION (m_menuMainRomListPos_y - ((POSITION_Y_INCREMENT/2)))
#define CURRENT_PATH_FONT_SIZE 21

#define FONT_SIZE 21 

#define NUM_ENTRY_PER_PAGE 12
#elif defined(__CELLOS_LV2__)
#define HARDCODE_FONT_SIZE 0.91f
#define POSITION_X 0.09f
#define POSITION_X_CENTER 0.5f
#define POSITION_Y_START 0.17f
#define POSITION_Y_INCREMENT 0.035f
#define POSITION_Y_BEGIN (POSITION_Y_START + POSITION_Y_INCREMENT)
#define COMMENT_TWO_Y_POSITION 0.91f
#define COMMENT_Y_POSITION 0.82f

#define MSG_QUEUE_X_POSITION g_settings.video.msg_pos_x
#define MSG_QUEUE_Y_POSITION 0.76f
#define MSG_QUEUE_FONT_SIZE 1.03f

#define MSG_PREV_NEXT_Y_POSITION 0.03f
#define CURRENT_PATH_Y_POSITION 0.15f

#define NUM_ENTRY_PER_PAGE 15
#endif

static void menu_set_default_pos(rmenu_default_positions_t *position)
{
   position->x_position = POSITION_X;
   position->x_position_center = POSITION_X_CENTER;
   position->y_position = POSITION_Y_BEGIN;
   position->comment_y_position = COMMENT_Y_POSITION;
   position->y_position_increment = POSITION_Y_INCREMENT;
   position->starting_y_position = POSITION_Y_START;
   position->comment_two_y_position = COMMENT_TWO_Y_POSITION;
   position->font_size = HARDCODE_FONT_SIZE;
   position->msg_queue_x_position = MSG_QUEUE_X_POSITION;
   position->msg_queue_y_position = MSG_QUEUE_Y_POSITION;
   position->msg_queue_font_size= MSG_QUEUE_FONT_SIZE;
   position->msg_prev_next_y_position = MSG_PREV_NEXT_Y_POSITION;
   position->current_path_y_position = CURRENT_PATH_Y_POSITION;
   position->entries_per_page = NUM_ENTRY_PER_PAGE;
#if defined(_XBOX1)
   position->current_path_font_size = CURRENT_PATH_FONT_SIZE;
   position->variable_font_size = FONT_SIZE;
   position->core_msg_x_position = position->x_position;
   position->core_msg_y_position = position->msg_prev_next_y_position + 0.01f;
   position->core_msg_font_size = position->font_size;
#elif defined(__CELLOS_LV2__)
   position->current_path_font_size = g_settings.video.font_size;
   position->variable_font_size = g_settings.video.font_size;
   position->core_msg_x_position = 0.3f;
   position->core_msg_y_position = 0.06f;
   position->core_msg_font_size = COMMENT_Y_POSITION;
#endif
}

/*============================================================
  EVENT CALLBACKS (AND RELATED)
  ============================================================ */

static void populate_setting_item(void *data, unsigned input)
{
   item *current_item = (item*)data;
   char fname[PATH_MAX];
   (void)fname;

   unsigned currentsetting = input;
   current_item->enum_id = input;

   switch(currentsetting)
   {
#ifdef __CELLOS_LV2__
      case SETTING_CHANGE_RESOLUTION:
         {
            unsigned width = gfx_ctx_get_resolution_width(g_extern.console.screen.resolutions.list[g_extern.console.screen.resolutions.current.idx]);
            unsigned height = gfx_ctx_get_resolution_height(g_extern.console.screen.resolutions.list[g_extern.console.screen.resolutions.current.idx]);
            snprintf(current_item->text, sizeof(current_item->text), "Resolution");
            snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%dx%d", width, height);
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Change the display resolution.");
         }
         break;
      case SETTING_PAL60_MODE:
         snprintf(current_item->text, sizeof(current_item->text), "PAL60 Mode");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_PAL_TEMPORAL_ENABLE)) ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_PAL_TEMPORAL_ENABLE)) ? "INFO - [PAL60 Mode] is set to 'ON'.\nconverts frames from 60Hz to 50Hz." : "INFO - [PAL60 Mode is set to 'OFF'.\nframes are not converted.");
         break;
#endif
#if defined(HAVE_CG) || defined(HAVE_HLSL) || defined(HAVE_GLSL)
      case SETTING_SHADER_PRESETS:
         snprintf(current_item->text, sizeof(current_item->text), "Shader Presets (CGP)");
         fill_pathname_base(fname, g_extern.file_state.cgp_path, sizeof(fname));
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), fname);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Select a [CG Preset] script.");
         break;
      case SETTING_SHADER:
         fill_pathname_base(fname, g_settings.video.cg_shader_path, sizeof(fname));
         snprintf(current_item->text, sizeof(current_item->text), "Shader #1");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%s", fname);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Select a shader as [Shader #1]. NOTE: Some shaders might be\ntoo slow at 1080p. If you experience any slowdown, try another shader.");
         break;
      case SETTING_SHADER_2:
         fill_pathname_base(fname, g_settings.video.second_pass_shader, sizeof(fname));
         snprintf(current_item->text, sizeof(current_item->text), "Shader #2");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%s", fname);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Select a shader as [Shader #2]. NOTE: Some shaders might be\ntoo slow at 1080p. If you experience any slowdown, try another shader.");
         break;
#endif
      case SETTING_EMU_SKIN:
         fill_pathname_base(fname, g_extern.console.menu_texture_path, sizeof(fname));
         snprintf(current_item->text, sizeof(current_item->text), "Menu Skin");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%s", fname);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Select a skin for the menu.");
         break;
      case SETTING_EMU_LOW_RAM_MODE_ENABLE:
         snprintf(current_item->text, sizeof(current_item->text), "Low RAM Mode");
         if (g_extern.lifecycle_mode_state & (1ULL <<MODE_MENU_LOW_RAM_MODE_ENABLE) ||
               g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_LOW_RAM_MODE_ENABLE_PENDING))
         {
            snprintf(current_item->setting_text, sizeof(current_item->setting_text), "ON");
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Will load skin at startup.");
         }
         else
         {
            snprintf(current_item->setting_text, sizeof(current_item->setting_text), "OFF");
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Will not load skin at startup to save up on RAM.");
         }
         break;
      case SETTING_FONT_SIZE:
         snprintf(current_item->text, sizeof(current_item->text), "Font Size");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%f", g_settings.video.font_size);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Increase or decrease the [Font Size].");
         break;
      case SETTING_KEEP_ASPECT_RATIO:
         snprintf(current_item->text, sizeof(current_item->text), "Aspect Ratio");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), aspectratio_lut[g_settings.video.aspect_ratio_idx].name);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Select an [Aspect Ratio].");
         break;
      case SETTING_HW_TEXTURE_FILTER:
         snprintf(current_item->text, sizeof(current_item->text), "Hardware filtering #1");
         if(g_settings.video.smooth)
            snprintf(current_item->setting_text, sizeof(current_item->setting_text), "Bilinear");
         else
            snprintf(current_item->setting_text, sizeof(current_item->setting_text), "Point");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Hardware filtering #1 is set to [%s].", current_item->setting_text);
         break;
#ifdef HAVE_FBO
      case SETTING_HW_TEXTURE_FILTER_2:
         snprintf(current_item->text, sizeof(current_item->text), "Hardware filtering #2");
         if(g_settings.video.second_pass_smooth)
            snprintf(current_item->setting_text, sizeof(current_item->setting_text), "Bilinear");
         else
            snprintf(current_item->setting_text, sizeof(current_item->setting_text), "Point");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Hardware filtering #2 is set to [%s].", current_item->setting_text);
         break;
      case SETTING_SCALE_ENABLED:
         snprintf(current_item->text, sizeof(current_item->text), "Custom Scaling/Dual Shaders");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_settings.video.render_to_texture ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), g_settings.video.render_to_texture ? "INFO - [Custom Scaling] is set to 'ON' - 2x shaders will look much\nbetter, and you can select a shader for [Shader #2]." : "INFO - [Custom Scaling] is set to 'OFF'.");
         break;
      case SETTING_SCALE_FACTOR:
         snprintf(current_item->text, sizeof(current_item->text), "Custom Scaling Factor");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%fx (X) / %fx (Y)", g_settings.video.fbo.scale_x, g_settings.video.fbo.scale_y);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Custom Scaling Factor] is set to: '%fx (X) / %fx (Y)'.", g_settings.video.fbo.scale_x, g_settings.video.fbo.scale_y);
         break;
#endif
#ifdef _XBOX1
      case SETTING_FLICKER_FILTER:
         snprintf(current_item->text, sizeof(current_item->text), "Flicker Filter");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%d", g_extern.console.screen.flicker_filter_index);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Toggle the [Flicker Filter].");
         break;
      case SETTING_SOFT_DISPLAY_FILTER:
         snprintf(current_item->text, sizeof(current_item->text), "Soft Display Filter");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_SOFT_FILTER_ENABLE)) ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Toggle the [Soft Display Filter].");
         break;
#endif
      case SETTING_HW_OVERSCAN_AMOUNT:
         snprintf(current_item->text, sizeof(current_item->text), "Overscan");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%f", g_extern.console.screen.overscan_amount);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Adjust or decrease [Overscan]. Set this to higher than 0.000\nif the screen doesn't fit on your TV/monitor.");
         break;
      case SETTING_REFRESH_RATE:
         snprintf(current_item->text, sizeof(current_item->text), "Refresh rate");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%fHz", g_settings.video.refresh_rate);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Adjust or decrease [Refresh Rate].");
         break;
      case SETTING_THROTTLE_MODE:
         snprintf(current_item->text, sizeof(current_item->text), "Throttle Mode");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_THROTTLE_ENABLE)) ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_THROTTLE_ENABLE)) ? "INFO - [Throttle Mode] is 'ON' - Vsync is enabled." : "INFO - [Throttle Mode] is 'OFF' - Vsync is disabled.");
         break;
      case SETTING_TRIPLE_BUFFERING:
         snprintf(current_item->text, sizeof(current_item->text), "Triple Buffering");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_TRIPLE_BUFFERING_ENABLE)) ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_TRIPLE_BUFFERING_ENABLE)) ? "INFO - [Triple Buffering] is set to 'ON'." : "INFO - [Triple Buffering] is set to 'OFF'.");
         break;
      case SETTING_ENABLE_SCREENSHOTS:
         snprintf(current_item->text, sizeof(current_item->text), "Screenshot Option");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_SCREENSHOTS_ENABLE)) ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Screenshots feature is set to '%s'.", (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_SCREENSHOTS_ENABLE)) ? "ON" : "OFF");
         break;
#if defined(HAVE_CG) || defined(HAVE_HLSL) || defined(HAVE_GLSL)
      case SETTING_APPLY_SHADER_PRESET_ON_STARTUP:
         snprintf(current_item->text, sizeof(current_item->text), "APPLY SHADER PRESET ON STARTUP");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Automatically load the currently selected [CG Preset] file on startup.");
         break;
#endif
      case SETTING_DEFAULT_VIDEO_ALL:
         snprintf(current_item->text, sizeof(current_item->text), "DEFAULTS");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set all [General Video Settings] back to their 'DEFAULT' values.");
         break;
      case SETTING_SOUND_MODE:
         snprintf(current_item->text, sizeof(current_item->text), "Sound Output");
         switch(g_extern.console.sound.mode)
         {
            case SOUND_MODE_NORMAL:
               snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Sound Output] is set to 'Normal'.");
               snprintf(current_item->setting_text, sizeof(current_item->setting_text), "Normal");
               break;
#ifdef HAVE_RSOUND
            case SOUND_MODE_RSOUND:
               snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Sound Output] is set to 'RSound'." );
               snprintf(current_item->setting_text, sizeof(current_item->setting_text), "RSound");
               break;
#endif
#ifdef HAVE_HEADSET
            case SOUND_MODE_HEADSET:
               snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Sound Output] is set to 'USB/Bluetooth Headset'.");
               snprintf(current_item->setting_text, sizeof(current_item->setting_text), "USB/Bluetooth Headset");
               break;
#endif
            default:
               break;
         }
         break;
#ifdef HAVE_RSOUND
      case SETTING_RSOUND_SERVER_IP_ADDRESS:
         snprintf(current_item->text, sizeof(current_item->text), "RSound Server IP Address");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_settings.audio.device);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Enter the IP Address of the [RSound Audio Server]. IP address\nmust be an IPv4 32-bits address, eg: '192.168.1.7'.");
         break;
#endif
      case SETTING_DEFAULT_AUDIO_ALL:
         snprintf(current_item->text, sizeof(current_item->text), "DEFAULTS");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set all [General Audio Settings] back to their 'DEFAULT' values.");
         break;
      case SETTING_RESAMPLER_TYPE:
         snprintf(current_item->text, sizeof(current_item->text), "Sound resampler");
#ifdef HAVE_SINC
         if (strstr(g_settings.audio.resampler, "sinc"))
         {
            snprintf(current_item->setting_text, sizeof(current_item->setting_text), "Sinc");
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Sinc resampler] - slightly slower but better sound quality at high frequencies.");
         }
         else
#endif
         {
            snprintf(current_item->setting_text, sizeof(current_item->setting_text), "Hermite");
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Hermite resampler] - faster but less accurate at high frequencies.");
         }
         break;
      case SETTING_EMU_CURRENT_SAVE_STATE_SLOT:
         snprintf(current_item->text, sizeof(current_item->text), "Current save state slot");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%d", g_extern.state_slot);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set the currently selected savestate slot.");
         break;
         /* emu-specific */
      case SETTING_EMU_SHOW_DEBUG_INFO_MSG:
         snprintf(current_item->text, sizeof(current_item->text), "Debug info messages");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), (g_extern.lifecycle_mode_state & (1ULL << MODE_FPS_DRAW)) ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Show onscreen debug messages.");
         break;
      case SETTING_EMU_SHOW_INFO_MSG:
         snprintf(current_item->text, sizeof(current_item->text), "Info messages");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW)) ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Show onscreen info messages in the menu.");
         break;
      case SETTING_EMU_REWIND_ENABLED:
         snprintf(current_item->text, sizeof(current_item->text), "Rewind option");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_settings.rewind_enable ? "ON" : "OFF");
         if(g_settings.rewind_enable)
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Rewind] feature is set to 'ON'.");
         else
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Rewind] feature is set to 'OFF'.");
         break;
      case SETTING_EMU_REWIND_GRANULARITY:
         snprintf(current_item->text, sizeof(current_item->text), "Rewind granularity");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%d", g_settings.rewind_granularity);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set the amount of frames to 'rewind'.\nIncrease this to lower CPU usage.");
         break;
      case SETTING_RARCH_DEFAULT_EMU:
         snprintf(current_item->text, sizeof(current_item->text), "Default libretro core");
         fill_pathname_base(fname, g_settings.libretro, sizeof(fname));
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%s", fname);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Select a default libretro core to launch at start-up.");
         break;
      case SETTING_QUIT_RARCH:
         snprintf(current_item->text, sizeof(current_item->text), "Quit RetroArch and save settings ");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Quits RetroArch and saves the settings.");
         break;
      case SETTING_EMU_AUDIO_MUTE:
         snprintf(current_item->text, sizeof(current_item->text), "Mute Audio");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_extern.audio_data.mute ? "ON" : "OFF");
         if(g_extern.audio_data.mute)
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Audio Mute] is set to 'ON'. The game audio will be muted.");
         else
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Audio Mute] is set to 'OFF'.");
         break;
#ifdef _XBOX1
      case SETTING_EMU_AUDIO_SOUND_VOLUME_LEVEL:
         snprintf(current_item->text, sizeof(current_item->text), "Volume Level");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_extern.console.sound.volume_level ? "Loud" : "Normal");
         if(g_extern.audio_data.mute)
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Volume Level] is set to 'Loud'");
         else
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Volume Level' is set to 'Normal'.");
         break;
#endif
      case SETTING_ENABLE_CUSTOM_BGM:
         snprintf(current_item->text, sizeof(current_item->text), "Custom BGM Option");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), (g_extern.lifecycle_mode_state & (1ULL << MODE_AUDIO_CUSTOM_BGM_ENABLE)) ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Custom BGM] is set to '%s'.", (g_extern.lifecycle_mode_state & (1ULL << MODE_AUDIO_CUSTOM_BGM_ENABLE)) ? "ON" : "OFF");
         break;
      case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
         snprintf(current_item->text, sizeof(current_item->text), "Startup ROM Directory");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_extern.console.main_wrap.default_rom_startup_dir);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set the default [Startup ROM directory]. NOTE: You will have to\nrestart the emulator for this change to have any effect.");
         break;
      case SETTING_PATH_SAVESTATES_DIRECTORY:
         snprintf(current_item->text, sizeof(current_item->text), "Savestate Directory");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_extern.console.main_wrap.default_savestate_dir);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set the default path where all the savestate files will be saved to.");
         break;
      case SETTING_PATH_SRAM_DIRECTORY:
         snprintf(current_item->text, sizeof(current_item->text), "SRAM Directory");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_extern.console.main_wrap.default_sram_dir);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set the default SRAM (SaveRAM) directory path. All the\nbattery backup saves will be stored in this directory.");
         break;
#ifdef HAVE_XML
      case SETTING_PATH_CHEATS:
         snprintf(current_item->text, sizeof(current_item->text), "Cheatfile Directory");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_settings.cheat_database);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set the default [Cheatfile directory] path. All CHT (cheat) files\nwill be stored here.");
         break;
#endif
      case SETTING_PATH_SYSTEM:
         snprintf(current_item->text, sizeof(current_item->text), "System Directory");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_settings.system_directory);
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set the default [System directory] path. System files like\nBIOS files, etc. will be stored here.");
         break;
      case SETTING_ENABLE_SRAM_PATH:
         snprintf(current_item->text, sizeof(current_item->text), "Custom SRAM Dir Enable");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), (g_extern.lifecycle_mode_state & (1ULL << MODE_LOAD_GAME_SRAM_DIR_ENABLE)) ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Custom SRAM Dir Path] is set to '%s'.", (g_extern.lifecycle_mode_state & (1ULL << MODE_LOAD_GAME_SRAM_DIR_ENABLE)) ? "ON" : "OFF");
         break;
      case SETTING_ENABLE_STATE_PATH:
         snprintf(current_item->text, sizeof(current_item->text), "Custom Savestate Dir Enable");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), (g_extern.lifecycle_mode_state & (1ULL << MODE_LOAD_GAME_STATE_DIR_ENABLE)) ? "ON" : "OFF");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [Custom Savestate Dir Path] is set to '%s'.", (g_extern.lifecycle_mode_state & (1ULL << MODE_LOAD_GAME_STATE_DIR_ENABLE)) ? "ON" : "OFF");
         break;
      case SETTING_CONTROLS_SCHEME:
         snprintf(current_item->text, sizeof(current_item->text), "Control Scheme Preset");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Input scheme preset [%s] is selected.", g_extern.file_state.input_cfg_path);
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), g_extern.file_state.input_cfg_path);
         break;
      case SETTING_CONTROLS_NUMBER:
         snprintf(current_item->text, sizeof(current_item->text), "Controller No");
         snprintf(current_item->comment, sizeof(current_item->comment), "Controller %d is currently selected.", currently_selected_controller_menu+1);
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "%d", currently_selected_controller_menu+1);
         break;
      case SETTING_DPAD_EMULATION:
         snprintf(current_item->text, sizeof(current_item->text), "D-Pad Emulation");
         switch(g_settings.input.dpad_emulation[currently_selected_controller_menu])
         {
            case ANALOG_DPAD_NONE:
               snprintf(current_item->comment, sizeof(current_item->comment), "[%s] from Controller %d is mapped to D-pad.", "None", currently_selected_controller_menu+1);
               snprintf(current_item->setting_text, sizeof(current_item->setting_text), "None");
               break;
            case ANALOG_DPAD_LSTICK:
               snprintf(current_item->comment, sizeof(current_item->comment), "[%s] from Controller %d is mapped to D-pad.", "Left Stick", currently_selected_controller_menu+1);
               snprintf(current_item->setting_text, sizeof(current_item->setting_text), "Left Stick");
               break;
            case ANALOG_DPAD_RSTICK:
               snprintf(current_item->comment, sizeof(current_item->comment), "[%s] from Controller %d is mapped to D-pad.", "Right Stick", currently_selected_controller_menu+1);
               snprintf(current_item->setting_text, sizeof(current_item->setting_text), "Right Stick");
               break;
         }
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_B:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_Y:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_SELECT:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_START:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_UP:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_DOWN:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_LEFT:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_RIGHT:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_A:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_X:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_L:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_R:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_L2:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_R2:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_L3:
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_R3:
         {
            unsigned id = currentsetting - FIRST_CONTROL_BIND;
            struct platform_bind key_label;
            strlcpy(key_label.desc, "Unknown", sizeof(key_label.desc));
            key_label.joykey = g_settings.input.binds[currently_selected_controller_menu][id].joykey;

            if (driver.input->set_keybinds)
               driver.input->set_keybinds(&key_label, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
            snprintf(current_item->text, sizeof(current_item->text), g_settings.input.binds[currently_selected_controller_menu][id].desc);
            snprintf(current_item->comment, sizeof(current_item->comment), "INFO - [%s] is mapped to action:\n[%s].", current_item->text, key_label.desc);
            snprintf(current_item->setting_text, sizeof(current_item->setting_text), key_label.desc);
         }
         break;
      case SETTING_CONTROLS_SAVE_CUSTOM_CONTROLS:
         snprintf(current_item->text, sizeof(current_item->text), "SAVE CUSTOM CONTROLS");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Save the [Custom Controls] settings to file.");
         break;
      case SETTING_CONTROLS_DEFAULT_ALL:
         snprintf(current_item->text, sizeof(current_item->text), "DEFAULTS");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "IFNO - Set all [Controls] back to their 'DEFAULT' values.");
         break;
      case SETTING_EMU_VIDEO_DEFAULT_ALL:
         snprintf(current_item->text, sizeof(current_item->text), "DEFAULTS");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set [all RetroArch Video settings] back to their 'DEFAULT' values.");
         break;
      case SETTING_EMU_AUDIO_DEFAULT_ALL:
         snprintf(current_item->text, sizeof(current_item->text), "DEFAULTS");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set all [RetroArch Audio settings] back to their 'DEFAULT' values.");
         break;
      case SETTING_PATH_DEFAULT_ALL:
         snprintf(current_item->text, sizeof(current_item->text), "DEFAULTS");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set all [Path settings] back to their 'DEFAULT' values.");
         break;
      case SETTING_EMU_DEFAULT_ALL:
         snprintf(current_item->text, sizeof(current_item->text), "DEFAULTS");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Set [all RetroArch settings] back to their 'DEFAULT' values.");
         break;
#if defined(HAVE_CG) || defined(HAVE_HLSL) || defined(HAVE_GLSL)
      case SETTING_SAVE_SHADER_PRESET:
         snprintf(current_item->text, sizeof(current_item->text), "SAVE SETTINGS AS CGP PRESET");
         snprintf(current_item->setting_text, sizeof(current_item->setting_text), "");
         snprintf(current_item->comment, sizeof(current_item->comment), "INFO - Save the current video settings to a [CG Preset] (CGP) file.");
         break;
#endif
      default:
         break;
   }
}

static void display_menubar(void *data)
{
   menu *current_menu = (menu*)data;
   DEVICE_CAST device_ptr = (DEVICE_CAST)driver.video_data;
   filebrowser_t *fb = browser;
   char msg[128];
   font_params_t font_parms = {0};

   rmenu_default_positions_t default_pos;
   menu_set_default_pos(&default_pos);

   font_parms.x = default_pos.x_position;
   font_parms.y = default_pos.current_path_y_position;
   font_parms.scale = default_pos.current_path_font_size;
   font_parms.color = WHITE;

   struct platform_bind key_label_r, key_label_l;
   strlcpy(key_label_r.desc, "Unknown", sizeof(key_label_r.desc));
   key_label_r.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_R;

   strlcpy(key_label_l.desc, "Unknown", sizeof(key_label_l.desc));
   key_label_l.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_L;

   switch(current_menu->enum_id)
   {
      case GENERAL_VIDEO_MENU:
         if (driver.input->set_keybinds)
            driver.input->set_keybinds(&key_label_r, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));

         snprintf(msg, sizeof(msg), "NEXT -> [%s]", key_label_r.desc);

         if (driver.video_poke->set_osd_msg)
            driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);
         break;
      case GENERAL_AUDIO_MENU:
      case EMU_GENERAL_MENU:
      case EMU_VIDEO_MENU:
      case EMU_AUDIO_MENU:
      case PATH_MENU:
         if (driver.input->set_keybinds)
         {
            driver.input->set_keybinds(&key_label_r, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
            driver.input->set_keybinds(&key_label_l, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         }
         snprintf(msg, sizeof(msg), "[%s] <- PREV | NEXT -> [%s]", key_label_l.desc, key_label_r.desc);

         if (driver.video_poke->set_osd_msg)
            driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);
         break;
      case CONTROLS_MENU:
      case INGAME_MENU_RESIZE:
         if (driver.input->set_keybinds)
            driver.input->set_keybinds(&key_label_l, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));

         snprintf(msg, sizeof(msg), "[%s] <- PREV", key_label_l.desc);

         if (driver.video_poke->set_osd_msg)
            driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);
         break;
      default:
         break;
   }

   switch(current_menu->enum_id)
   {
      case SHADER_CHOICE:
      case PRESET_CHOICE:
      case BORDER_CHOICE:
      case LIBRETRO_CHOICE:
      case INPUT_PRESET_CHOICE:
      case PATH_SAVESTATES_DIR_CHOICE:
      case PATH_DEFAULT_ROM_DIR_CHOICE:
#ifdef HAVE_XML
      case PATH_CHEATS_DIR_CHOICE:
#endif
      case PATH_SRAM_DIR_CHOICE:
      case PATH_SYSTEM_DIR_CHOICE:
         fb = tmpBrowser;
      case FILE_BROWSER_MENU:
         snprintf(msg, sizeof(msg), "PATH: %s", filebrowser_get_current_dir(fb));

         if (driver.video_poke->set_osd_msg)
            driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);
         break;
      default:
         break;
   }

   rarch_position_t position = {0};
   device_ptr->ctx_driver->rmenu_draw_bg(&position);
   
   font_parms.x = default_pos.core_msg_x_position;
   font_parms.y = default_pos.core_msg_y_position;
   font_parms.scale = default_pos.core_msg_font_size;
   font_parms.color = WHITE;

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, g_extern.title_buf, &font_parms);
#ifdef __CELLOS_LV2__

   font_parms.x = default_pos.x_position;
   font_parms.y = 0.05f;
   font_parms.scale = 1.4f;
   font_parms.color = WHITE;

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, current_menu->title, &font_parms);

   font_parms.x = 0.80f;
   font_parms.y = 0.015f;
   font_parms.scale = 0.82f;
   font_parms.color = WHITE;
   snprintf(msg, sizeof(msg), "v%s", PACKAGE_VERSION);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);
#endif
}

static void browser_update(void *data, uint64_t input, const char *extensions)
{
   filebrowser_t *b = (filebrowser_t*)data;
   filebrowser_action_t action = FILEBROWSER_ACTION_NOOP;
   bool ret = true;

   if (input & (1ULL << RMENU_DEVICE_NAV_DOWN))
      action = FILEBROWSER_ACTION_DOWN;
   else if (input & (1ULL << RMENU_DEVICE_NAV_UP))
      action = FILEBROWSER_ACTION_UP;
   else if (input & (1ULL << RMENU_DEVICE_NAV_RIGHT))
      action = FILEBROWSER_ACTION_RIGHT;
   else if (input & (1ULL << RMENU_DEVICE_NAV_LEFT))
      action = FILEBROWSER_ACTION_LEFT;
   else if (input & (1ULL << RMENU_DEVICE_NAV_R2))
      action = FILEBROWSER_ACTION_SCROLL_DOWN;
   else if (input & (1ULL << RMENU_DEVICE_NAV_L2))
      action = FILEBROWSER_ACTION_SCROLL_UP;
   else if (input & (1ULL << RMENU_DEVICE_NAV_A))
      action = FILEBROWSER_ACTION_CANCEL;
   else if (input & (1ULL << RMENU_DEVICE_NAV_START))
   {
      action = FILEBROWSER_ACTION_RESET;
      filebrowser_set_root_and_ext(b, NULL, default_paths.filesystem_root_dir);
      strlcpy(b->extensions, extensions, sizeof(b->extensions));
   }

   if(action != FILEBROWSER_ACTION_NOOP)
      ret = filebrowser_iterate(b, action);

   if(!ret)
      menu_settings_msg(S_MSG_DIR_LOADING_ERROR, 180);
}

void browser_render(void *data)
{
   filebrowser_t *b = (filebrowser_t*)data;
   DEVICE_CAST device_ptr = (DEVICE_CAST)driver.video_data;
   unsigned file_count = b->current_dir.list->size;
   unsigned int current_index, page_number, page_base, i;
   font_params_t font_parms = {0};
   rmenu_default_positions_t default_pos = {0};

   menu_set_default_pos(&default_pos);

   current_index = b->current_dir.ptr;
   page_number = current_index / default_pos.entries_per_page;
   page_base = page_number * default_pos.entries_per_page;

   font_parms.scale = default_pos.variable_font_size;

   for (i = page_base; i < file_count && i < page_base + default_pos.entries_per_page; ++i)
   {
      char fname_tmp[256];
      fill_pathname_base(fname_tmp, b->current_dir.list->elems[i].data, sizeof(fname_tmp));
      default_pos.starting_y_position += default_pos.y_position_increment;

      //check if this is the currently selected file
      const char *current_pathname = filebrowser_get_current_path(b);
      if(strcmp(current_pathname, b->current_dir.list->elems[i].data) == 0)
      {
         rarch_position_t position = {0};
         position.x = default_pos.x_position;
         position.y = default_pos.starting_y_position;
         device_ptr->ctx_driver->rmenu_draw_panel(&position);
      }

      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.starting_y_position;
      font_parms.color = i == current_index ? RED : b->current_dir.list->elems[i].attr.b ? GREEN : WHITE;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, fname_tmp, &font_parms);
   }
}

int select_file(void *data, void *state)
{
   menu *current_menu = (menu*)data;
   rmenu_state_t *rstate = (rmenu_state_t*)state;
   font_params_t font_parms = {0};

   uint64_t input = rstate->input;

   char extensions[256], comment[256], path[PATH_MAX];
   bool ret = true;

   filebrowser_t *filebrowser = tmpBrowser;
   rmenu_default_positions_t default_pos;
   menu_set_default_pos(&default_pos);

   struct platform_bind key_label_b;
   strlcpy(key_label_b.desc, "Unknown", sizeof(key_label_b.desc));
   key_label_b.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_B;

   if (driver.input->set_keybinds)
      driver.input->set_keybinds(&key_label_b, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));

   switch(current_menu->enum_id)
   {
      case SHADER_CHOICE:
         strlcpy(extensions, EXT_SHADERS, sizeof(extensions));
         snprintf(comment, sizeof(comment), "INFO - Select a shader by pressing [%s].", key_label_b.desc);
         break;
      case PRESET_CHOICE:
         strlcpy(extensions, EXT_CGP_PRESETS, sizeof(extensions));
         snprintf(comment, sizeof(comment), "INFO - Select a shader preset by pressing [%s].", key_label_b.desc);
         break;
      case INPUT_PRESET_CHOICE:
         strlcpy(extensions, EXT_INPUT_PRESETS, sizeof(extensions));
         snprintf(comment, sizeof(comment), "INFO - Select an input preset by pressing [%s].", key_label_b.desc);
         break;
      case BORDER_CHOICE:
         strlcpy(extensions, EXT_IMAGES, sizeof(extensions));
         snprintf(comment, sizeof(comment), "INFO - Select a border image file by pressing [%s].", key_label_b.desc);
         break;
      case LIBRETRO_CHOICE:
         strlcpy(extensions, EXT_EXECUTABLES, sizeof(extensions));
         snprintf(comment, sizeof(comment), "INFO - Select a Libretro core by pressing [%s].", key_label_b.desc);
         break;
   }

   browser_update(filebrowser, input, extensions);

   if (input & (1ULL << RMENU_DEVICE_NAV_B))
   {
      if (filebrowser_iterate(filebrowser, FILEBROWSER_ACTION_PATH_ISDIR))
         ret = filebrowser_iterate(filebrowser, FILEBROWSER_ACTION_OK);
      else
      {
         snprintf(path, sizeof(path), filebrowser_get_current_path(filebrowser));

         switch(current_menu->enum_id)
         {
#if defined(HAVE_CG) || defined(HAVE_HLSL) || defined(HAVE_GLSL)
            case SHADER_CHOICE:
               if (g_extern.lifecycle_mode_state & (1ULL << MODE_LOAD_FIRST_SHADER))
               {
                  strlcpy(g_settings.video.cg_shader_path, path, sizeof(g_settings.video.cg_shader_path));

                  if (g_settings.video.shader_type != RARCH_SHADER_NONE)
                  {
                     driver.video->set_shader(driver.video_data, (enum rarch_shader_type)g_settings.video.shader_type, path, RARCH_SHADER_INDEX_PASS0);
                     if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
                        menu_settings_msg(S_MSG_SHADER_LOADING_SUCCEEDED, 180);
                  }
                  else
                     RARCH_ERR("Shaders are unsupported on this platform.\n");

                  g_extern.lifecycle_mode_state &= ~(1ULL << MODE_LOAD_FIRST_SHADER);
               }

               if (g_extern.lifecycle_mode_state & (1ULL << MODE_LOAD_SECOND_SHADER))
               {
                  strlcpy(g_settings.video.second_pass_shader, path, sizeof(g_settings.video.second_pass_shader));

                  if (g_settings.video.shader_type != RARCH_SHADER_NONE)
                  {
                     driver.video->set_shader(driver.video_data, (enum rarch_shader_type)g_settings.video.shader_type, path, RARCH_SHADER_INDEX_PASS1);
                     if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
                        menu_settings_msg(S_MSG_SHADER_LOADING_SUCCEEDED, 180);
                  }
                  else
                     RARCH_ERR("Shaders are unsupported on this platform.\n");

                  g_extern.lifecycle_mode_state &= ~(1ULL << MODE_LOAD_SECOND_SHADER);
               }
               break;
            case PRESET_CHOICE:
               strlcpy(g_extern.file_state.cgp_path, path, sizeof(g_extern.file_state.cgp_path));

               if (driver.video_poke->set_fbo_state)
                  driver.video_poke->set_fbo_state(driver.video_data, FBO_DEINIT);
#ifdef HAVE_OPENGL
               gl_cg_reinit(path);
#endif

               if (driver.video_poke->set_fbo_state)
                  driver.video_poke->set_fbo_state(driver.video_data, FBO_INIT);
               break;
#endif
            case INPUT_PRESET_CHOICE:
               strlcpy(g_extern.file_state.input_cfg_path, path, sizeof(g_extern.file_state.input_cfg_path));
               config_read_keybinds(path);
               break;
            case BORDER_CHOICE:
#ifdef __CELLOS_LV2__
               texture_image_border_load(path);
               snprintf(g_extern.console.menu_texture_path, sizeof(g_extern.console.menu_texture_path),
                     "%s", path);
#endif
               break;
            case LIBRETRO_CHOICE:
               strlcpy(g_settings.libretro, path, sizeof(g_settings.libretro));

               if(set_libretro_core_as_launch)
               {
                  strlcpy(g_extern.fullpath, path, sizeof(g_extern.fullpath));
                  set_libretro_core_as_launch = false;
                  g_extern.lifecycle_mode_state |= (1ULL << MODE_EXIT);
                  g_extern.lifecycle_mode_state |= (1ULL << MODE_EXITSPAWN);
                  return -1;
               }
               else
               {
                  if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
                     menu_settings_msg(S_MSG_RESTART_RARCH, 180);
               }
               break;
         }

         menu_stack_pop();
      }

      if(!ret)
         menu_settings_msg(S_MSG_DIR_LOADING_ERROR, 180);
   }
   else if (input & (1ULL << RMENU_DEVICE_NAV_X))
      menu_stack_pop();

   display_menubar(current_menu);

   font_parms.x = default_pos.x_position;
   font_parms.y = default_pos.comment_y_position;
   font_parms.scale = default_pos.font_size;
   font_parms.color = WHITE;

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, comment, &font_parms);

   struct platform_bind key_label_x, key_label_start;
   strlcpy(key_label_x.desc, "Unknown", sizeof(key_label_x.desc));
   key_label_x.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_X;

   strlcpy(key_label_start.desc, "Unknown", sizeof(key_label_start.desc));
   key_label_start.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_START;

   if (driver.input->set_keybinds)
   {
      driver.input->set_keybinds(&key_label_x, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_start, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
   }

   snprintf(comment, sizeof(comment), "[%s] - return to settings [%s] - Reset Startdir", key_label_x.desc, key_label_start.desc);
   font_parms.y = default_pos.comment_two_y_position;
   font_parms.color = YELLOW;

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, comment, &font_parms);

   if(current_menu->browser_draw)
      current_menu->browser_draw(filebrowser);

   return 0;
}

int select_directory(void *data, void *state)
{
   menu *current_menu = (menu*)data;
   rmenu_state_t *rstate = (rmenu_state_t*)state;
   font_params_t font_parms = {0};

   uint64_t input = rstate->input;

   char path[PATH_MAX], msg[256];
   bool ret = true;

   filebrowser_t *filebrowser = tmpBrowser;
   rmenu_default_positions_t default_pos;

   menu_set_default_pos(&default_pos);

   bool is_dir = filebrowser_iterate(filebrowser, FILEBROWSER_ACTION_PATH_ISDIR);
   browser_update(filebrowser, input, "empty");

   if (input & (1ULL << RMENU_DEVICE_NAV_Y))
   {
      if(is_dir)
      {
         snprintf(path, sizeof(path), filebrowser_get_current_path(filebrowser));

         switch(current_menu->enum_id)
         {
            case PATH_SAVESTATES_DIR_CHOICE:
               strlcpy(g_extern.console.main_wrap.default_savestate_dir, path, sizeof(g_extern.console.main_wrap.default_savestate_dir));
               break;
            case PATH_SRAM_DIR_CHOICE:
               strlcpy(g_extern.console.main_wrap.default_sram_dir, path, sizeof(g_extern.console.main_wrap.default_sram_dir));
               break;
            case PATH_DEFAULT_ROM_DIR_CHOICE:
               strlcpy(g_extern.console.main_wrap.default_rom_startup_dir, path, sizeof(g_extern.console.main_wrap.default_rom_startup_dir));
               break;
#ifdef HAVE_XML
            case PATH_CHEATS_DIR_CHOICE:
               strlcpy(g_settings.cheat_database, path, sizeof(g_settings.cheat_database));
               break;
#endif
            case PATH_SYSTEM_DIR_CHOICE:
               strlcpy(g_settings.system_directory, path, sizeof(g_settings.system_directory));
               break;
         }
         menu_stack_pop();
      }
   }
   else if (input & (1ULL << RMENU_DEVICE_NAV_X))
   {
      strlcpy(path, default_paths.port_dir, sizeof(path));
      switch(current_menu->enum_id)
      {
         case PATH_SAVESTATES_DIR_CHOICE:
            strlcpy(g_extern.console.main_wrap.default_savestate_dir, path, sizeof(g_extern.console.main_wrap.default_savestate_dir));
            break;
         case PATH_SRAM_DIR_CHOICE:
            strlcpy(g_extern.console.main_wrap.default_sram_dir, path, sizeof(g_extern.console.main_wrap.default_sram_dir));
            break;
         case PATH_DEFAULT_ROM_DIR_CHOICE:
            strlcpy(g_extern.console.main_wrap.default_rom_startup_dir, path, sizeof(g_extern.console.main_wrap.default_rom_startup_dir));
            break;
#ifdef HAVE_XML
         case PATH_CHEATS_DIR_CHOICE:
            strlcpy(g_settings.cheat_database, path, sizeof(g_settings.cheat_database));
            break;
#endif
         case PATH_SYSTEM_DIR_CHOICE:
            strlcpy(g_settings.system_directory, path, sizeof(g_settings.system_directory));
            break;
      }

      menu_stack_pop();
   }
   else if (input & (1ULL << RMENU_DEVICE_NAV_B))
   {
      if(is_dir)
         ret = filebrowser_iterate(filebrowser, FILEBROWSER_ACTION_OK);
   }

   if(!ret)
      menu_settings_msg(S_MSG_DIR_LOADING_ERROR, 180);

   display_menubar(current_menu);

   struct platform_bind key_label_b, key_label_x, key_label_y, key_label_start;
   strlcpy(key_label_b.desc, "Unknown", sizeof(key_label_b.desc));
   key_label_b.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_B;
   strlcpy(key_label_x.desc, "Unknown", sizeof(key_label_x.desc));
   key_label_x.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_X;
   strlcpy(key_label_y.desc, "Unknown", sizeof(key_label_y.desc));
   key_label_y.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_Y;
   strlcpy(key_label_start.desc, "Unknown", sizeof(key_label_start.desc));
   key_label_start.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_START;

   if (driver.input->set_keybinds)
   {
      driver.input->set_keybinds(&key_label_x, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_y, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_b, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_start, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
   }

   snprintf(msg, sizeof(msg), "[%s] - Enter dir | [%s] - Go back", key_label_b.desc, key_label_x.desc);

   font_parms.x = default_pos.x_position;
   font_parms.y = default_pos.comment_two_y_position;
   font_parms.scale = default_pos.font_size;
   font_parms.color = YELLOW;

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

   snprintf(msg, sizeof(msg), "[%s] - Reset to startdir", key_label_start.desc);

   font_parms.y = default_pos.comment_two_y_position + (default_pos.y_position_increment * 1);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

   snprintf(msg, sizeof(msg), "INFO - Browse to a directory and assign it as the path by\npressing [%s].", key_label_y.desc);

   font_parms.y = default_pos.comment_y_position;
   font_parms.color = WHITE;

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

   if(current_menu->browser_draw)
      current_menu->browser_draw(filebrowser);

   return 0;
}

static void set_keybind_digital(unsigned default_retro_joypad_id, uint64_t input)
{
   if (!driver.input->set_keybinds)
      return;

   unsigned keybind_action = KEYBINDS_ACTION_NONE;

   if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
      keybind_action = (1ULL << KEYBINDS_ACTION_DECREMENT_BIND);

   if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
      keybind_action = (1ULL << KEYBINDS_ACTION_INCREMENT_BIND);

   if(input & (1ULL << RMENU_DEVICE_NAV_START))
      keybind_action = (1ULL << KEYBINDS_ACTION_SET_DEFAULT_BIND);


   if (keybind_action)
      driver.input->set_keybinds(driver.input_data, NULL, currently_selected_controller_menu,
            default_retro_joypad_id, keybind_action);
}

#if defined(HAVE_OSKUTIL)
#ifdef __CELLOS_LV2__

static bool osk_callback_enter_rsound(void *data)
{
   if (g_extern.lifecycle_mode_state & (1ULL << MODE_OSK_ENTRY_SUCCESS))
   {
      RARCH_LOG("OSK - Applying input data.\n");
      char tmp_str[256];
      int num = wcstombs(tmp_str, g_extern.console.misc.oskutil_handle.text_buf, sizeof(tmp_str));
      tmp_str[num] = 0;
      snprintf(g_settings.audio.device, sizeof(g_settings.audio.device), "%s", tmp_str);
      goto do_exit;
   }
   else if (g_extern.lifecycle_mode_state & (1ULL << MODE_OSK_ENTRY_FAIL))
      goto do_exit;

   return false;

do_exit:
   g_extern.lifecycle_mode_state &= ~((1ULL << MODE_OSK_DRAW) | (1ULL << MODE_OSK_ENTRY_SUCCESS) |
         (1ULL << MODE_OSK_ENTRY_FAIL));
   return true;
}

static bool osk_callback_enter_rsound_init(void *data)
{
   oskutil_write_initial_message(&g_extern.console.misc.oskutil_handle, L"192.168.1.1");
   oskutil_write_message(&g_extern.console.misc.oskutil_handle, L"Enter IP address for the RSound Server.");
   oskutil_start(&g_extern.console.misc.oskutil_handle);

   return true;
}

static bool osk_callback_enter_filename(void *data)
{
   if (g_extern.lifecycle_mode_state & (1ULL << MODE_OSK_ENTRY_SUCCESS))
   {
      RARCH_LOG("OSK - Applying input data.\n");
      char tmp_str[256];
      char filepath[PATH_MAX];
      int num = wcstombs(tmp_str, g_extern.console.misc.oskutil_handle.text_buf, sizeof(tmp_str));
      tmp_str[num] = 0;

      switch(rmenu_state.osk_param)
      {
         case CONFIG_FILE:
            break;
         case SHADER_PRESET_FILE:
            snprintf(filepath, sizeof(filepath), "%s/%s.cgp", default_paths.cgp_dir, tmp_str);
            RARCH_LOG("[osk_callback_enter_filename]: filepath is: %s.\n", filepath);

            struct gl_cg_cgp_info current_settings;
            memset(&current_settings, 0, sizeof(current_settings));
            current_settings.shader[0] = g_settings.video.cg_shader_path;
            current_settings.shader[1] = g_settings.video.second_pass_shader;
            current_settings.filter_linear[0] = g_settings.video.smooth;
            current_settings.filter_linear[1] = g_settings.video.second_pass_smooth;
            current_settings.render_to_texture = true;
            current_settings.fbo_scale = g_settings.video.fbo.scale_x; //fbo.scale_x and y are the same anyway
            gl_cg_save_cgp(filepath, &current_settings);
            break;
         case INPUT_PRESET_FILE:
            snprintf(filepath, sizeof(filepath), "%s/%s.cfg", default_paths.input_presets_dir, tmp_str);
            RARCH_LOG("[osk_callback_enter_filename]: filepath is: %s.\n", filepath);
            config_save_keybinds(filepath);
            break;
      }

      goto do_exit;
   }
   else if (g_extern.lifecycle_mode_state & (1ULL << MODE_OSK_ENTRY_FAIL))
      goto do_exit;

   return false;
do_exit:
   g_extern.lifecycle_mode_state &= ~((1ULL << MODE_OSK_DRAW) | (1ULL << MODE_OSK_ENTRY_SUCCESS) |
         (1ULL << MODE_OSK_ENTRY_FAIL));
   return true;
}

static bool osk_callback_enter_filename_init(void *data)
{
   oskutil_write_initial_message(&g_extern.console.misc.oskutil_handle, L"example");
   oskutil_write_message(&g_extern.console.misc.oskutil_handle, L"Enter filename for preset");
   oskutil_start(&g_extern.console.misc.oskutil_handle);

   return true;
}

#endif
#endif

static int set_setting_action(void *data, unsigned switchvalue, uint64_t input)
{
   (void)data;
   DEVICE_CAST device_ptr = (DEVICE_CAST)driver.video_data;
   filebrowser_t *filebrowser = tmpBrowser;

   switch(switchvalue)
   {
#ifdef __CELLOS_LV2__
      case SETTING_CHANGE_RESOLUTION:
         if(input & (1ULL << RMENU_DEVICE_NAV_RIGHT))
            menu_settings_set(S_RESOLUTION_NEXT);
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
            menu_settings_set(S_RESOLUTION_PREVIOUS);
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
         {
            if (g_extern.console.screen.resolutions.list[g_extern.console.screen.resolutions.current.idx] == CELL_VIDEO_OUT_RESOLUTION_576)
            {
               if(gfx_ctx_check_resolution(CELL_VIDEO_OUT_RESOLUTION_576))
                  g_extern.lifecycle_mode_state |= (1ULL<< MODE_VIDEO_PAL_ENABLE);
            }
            else
            {
               g_extern.lifecycle_mode_state &= ~(1ULL << MODE_VIDEO_PAL_ENABLE);
               g_extern.lifecycle_mode_state &= ~(1ULL << MODE_VIDEO_PAL_TEMPORAL_ENABLE);
               g_extern.lifecycle_mode_state &= ~(1ULL << MODE_VIDEO_PAL_VSYNC_BLOCK);
            }

            driver.video->restart();
         }
         break;
      case SETTING_PAL60_MODE:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_PAL_ENABLE))
            {
               if (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_PAL_TEMPORAL_ENABLE))
               {
                  g_extern.lifecycle_mode_state &= ~(1ULL << MODE_VIDEO_PAL_TEMPORAL_ENABLE);
                  g_extern.lifecycle_mode_state &= ~(1ULL << MODE_VIDEO_PAL_VSYNC_BLOCK);
               }
               else
               {
                  g_extern.lifecycle_mode_state |= (1ULL << MODE_VIDEO_PAL_TEMPORAL_ENABLE);
                  g_extern.lifecycle_mode_state |= (1ULL << MODE_VIDEO_PAL_VSYNC_BLOCK);
               }

               driver.video->restart();
            }
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            if (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_PAL_ENABLE))
            {
               g_extern.lifecycle_mode_state &= ~(1ULL << MODE_VIDEO_PAL_TEMPORAL_ENABLE);
               g_extern.lifecycle_mode_state &= ~(1ULL << MODE_VIDEO_PAL_VSYNC_BLOCK);
               driver.video->restart();
            }
         }
         break;
#endif
#if defined(HAVE_CG) || defined(HAVE_HLSL) || defined(HAVE_GLSL)
      case SETTING_SHADER_PRESETS:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if(g_extern.main_is_init)
            {
               menu_stack_push(PRESET_CHOICE);
               filebrowser_set_root_and_ext(filebrowser, EXT_CGP_PRESETS, default_paths.cgp_dir);
            }
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            strlcpy(g_extern.file_state.cgp_path, "", sizeof(g_extern.file_state.cgp_path));
         break;
      case SETTING_SHADER:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_stack_push(SHADER_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, EXT_SHADERS, default_paths.shader_dir);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_LOAD_FIRST_SHADER);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            strlcpy(g_settings.video.cg_shader_path, default_paths.shader_file, sizeof(g_settings.video.cg_shader_path));
            if (g_settings.video.shader_type != RARCH_SHADER_NONE)
            {
               driver.video->set_shader(driver.video_data, (enum rarch_shader_type)g_settings.video.shader_type, NULL, RARCH_SHADER_INDEX_PASS0);
               if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
                  menu_settings_msg(S_MSG_SHADER_LOADING_SUCCEEDED, 180);
            }
            else
               RARCH_ERR("Shaders are unsupported on this platform.\n");
         }
         break;
      case SETTING_SHADER_2:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_stack_push(SHADER_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, EXT_SHADERS, default_paths.shader_dir);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_LOAD_SECOND_SHADER);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            strlcpy(g_settings.video.second_pass_shader, default_paths.shader_file, sizeof(g_settings.video.second_pass_shader));
            if (g_settings.video.shader_type != RARCH_SHADER_NONE)
            {
               driver.video->set_shader(driver.video_data, (enum rarch_shader_type)g_settings.video.shader_type, NULL, RARCH_SHADER_INDEX_PASS1);
               if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
                  menu_settings_msg(S_MSG_SHADER_LOADING_SUCCEEDED, 180);
            }
            else
               RARCH_ERR("Shaders are unsupported on this platform.\n");
         }
         break;
      case SETTING_EMU_SKIN:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_stack_push(BORDER_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, EXT_IMAGES, default_paths.border_dir);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            if (!texture_image_load(default_paths.menu_border_file, &g_extern.console.menu_texture))
            {
               RARCH_ERR("Failed to load texture image for menu.\n");
               return false;
            }
         }
         break;
#endif
      case SETTING_EMU_LOW_RAM_MODE_ENABLE:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if (!(g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_LOW_RAM_MODE_ENABLE_PENDING)))
            {
               if (g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_LOW_RAM_MODE_ENABLE))
                  g_extern.lifecycle_mode_state &= ~(1ULL << MODE_MENU_LOW_RAM_MODE_ENABLE);
               else
                  g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_LOW_RAM_MODE_ENABLE_PENDING);

               if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
                  menu_settings_msg(S_MSG_RESTART_RARCH, 180);
            }
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            if (!(g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_LOW_RAM_MODE_ENABLE)))
            {
               if (!(g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_LOW_RAM_MODE_ENABLE_PENDING)))
               {
                  g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_LOW_RAM_MODE_ENABLE);
                  g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_LOW_RAM_MODE_ENABLE_PENDING);

                  if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
                     menu_settings_msg(S_MSG_RESTART_RARCH, 180);
               }
            }
         }
         break;
      case SETTING_FONT_SIZE:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            if(g_settings.video.font_size > 0) 
               g_settings.video.font_size -= 0.01f;
         }
         if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if((g_settings.video.font_size < 2.0f))
               g_settings.video.font_size += 0.01f;
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            g_settings.video.font_size = 1.0f;
         break;
      case SETTING_KEEP_ASPECT_RATIO:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            menu_settings_set(S_ASPECT_RATIO_DECREMENT);

            if (driver.video_poke->set_aspect_ratio)
               driver.video_poke->set_aspect_ratio(driver.video_data, g_settings.video.aspect_ratio_idx);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_RIGHT))
         {
            menu_settings_set(S_ASPECT_RATIO_INCREMENT);

            if (driver.video_poke->set_aspect_ratio)
               driver.video_poke->set_aspect_ratio(driver.video_data, g_settings.video.aspect_ratio_idx);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            menu_settings_set_default(S_DEF_ASPECT_RATIO);

            if (driver.video_poke->set_aspect_ratio)
               driver.video_poke->set_aspect_ratio(driver.video_data, g_settings.video.aspect_ratio_idx);
         }
         break;
      case SETTING_HW_TEXTURE_FILTER:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_settings_set(S_HW_TEXTURE_FILTER);

            if (driver.video_poke->set_filtering)
               driver.video_poke->set_filtering(driver.video_data, 1, g_settings.video.smooth);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            menu_settings_set(S_DEF_HW_TEXTURE_FILTER);

            if (driver.video_poke->set_filtering)
               driver.video_poke->set_filtering(driver.video_data, 1, g_settings.video.smooth);
         }
         break;
#ifdef HAVE_FBO
      case SETTING_HW_TEXTURE_FILTER_2:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_settings_set(S_HW_TEXTURE_FILTER_2);

            if (driver.video_poke->set_filtering)
               driver.video_poke->set_filtering(driver.video_data, 2, g_settings.video.second_pass_smooth);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            menu_settings_set(S_DEF_HW_TEXTURE_FILTER_2);

            if (driver.video_poke->set_filtering)
               driver.video_poke->set_filtering(driver.video_data, 2, g_settings.video.second_pass_smooth);
         }
         break;
      case SETTING_SCALE_ENABLED:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_settings_set(S_SCALE_ENABLED);

            if (driver.video_poke->set_fbo_state)
            {
               if (g_settings.video.render_to_texture)
                  driver.video_poke->set_fbo_state(driver.video_data, FBO_INIT);
               else
                  driver.video_poke->set_fbo_state(driver.video_data, FBO_DEINIT);
            }
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            menu_settings_set_default(S_DEF_SCALE_ENABLED);

            if (driver.video_poke->set_fbo_state)
               driver.video_poke->set_fbo_state(driver.video_data, FBO_REINIT);
         }
         break;
      case SETTING_SCALE_FACTOR:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            if(g_settings.video.render_to_texture)
            {
               bool should_decrement = g_settings.video.fbo.scale_x > MIN_SCALING_FACTOR;

               if(should_decrement)
               {
                  menu_settings_set(S_SCALE_FACTOR_DECREMENT);

                  if (driver.video_poke->set_fbo_state)
                     driver.video_poke->set_fbo_state(driver.video_data, FBO_REINIT);
               }
            }
         }
         if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if(g_settings.video.render_to_texture)
            {
               bool should_increment = g_settings.video.fbo.scale_x < MAX_SCALING_FACTOR;
               if(should_increment)
               {
                  menu_settings_set(S_SCALE_FACTOR_INCREMENT);

                  if (driver.video_poke->set_fbo_state)
                     driver.video_poke->set_fbo_state(driver.video_data, FBO_REINIT);
               }
            }
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            menu_settings_set_default(S_DEF_SCALE_FACTOR);

            if (driver.video_poke->set_fbo_state)
               driver.video_poke->set_fbo_state(driver.video_data, FBO_REINIT);
         }
         break;
#endif
#ifdef _XBOX1
      case SETTING_FLICKER_FILTER:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            if(g_extern.console.screen.flicker_filter_index > 0)
               g_extern.console.screen.flicker_filter_index--;
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_RIGHT))
         {
            if(g_extern.console.screen.flicker_filter_index < 5)
               g_extern.console.screen.flicker_filter_index++;
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            g_extern.console.screen.flicker_filter_index = 0;
         break;
      case SETTING_SOFT_DISPLAY_FILTER:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_SOFT_FILTER_ENABLE))
               g_extern.lifecycle_mode_state &= ~(1ULL << MODE_VIDEO_SOFT_FILTER_ENABLE);
            else
               g_extern.lifecycle_mode_state |= (1ULL << MODE_VIDEO_SOFT_FILTER_ENABLE);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            g_extern.lifecycle_mode_state |= (1ULL << MODE_VIDEO_SOFT_FILTER_ENABLE);
         break;
#endif
      case SETTING_HW_OVERSCAN_AMOUNT:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            menu_settings_set(S_OVERSCAN_DECREMENT);

            if (driver.video_poke->apply_state_changes)
               driver.video_poke->apply_state_changes(driver.video_data);
         }
         if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_settings_set(S_OVERSCAN_INCREMENT);

            if (driver.video_poke->apply_state_changes)
               driver.video_poke->apply_state_changes(driver.video_data);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            menu_settings_set_default(S_DEF_OVERSCAN);

            if (driver.video_poke->apply_state_changes)
               driver.video_poke->apply_state_changes(driver.video_data);
         }
         break;
      case SETTING_REFRESH_RATE:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            menu_settings_set(S_REFRESH_RATE_DECREMENT);
            driver_set_monitor_refresh_rate(g_settings.video.refresh_rate);
         }
         if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_settings_set(S_REFRESH_RATE_INCREMENT);
            driver_set_monitor_refresh_rate(g_settings.video.refresh_rate);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            menu_settings_set_default(S_DEF_REFRESH_RATE);
            driver_set_monitor_refresh_rate(g_settings.video.refresh_rate);
         }
         break;
      case SETTING_THROTTLE_MODE:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if (!(g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_PAL_VSYNC_BLOCK)))
            {
               menu_settings_set(S_THROTTLE);
               device_ptr->ctx_driver->swap_interval((g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_THROTTLE_ENABLE)) ? true : false);
            }
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            if (!(g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_PAL_VSYNC_BLOCK)))
            {
               menu_settings_set_default(S_DEF_THROTTLE);
               device_ptr->ctx_driver->swap_interval((g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_THROTTLE_ENABLE)) ? true : false);
            }
         }
         break;
      case SETTING_TRIPLE_BUFFERING:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_settings_set(S_TRIPLE_BUFFERING);
            driver.video->restart();
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            menu_settings_set_default(S_DEF_TRIPLE_BUFFERING);

            if(!(g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_TRIPLE_BUFFERING_ENABLE)))
               driver.video->restart();
         }
         break;
      case SETTING_ENABLE_SCREENSHOTS:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if (g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_SCREENSHOTS_ENABLE))
               g_extern.lifecycle_mode_state &= ~(1ULL << MODE_VIDEO_SCREENSHOTS_ENABLE);
            else
               g_extern.lifecycle_mode_state |= (1ULL << MODE_VIDEO_SCREENSHOTS_ENABLE);
            device_ptr->ctx_driver->rmenu_screenshot_enable((g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_SCREENSHOTS_ENABLE)) ? true : false);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            g_extern.lifecycle_mode_state |= (1ULL << MODE_VIDEO_SCREENSHOTS_ENABLE);
            device_ptr->ctx_driver->rmenu_screenshot_enable((g_extern.lifecycle_mode_state & (1ULL << MODE_VIDEO_SCREENSHOTS_ENABLE)) ? true : false);
         }
         break;
#if defined(HAVE_CG) || defined(HAVE_HLSL) || defined(HAVE_GLSL)
      case SETTING_SAVE_SHADER_PRESET:
#ifdef HAVE_OSKUTIL
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if(g_extern.main_is_init)
            {
               rmenu_state.osk_param = SHADER_PRESET_FILE;
               rmenu_state.osk_init = osk_callback_enter_filename_init;
               rmenu_state.osk_callback = osk_callback_enter_filename;
            }
         }
#endif
         break;
      case SETTING_APPLY_SHADER_PRESET_ON_STARTUP:
         break;
#endif
      case SETTING_DEFAULT_VIDEO_ALL:
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
#if defined(HAVE_CG) || defined(HAVE_HLSL) || defined(HAVE_GLSL)
            set_setting_action(NULL, SETTING_SHADER, 1ULL << RMENU_DEVICE_NAV_START);
            set_setting_action(NULL, SETTING_SHADER_2, 1ULL << RMENU_DEVICE_NAV_START);
#endif
         }
         break;
      case SETTING_SOUND_MODE:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            if(g_extern.console.sound.mode != SOUND_MODE_NORMAL)
               g_extern.console.sound.mode--;
         }
         if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if(g_extern.console.sound.mode < (SOUND_MODE_LAST-1))
               g_extern.console.sound.mode++;
         }
         if((input & (1ULL << RMENU_DEVICE_NAV_UP)) || (input & (1ULL << RMENU_DEVICE_NAV_DOWN)))
         {
#ifdef HAVE_RSOUND
            if(g_extern.console.sound.mode != SOUND_MODE_RSOUND)
               rarch_rsound_stop();
            else
               rarch_rsound_start(g_settings.audio.device);
#endif
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            g_extern.console.sound.mode = SOUND_MODE_NORMAL;
#ifdef HAVE_RSOUND
            rarch_rsound_stop();
#endif
         }
         break;
#ifdef HAVE_RSOUND
      case SETTING_RSOUND_SERVER_IP_ADDRESS:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
#ifdef HAVE_OSKUTIL
            rmenu_state.osk_init = osk_callback_enter_rsound_init;
            rmenu_state.osk_callback = osk_callback_enter_rsound;
#endif
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            strlcpy(g_settings.audio.device, "0.0.0.0", sizeof(g_settings.audio.device));
         break;
#endif
      case SETTING_DEFAULT_AUDIO_ALL:
         break;
      case SETTING_EMU_CURRENT_SAVE_STATE_SLOT:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
            menu_settings_set(S_SAVESTATE_DECREMENT);
         if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
            menu_settings_set(S_SAVESTATE_INCREMENT);
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            menu_settings_set_default(S_DEF_SAVE_STATE);
         break;
      case SETTING_EMU_SHOW_DEBUG_INFO_MSG:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
            menu_settings_set(S_INFO_DEBUG_MSG_TOGGLE);
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            menu_settings_set_default(S_DEF_INFO_DEBUG_MSG);
         break;
      case SETTING_EMU_SHOW_INFO_MSG:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
            menu_settings_set(S_INFO_MSG_TOGGLE);
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            menu_settings_set_default(S_DEF_INFO_MSG);
         break;
      case SETTING_EMU_REWIND_ENABLED:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_settings_set(S_REWIND);

            if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
               menu_settings_msg(S_MSG_RESTART_RARCH, 180);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            g_settings.rewind_enable = false;
         break;
      case SETTING_EMU_REWIND_GRANULARITY:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            if (g_settings.rewind_granularity > 1)
               g_settings.rewind_granularity--;
         }
         if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
            g_settings.rewind_granularity++;
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            g_settings.rewind_granularity = 1;
         break;
      case SETTING_RARCH_DEFAULT_EMU:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_stack_push(LIBRETRO_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, EXT_EXECUTABLES, default_paths.core_dir);
            set_libretro_core_as_launch = true;
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
         }
         break;
      case SETTING_QUIT_RARCH:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            g_extern.lifecycle_mode_state &= ~((1ULL << MODE_GAME));
            g_extern.lifecycle_mode_state |= (1ULL << MODE_EXIT);
            return -1;
         }
         break;
      case SETTING_RESAMPLER_TYPE:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
#ifdef HAVE_SINC
            if( strstr(g_settings.audio.resampler, "hermite"))
               snprintf(g_settings.audio.resampler, sizeof(g_settings.audio.resampler), "sinc");
            else
#endif
               snprintf(g_settings.audio.resampler, sizeof(g_settings.audio.resampler), "hermite");

            if (g_extern.main_is_init)
            {
               if (!rarch_resampler_realloc(&g_extern.audio_data.resampler_data, &g_extern.audio_data.resampler,
                        g_settings.audio.resampler, g_extern.audio_data.orig_src_ratio == 0.0 ? 1.0 : g_extern.audio_data.orig_src_ratio))
               {
                  RARCH_ERR("Failed to initialize resampler \"%s\".\n", g_settings.audio.resampler);
                  g_extern.audio_active = false;
               }
            }

         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
#ifdef HAVE_SINC
            snprintf(g_settings.audio.resampler, sizeof(g_settings.audio.resampler), "sinc");
#else
            snprintf(g_settings.audio.resampler, sizeof(g_settings.audio.resampler), "hermite");
#endif

            if (g_extern.main_is_init)
            {
               if (!rarch_resampler_realloc(&g_extern.audio_data.resampler_data, &g_extern.audio_data.resampler,
                        g_settings.audio.resampler, g_extern.audio_data.orig_src_ratio == 0.0 ? 1.0 : g_extern.audio_data.orig_src_ratio))
               {
                  RARCH_ERR("Failed to initialize resampler \"%s\".\n", g_settings.audio.resampler);
                  g_extern.audio_active = false;
               }
            }
         }
         break;
      case SETTING_EMU_AUDIO_MUTE:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
            menu_settings_set(S_AUDIO_MUTE);

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            menu_settings_set_default(S_DEF_AUDIO_MUTE);
         break;
#ifdef _XBOX1
      case SETTING_EMU_AUDIO_SOUND_VOLUME_LEVEL:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            g_extern.console.sound.volume_level = !g_extern.console.sound.volume_level;
            if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
               menu_settings_msg(S_MSG_RESTART_RARCH, 180);
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            g_extern.console.sound.volume_level = 0;
            if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
               menu_settings_msg(S_MSG_RESTART_RARCH, 180);
         }
         break;
#endif
      case SETTING_ENABLE_CUSTOM_BGM:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
#if(CELL_SDK_VERSION > 0x340000)
            if (g_extern.lifecycle_mode_state & (1ULL << MODE_AUDIO_CUSTOM_BGM_ENABLE))
               g_extern.lifecycle_mode_state &= ~(1ULL << MODE_AUDIO_CUSTOM_BGM_ENABLE);
            else
               g_extern.lifecycle_mode_state |= (1ULL << MODE_AUDIO_CUSTOM_BGM_ENABLE);
            if (g_extern.lifecycle_mode_state & (1ULL << MODE_AUDIO_CUSTOM_BGM_ENABLE))
               cellSysutilEnableBgmPlayback();
            else
               cellSysutilDisableBgmPlayback();

#endif
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
#if(CELL_SDK_VERSION > 0x340000)
            g_extern.lifecycle_mode_state |= (1ULL << MODE_AUDIO_CUSTOM_BGM_ENABLE);
#endif
         }
         break;
      case SETTING_EMU_VIDEO_DEFAULT_ALL:
         break;
      case SETTING_EMU_AUDIO_DEFAULT_ALL:
         break;
      case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_stack_push(PATH_DEFAULT_ROM_DIR_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, "empty", default_paths.filesystem_root_dir);
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            strlcpy(g_extern.console.main_wrap.default_rom_startup_dir, default_paths.filesystem_root_dir, sizeof(g_extern.console.main_wrap.default_rom_startup_dir));
         break;
      case SETTING_PATH_SAVESTATES_DIRECTORY:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_stack_push(PATH_SAVESTATES_DIR_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, "empty", default_paths.filesystem_root_dir);
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            strlcpy(g_extern.console.main_wrap.default_savestate_dir, default_paths.savestate_dir, sizeof(g_extern.console.main_wrap.default_savestate_dir));

         break;
      case SETTING_PATH_SRAM_DIRECTORY:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_stack_push(PATH_SRAM_DIR_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, "empty", default_paths.filesystem_root_dir);
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            strlcpy(g_extern.console.main_wrap.default_sram_dir, default_paths.sram_dir, sizeof(g_extern.console.main_wrap.default_sram_dir));
         break;
#ifdef HAVE_XML
      case SETTING_PATH_CHEATS:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_stack_push(PATH_CHEATS_DIR_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, "empty", default_paths.filesystem_root_dir);
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            strlcpy(g_settings.cheat_database, default_paths.port_dir, sizeof(g_settings.cheat_database));
         break;
#endif
      case SETTING_PATH_SYSTEM:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_stack_push(PATH_SYSTEM_DIR_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, "empty", default_paths.system_dir);
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            strlcpy(g_settings.system_directory, default_paths.system_dir, sizeof(g_settings.system_directory));
         break;
      case SETTING_ENABLE_SRAM_PATH:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)))
         {
            if (g_extern.lifecycle_mode_state & (1ULL << MODE_LOAD_GAME_SRAM_DIR_ENABLE))
               g_extern.lifecycle_mode_state &= ~(1ULL << MODE_LOAD_GAME_SRAM_DIR_ENABLE);
            else
               g_extern.lifecycle_mode_state |= (1ULL << MODE_LOAD_GAME_SRAM_DIR_ENABLE);
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            g_extern.lifecycle_mode_state |= (1ULL << MODE_LOAD_GAME_SRAM_DIR_ENABLE);
         break;
      case SETTING_ENABLE_STATE_PATH:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)))
         {
            if (g_extern.lifecycle_mode_state & (1ULL << MODE_LOAD_GAME_STATE_DIR_ENABLE))
               g_extern.lifecycle_mode_state &= ~(1ULL << MODE_LOAD_GAME_STATE_DIR_ENABLE);
            else
               g_extern.lifecycle_mode_state |= (1ULL << MODE_LOAD_GAME_STATE_DIR_ENABLE);
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            g_extern.lifecycle_mode_state |= (1ULL << MODE_LOAD_GAME_STATE_DIR_ENABLE);
         break;
      case SETTING_PATH_DEFAULT_ALL:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)) || (input & (1ULL << RMENU_DEVICE_NAV_START)))
         {
            strlcpy(g_extern.console.main_wrap.default_rom_startup_dir, "/", sizeof(g_extern.console.main_wrap.default_rom_startup_dir));
            strlcpy(g_extern.console.main_wrap.default_savestate_dir, default_paths.port_dir, sizeof(g_extern.console.main_wrap.default_savestate_dir));
#ifdef HAVE_XML
            strlcpy(g_settings.cheat_database, default_paths.port_dir, sizeof(g_settings.cheat_database));
#endif
            strlcpy(g_extern.console.main_wrap.default_sram_dir, "", sizeof(g_extern.console.main_wrap.default_sram_dir));
         }
         break;
      case SETTING_CONTROLS_SCHEME:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)) || (input & (1ULL << RMENU_DEVICE_NAV_START)))
         {
            menu_stack_push(INPUT_PRESET_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, EXT_INPUT_PRESETS, default_paths.input_presets_dir);
         }
         break;
      case SETTING_CONTROLS_NUMBER:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            if(currently_selected_controller_menu != 0)
               currently_selected_controller_menu--;
         }

         if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if (currently_selected_controller_menu < 6)
               currently_selected_controller_menu++;
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            currently_selected_controller_menu = 0;
         break; 
      case SETTING_DPAD_EMULATION:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            if (driver.input->set_keybinds)
            {
               unsigned keybind_action = 0;

               switch(g_settings.input.dpad_emulation[currently_selected_controller_menu])
               {
                  case ANALOG_DPAD_NONE:
                     break;
                  case ANALOG_DPAD_LSTICK:
                     keybind_action = (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_NONE);
                     break;
                  case ANALOG_DPAD_RSTICK:
                     keybind_action = (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_LSTICK);
                     break;
                  default:
                     break;
               }

               if (keybind_action)
                  driver.input->set_keybinds(driver.input_data, g_settings.input.device[currently_selected_controller_menu], currently_selected_controller_menu, 0, keybind_action);
            }
         }

         if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            if (driver.input->set_keybinds)
            {
               unsigned keybind_action = 0;

               switch(g_settings.input.dpad_emulation[currently_selected_controller_menu])
               {
                  case ANALOG_DPAD_NONE:
                     keybind_action = (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_LSTICK);
                     break;
                  case ANALOG_DPAD_LSTICK:
                     keybind_action = (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_RSTICK);
                     break;
                  case ANALOG_DPAD_RSTICK:
                     break;
               }

               if (keybind_action)
                  driver.input->set_keybinds(driver.input_data, g_settings.input.device[currently_selected_controller_menu], currently_selected_controller_menu, 0, keybind_action);
            }
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
            if (driver.input->set_keybinds)
                  driver.input->set_keybinds(driver.input_data, g_settings.input.device[currently_selected_controller_menu], currently_selected_controller_menu, 0, (1ULL << KEYBINDS_ACTION_SET_ANALOG_DPAD_LSTICK));
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_UP:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_UP, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_DOWN:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_DOWN, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_LEFT:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_LEFT, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_RIGHT:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_RIGHT, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_A:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_A, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_B:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_B, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_X:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_X, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_Y:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_Y, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_SELECT:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_SELECT, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_START:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_START, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_L:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_L, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_R:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_R, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_L2:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_L2, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_R2:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_R2, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_L3:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_L3, input);
         break;
      case SETTING_CONTROLS_RETRO_DEVICE_ID_JOYPAD_R3:
         set_keybind_digital(RETRO_DEVICE_ID_JOYPAD_R3, input);
         break;
#ifdef HAVE_OSKUTIL
      case SETTING_CONTROLS_SAVE_CUSTOM_CONTROLS:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)) || (input & (1ULL << RMENU_DEVICE_NAV_START)))
         {
            rmenu_state_t *rstate = (rmenu_state_t*)&rmenu_state;
            rstate->osk_param = INPUT_PRESET_FILE;
            rstate->osk_init = osk_callback_enter_filename_init;
            rstate->osk_callback = osk_callback_enter_filename;
         }
         break;
#endif
      case SETTING_CONTROLS_DEFAULT_ALL:
         if((input & (1ULL << RMENU_DEVICE_NAV_LEFT)) || (input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)) || (input & (1ULL << RMENU_DEVICE_NAV_START)))
            if (driver.input->set_keybinds)
               driver.input->set_keybinds(driver.input_data, g_settings.input.device[currently_selected_controller_menu], currently_selected_controller_menu, 0,
                     (1ULL << KEYBINDS_ACTION_SET_DEFAULT_BINDS));
         break;
   }

   return 0;
}

static int select_setting(void *data, void *state)
{
   menu *current_menu = (menu*)data;
   rmenu_state_t *rstate = (rmenu_state_t*)state;
   font_params_t font_parms = {0};

   uint64_t input = rstate->input;
   int ret = 0;

   DEVICE_CAST device_ptr = (DEVICE_CAST)driver.video_data;
   item *items = (item*)malloc(current_menu->max_settings * sizeof(*items));
   unsigned i;
   char msg[256];

   filebrowser_t *filebrowser = NULL;
   rmenu_default_positions_t default_pos;

   menu_set_default_pos(&default_pos);

   unsigned j = 0;
   int page = 0;
   for(i = current_menu->first_setting; i < current_menu->max_settings; i++)
   {
      populate_setting_item(&items[i], i);

      if(!(j < default_pos.entries_per_page))
      {
         j = 0;
         page++;
      }

      items[i].page = page;
      j++;
   }

   /* back to ROM menu if CIRCLE is pressed */
   if ((input & (1ULL << RMENU_DEVICE_NAV_L1)) || (input & (1ULL << RMENU_DEVICE_NAV_A)))
      menu_stack_pop();
   else if (input & (1ULL << RMENU_DEVICE_NAV_R1))
   {
      switch(current_menu->enum_id)
      {
         case GENERAL_VIDEO_MENU:
         case GENERAL_AUDIO_MENU:
         case EMU_GENERAL_MENU:
         case EMU_VIDEO_MENU:
         case EMU_AUDIO_MENU:
         case PATH_MENU:
            menu_stack_push(current_menu->enum_id + 1);
            break;
         case CONTROLS_MENU:
         default:
            break;
      }
   }
   else if (input & (1ULL << RMENU_DEVICE_NAV_DOWN))
   {
      current_menu->selected++;

      if (current_menu->selected >= current_menu->max_settings)
         current_menu->selected = current_menu->first_setting; 
      if (items[current_menu->selected].page != current_menu->page)
         current_menu->page = items[current_menu->selected].page;
   }
   else if (input & (1ULL << RMENU_DEVICE_NAV_UP))
   {
      if (current_menu->selected == current_menu->first_setting)
         current_menu->selected = current_menu->max_settings-1;
      else
         current_menu->selected--;

      if (items[current_menu->selected].page != current_menu->page)
         current_menu->page = items[current_menu->selected].page;
   }

   ret = set_setting_action(current_menu, current_menu->selected, input);

   if (ret != 0)
      return ret;

   display_menubar(current_menu);

   for(i = current_menu->first_setting; i < current_menu->max_settings; i++)
   {
      if(items[i].page == current_menu->page)
      {
         default_pos.starting_y_position += default_pos.y_position_increment;

         font_parms.x = default_pos.x_position;
         font_parms.y = default_pos.starting_y_position;
         font_parms.scale = default_pos.variable_font_size;
         font_parms.color = current_menu->selected == items[i].enum_id ? YELLOW : WHITE;

         if (driver.video_poke->set_osd_msg)
            driver.video_poke->set_osd_msg(driver.video_data, items[i].text, &font_parms);

         font_parms.x = default_pos.x_position_center;
         font_parms.color = WHITE;

         if (driver.video_poke->set_osd_msg)
            driver.video_poke->set_osd_msg(driver.video_data, items[i].setting_text, &font_parms);

         if(current_menu->selected == items[i].enum_id)
         {
            rarch_position_t position = {0};
            position.x = default_pos.x_position;
            position.y = default_pos.starting_y_position;

            device_ptr->ctx_driver->rmenu_draw_panel(&position);

            font_parms.x = default_pos.x_position;
            font_parms.y = default_pos.comment_y_position;
            font_parms.scale = default_pos.font_size;
            font_parms.color = WHITE;

            if (driver.video_poke->set_osd_msg)
               driver.video_poke->set_osd_msg(driver.video_data, items[i].comment, &font_parms);
         }
      }
   }

   free(items);

   struct platform_bind key_label_l3, key_label_r3, key_label_start;
   strlcpy(key_label_l3.desc, "Unknown", sizeof(key_label_l3.desc));
   key_label_l3.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_L3;
   strlcpy(key_label_r3.desc, "Unknown", sizeof(key_label_r3.desc));
   key_label_r3.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_R3;
   strlcpy(key_label_start.desc, "Unknown", sizeof(key_label_start.desc));
   key_label_start.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_START;

   if (driver.input->set_keybinds)
   {
      driver.input->set_keybinds(&key_label_l3, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_r3, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_start, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
   }

   snprintf(msg, sizeof(msg), "[%s] + [%s] - Resume game", key_label_l3.desc, key_label_r3.desc);

   font_parms.x = default_pos.x_position;
   font_parms.y = default_pos.comment_two_y_position;
   font_parms.scale = default_pos.font_size;
   font_parms.color = YELLOW;

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

   snprintf(msg, sizeof(msg), "[%s] - Reset to default", key_label_start.desc);
   font_parms.y = default_pos.comment_two_y_position + (default_pos.y_position_increment * 1);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

   if(current_menu->browser_draw)
      current_menu->browser_draw(filebrowser);

   return 0;
}


int select_rom(void *data, void *state)
{
   menu *current_menu = (menu*)data;
   rmenu_state_t *rstate = (rmenu_state_t*)state;
   font_params_t font_parms = {0};

   uint64_t input = rstate->input;

   char msg[128];
   rmenu_default_positions_t default_pos;
   filebrowser_t *filebrowser = browser;

   struct platform_bind key_label_b, key_label_l3, key_label_r3, key_label_select;
   strlcpy(key_label_b.desc, "Unknown", sizeof(key_label_b.desc));
   key_label_b.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_B;
   strlcpy(key_label_l3.desc, "Unknown", sizeof(key_label_l3.desc));
   key_label_l3.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_L3;
   strlcpy(key_label_r3.desc, "Unknown", sizeof(key_label_r3.desc));
   key_label_r3.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_R3;
   strlcpy(key_label_select.desc, "Unknown", sizeof(key_label_select.desc));
   key_label_select.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_SELECT;

   if (driver.input->set_keybinds)
   {
      driver.input->set_keybinds(&key_label_l3, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_r3, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_select, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_b, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
   }

   menu_set_default_pos(&default_pos);

   browser_update(filebrowser, input, g_extern.system.valid_extensions);

   if (input & (1ULL << RMENU_DEVICE_NAV_SELECT))
      menu_stack_push(GENERAL_VIDEO_MENU);
   else if (input & (1ULL << RMENU_DEVICE_NAV_B))
   {
      if (filebrowser_iterate(filebrowser, FILEBROWSER_ACTION_PATH_ISDIR))
      {
         bool ret = filebrowser_iterate(filebrowser, FILEBROWSER_ACTION_OK);

         if(!ret)
            menu_settings_msg(S_MSG_DIR_LOADING_ERROR, 180);
      }
      else
      {
         strlcpy(g_extern.fullpath, filebrowser_get_current_path(filebrowser), sizeof(g_extern.fullpath));
         g_extern.lifecycle_mode_state |= (1ULL << MODE_LOAD_GAME);
      }
   }
   else if (input & (1ULL << RMENU_DEVICE_NAV_L1))
   {
      const char * drive_map = menu_drive_mapping_previous();
      if(drive_map != NULL)
      {
         filebrowser_set_root_and_ext(filebrowser, g_extern.system.valid_extensions, drive_map);
         browser_update(filebrowser, 1ULL << RMENU_DEVICE_NAV_B, g_extern.system.valid_extensions);
      }
   }
   else if (input & (1ULL << RMENU_DEVICE_NAV_R1))
   {
      const char * drive_map = menu_drive_mapping_next();
      if(drive_map != NULL)
      {
         filebrowser_set_root_and_ext(filebrowser, g_extern.system.valid_extensions, drive_map);
         browser_update(filebrowser, 1ULL << RMENU_DEVICE_NAV_B, g_extern.system.valid_extensions);
      }
   }

   if (filebrowser_iterate(filebrowser, FILEBROWSER_ACTION_PATH_ISDIR))
   {
      const char *current_path = filebrowser_get_current_path(filebrowser);
      snprintf(msg, sizeof(msg), "INFO - Press [%s] to enter the directory.", key_label_b.desc);
   }
   else
      snprintf(msg, sizeof(msg), "INFO - Press [%s] to load the game.", key_label_b.desc);

   font_parms.x = default_pos.x_position;
   font_parms.y = default_pos.comment_y_position;
   font_parms.scale = default_pos.font_size;
   font_parms.color = WHITE;

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

   display_menubar(current_menu);

   snprintf(msg, sizeof(msg), "[%s] + [%s] - resume game", key_label_l3.desc, key_label_r3.desc);

   font_parms.y = default_pos.comment_two_y_position;
   font_parms.color = YELLOW;

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

   snprintf(msg, sizeof(msg), "[%s] - Settings", key_label_select.desc);

   font_parms.y = default_pos.comment_two_y_position + (default_pos.y_position_increment * 1);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

   if(current_menu->browser_draw)
      current_menu->browser_draw(filebrowser);

   return 0;
}

int ingame_menu_resize(void *data, void *state)
{
   menu *current_menu = (menu*)data;
   rmenu_state_t *rstate = (rmenu_state_t*)state;
   font_params_t font_parms = {0};

   uint64_t input = rstate->input;
   DEVICE_CAST device_ptr = (DEVICE_CAST)driver.video_data;

   filebrowser_t *filebrowser = NULL;
   rmenu_default_positions_t default_pos;
   menu_set_default_pos(&default_pos);

   g_settings.video.aspect_ratio_idx = ASPECT_RATIO_CUSTOM;
   
   if (driver.video_poke->set_aspect_ratio)
      driver.video_poke->set_aspect_ratio(driver.video_data, g_settings.video.aspect_ratio_idx);

   if(input & (1ULL << RMENU_DEVICE_NAV_LEFT_ANALOG_L))
   {
#ifdef _XBOX
      if(g_extern.console.screen.viewports.custom_vp.x >= 4)
#endif
         g_extern.console.screen.viewports.custom_vp.x -= 4;
   }
   else if(input & (1ULL << RMENU_DEVICE_NAV_LEFT) && (input & ~(1ULL << RMENU_DEVICE_NAV_LEFT_ANALOG_L)))
   {
#ifdef _XBOX
      if(g_extern.console.screen.viewports.custom_vp.x > 0)
#endif
         g_extern.console.screen.viewports.custom_vp.x -= 1;
   }

   if(input & (1ULL << RMENU_DEVICE_NAV_RIGHT_ANALOG_L))
      g_extern.console.screen.viewports.custom_vp.x += 4;
   else if(input & (1ULL << RMENU_DEVICE_NAV_RIGHT) && (input & ~(1ULL << RMENU_DEVICE_NAV_RIGHT_ANALOG_L)))
      g_extern.console.screen.viewports.custom_vp.x += 1;

   if(input & (1ULL << RMENU_DEVICE_NAV_UP_ANALOG_L))
      g_extern.console.screen.viewports.custom_vp.y += 4;
   else if(input & (1ULL << RMENU_DEVICE_NAV_UP) && (input & ~(1ULL << RMENU_DEVICE_NAV_UP_ANALOG_L)))
      g_extern.console.screen.viewports.custom_vp.y += 1;

   if(input & (1ULL << RMENU_DEVICE_NAV_DOWN_ANALOG_L))
   {
#ifdef _XBOX
      if(g_extern.console.screen.viewports.custom_vp.y >= 4)
#endif
         g_extern.console.screen.viewports.custom_vp.y -= 4;
   }
   else if(input & (1ULL << RMENU_DEVICE_NAV_DOWN) && (input & ~(1ULL << RMENU_DEVICE_NAV_DOWN_ANALOG_L)))
   {
#ifdef _XBOX
      if(g_extern.console.screen.viewports.custom_vp.y > 0)
#endif
         g_extern.console.screen.viewports.custom_vp.y -= 1;
   }

   if(input & (1ULL << RMENU_DEVICE_NAV_LEFT_ANALOG_R))
      g_extern.console.screen.viewports.custom_vp.width -= 4;
   else if(input & (1ULL << RMENU_DEVICE_NAV_L1) && (input && ~(1ULL << RMENU_DEVICE_NAV_LEFT_ANALOG_R)))
      g_extern.console.screen.viewports.custom_vp.width -= 1;

   if (input & (1ULL << RMENU_DEVICE_NAV_RIGHT_ANALOG_R))
      g_extern.console.screen.viewports.custom_vp.width += 4;
   else if(input & (1ULL << RMENU_DEVICE_NAV_R1) && (input & ~(1ULL << RMENU_DEVICE_NAV_RIGHT_ANALOG_R)))
      g_extern.console.screen.viewports.custom_vp.width += 1;

   if(input & (1ULL << RMENU_DEVICE_NAV_UP_ANALOG_R))
      g_extern.console.screen.viewports.custom_vp.height += 4;
   else if(input & (1ULL << RMENU_DEVICE_NAV_L2) && (input & ~(1ULL << RMENU_DEVICE_NAV_UP_ANALOG_R)))
      g_extern.console.screen.viewports.custom_vp.height += 1;

   if(input & (1ULL << RMENU_DEVICE_NAV_DOWN_ANALOG_R))
      g_extern.console.screen.viewports.custom_vp.height -= 4;
   else if (input & (1ULL << RMENU_DEVICE_NAV_R2) && (input & ~(1ULL << RMENU_DEVICE_NAV_DOWN_ANALOG_R)))
      g_extern.console.screen.viewports.custom_vp.height -= 1;

   if (input & (1ULL << RMENU_DEVICE_NAV_X))
   {
      g_extern.console.screen.viewports.custom_vp.x = 0;
      g_extern.console.screen.viewports.custom_vp.y = 0;
      g_extern.console.screen.viewports.custom_vp.width = device_ptr->win_width;
      g_extern.console.screen.viewports.custom_vp.height = device_ptr->win_height;
   }

   if (input & (1ULL << RMENU_DEVICE_NAV_A))
   {
      menu_stack_pop();
      g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_DRAW);
   }

   if((input & (1ULL << RMENU_DEVICE_NAV_Y)))
   {
      if (g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_DRAW))
         g_extern.lifecycle_mode_state &= ~(1ULL << MODE_MENU_DRAW);
      else
         g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_DRAW);
   }

   if(g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_DRAW))
   {
      char viewport_x[32], viewport_y[32], viewport_w[32], viewport_h[32];
      char msg[256];
      struct platform_bind key_label_b, key_label_a,
                           key_label_y, key_label_x,
                           key_label_l1, key_label_l2,
                           key_label_r1, key_label_r2, key_label_select,
                           key_label_dpad_left, key_label_dpad_right,
                           key_label_dpad_up, key_label_dpad_down;
      strlcpy(key_label_b.desc, "Unknown", sizeof(key_label_b.desc));
      key_label_b.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_B;
      strlcpy(key_label_a.desc, "Unknown", sizeof(key_label_a.desc));
      key_label_a.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_A;
      strlcpy(key_label_x.desc, "Unknown", sizeof(key_label_x.desc));
      key_label_x.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_X;
      strlcpy(key_label_y.desc, "Unknown", sizeof(key_label_y.desc));
      key_label_y.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_Y;
      strlcpy(key_label_l1.desc, "Unknown", sizeof(key_label_l1.desc));
      key_label_l1.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_L;
      strlcpy(key_label_r1.desc, "Unknown", sizeof(key_label_r1.desc));
      key_label_r1.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_R;
      strlcpy(key_label_l2.desc, "Unknown", sizeof(key_label_l2.desc));
      key_label_l2.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_L2;
      strlcpy(key_label_r2.desc, "Unknown", sizeof(key_label_r2.desc));
      key_label_r2.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_R2;
      strlcpy(key_label_select.desc, "Unknown", sizeof(key_label_select.desc));
      key_label_select.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_SELECT;
      strlcpy(key_label_dpad_left.desc, "Unknown", sizeof(key_label_dpad_left.desc));
      key_label_dpad_left.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_LEFT;
      strlcpy(key_label_dpad_right.desc, "Unknown", sizeof(key_label_dpad_left.desc));
      key_label_dpad_right.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_RIGHT;
      strlcpy(key_label_dpad_up.desc, "Unknown", sizeof(key_label_dpad_up.desc));
      key_label_dpad_up.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_UP;
      strlcpy(key_label_dpad_down.desc, "Unknown", sizeof(key_label_dpad_down.desc));
      key_label_dpad_down.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_DOWN;

      if (driver.input->set_keybinds)
      {
         driver.input->set_keybinds(&key_label_l1, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_r1, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_l2, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_r2, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_select, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_b, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_a, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_y, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_x, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_dpad_left, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_dpad_right, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_dpad_up, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
         driver.input->set_keybinds(&key_label_dpad_down, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      }

      display_menubar(current_menu);

      snprintf(viewport_x, sizeof(viewport_x), "Viewport X: #%d", g_extern.console.screen.viewports.custom_vp.x);
      snprintf(viewport_y, sizeof(viewport_y), "Viewport Y: #%d", g_extern.console.screen.viewports.custom_vp.y);
      snprintf(viewport_w, sizeof(viewport_w), "Viewport W: #%d", g_extern.console.screen.viewports.custom_vp.width);
      snprintf(viewport_h, sizeof(viewport_h), "Viewport H: #%d", g_extern.console.screen.viewports.custom_vp.height);

      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position;
      font_parms.scale = default_pos.font_size;
      font_parms.color = GREEN;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, viewport_x, &font_parms);

      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 1);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, viewport_y, &font_parms);

      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 2);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, viewport_w, &font_parms);

      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 3);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, viewport_h, &font_parms);

      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 4);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "CONTROLS:", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_dpad_left.desc);

      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 5);
      font_parms.color = WHITE;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 5);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "- Viewport X--", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_dpad_right.desc);

      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 6);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "- Viewport X++", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_dpad_up.desc);
      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 7);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "- Viewport Y++", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_dpad_down.desc);

      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 8);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "- Viewport Y--", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_l1.desc);

      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 9);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "- Viewport W--", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_r1.desc);

      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 10);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "- Viewport W++", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_l2.desc);

      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 11);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "- Viewport H++", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_r2.desc);

      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 12);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;
      
      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "- Viewport H--", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_x.desc);

      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 13);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "- Reset To Defaults", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_y.desc);

      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 14);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, "- Show Game", &font_parms);

      snprintf(msg, sizeof(msg), "[%s]", key_label_a.desc);
      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.y_position + (default_pos.y_position_increment * 15);

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(device_ptr, msg, &font_parms);

      font_parms.x = default_pos.x_position_center;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(device_ptr, "- Go back", &font_parms);

      snprintf(msg, sizeof(msg), "Press [%s] to reset to defaults.", key_label_x.desc);
      font_parms.x = default_pos.x_position;
      font_parms.y = default_pos.comment_y_position;

      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);
   }

   if(current_menu->browser_draw)
      current_menu->browser_draw(filebrowser);

   return 0;
}

int ingame_menu_screenshot(void *data, void *state)
{
   menu *current_menu = (menu*)data;
   rmenu_state_t *rstate = (rmenu_state_t*)state;

   uint64_t input = rstate->input;

   DEVICE_CAST device_ptr = (DEVICE_CAST)driver.video_data;
   filebrowser_t *filebrowser = NULL;

   g_extern.lifecycle_mode_state &= ~(1ULL << MODE_MENU_DRAW);

   if (g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_INGAME))
   {
      if (input & (1ULL << RMENU_DEVICE_NAV_A))
      {
         menu_stack_pop();
         g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_DRAW);
      }

      if(input & (1ULL << RMENU_DEVICE_NAV_B))
         if(device_ptr->ctx_driver->rmenu_screenshot_dump)
            device_ptr->ctx_driver->rmenu_screenshot_dump(NULL);
   }

   if(current_menu->browser_draw)
      current_menu->browser_draw(filebrowser);

   return 0;
}

#define MENU_ITEM_SELECTED(index) (menuitem_colors[index])

int ingame_menu(void *data, void *state)
{
   menu *current_menu    = (menu*)data;
   rmenu_state_t *rstate = (rmenu_state_t*)state;

   int ret = 0;
   uint64_t input = rstate->input;
   DEVICE_CAST device_ptr = (DEVICE_CAST)driver.video_data;
   char strw_buffer[256];
   unsigned menuitem_colors[MENU_ITEM_LAST];
   static unsigned menu_idx = 0;
   font_params_t font_parms = {0};

   filebrowser_t *filebrowser = tmpBrowser;
   rmenu_default_positions_t default_pos;
   menu_set_default_pos(&default_pos);

   for(int i = 0; i < MENU_ITEM_LAST; i++)
      menuitem_colors[i] = WHITE;

   menuitem_colors[menu_idx] = RED;

   if(input & (1ULL << RMENU_DEVICE_NAV_A))
   {
      g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_INGAME_EXIT);
      g_extern.lifecycle_mode_state |= (1ULL << MODE_GAME);
      return -1;
   }

   struct platform_bind key_label_b, key_label_a,
                        key_label_start;
   strlcpy(key_label_b.desc, "Unknown", sizeof(key_label_b.desc));
   key_label_b.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_B;
   strlcpy(key_label_a.desc, "Unknown", sizeof(key_label_a.desc));
   key_label_a.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_A;
   strlcpy(key_label_start.desc, "Unknown", sizeof(key_label_start.desc));
   key_label_start.joykey = 1ULL << RETRO_DEVICE_ID_JOYPAD_START;

   if (driver.input->set_keybinds)
   {
      driver.input->set_keybinds(&key_label_start, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_b, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
      driver.input->set_keybinds(&key_label_a, 0, 0, 0, (1ULL << KEYBINDS_ACTION_GET_BIND_LABEL));
   }

   switch(menu_idx)
   {
      case MENU_ITEM_LOAD_STATE:
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
         {
            rarch_load_state();
            g_extern.lifecycle_mode_state |= (1ULL << MODE_GAME);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_INGAME_EXIT);
            return -1;
         }
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
            rarch_state_slot_decrease();
         if(input & (1ULL << RMENU_DEVICE_NAV_RIGHT))
            rarch_state_slot_increase();

         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to load the current state.", key_label_b.desc);
         break;
      case MENU_ITEM_SAVE_STATE:
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
         {
            rarch_save_state();
            g_extern.lifecycle_mode_state |= (1ULL << MODE_GAME);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_INGAME_EXIT);
            return -1;
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
            rarch_state_slot_decrease();
         if(input & (1ULL << RMENU_DEVICE_NAV_RIGHT))
            rarch_state_slot_increase();

         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to save the current state.", key_label_b.desc);
         break;
      case MENU_ITEM_KEEP_ASPECT_RATIO:
         ret = set_setting_action(current_menu, SETTING_KEEP_ASPECT_RATIO, input);

         if (ret != 0)
            return ret;

         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to reset back to default values.", key_label_start.desc);
         break;
      case MENU_ITEM_OVERSCAN_AMOUNT:
         ret = set_setting_action(current_menu, SETTING_HW_OVERSCAN_AMOUNT, input);

         if (ret != 0)
            return ret;

         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to reset back to default values.", key_label_start.desc);
         break;
      case MENU_ITEM_ORIENTATION:
         if(input & (1ULL << RMENU_DEVICE_NAV_LEFT))
         {
            menu_settings_set(S_ROTATION_DECREMENT);
            driver.video->set_rotation(NULL, g_extern.console.screen.orientation);
         }

         if((input & (1ULL << RMENU_DEVICE_NAV_RIGHT)) || (input & (1ULL << RMENU_DEVICE_NAV_B)))
         {
            menu_settings_set(S_ROTATION_INCREMENT);
            driver.video->set_rotation(NULL, g_extern.console.screen.orientation);
         }

         if(input & (1ULL << RMENU_DEVICE_NAV_START))
         {
            menu_settings_set_default(S_DEF_ROTATION);
            driver.video->set_rotation(NULL, g_extern.console.screen.orientation);
         }
         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to reset back to default values.", key_label_start.desc);
         break;
#ifdef HAVE_FBO
      case MENU_ITEM_SCALE_FACTOR:
         ret = set_setting_action(current_menu, SETTING_SCALE_FACTOR, input);

         if (ret != 0)
            return ret;

         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to reset back to default values.", key_label_start.desc);
         break;
#endif
      case MENU_ITEM_FRAME_ADVANCE:
         if((input & (1ULL << RMENU_DEVICE_NAV_B)) || (input & (1ULL << RMENU_DEVICE_NAV_R2)) || (input & (1ULL << RMENU_DEVICE_NAV_L2)))
         {
            g_extern.lifecycle_state |= (1ULL << RARCH_FRAMEADVANCE);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_INGAME_EXIT);
            menu_settings_set(S_FRAME_ADVANCE);
            menu_idx = MENU_ITEM_FRAME_ADVANCE;
            return -1;
         }
         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to step one frame.", key_label_b.desc);
         break;
      case MENU_ITEM_RESIZE_MODE:
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
            menu_stack_push(INGAME_MENU_RESIZE);
         snprintf(strw_buffer, sizeof(strw_buffer), "Allows you to resize the screen.");
         break;
      case MENU_ITEM_SCREENSHOT_MODE:
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
            menu_stack_push(INGAME_MENU_SCREENSHOT);
         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to go back to the in-game menu.", key_label_a.desc);
         break;
      case MENU_ITEM_RETURN_TO_GAME:
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
         {
            g_extern.lifecycle_mode_state |= (1ULL << MODE_GAME);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_INGAME_EXIT);
            return -1;
         }

         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to return to the game.", key_label_b.desc);
         break;
      case MENU_ITEM_RESET:
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
         {
            rarch_game_reset();
            g_extern.lifecycle_mode_state |= (1ULL << MODE_GAME);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_INGAME_EXIT);
            return -1;
         }
         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to reset the game.", key_label_b.desc);
         break;
      case MENU_ITEM_RETURN_TO_MENU:
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
         {
            menu_idx = 0;
            g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_INGAME_EXIT);
            return 0;
         }
         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to return to the ROM Browser.", key_label_b.desc);
         break;
      case MENU_ITEM_CHANGE_LIBRETRO:
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
         {
            menu_stack_push(LIBRETRO_CHOICE);
            filebrowser_set_root_and_ext(filebrowser, EXT_EXECUTABLES, default_paths.core_dir);
            set_libretro_core_as_launch = true;
         }
         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to choose another core.", key_label_b.desc);
         break;
#ifdef HAVE_MULTIMAN
      case MENU_ITEM_RETURN_TO_MULTIMAN:
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
         {
            RARCH_LOG("Boot Multiman: %s.\n", default_paths.multiman_self_file);
            strlcpy(g_extern.fullpath, default_paths.multiman_self_file, sizeof(g_extern.fullpath));
            g_extern.lifecycle_mode_state &= ~(1ULL << MODE_GAME);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_INGAME_EXIT);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_EXIT);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_EXITSPAWN);
            return -1;
         }
         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to quit RetroArch and return to multiMAN.", key_label_b.desc);
         break;
#endif
      case MENU_ITEM_QUIT_RARCH:
         if(input & (1ULL << RMENU_DEVICE_NAV_B))
         {
            g_extern.lifecycle_mode_state &= ~(1ULL << MODE_GAME);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_EXIT);
            g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_INGAME_EXIT);
            return -1;
         }

         snprintf(strw_buffer, sizeof(strw_buffer), "Press [%s] to quit RetroArch.", key_label_b.desc);
         break;
   }

   if(input & (1ULL << RMENU_DEVICE_NAV_UP))
   {
      if (menu_idx > 0)
         menu_idx--;
      else
         menu_idx = MENU_ITEM_LAST - 1;
   }

   if(input & (1ULL << RMENU_DEVICE_NAV_DOWN))
   {
      if(menu_idx < (MENU_ITEM_LAST-1))
         menu_idx++;
      else
         menu_idx = 0;
   }

   display_menubar(current_menu);

   font_parms.x = default_pos.x_position;
   font_parms.y = default_pos.comment_y_position;
   font_parms.scale = default_pos.font_size;
   font_parms.color = WHITE;

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, strw_buffer, &font_parms);

   menu_settings_create_menu_item_label(strw_buffer, S_LBL_LOAD_STATE_SLOT, sizeof(strw_buffer));

   font_parms.y = default_pos.y_position;
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_LOAD_STATE);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, strw_buffer, &font_parms);

   menu_settings_create_menu_item_label(strw_buffer, S_LBL_SAVE_STATE_SLOT, sizeof(strw_buffer));

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_SAVE_STATE);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_SAVE_STATE);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, strw_buffer, &font_parms);

   menu_settings_create_menu_item_label(strw_buffer, S_LBL_ASPECT_RATIO, sizeof(strw_buffer));

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_KEEP_ASPECT_RATIO);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_KEEP_ASPECT_RATIO);
   
   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, strw_buffer, &font_parms);

   snprintf(strw_buffer, sizeof(strw_buffer), "Overscan: %f", g_extern.console.screen.overscan_amount);

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_OVERSCAN_AMOUNT);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_OVERSCAN_AMOUNT);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, strw_buffer, &font_parms);

   menu_settings_create_menu_item_label(strw_buffer, S_LBL_ROTATION, sizeof(strw_buffer));

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_ORIENTATION);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_ORIENTATION);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, strw_buffer, &font_parms);

#ifdef HAVE_FBO
   menu_settings_create_menu_item_label(strw_buffer, S_LBL_SCALE_FACTOR, sizeof(strw_buffer));

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_SCALE_FACTOR);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_SCALE_FACTOR);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, strw_buffer, &font_parms);
#endif

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_RESIZE_MODE);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_RESIZE_MODE);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, "Resize Mode", &font_parms);

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_FRAME_ADVANCE);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_FRAME_ADVANCE);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, "Frame Advance", &font_parms);

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_SCREENSHOT_MODE);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_SCREENSHOT_MODE);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, "Screenshot Mode", &font_parms);

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_RESET);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_RESET);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, "Reset", &font_parms);

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_RETURN_TO_GAME);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_RETURN_TO_GAME);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, "Return To Game", &font_parms);

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_RETURN_TO_MENU);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_RETURN_TO_MENU);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, "Return To Menu", &font_parms);

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_CHANGE_LIBRETRO);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_CHANGE_LIBRETRO);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, "Change libretro core", &font_parms);

#ifdef HAVE_MULTIMAN
   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_RETURN_TO_MULTIMAN);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_RETURN_TO_MULTIMAN);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, "Return to multiMAN", &font_parms);
#endif

   font_parms.y = default_pos.y_position + (default_pos.y_position_increment * MENU_ITEM_QUIT_RARCH);
   font_parms.color = MENU_ITEM_SELECTED(MENU_ITEM_QUIT_RARCH);

   if (driver.video_poke->set_osd_msg)
      driver.video_poke->set_osd_msg(driver.video_data, "Quit RetroArch", &font_parms);

   rarch_position_t position = {0};
   position.x = default_pos.x_position;
   position.y = (default_pos.y_position+(default_pos.y_position_increment * menu_idx));
   device_ptr->ctx_driver->rmenu_draw_panel(&position);

   if(current_menu->browser_draw)
      current_menu->browser_draw(filebrowser);

   return 0;
}

/*============================================================
  INPUT POLL CALLBACK
  ============================================================ */

void menu_input_poll(void *data, void *state)
{
   menu *current_menu    = (menu*)data;

   //first button input frame
   uint64_t input_state_first_frame = 0;
   uint64_t input_state = 0;
   static bool first_held = false;

   driver.input->poll(NULL);

   for (unsigned i = 0; i < RMENU_DEVICE_NAV_LAST; i++)
      input_state |= driver.input->input_state(NULL, rmenu_nav_binds, 0,
            RETRO_DEVICE_JOYPAD, 0, i) ? (1ULL << i) : 0;

   //set first button input frame as trigger
   rmenu_state.input = input_state & ~(rmenu_state.old_state);
   //hold onto first button input frame
   input_state_first_frame = input_state;          

   //second button input frame
   input_state = 0;
   driver.input->poll(NULL);

   for (unsigned i = 0; i < RMENU_DEVICE_NAV_LAST; i++)
   {
      input_state |= driver.input->input_state(NULL, rmenu_nav_binds, 0,
            RETRO_DEVICE_JOYPAD, 0, i) ? (1ULL << i) : 0;
   }

   bool analog_sticks_pressed = (input_state & (1ULL << RMENU_DEVICE_NAV_LEFT_ANALOG_L)) || (input_state & (1ULL << RMENU_DEVICE_NAV_RIGHT_ANALOG_L)) || (input_state & (1ULL << RMENU_DEVICE_NAV_UP_ANALOG_L)) || (input_state & (1ULL << RMENU_DEVICE_NAV_DOWN_ANALOG_L)) || (input_state & (1ULL << RMENU_DEVICE_NAV_LEFT_ANALOG_R)) || (input_state & (1ULL << RMENU_DEVICE_NAV_RIGHT_ANALOG_R)) || (input_state & (1ULL << RMENU_DEVICE_NAV_UP_ANALOG_R)) || (input_state & (1ULL << RMENU_DEVICE_NAV_DOWN_ANALOG_R));
   bool shoulder_buttons_pressed = ((input_state & (1ULL << RMENU_DEVICE_NAV_L2)) || (input_state & (1ULL << RMENU_DEVICE_NAV_R2))) && current_menu->category_id != CATEGORY_SETTINGS;
   bool do_held = analog_sticks_pressed || shoulder_buttons_pressed;

   if(do_held)
   {
      if(!first_held)
      {
         first_held = true;
         g_extern.delay_timer[1] = g_extern.frame_count + 7;
      }

      if(!(g_extern.frame_count < g_extern.delay_timer[1]))
      {
         first_held = false;
         rmenu_state.input = input_state; //second input frame set as current frame
      }
   }

   rmenu_state.old_state = input_state_first_frame;
}

/*============================================================
  INPUT PROCESS CALLBACK
  ============================================================ */

int menu_input_process(void *data, void *state)
{
   (void)data;
   DEVICE_CAST device_ptr = (DEVICE_CAST)driver.video_data;
   rmenu_state_t *rstate = (rmenu_state_t*)state;

   if (g_extern.lifecycle_mode_state & (1ULL << MODE_LOAD_GAME))
   {
      if (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW))
         menu_settings_msg(S_MSG_LOADING_ROM, 100);

      g_extern.lifecycle_mode_state |= (1ULL << MODE_INIT);
      g_extern.lifecycle_mode_state &= ~(1ULL << MODE_LOAD_GAME);
      return -1;
   }

   if (!(g_extern.frame_count < g_extern.delay_timer[0]))
   {
      bool return_to_game_enable = (((rstate->old_state & (1ULL << RMENU_DEVICE_NAV_L3)) && (rstate->old_state & (1ULL << RMENU_DEVICE_NAV_R3)) && g_extern.main_is_init));

      if (return_to_game_enable)
      {
         if (!(g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_INGAME)))
         {
            g_extern.lifecycle_mode_state |= (1ULL << MODE_GAME);
            return -1;
         }
      }
   }

   bool quit, resize;
   unsigned width, height, frame_count;
   frame_count = 0;
   device_ptr->ctx_driver->check_window(&quit, &resize, &width, &height, frame_count);

   if (quit)
   {
      g_extern.lifecycle_mode_state |= (1ULL << MODE_EXIT);
      return -1;
   }

   return 0;
}

/*============================================================
  RESOURCE CALLBACKS
  ============================================================ */

void init_filebrowser(void *data)
{
   (void)data;

   browser    = (filebrowser_t*)filebrowser_init(g_extern.console.main_wrap.default_rom_startup_dir, g_extern.system.valid_extensions);
   tmpBrowser = (filebrowser_t*)filebrowser_init(default_paths.filesystem_root_dir, "");

   menu_stack_push(FILE_BROWSER_MENU);
   filebrowser_set_root_and_ext(browser, g_extern.system.valid_extensions, g_extern.console.main_wrap.default_rom_startup_dir);
   filebrowser_set_root_and_ext(tmpBrowser, NULL, default_paths.filesystem_root_dir);
}

void free_filebrowser(void *data)
{
   (void)data;

   filebrowser_free(browser);
   filebrowser_free(tmpBrowser);
}

/*============================================================
  RMENU API
  ============================================================ */


void menu_init(void)
{
   DEVICE_CAST device_ptr = (DEVICE_CAST)driver.video_data;

   rmenu_state.init_resources = init_filebrowser;
   rmenu_state.free_resources = free_filebrowser;
   rmenu_state.input          = 0;
   rmenu_state.old_state      = 0;

   if(rmenu_state.init_resources)
      rmenu_state.init_resources(&rmenu_state);

   device_ptr->ctx_driver->rmenu_init();
}

void menu_free(void)
{
   if(rmenu_state.free_resources)
      rmenu_state.free_resources(&rmenu_state);
}

bool menu_iterate(void)
{
   const char *msg;
   font_params_t font_parms = {0};
   DEVICE_CAST device_ptr = (DEVICE_CAST)driver.video_data;
   static menu current_menu;

   if (g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_PREINIT))
   {
      if (g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_INGAME))
         menu_stack_push(INGAME_MENU);

      menu_stack_force_refresh();
      g_extern.lifecycle_mode_state |= (1ULL << MODE_MENU_DRAW);

#ifndef __CELLOS_LV2__
      device_ptr->ctx_driver->rmenu_init();
#endif

      g_extern.lifecycle_mode_state &= ~(1ULL << MODE_MENU_PREINIT);
   }

   g_extern.frame_count++;

   menu_stack_get_current_ptr(&current_menu);

   rmenu_default_positions_t default_pos;
   menu_set_default_pos(&default_pos);

   if ((g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_LOW_RAM_MODE_ENABLE)))
   {
#if defined(HAVE_OPENGL)
      glClear(GL_COLOR_BUFFER_BIT);
#elif defined(HAVE_D3D8) || defined(HAVE_D3D9)
      xdk_d3d_video_t *d3d = (xdk_d3d_video_t*)driver.video_data;
      LPDIRECT3DDEVICE d3dr = (LPDIRECT3DDEVICE)d3d->d3d_render_device;
      d3dr->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f, 0);
#endif
   }
   else
   {
      if (g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_DRAW))
      {
         if (driver.video_poke->set_blend)
            driver.video_poke->set_blend(driver.video_data, true);
      }

      rarch_render_cached_frame();
   }

   if(current_menu.input_poll)
      menu_input_poll(&current_menu, &rmenu_state);

#ifdef HAVE_OSKUTIL
   if(rmenu_state.osk_init != NULL)
   {
      if (rmenu_state.osk_init(&rmenu_state))
         rmenu_state.osk_init = NULL;
   }

   if(rmenu_state.osk_callback != NULL)
   {
      if (rmenu_state.osk_callback(&rmenu_state))
         rmenu_state.osk_callback = NULL;
   }
#endif

   int input_entry_ret = 0;
   int input_process_ret = 0;

   if(current_menu.entry)
      input_entry_ret = current_menu.entry(&current_menu, &rmenu_state);

   if(current_menu.input_process)
      input_process_ret = current_menu.input_process(&current_menu, &rmenu_state);

   msg = msg_queue_pull(g_extern.msg_queue);

   font_parms.x = default_pos.msg_queue_x_position;
   font_parms.y = default_pos.msg_queue_y_position;
   font_parms.scale = default_pos.msg_queue_font_size;
   font_parms.color = WHITE;

   if (msg && (g_extern.lifecycle_mode_state & (1ULL << MODE_INFO_DRAW)))
   {
      if (driver.video_poke->set_osd_msg)
         driver.video_poke->set_osd_msg(driver.video_data, msg, &font_parms);
   }

   device_ptr->ctx_driver->swap_buffers();

   if (g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_DRAW))
   {
      if (driver.video_poke->set_blend)
         driver.video_poke->set_blend(driver.video_data, false);
   }

   if (g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_INGAME_EXIT) &&
         g_extern.lifecycle_mode_state & (1ULL << MODE_MENU_INGAME))
   {
      menu_stack_pop();
      g_extern.lifecycle_mode_state &= ~((1ULL << MODE_MENU_INGAME) | (1ULL << MODE_MENU_INGAME_EXIT));
   }

   if (input_entry_ret != 0 || input_process_ret != 0)
      goto deinit;

   return true;

deinit:
   // set a timer delay so that we don't instantly switch back to the menu when
   // press and holding L3 + R3 in the emulation loop (lasts for 30 frame ticks)
   if (!(g_extern.lifecycle_state & (1ULL << RARCH_FRAMEADVANCE)))
      g_extern.delay_timer[0] = g_extern.frame_count + 30;

   g_extern.lifecycle_mode_state &= ~(1ULL << MODE_MENU_DRAW);

#ifndef __CELLOS_LV2__
   device_ptr->ctx_driver->rmenu_free();
#endif

   return false;
}
