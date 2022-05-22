// Copyright (c) 2022 KingK
// 
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef __BUBBLE_H__
#define __BUBBLE_H__

#ifndef __WIN32
#error Only Windows Supported
#endif

#ifndef UNICODE
#error Enable UNICODE for your compiler
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <windows.h>

#define KEYS 256

#define BUTTONS 5

#define MAX_APPNAME 100

#define linear( x , y , a ) ( (y) * (a) + (x) )

#define arange( min , val , max ) ( (min) <= (val) && (val) <= (max) )

// Virtual
bool OnUserCreate(void);
bool OnUserUpdate(double dt);
bool OnUserDestroy(void);

enum COLOUR
{
	FG_BLACK		= 0x0000,
	FG_DARK_BLUE    = 0x0001,	
	FG_DARK_GREEN   = 0x0002,
	FG_DARK_CYAN    = 0x0003,
	FG_DARK_RED     = 0x0004,
	FG_DARK_MAGENTA = 0x0005,
	FG_DARK_YELLOW  = 0x0006,
	FG_GREY			= 0x0007,
	FG_DARK_GREY    = 0x0008,
	FG_BLUE			= 0x0009,
	FG_GREEN		= 0x000A,
	FG_CYAN			= 0x000B,
	FG_RED			= 0x000C,
	FG_MAGENTA		= 0x000D,
	FG_YELLOW		= 0x000E,
	FG_WHITE		= 0x000F,
	BG_BLACK		= 0x0000,
	BG_DARK_BLUE	= 0x0010,
	BG_DARK_GREEN	= 0x0020,
	BG_DARK_CYAN	= 0x0030,
	BG_DARK_RED		= 0x0040,
	BG_DARK_MAGENTA = 0x0050,
	BG_DARK_YELLOW	= 0x0060,
	BG_GREY			= 0x0070,
	BG_DARK_GREY	= 0x0080,
	BG_BLUE			= 0x0090,
	BG_GREEN		= 0x00A0,
	BG_CYAN			= 0x00B0,
	BG_RED			= 0x00C0,
	BG_MAGENTA		= 0x00D0,
	BG_YELLOW		= 0x00E0,
	BG_WHITE		= 0x00F0,
};

enum PIXEL_TYPE
{
	PIXEL_SOLID = 0x2588,
	PIXEL_THREEQUARTERS = 0x2593,
	PIXEL_HALF = 0x2592,
	PIXEL_QUARTER = 0x2591,
};

typedef struct sprite_info
{
    int width , height;
    wchar_t * glyph;
    short * color;
} sprite_t , *sprite_p;

typedef struct keystroke_states
{
    bool bPressed , bHeld , bReleased;
}keystate_t , * keystate_p ;

keystate_t keys[KEYS] , mouse[BUTTONS];

bool bubble_console_int(int width , int height , int tile_w , int tile_h);
void * galloc(size_t _NumElems , size_t _ElemSize);
bool ErrorExit(wchar_t * _msg);
sprite_p bubble_sprite_create(int w , int h);
sprite_p bubble_sprite_load(wchar_t * file);
bool bubble_sprite_set_glyph(sprite_p sprite , int x , int y , wchar_t w);
bool bubble_sprite_set_color(sprite_p sprite , int x , int y , short c);
wchar_t bubble_sprite_get_glyph(sprite_p sprite , int x , int y);
short bubble_sprite_get_color(sprite_p sprite , int x , int y);
wchar_t bubble_sprite_sample_glyph(sprite_p sprite , float x , float y);
short bubble_sprite_sample_color(sprite_p sprite , float x , float y);
bool bubble_sprite_save(sprite_p sprite , wchar_t * file);
void bubble_sprite_destruct(sprite_p * sprite);

void bubble_console_clip( int * x , int * y );
void bubble_console_draw(int x , int y , wchar_t w , short c);
void bubble_console_fill(int x1 , int y1 , int x2 , int y2 , wchar_t w , short c);
void bubble_console_draw_string(int x , int y , short col , wchar_t * _format,...);
void bubble_console_draw_string_a(int x , int y , short col , wchar_t * _format, ...);
void bubble_console_draw_line(int x1 , int y1 , int x2 , int y2 , wchar_t w , short c);
void bubble_console_draw_triangle(int x1 , int y1 , int x2 , int y2 , int x3 , int y3 , wchar_t w , short c);
void bubble_console_fill_triangle(int x1 , int y1 , int x2 , int y2 , int x3 , int y3 , wchar_t w , short c);
void bubble_console_draw_circle(int xc , int yc , int r , wchar_t w , short c);
void bubble_console_fill_circle(int xc , int yc , int r , wchar_t w , short c);
void bubble_console_draw_sprite(int x , int y , sprite_p sprite);
void bubble_console_draw_sprite_partial(int x, int y, sprite_p sprite, int ox, int oy, int w, int h);
void bubble_console_draw_WireFrameModel(COORD *vecModelCoordinates, size_t vecSize , float x, float y, float r, float s , short col , short c );

keystate_t bubble_get_key(int key_id);
keystate_t bubble_get_mouse(int mouse_button_id);
COORD bubble_get_mouse_coords(void);
bool bubble_get_isfocused(void);

#endif