/**
 * Copyright (c) 2022 KingK
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
#include "bubble.h"

struct console_buffer_info
{
    int ScreenWidth , ScreenHeight , TileWidth , TileHeight ;
    HANDLE hStdout , hStdin , hConsole;
    short NewKeyState[KEYS] , OldKeyState[KEYS] , NewMouseState[BUTTONS] , OldMouseState[BUTTONS];
    COORD MousePos;
    wchar_t AppName[MAX_APPNAME];
    SMALL_RECT WindowRect;
    PHANDLER_ROUTINE CloseHandle;
    CHAR_INFO * ScreenBuffer;
    bool bAtomActive , bConsoleInFocus , bEnableSound;
    pthread_t thread;
}Console;

bool Error(wchar_t * _msg)
{
    wchar_t buf[256];
    DWORD ErrCode =  GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
    SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
    wprintf(L"ERROR: %s\n\t%s\n", _msg, buf);
    return false;
};

void * GameThread( void * v)
{
	// Create user resources as part of this thread
	if (!OnUserCreate())  Console.bAtomActive = false;
	// Check if sound system should be enabled
	/*
	if (Console.bEnableSound)
	{
		if (!CreateAudio())
		{
			Console.bAtomActive = false; // Failed to create audio system			
			Console.bEnableSound = false;
		}
	}
	*/
	time_t tp1 = time(NULL);
	time_t tp2 = time(NULL);
	while (Console.bAtomActive)
	{
		// Run as fast as possible
		while (Console.bAtomActive)
		{
			// Handle Timing
			tp2 = time(NULL);
			tp1 = tp2;
			double fElapsedTime = (tp2-tp1)/(double)(CLOCKS_PER_SEC);
			// Handle Keyboard Input
			for (int i = 0; i < 256; i++)
			{
				Console.NewKeyState[i] = GetAsyncKeyState(i);
				keys[i].bPressed = false;
				keys[i].bReleased = false;
				if (Console.NewKeyState[i] != Console.OldKeyState[i])
				{
					if (Console.NewKeyState[i] & 0x8000)
					{
						keys[i].bPressed = !keys[i].bHeld;
						keys[i].bHeld = true;
					}
					else
					{
						keys[i].bReleased = true;
						keys[i].bHeld = false;
					}
				}
				Console.OldKeyState[i] = Console.NewKeyState[i];
			}
			// Handle Mouse Input - Check for window events
			INPUT_RECORD inBuf[32];
			DWORD events = 0;
			GetNumberOfConsoleInputEvents(Console.hStdin, &events);
			if (events > 0)
				ReadConsoleInput(Console.hStdin, inBuf, events, &events);
			// Handle events - we only care about mouse clicks and movement
			// for now
			for (DWORD i = 0; i < events; i++)
			{
				switch (inBuf[i].EventType)
				{
				case FOCUS_EVENT:
				{
					Console.bConsoleInFocus = inBuf[i].Event.FocusEvent.bSetFocus;
				}
				break;
				case MOUSE_EVENT:
				{
					switch (inBuf[i].Event.MouseEvent.dwEventFlags)
					{
					case MOUSE_MOVED:
					{
						Console.MousePos.X = inBuf[i].Event.MouseEvent.dwMousePosition.X;
						Console.MousePos.Y = inBuf[i].Event.MouseEvent.dwMousePosition.Y;
					}
					break;
					case 0:
					{
						for (int m = 0; m < 5; m++)
							Console.NewMouseState[m] = (inBuf[i].Event.MouseEvent.dwButtonState & (1 << m)) > 0;
					}
					break;
					default:
						break;
					}
				}
				break;
				default:
					break;
					// We don't care just at the moment
				}
			}
			for (int m = 0; m < 5; m++)
			{
				mouse[m].bPressed = false;
				mouse[m].bReleased = false;
				if (Console.NewMouseState[m] != Console.OldMouseState[m])
				{
					if (Console.NewMouseState[m])
					{
						mouse[m].bPressed = true;
						mouse[m].bHeld = true;
					}
					else
					{
						mouse[m].bReleased = true;
						mouse[m].bHeld = false;
					}
				}
				Console.OldMouseState[m] = Console.NewMouseState[m];
			}
			// Handle Frame Update
			if (!OnUserUpdate(fElapsedTime))
				Console.bAtomActive = false;
			// Update Title & Present Screen Buffer
			wchar_t s[256];
			swprintf_s(s, 256, L"OneLoneCoder.com - Console Game Engine - %s - FPS: %3.2f",Console.AppName, 1.0f / fElapsedTime);
			SetConsoleTitle(s);
			WriteConsoleOutput(Console.hConsole, Console.ScreenBuffer, (COORD){ (short)Console.ScreenWidth, (short)Console.ScreenHeight }, (COORD){ 0,0 }, &Console.WindowRect);
		}
		if (Console.bEnableSound)
		{
			// Close and Clean up audio system
		}
		// Allow the user to free resources if they have overrided the destroy function
		if (OnUserDestroy())
		{
			// User has permitted destroy, so exit and clean up
			free(Console.ScreenBuffer);
			SetConsoleActiveScreenBuffer(Console.hStdout);
			// m_cvGameFinished.notify_one();
		}
		else
		{
			// User denied destroy for some reason, so continue running
			Console.bAtomActive = true;
		}
	}
    return v;
}

