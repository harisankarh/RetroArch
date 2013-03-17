/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2013 - Hans-Kristian Arntzen
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

#include "../driver.h"

#include "SDL.h"
#include "../gfx/gfx_context.h"
#include "../boolean.h"
#include "../general.h"
#include <stdint.h>
#include <stdlib.h>
#include "../libretro.h"
#include "input_common.h"
#include <stdio.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024
#define COUNT 5

/*
 * error - wrapper for perror
 */
static void error(char *msg) {
  perror(msg);
  exit(1);
}


static void *start_server(void *ptr);

static Uint8 global[500];

static void start(void)
{
  pthread_t thread1, thread2;
  char *message1 = "5000";
  char *message2 = "5001";
  int  iret1, iret2;
  int iter1;

  /* Create independent threads each of which will execute function */

  iret1 = pthread_create( &thread1, NULL, start_server, (void*) message1);
  iret2 = pthread_create( &thread2, NULL, start_server, (void*) message2);
  /* Wait till threads are complete before main continues. Unless we  */
  /* wait we run the risk of executing an exit which will terminate   */
  /* the process and all threads before the threads have completed.   */
  printf("Servers started \n");
  return;
}

typedef struct sdl_input
{
  const rarch_joypad_driver_t *joypad;

  int mouse_x, mouse_y;
  int mouse_abs_x, mouse_abs_y;
  int mouse_l, mouse_r, mouse_m;
} sdl_input_t;


static void *start_server( void *ptr )
{
  char *message;
  message = (char *) ptr;
  printf("%s \n", message);

  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char *buf; /* message buf */
  buf=malloc(sizeof(char)*BUFSIZE);
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  portno = atoi(message);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    /* printf("*********************************\n"); */
    if(buf[0] == 'b') 
      {
     
	if(portno%2==0) 
	  {
	    if(buf[13] == 'p')
	      /* case 'q': */
	      global[113] = 1;
	    else 
	      global[113] = 0;
	    

	    if(buf[9] == 'p')
	      /* case 'z': */
	      global[122] = 1;
	    else 
	      global[122] = 0;
	    

	    if(buf[5] == 'p')
	      /* case 's': */
	      global[115] = 1;
	    else 
	      global[115] = 0;
	    

	    if(buf[1] == 'p')
	      /* case 'x': */
	      global[120] = 1;
	    else 
	      global[120] = 0;
	    

	    if(buf[25] == 'p')
	      /* case 'a': */
	      global[97] = 1;
	    else 
	      global[97] = 0;
	    

	    if(buf[29] == 'p')
	      /* case 'd': */
	      global[100] = 1;
	    else 
	      global[100] = 0;
	    

	    if(buf[17] == 'p')
	      /* case 'w': */
	      global[119] = 1;
	    else 
	      global[119] = 0;
	    

	    if(buf[21] == 'p')
	      /* case 'c': */
	      global[99] = 1;
	    else 
	      global[99] = 0;
	    
	  }
	else 
	  {
	    if(buf[13] == 'p')
	      /* case 'r': */
	      global[114] = 1;
	    else 
	      global[114] = 0;
	    

	    if(buf[9] == 'p')
	      /* case 't': */
	      global[116] = 1;
	    else 
	      global[116] = 0;
	    

	    if(buf[5] == 'p')
	      /* case 'y': */
	      global[121] = 1;
	    else 
	      global[121] = 0;
	    

	    if(buf[1] == 'p')
	      /* case 'v': */
	      global[118] = 1;
	    else 
	      global[118] = 0;
	    

	    if(buf[25] == 'p')
	      /* case 'b': */
	      global[98] = 1;
	    else 
	      global[98] = 0;
	    

	    if(buf[29] == 'p')
	      /* case 'n': */
	      global[110] = 1;
	    else 
	      global[110] = 0;
	    

	    if(buf[17] == 'p')
	      /* case 'm': */
	      global[109] = 1;
	    else 
	      global[109] = 0;
	    

	    if(buf[21] == 'p')
	      /* case 'g': */
	      global[103] = 1;
	    else 
	      global[103] = 0;
	    
	  }
      }

    buf[0]='\0';
#ifdef DEBUG
    if (n < 0)
      error("ERROR in recvfrom");
    printf("Received %s\n",buf);
#endif  
  }
}