void start(void)
{
    Console.bAtomActive = true;
    pthread_create(&Console.thread,NULL,GameThread,NULL);
    pthread_join(Console.thread,NULL);
}

static BOOL CloseHandler(DWORD evt)
{
	// Note this gets called in a seperate OS thread, so it must
	// only exit when the game has finished cleaning up, or else
	// the process will be killed before OnUserDestroy() has finished
	if (evt == CTRL_CLOSE_EVENT)
	{
		Console.bAtomActive = false;
		// Wait for thread to be exited
        /*
		std::unique_lock<std::mutex> ul(m_muxGame);
		m_cvGameFinished.wait(ul);
        */
       pthread_join(Console.thread,NULL);
	}
	return true;
}

void * galloc(size_t _NumElems , size_t _ElemSize)
{
    void * nBuffer = calloc(_NumElems,_ElemSize);
    if(!nBuffer)
    {
        Error(L"Memory Allocation Failed");
        exit(0);
    }
    return nBuffer;
};

sprite_p bubble_sprite_create(int w , int h)
{
    sprite_p nSprite = galloc( 1 , sizeof(sprite_t));
    nSprite->width = w;
    nSprite->height = h;
    nSprite->glyph = galloc( w * h , sizeof(wchar_t) );
    nSprite->color = galloc( w * h , sizeof(short) );
    return nSprite;
};

sprite_p bubble_sprite_load(wchar_t * file)
{
    sprite_p nSprite = NULL;
    int w , h;
    FILE * fp = _wfopen(file , L"rb");
    if( fp )
    {
        fread( &w , 1 , sizeof(int) , fp );
        fread( &h , 1 , sizeof(int) , fp );
        nSprite = bubble_sprite_create(w,h);
        fread(nSprite->glyph,w*h,sizeof(wchar_t),fp);
        fread(nSprite->color,w*h,sizeof(short),fp);
        fclose(fp);
    }
    return nSprite;
};

bool bubble_sprite_set_glyph(sprite_p sprite , int x , int y , wchar_t w)
{
    if( sprite && arange(0,x,sprite->width) && arange(0,y,sprite->height))
    {
        sprite->glyph[linear(x,y,sprite->width)] = w;
        return true;
    }
    return false;
};

bool bubble_sprite_set_color(sprite_p sprite , int x , int y , short c)
{
    if( sprite && arange(0,x,sprite->width) && arange(0,y,sprite->height))
    {
        sprite->color[linear(x,y,sprite->width)] = c;
        return true;
    }
    return false;
};

wchar_t bubble_sprite_get_glyph(sprite_p sprite , int x , int y)
{
    if( sprite && arange(0,x,sprite->width) && arange(0,y,sprite->height))
        return sprite->glyph[linear(x,y,sprite->width)];
    return L' ';
};

short bubble_sprite_get_color(sprite_p sprite , int x , int y)
{
    if( sprite && arange(0,x,sprite->width) && arange(0,y,sprite->height))
        return sprite->color[linear(x,y,sprite->width)];
    return FG_BLACK;
};

wchar_t bubble_sprite_sample_glyph(sprite_p sprite , float x , float y)
{
	int sx = (int)(x * (float)sprite->width);
	int sy = (int)(y * (float)sprite->height-1.0f);
	if (sx <0 || sx >= sprite->width || sy < 0 || sy >= sprite->height)
		return L' ';
	else
		return sprite->glyph[sy * sprite->width + sx];
};

short bubble_sprite_sample_color(sprite_p sprite , float x , float y)
{
	int sx = (int)(x * (float)sprite->width);
	int sy = (int)(y * (float)sprite->height-1.0f);
	if (sx <0 || sx >= sprite->width || sy < 0 || sy >= sprite->height)
		return L' ';
	else
		return sprite->color[sy * sprite->width + sx];
};

bool bubble_sprite_save(sprite_p sprite , wchar_t * file)
{
    if(sprite && file)
    {
        FILE * fp = _wfopen( file , L"wb");
        if(fp)
        {
            fwrite(&sprite->width,1,sizeof(int),fp);
            fwrite(&sprite->height,1,sizeof(int),fp);
            fwrite(sprite->glyph,sprite->width*sprite->height,sizeof(wchar_t),fp);
            fwrite(sprite->color,sprite->width*sprite->height,sizeof(short),fp);
            fclose(fp);
            return true;
        }
    }
    return false;
};

void bubble_sprite_destruct(sprite_p * sprite)
{
	if( sprite )
	{
		sprite_p sp = *sprite;
		if( sp )
		{
			free(sp->glyph);
			free(sp->color);
		}
		*sprite = NULL;
	}
};

void bubble_console_clip(int *x, int *y)
{
	if (!x || !y)
		return;
	if (*x < 0)
		*x = 0;
	if (*x > Console.ScreenWidth)
		*x = Console.ScreenWidth;
	if (*y < 0)
		*y = 0;
	if (*y > Console.ScreenHeight)
		*y = Console.ScreenHeight;
};

void bubble_console_draw(int x , int y , wchar_t w , short c)
{
	if(arange(0,x,Console.ScreenWidth) && arange(0,y,Console.ScreenHeight))
	{
		Console.ScreenBuffer[linear(x,y,Console.ScreenWidth)].Char.UnicodeChar = w;
		Console.ScreenBuffer[linear(x,y,Console.ScreenWidth)].Attributes = c;
	}
};

void bubble_console_fill(int x1 , int y1 , int x2 , int y2 , wchar_t w , short c)
{
	bubble_console_clip(&x1, &y1);
	bubble_console_clip(&x2, &y2);
	for (int x = x1; x < x2; x++)
		for (int y = y1; y < y2; y++)
			bubble_console_draw(x, y, w, c);
};

void bubble_console_draw_string(int x , int y , short col , wchar_t * _format,...)
{
	if(arange(0,x,Console.ScreenWidth) && arange(0,y,Console.ScreenHeight))
	{
		wchar_t writeBuffer[250];
		va_list va;
		va_start(va,_format);
		swprintf(writeBuffer,250,_format,va);
		size_t c = wcslen(writeBuffer);
		for(int i = 1 , l = 0 ; i < c ; i++ )
		{
			wchar_t wc = writeBuffer[i-1];
			switch(wc)
			{
				case L'\n': { ++y;  l = 0;} break;
				default:
				{
					Console.ScreenBuffer[linear(x+l,y,Console.ScreenWidth)].Char.UnicodeChar = wc;
					Console.ScreenBuffer[linear(x+l,y,Console.ScreenWidth)].Attributes = col;
					l = i;
				} break;
			}
		}
		va_end(va);
	}
};

void bubble_console_draw_string_a(int x , int y , short col , wchar_t * _format, ...)
{
	va_list va;
	va_start(va,_format);
	bubble_console_draw_string(x,y,col,_format,va);
};

void bubble_console_draw_line(int x1 , int y1 , int x2 , int y2 , wchar_t w , short c)
{
	int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
	dx = x2 - x1; dy = y2 - y1;
	dx1 = abs(dx); dy1 = abs(dy);
	px = 2 * dy1 - dx1;	py = 2 * dx1 - dy1;
	if (dy1 <= dx1)
	{
		if (dx >= 0)
			{ x = x1; y = y1; xe = x2; }
		else
			{ x = x2; y = y2; xe = x1;}
		bubble_console_draw(x, y, c, c);
		
		for (i = 0; x<xe; i++)
		{
			x = x + 1;
			if (px<0)
				px = px + 2 * dy1;
			else
			{
				if ((dx<0 && dy<0) || (dx>0 && dy>0)) y = y + 1; else y = y - 1;
				px = px + 2 * (dy1 - dx1);
			}
			bubble_console_draw(x, y, c, c);
		}
	}
	else
	{
		if (dy >= 0)
			{ x = x1; y = y1; ye = y2; }
		else
			{ x = x2; y = y2; ye = y1; }
		bubble_console_draw(x, y, c, c);
		for (i = 0; y<ye; i++)
		{
			y = y + 1;
			if (py <= 0)
				py = py + 2 * dx1;
			else
			{
				if ((dx<0 && dy<0) || (dx>0 && dy>0)) x = x + 1; else x = x - 1;
				py = py + 2 * (dx1 - dy1);
			}
			bubble_console_draw(x, y, c, c);
		}
	}
};