static void *sdl_input_init(void)
{
  input_init_keyboard_lut(rarch_key_map_sdl);
  sdl_input_t *sdl = (sdl_input_t*)calloc(1, sizeof(*sdl));
  if (!sdl)
    return NULL;

  // Add the server here 
  start();
  sdl->joypad = input_joypad_init_first();
  return sdl;
}

static bool sdl_key_pressed(int key)
{
  static int i;
  if (key >= RETROK_LAST)
    return false;

  int sym = input_translate_rk_to_keysym((enum retro_key)key);
  int num_keys;
  Uint8 *keymap = SDL_GetKeyState(&num_keys);
  if (sym < 0 || sym >= num_keys)
    return false;
  memcpy(keymap, global, sizeof(Uint8)*num_keys);
  /* if(global[sym] == 1) */
  /*   { */
  /*     global[sym]=0; */
  /*     return 1; */
  /*     //      global[key] = 0; */
  /*   } */
  // printf("Key ?balaji? i=%d keymap = %d sym = %d key %d\n",i++,keymap[sym],sym,key); 
   return keymap[sym];
}

static bool sdl_is_pressed(sdl_input_t *sdl, unsigned port_num, const struct retro_keybind *key)
{
   if (sdl_key_pressed(key->key))
      return true;

   return input_joypad_pressed(sdl->joypad, port_num, key);
}

static bool sdl_bind_button_pressed(void *data, int key)
{
  static int i;
  /* printf("balaji %d cecking pol ? \n",i++); */
   const struct retro_keybind *binds = g_settings.input.binds[0];
   if (key >= 0 && key < RARCH_BIND_LIST_END)
   {
      const struct retro_keybind *bind = &binds[key];
      return sdl_is_pressed((sdl_input_t*)data, 0, bind);
   }
   else
      return false;
}

static int16_t sdl_joypad_device_state(sdl_input_t *sdl, const struct retro_keybind **binds_, 
      unsigned port_num, unsigned id)
{
   const struct retro_keybind *binds = binds_[port_num];
   if (id < RARCH_BIND_LIST_END)
   {
      const struct retro_keybind *bind = &binds[id];
      return bind->valid && sdl_is_pressed(sdl, port_num, bind);
   }
   else
      return 0;
}

static int16_t sdl_analog_device_state(sdl_input_t *sdl, const struct retro_keybind **binds,
      unsigned port_num, unsigned index, unsigned id)
{
   return input_joypad_analog(sdl->joypad, port_num, index, id, binds[port_num]);
}

static int16_t sdl_keyboard_device_state(sdl_input_t *sdl, unsigned id)
{
   return sdl_key_pressed(id);
}

static int16_t sdl_mouse_device_state(sdl_input_t *sdl, unsigned id)
{
   switch (id)
   {
      case RETRO_DEVICE_ID_MOUSE_LEFT:
         return sdl->mouse_l;
      case RETRO_DEVICE_ID_MOUSE_RIGHT:
         return sdl->mouse_r;
      case RETRO_DEVICE_ID_MOUSE_X:
         return sdl->mouse_x;
      case RETRO_DEVICE_ID_MOUSE_Y:
         return sdl->mouse_y;
      default:
         return 0;
   }
}

static int16_t sdl_pointer_device_state(sdl_input_t *sdl, unsigned index, unsigned id, bool screen)
{
   if (index != 0)
      return 0;

   int16_t res_x = 0, res_y = 0, res_screen_x = 0, res_screen_y = 0;
   bool valid = input_translate_coord_viewport(sdl->mouse_abs_x, sdl->mouse_abs_y,
         &res_x, &res_y, &res_screen_x, &res_screen_y);

   if (!valid)
      return 0;

   if (screen)
   {
      res_x = res_screen_x;
      res_y = res_screen_y;
   }

   bool inside = (res_x >= -0x7fff) && (res_x <= 0x7fff) &&
      (res_y >= -0x7fff) && (res_y <= 0x7fff);

   if (!inside)
      return 0;

   switch (id)
   {
      case RETRO_DEVICE_ID_POINTER_X:
         return res_x;
      case RETRO_DEVICE_ID_POINTER_Y:
         return res_y;
      case RETRO_DEVICE_ID_POINTER_PRESSED:
         return sdl->mouse_l;
      default:
         return 0;
   }
}