void bubble_console_draw_triangle(int x1 , int y1 , int x2 , int y2 , int x3 , int y3 , wchar_t w , short c)
{
	bubble_console_draw_line(x1, y1, x2, y2, w, c);
	bubble_console_draw_line(x2, y2, x3, y3, w, c);
	bubble_console_draw_line(x3, y3, x1, y1, w, c);
};

void SWAP(int *x , int *y){int t = *x ; *x = *y ; *y = t;};

void drawline(int sx ,int ex , int ny , wchar_t w, short c){for (int i = sx; i <= ex; i++) bubble_console_draw(i, ny, w, c); };

void bubble_console_fill_triangle(int x1 , int y1 , int x2 , int y2 , int x3 , int y3 , wchar_t w , short c)
{
	int t1x, t2x, y, minx, maxx, t1xp, t2xp;
	bool changed1 = false;
	bool changed2 = false;
	int signx1, signx2, dx1, dy1, dx2, dy2;
	int e1, e2;
	// Sort vertices
	if (y1>y2) { SWAP(&y1, &y2); SWAP(&x1, &x2); }
	if (y1>y3) { SWAP(&y1, &y3); SWAP(&x1, &x3); }
	if (y2>y3) { SWAP(&y2, &y3); SWAP(&x2, &x3); }
	t1x = t2x = x1; y = y1;   // Starting points
	dx1 = (int)(x2 - x1); if (dx1<0) { dx1 = -dx1; signx1 = -1; }
	else signx1 = 1;
	dy1 = (int)(y2 - y1);
	dx2 = (int)(x3 - x1); if (dx2<0) { dx2 = -dx2; signx2 = -1; }
	else signx2 = 1;
	dy2 = (int)(y3 - y1);
	if (dy1 > dx1) {   // swap values
		SWAP(&dx1,& dy1);
		changed1 = true;
	}
	if (dy2 > dx2) {   // swap values
		SWAP(&dy2,& dx2);
		changed2 = true;
	}
	e2 = (int)(dx2 >> 1);
	// Flat top, just process the second half
	if (y1 == y2) goto next;
	e1 = (int)(dx1 >> 1);
	for (int i = 0; i < dx1;) {
		t1xp = 0; t2xp = 0;
		if (t1x<t2x) { minx = t1x; maxx = t2x; }
		else { minx = t2x; maxx = t1x; }
		// process first line until y value is about to change
		while (i<dx1) {
			i++;
			e1 += dy1;
			while (e1 >= dx1) {
				e1 -= dx1;
				if (changed1) t1xp = signx1;//t1x += signx1;
				else          goto next1;
			}
			if (changed1) break;
			else t1x += signx1;
		}
		// Move line
	next1:
		// process second line until y value is about to change
		while (1) {
			e2 += dy2;
			while (e2 >= dx2) {
				e2 -= dx2;
				if (changed2) t2xp = signx2;//t2x += signx2;
				else          goto next2;
			}
			if (changed2)     break;
			else              t2x += signx2;
		}
	next2:
		if (minx>t1x) minx = t1x; if (minx>t2x) minx = t2x;
		if (maxx<t1x) maxx = t1x; if (maxx<t2x) maxx = t2x;
		drawline(minx, maxx, y,w,c);    // bubble_console_draw line from min to max points found on the y
									 // Now increase y
		if (!changed1) t1x += signx1;
		t1x += t1xp;
		if (!changed2) t2x += signx2;
		t2x += t2xp;
		y += 1;
		if (y == y2) break;
	}
	next:
	// Second half
	dx1 = (int)(x3 - x2); if (dx1<0) { dx1 = -dx1; signx1 = -1; }
	else signx1 = 1;
	dy1 = (int)(y3 - y2);
	t1x = x2;
	if (dy1 > dx1) {   // swap values
		SWAP(&dy1,& dx1);
		changed1 = true;
	}
	else changed1 = false;
	e1 = (int)(dx1 >> 1);
	for (int i = 0; i <= dx1; i++) {
		t1xp = 0; t2xp = 0;
		if (t1x<t2x) { minx = t1x; maxx = t2x; }
		else { minx = t2x; maxx = t1x; }
		// process first line until y value is about to change
		while (i<dx1) {
			e1 += dy1;
			while (e1 >= dx1) {
				e1 -= dx1;
				if (changed1) { t1xp = signx1; break; }//t1x += signx1;
				else          goto next3;
			}
			if (changed1) break;
			else   	   	  t1x += signx1;
			if (i<dx1) i++;
		}
	next3:
		// process second line until y value is about to change
		while (t2x != x3) {
			e2 += dy2;
			while (e2 >= dx2) {
				e2 -= dx2;
				if (changed2) t2xp = signx2;
				else          goto next4;
			}
			if (changed2)     break;
			else              t2x += signx2;
		}
	next4:
		if (minx>t1x) minx = t1x; 
		if (minx>t2x) minx = t2x;
		if (maxx<t1x) maxx = t1x; 
		if (maxx<t2x) maxx = t2x;
		drawline(minx, maxx, y,w,c);   										
		if (!changed1) t1x += signx1;
		t1x += t1xp;
		if (!changed2) t2x += signx2;
		t2x += t2xp;
		y += 1;
		if (y>y3) return;
	}
};

void bubble_console_draw_circle(int xc , int yc , int r , wchar_t w , short c)
{
	int x = 0;
	int y = r;
	int p = 3 - 2 * r;
	if (!r) return;
	while (y >= x) // only formulate 1/8 of circle
	{
		bubble_console_draw(xc - x, yc - y, w, c);//upper left left
		bubble_console_draw(xc - y, yc - x, w, c);//upper upper left
		bubble_console_draw(xc + y, yc - x, w, c);//upper upper right
		bubble_console_draw(xc + x, yc - y, w, c);//upper right right
		bubble_console_draw(xc - x, yc + y, w, c);//lower left left
		bubble_console_draw(xc - y, yc + x, w, c);//lower lower left
		bubble_console_draw(xc + y, yc + x, w, c);//lower lower right
		bubble_console_draw(xc + x, yc + y, w, c);//lower right right
		if (p < 0) p += 4 * x++ + 6;
		else p += 4 * (x++ - y--) + 10;
	}
};

void bubble_console_fill_circle(int xc , int yc , int r , wchar_t w , short c)
{
	// Taken from wikipedia
	int x = 0;
	int y = r;
	int p = 3 - 2 * r;
	if (!r) return;
	while (y >= x)
	{
		// Modified to draw scan-lines instead of edges
		drawline(xc - x, xc + x, yc - y,w,c);
		drawline(xc - y, xc + y, yc - x,w,c);
		drawline(xc - x, xc + x, yc + y,w,c);
		drawline(xc - y, xc + y, yc + x,w,c);
		if (p < 0) p += 4 * x++ + 6;
		else p += 4 * (x++ - y--) + 10;
	}
};

void bubble_console_draw_sprite(int x , int y , sprite_p sprite)
{
	if (sprite == NULL) return;
	for (int i = 0; i < sprite->width; i++)
		for (int j = 0; j < sprite->height; j++)
			if (sprite->glyph[linear((x+i),(y+j),sprite->width)] != L' ')
				bubble_console_draw(x + i, y + j, sprite->glyph[linear((x+i),(y+j),sprite->width)],
								   sprite->color[linear((x+i),(y+j),sprite->width)]);
};

void bubble_console_draw_sprite_partial(int x, int y, sprite_p sprite, int ox, int oy, int w, int h)
{
	if (sprite == NULL) return;
	for (int i = 0; i < w; i++)
		for (int j = 0; j < h; j++)
			if (sprite->glyph[linear((x+ox),(y+oy),sprite->width)] != L' ')
				bubble_console_draw(x + i, y + j, sprite->glyph[linear((x+ox),(y+oy),sprite->width)], 
								   sprite->color[linear((x+ox),(y+oy),sprite->width)]);
};

void bubble_console_draw_WireFrameModel(COORD *vecModelCoordinates, size_t vecSize , float x, float y, float r, float s , short col , short c );