static int16_t sdl_lightgun_device_state(sdl_input_t *sdl, unsigned id)
{
   switch (id)
   {
      case RETRO_DEVICE_ID_LIGHTGUN_X:
         return sdl->mouse_x;
      case RETRO_DEVICE_ID_LIGHTGUN_Y:
         return sdl->mouse_y;
      case RETRO_DEVICE_ID_LIGHTGUN_TRIGGER:
         return sdl->mouse_l;
      case RETRO_DEVICE_ID_LIGHTGUN_CURSOR:
         return sdl->mouse_m;
      case RETRO_DEVICE_ID_LIGHTGUN_TURBO:
         return sdl->mouse_r;
      case RETRO_DEVICE_ID_LIGHTGUN_START:
         return sdl->mouse_m && sdl->mouse_r; 
      case RETRO_DEVICE_ID_LIGHTGUN_PAUSE:
         return sdl->mouse_m && sdl->mouse_l; 
      default:
         return 0;
   }
}

static int16_t sdl_input_state(void *data_, const struct retro_keybind **binds, unsigned port, unsigned device, unsigned index, unsigned id)
{
   sdl_input_t *data = (sdl_input_t*)data_;
   switch (device)
   {
      case RETRO_DEVICE_JOYPAD:
         return sdl_joypad_device_state(data, binds, port, id);
      case RETRO_DEVICE_ANALOG:
         return sdl_analog_device_state(data, binds, port, index, id);
      case RETRO_DEVICE_MOUSE:
         return sdl_mouse_device_state(data, id);
      case RETRO_DEVICE_POINTER:
      case RARCH_DEVICE_POINTER_SCREEN:
         return sdl_pointer_device_state(data, index, id, device == RARCH_DEVICE_POINTER_SCREEN);
      case RETRO_DEVICE_KEYBOARD:
         return sdl_keyboard_device_state(data, id);
      case RETRO_DEVICE_LIGHTGUN:
         return sdl_lightgun_device_state(data, id);

      default:
         return 0;
   }
}

static void sdl_input_free(void *data)
{
   if (!data)
      return;

   // Flush out all pending events.
   SDL_Event event;
   while (SDL_PollEvent(&event));

   sdl_input_t *sdl = (sdl_input_t*)data;

   if (sdl->joypad)
      sdl->joypad->destroy();

   free(data);
}

static void sdl_poll_mouse(sdl_input_t *sdl)
{
   Uint8 btn = SDL_GetRelativeMouseState(&sdl->mouse_x, &sdl->mouse_y);
   SDL_GetMouseState(&sdl->mouse_abs_x, &sdl->mouse_abs_y);
   sdl->mouse_l = SDL_BUTTON(SDL_BUTTON_LEFT) & btn ? 1 : 0;
   sdl->mouse_r = SDL_BUTTON(SDL_BUTTON_RIGHT) & btn ? 1 : 0;
   sdl->mouse_m = SDL_BUTTON(SDL_BUTTON_MIDDLE) & btn ? 1 : 0;
}

static void sdl_input_poll(void *data)
{
   SDL_PumpEvents();
   sdl_input_t *sdl = (sdl_input_t*)data;
   static int i;
   /* printf("Balaji %d 000 %s \n",i++,sdl->joypad->ident); */
   input_joypad_poll(sdl->joypad);
   sdl_poll_mouse(sdl);
}

const input_driver_t input_sdl = {
   sdl_input_init,
   sdl_input_poll,
   sdl_input_state,
   sdl_bind_button_pressed,
   sdl_input_free,
   NULL,
   "sdl",
};