bool bubble_console_int(int width , int height , int tile_w , int tile_h)
{
    Console.bEnableSound = false;
    Console.ScreenWidth = width;
    Console.ScreenHeight = height;
    Console.TileWidth = tile_w;
    Console.TileHeight = tile_h;

    Console.hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    Console.hStdin = GetStdHandle(STD_INPUT_HANDLE);
    Console.hConsole = CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE , 0 , NULL , CONSOLE_TEXTMODE_BUFFER , NULL );
    if( Console.hConsole == INVALID_HANDLE_VALUE )
        return Error(L"Bad Handle");
    memset(Console.NewKeyState , 0 , 256 * sizeof(short));
    memset(Console.OldKeyState , 0 , 256 * sizeof(short));
    memset(keys , 0 , 256 * sizeof(keystate_t));

    Console.MousePos.X = 0;
    Console.MousePos.Y = 0;

    wcscpy_s(Console.AppName,MAX_APPNAME,L"Default");

	// and I'm unsure why this is. It could be to do with windows default settings, or
    // Update 13/09/2017 - It seems that the console behaves differently on some systems
	// screen resolutions, or system languages. Unfortunately, MSDN does not offer much
	// by way of useful information, and so the resulting sequence is the reult of experiment
	// that seems to work in multiple cases.
	//
	// The problem seems to be that the SetConsoleXXX functions are somewhat circular and 
	// fail depending on the state of the current console properties, i.e. you can't set
	// the buffer size until you set the screen size, but you can't change the screen size
	// until the buffer size is correct. This coupled with a precise ordering of calls
	// makes this procedure seem a little mystical :-P. Thanks to wowLinh for helping - Jx9
	// Change console visual size to a minimum so ScreenBuffer can shrink
	// below the actual visual size
	Console.WindowRect = (SMALL_RECT){ 0, 0, 1, 1 };
	SetConsoleWindowInfo(Console.hConsole, TRUE, &Console.WindowRect);
	// Set the size of the screen buffer
	COORD coord = { (short)Console.ScreenWidth, (short)Console.ScreenHeight };
	if (!SetConsoleScreenBufferSize(Console.hConsole, coord))
		Error(L"SetConsoleScreenBufferSize");
	// Assign screen buffer to the console
	if (!SetConsoleActiveScreenBuffer(Console.hConsole))
		return Error(L"SetConsoleActiveScreenBuffer");
	
	// Set the font size now that the screen buffer has been assigned to the console
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = tile_w;
	cfi.dwFontSize.Y = tile_h;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	/*DWORD version = GetVersion();
	DWORD major = (DWORD)(LOBYTE(LOWORD(version)));
	DWORD minor = (DWORD)(HIBYTE(LOWORD(version)));*/
	//if ((major > 6) || ((major == 6) && (minor >= 2) && (minor < 4)))		
	//	wcscpy_s(cfi.FaceName, L"Raster"); // Windows 8 :(
	//else
	//	wcscpy_s(cfi.FaceName, L"Lucida Console"); // Everything else :P
	//wcscpy_s(cfi.FaceName, L"Liberation Mono");
	wcscpy_s(cfi.FaceName, 32,L"Consolas");
	if (!SetCurrentConsoleFontEx(Console.hConsole, false, &cfi))
		return Error(L"SetCurrentConsoleFontEx");
	// Get screen buffer info and check the maximum allowed window size. Return
	// error if exceeded, so user knows their dimensions/fontsize are too large
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(Console.hConsole, &csbi))
		return Error(L"GetConsoleScreenBufferInfo");
	if (Console.ScreenHeight > csbi.dwMaximumWindowSize.Y)
		return Error(L"Screen Height / Font Height Too Big");
	if (Console.ScreenWidth > csbi.dwMaximumWindowSize.X)
		return Error(L"Screen Width / Font Width Too Big");
	// Set Physical Console Window Size
	Console.WindowRect = (SMALL_RECT){ 0, 0, (short)Console.ScreenWidth - 1, (short)Console.ScreenHeight - 1 };
	if (!SetConsoleWindowInfo(Console.hConsole, TRUE, &Console.WindowRect))
		return Error(L"SetConsoleWindowInfo");
	// Set flags to allow mouse input		
	if (!SetConsoleMode(Console.hStdin, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT))
		return Error(L"SetConsoleMode");
	// Allocate memory for screen buffer
	Console.ScreenBuffer = galloc(width*height,sizeof(CHAR_INFO));
	memset(Console.ScreenBuffer, 0, sizeof(CHAR_INFO) * Console.ScreenWidth * Console.ScreenHeight);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CloseHandler, TRUE);
    start();
    return true;
};

keystate_t bubble_get_key(int key_id) { return keys[key_id];};

keystate_t bubble_get_mouse(int mouse_button_id){ return mouse[mouse_button_id];};

COORD bubble_get_mouse_coords(void) { return Console.MousePos;};

bool bubble_get_isfocused(void) { return Console.bConsoleInFocus;};