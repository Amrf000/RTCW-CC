/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//
// string allocation/managment

#include "ui_shared.h"

#define SCROLL_TIME_START                   500
#define SCROLL_TIME_ADJUST              150
#define SCROLL_TIME_ADJUSTOFFSET    40
#define SCROLL_TIME_FLOOR                   20

typedef struct scrollInfo_s {
	int nextScrollTime;
	int nextAdjustTime;
	int adjustValue;
	int scrollKey;
	float xStart;
	float yStart;
	itemDef_t *item;
	bool scrollDir;
} scrollInfo_t;

static scrollInfo_t scrollInfo;

static void (displayContextDef_t::*captureFunc )( void *p ) = NULL;
static void *captureData = NULL;
static itemDef_t *itemCapture = NULL;   // item that has the mouse captured ( if any )

//displayContextDef_t *DC = NULL;

bool g_waitingForKey = false;
bool g_editingField = false;

static itemDef_t *g_bindItem = NULL;
itemDef_t *g_editItem = NULL;

menuDef_t Menus[MAX_MENUS];      // defined menus
int menuCount = 0;               // how many

// TTimo
// a stack for modal menus only, stores the menus to come back to
// (an item can be NULL, goes back to main menu / no action required)
menuDef_t *modalMenuStack[MAX_MODAL_MENUS];
int modalMenuCount = 0;

static bool debugMode = false;

#define DOUBLE_CLICK_DELAY 300
static int lastListBoxClickTime = 0;

#ifdef CGAME
#define MEM_POOL_SIZE  128 * 1024
#else
#define MEM_POOL_SIZE  1024 * 1024
#endif

static char memoryPool[MEM_POOL_SIZE];
static int allocPoint, outOfMemory;


/*
===============
UI_Alloc
===============
*/
void* displayContextDef_t::UI_Alloc( int size ) {
	auto&& DC = this;
	char    *p;

	if ( allocPoint + size > MEM_POOL_SIZE ) {
		outOfMemory = true;
		if ( DC->Print ) {
			DC->Print( "UI_Alloc: Failure. Out of memory!\n" );
		}
		//DC->trap_Print(S_COLOR_YELLOW"WARNING: UI Out of Memory!\n");
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 15 ) & ~15;

	return p;
}

/*
===============
UI_InitMemory
===============
*/
void displayContextDef_t::UI_InitMemory( void ) {
	allocPoint = 0;
	outOfMemory = false;
}

bool displayContextDef_t::UI_OutOfMemory() {
	return outOfMemory;
}





#define HASH_TABLE_SIZE 2048
/*
================
return a hash value for the string
================
*/
static long hashForString( const char *str ) {
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while ( str[i] != '\0' ) {
		letter = tolower( str[i] );
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( HASH_TABLE_SIZE - 1 );
	return hash;
}

typedef struct stringDef_s {
	struct stringDef_s *next;
	const char *str;
} stringDef_t;

static int strPoolIndex = 0;
static char strPool[STRING_POOL_SIZE];

static int strHandleCount = 0;
static stringDef_t *strHandle[HASH_TABLE_SIZE];


const char * displayContextDef_t::String_Alloc( const char *p ) {
	int len;
	long hash;
	stringDef_t *str, *last;
	static const char *staticNULL = "";

	if ( p == NULL ) {
		return NULL;
	}

	if ( *p == 0 ) {
		return staticNULL;
	}

	hash = hashForString( p );

	str = strHandle[hash];
	while ( str ) {
		if ( strcmp( p, str->str ) == 0 ) {
			return str->str;
		}
		str = str->next;
	}

	len = strlen( p );
	if ( len + strPoolIndex + 1 < STRING_POOL_SIZE ) {
		int ph = strPoolIndex;
		strcpy( &strPool[strPoolIndex], p );
		strPoolIndex += len + 1;

		str = strHandle[hash];
		last = str;
		while ( str && str->next ) {
			last = str;
			str = str->next;
		}

		str  = (stringDef_t*)UI_Alloc( sizeof( stringDef_t ) );
		str->next = NULL;
		str->str = &strPool[ph];
		if ( last ) {
			last->next = str;
		} else {
			strHandle[hash] = str;
		}
		return &strPool[ph];
	}
	return NULL;
}

void displayContextDef_t::String_Report() {
	float f;
	Com_Printf( "Memory/String Pool Info\n" );
	Com_Printf( "----------------\n" );
	f = strPoolIndex;
	f /= STRING_POOL_SIZE;
	f *= 100;
	Com_Printf( "String Pool is %.1f%% full, %i bytes out of %i used.\n", f, strPoolIndex, STRING_POOL_SIZE );
	f = allocPoint;
	f /= MEM_POOL_SIZE;
	f *= 100;
	Com_Printf( "Memory Pool is %.1f%% full, %i bytes out of %i used.\n", f, allocPoint, MEM_POOL_SIZE );
}

/*
=================
String_Init
=================
*/
void displayContextDef_t::String_Init() {
	auto&& DC = this;
	int i;
	for ( i = 0; i < HASH_TABLE_SIZE; i++ ) {
		strHandle[i] = 0;
	}
	strHandleCount = 0;
	strPoolIndex = 0;
	menuCount = 0;
	modalMenuCount = 0;
	UI_InitMemory();
	Item_SetupKeywordHash();
	Menu_SetupKeywordHash();
	if ( DC && DC->getBindingBuf ) {
		Controls_GetConfig();
	}
}

/*
=================
PC_SourceWarning
=================
*/
void displayContextDef_t::PC_SourceWarning( int handle, char *format, ... ) {
	int line;
	char filename[128];
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	vsprintf( string, format, argptr );
	va_end( argptr );

	filename[0] = '\0';
	line = 0;
	trap_PC_SourceFileAndLine( handle, filename, &line );

	Com_Printf( S_COLOR_YELLOW "WARNING: %s, line %d: %s\n", filename, line, string );
}

/*
=================
PC_SourceError
=================
*/
void displayContextDef_t::PC_SourceError( int handle, char *format, ... ) {
	int line;
	char filename[128];
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	vsprintf( string, format, argptr );
	va_end( argptr );

	filename[0] = '\0';
	line = 0;
	trap_PC_SourceFileAndLine( handle, filename, &line );

	Com_Printf( S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string );
}

/*
=================
LerpColor
	lerp and clamp each component of <a> and <b> into <c> by the fraction <t>
=================
*/
void displayContextDef_t::LerpColor( vec4_t a, vec4_t b, vec4_t c, float t ) {
	int i;
	for ( i = 0; i < 4; i++ )
	{
		c[i] = a[i] + t * ( b[i] - a[i] );
		if ( c[i] < 0 ) {
			c[i] = 0;
		} else if ( c[i] > 1.0 ) {
			c[i] = 1.0;
		}
	}
}

/*
=================
Float_Parse
=================
*/
bool displayContextDef_t::Float_Parse( char **p, float *f ) {
	char    *token;
	token = COM_ParseExt( p, false );
	if ( token && token[0] != 0 ) {
		*f = atof( token );
		return true;
	} else {
		return false;
	}
}

/*
=================
PC_Float_Parse
=================
*/
bool displayContextDef_t::PC_Float_Parse( int handle, float *f ) {
	pc_token_t token;
	int negative = false;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}
	if ( token.string[0] == '-' ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			return false;
		}
		negative = true;
	}
	if ( token.type != TT_NUMBER ) {
		PC_SourceError( handle, "expected float but found %s\n", token.string );
		return false;
	}
	if ( negative ) {
		*f = -token.floatvalue;
	} else {
		*f = token.floatvalue;
	}
	return true;
}

/*
=================
Color_Parse
=================
*/
bool displayContextDef_t::Color_Parse( char **p, vec4_t *c ) {
	int i;
	float f;

	for ( i = 0; i < 4; i++ ) {
		if ( !Float_Parse( p, &f ) ) {
			return false;
		}
		( *c )[i] = f;
	}
	return true;
}

/*
=================
PC_Color_Parse
=================
*/
bool displayContextDef_t::PC_Color_Parse( int handle, vec4_t *c ) {
	int i;
	float f;

	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return false;
		}
		( *c )[i] = f;
	}
	return true;
}

/*
=================
Int_Parse
=================
*/
bool displayContextDef_t::Int_Parse( char **p, int *i ) {
	char    *token;
	token = COM_ParseExt( p, false );

	if ( token && token[0] != 0 ) {
		*i = atoi( token );
		return true;
	} else {
		return false;
	}
}

/*
=================
PC_Int_Parse
=================
*/
bool displayContextDef_t::PC_Int_Parse( int handle, int *i ) {
	pc_token_t token;
	int negative = false;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}
	if ( token.string[0] == '-' ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			return false;
		}
		negative = true;
	}
	if ( token.type != TT_NUMBER ) {
		PC_SourceError( handle, "expected integer but found %s\n", token.string );
		return false;
	}
	*i = token.intvalue;
	if ( negative ) {
		*i = -*i;
	}
	return true;
}

/*
=================
Rect_Parse
=================
*/
bool displayContextDef_t::Rect_Parse( char **p, rectDef_t *r ) {
	if ( Float_Parse( p, &r->x ) ) {
		if ( Float_Parse( p, &r->y ) ) {
			if ( Float_Parse( p, &r->w ) ) {
				if ( Float_Parse( p, &r->h ) ) {
					return true;
				}
			}
		}
	}
	return false;
}

/*
=================
PC_Rect_Parse
=================
*/
bool displayContextDef_t::PC_Rect_Parse( int handle, rectDef_t *r ) {
	if ( PC_Float_Parse( handle, &r->x ) ) {
		if ( PC_Float_Parse( handle, &r->y ) ) {
			if ( PC_Float_Parse( handle, &r->w ) ) {
				if ( PC_Float_Parse( handle, &r->h ) ) {
					return true;
				}
			}
		}
	}
	return false;
}

/*
=================
String_Parse
=================
*/
bool displayContextDef_t::String_Parse( char **p, const char **out ) {
	char *token;

	token = COM_ParseExt( p, false );
	if ( token && token[0] != 0 ) {
		*( out ) = String_Alloc( token );
		return true;
	}
	return false;
}

/*
=================
PC_String_Parse
=================
*/
bool displayContextDef_t::PC_String_Parse( int handle, const char **out ) {
	pc_token_t token;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}

	*( out ) = String_Alloc( token.string );
	return true;
}

/*
=================
PC_String_Parse_Trans

NERVE - SMF - translates string
=================
*/
bool displayContextDef_t::PC_String_Parse_Trans( int handle, const char **out ) {
	auto&& DC = this;
	pc_token_t token;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}

	*( out ) = String_Alloc( DC->translateString( token.string ) );
	return true;
}

// NERVE - SMF
/*
=================
PC_Char_Parse
=================
*/
bool displayContextDef_t::PC_Char_Parse( int handle, char *out ) {
	pc_token_t token;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}

	*( out ) = token.string[0];
	return true;
}
// -NERVE - SMF

/*
=================
PC_Script_Parse
=================
*/
bool displayContextDef_t::PC_Script_Parse( int handle, const char **out ) {
	char script[1024];
	pc_token_t token;

	memset( script, 0, sizeof( script ) );
	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}
	if ( Q_stricmp( token.string, "{" ) != 0 ) {
		return false;
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			return false;
		}

		if ( Q_stricmp( token.string, "}" ) == 0 ) {
			*out = String_Alloc( script );
			return true;
		}

		if ( token.string[1] != '\0' ) {
			Q_strcat( script, 1024, va( "\"%s\"", token.string ) );
		} else {
			Q_strcat( script, 1024, token.string );
		}
		Q_strcat( script, 1024, " " );
	}
	return false;  // bk001105 - LCC   missing return value
}

// display, window, menu, item code
//

/*
==================
Init_Display

Initializes the display with a structure to all the drawing routines
 ==================
*/
void displayContextDef_t::Init_Display( displayContextDef_t *dc ) {
	//DC = dc;
}



// type and style painting

void displayContextDef_t::GradientBar_Paint( rectDef_t *rect, vec4_t color ) {
	auto&& DC = this;
	// gradient bar takes two paints
	DC->setColor( color );
	DC->drawHandlePic( rect->x, rect->y, rect->w, rect->h, DC->Assets.gradientBar );
	DC->setColor( NULL );
}


/*
==================
Window_Init

Initializes a window structure ( windowDef_t ) with defaults

==================
*/
void displayContextDef_t::Window_Init( Window *w ) {
	memset( w, 0, sizeof( windowDef_t ) );
	w->borderSize = 1;
	w->foreColor[0] = w->foreColor[1] = w->foreColor[2] = w->foreColor[3] = 1.0;
	w->cinematic = -1;
}

void displayContextDef_t::Fade( int *flags, float *f, float clamp, int *nextTime, int offsetTime, bool bFlags, float fadeAmount ) {
	auto&& DC = this;
	if ( *flags & ( WINDOW_FADINGOUT | WINDOW_FADINGIN ) ) {
		if ( DC->realTime > *nextTime ) {
			*nextTime = DC->realTime + offsetTime;
			if ( *flags & WINDOW_FADINGOUT ) {
				*f -= fadeAmount;
				if ( bFlags && *f <= 0.0 ) {
					*flags &= ~( WINDOW_FADINGOUT | WINDOW_VISIBLE );
				}
			} else {
				*f += fadeAmount;
				if ( *f >= clamp ) {
					*f = clamp;
					if ( bFlags ) {
						*flags &= ~WINDOW_FADINGIN;
					}
				}
			}
		}
	}
}



void displayContextDef_t::Window_Paint( Window *w, float fadeAmount, float fadeClamp, float fadeCycle ) {
	auto&& DC = this;
	//float bordersize = 0;
	vec4_t color;
	rectDef_t fillRect = w->rect;

	if ( debugMode ) {
		color[0] = color[1] = color[2] = color[3] = 1;
		DC->drawRect( w->rect.x, w->rect.y, w->rect.w, w->rect.h, 1, color );
	}

	if ( w == NULL || ( w->style == 0 && w->border == 0 ) ) {
		return;
	}

	if ( w->border != 0 ) {
		fillRect.x += w->borderSize;
		fillRect.y += w->borderSize;
		fillRect.w -= w->borderSize + 1;
		fillRect.h -= w->borderSize + 1;
	}

	if ( w->style == WINDOW_STYLE_FILLED ) {
		// box, but possible a shader that needs filled
		if ( w->background ) {
			Fade( &w->flags, &w->backColor[3], fadeClamp, &w->nextTime, fadeCycle, true, fadeAmount );
			DC->setColor( w->backColor );
			DC->drawHandlePic( fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background );
			DC->setColor( NULL );
		} else {
			DC->fillRect( fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->backColor );
		}
	} else if ( w->style == WINDOW_STYLE_GRADIENT ) {
		GradientBar_Paint( &fillRect, w->backColor );
		// gradient bar
	} else if ( w->style == WINDOW_STYLE_SHADER ) {
		if ( w->flags & WINDOW_FORECOLORSET ) {
			DC->setColor( w->foreColor );
		}
		DC->drawHandlePic( fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background );
		DC->setColor( NULL );
	} else if ( w->style == WINDOW_STYLE_TEAMCOLOR ) {
		if ( DC->getTeamColor ) {
			DC->getTeamColor( &color );
			DC->fillRect( fillRect.x, fillRect.y, fillRect.w, fillRect.h, color );
		}
	} else if ( w->style == WINDOW_STYLE_CINEMATIC ) {
		if ( w->cinematic == -1 ) {
			w->cinematic = DC->playCinematic( w->cinematicName, fillRect.x, fillRect.y, fillRect.w, fillRect.h );
			if ( w->cinematic == -1 ) {
				w->cinematic = -2;
			}
		}
		if ( w->cinematic >= 0 ) {
			DC->runCinematicFrame( w->cinematic );
			DC->drawCinematic( w->cinematic, fillRect.x, fillRect.y, fillRect.w, fillRect.h );
		}
	}

	if ( w->border == WINDOW_BORDER_FULL ) {
		// full
		// HACK HACK HACK
		if ( w->style == WINDOW_STYLE_TEAMCOLOR ) {
			if ( color[0] > 0 ) {
				// red
				color[0] = 1;
				color[1] = color[2] = .5;

			} else {
				color[2] = 1;
				color[0] = color[1] = .5;
			}
			color[3] = 1;
			DC->drawRect( w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, color );
		} else {
			DC->drawRect( w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, w->borderColor );
		}
	} else if ( w->border == WINDOW_BORDER_HORZ ) {
		// top/bottom
		DC->setColor( w->borderColor );
		DC->drawTopBottom( w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize );
		DC->setColor( NULL );
	} else if ( w->border == WINDOW_BORDER_VERT ) {
		// left right
		DC->setColor( w->borderColor );
		DC->drawSides( w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize );
		DC->setColor( NULL );
	} else if ( w->border == WINDOW_BORDER_KCGRADIENT ) {
		// this is just two gradient bars along each horz edge
		rectDef_t r = w->rect;
		r.h = w->borderSize;
		GradientBar_Paint( &r, w->borderColor );
		r.y = w->rect.y + w->rect.h - 1;
		GradientBar_Paint( &r, w->borderColor );
	}

}


void displayContextDef_t::Item_SetScreenCoords( itemDef_t *item, float x, float y ) {

	if ( item == NULL ) {
		return;
	}

	if ( item->window.border != 0 ) {
		x += item->window.borderSize;
		y += item->window.borderSize;
	}

	item->window.rect.x = x + item->window.rectClient.x;
	item->window.rect.y = y + item->window.rectClient.y;
	item->window.rect.w = item->window.rectClient.w;
	item->window.rect.h = item->window.rectClient.h;

	// force the text rects to recompute
	item->textRect.w = 0;
	item->textRect.h = 0;
}

// FIXME: consolidate this with nearby stuff
void displayContextDef_t::Item_UpdatePosition( itemDef_t *item ) {
	float x, y;
	menuDef_t *menu;

	if ( item == NULL || item->parent == NULL ) {
		return;
	}

	menu = (menuDef_t*)item->parent;

	x = menu->window.rect.x;
	y = menu->window.rect.y;

	if ( menu->window.border != 0 ) {
		x += menu->window.borderSize;
		y += menu->window.borderSize;
	}

	Item_SetScreenCoords( item, x, y );

}

// menus
void displayContextDef_t::Menu_UpdatePosition( menuDef_t *menu ) {
	int i;
	float x, y;

	if ( menu == NULL ) {
		return;
	}

	x = menu->window.rect.x;
	y = menu->window.rect.y;
	if ( menu->window.border != 0 ) {
		x += menu->window.borderSize;
		y += menu->window.borderSize;
	}

	for ( i = 0; i < menu->itemCount; i++ ) {
		Item_SetScreenCoords( menu->items[i], x, y );
	}
}

void displayContextDef_t::Menu_PostParse( menuDef_t *menu ) {
	if ( menu == NULL ) {
		return;
	}
	if ( menu->fullScreen ) {
		menu->window.rect.x = 0;
		menu->window.rect.y = 0;
		menu->window.rect.w = 640;
		menu->window.rect.h = 480;
	}
	Menu_UpdatePosition( menu );
}

itemDef_t * displayContextDef_t::Menu_ClearFocus( menuDef_t *menu ) {
	int i;
	itemDef_t *ret = NULL;

	if ( menu == NULL ) {
		return NULL;
	}

	for ( i = 0; i < menu->itemCount; i++ ) {
		if ( menu->items[i]->window.flags & WINDOW_HASFOCUS ) {
			ret = menu->items[i];
		}
		menu->items[i]->window.flags &= ~WINDOW_HASFOCUS;
		if ( menu->items[i]->leaveFocus ) {
			Item_RunScript( menu->items[i], menu->items[i]->leaveFocus );
		}
	}

	return ret;
}

bool displayContextDef_t::IsVisible( int flags ) {
	return ( flags & WINDOW_VISIBLE && !( flags & WINDOW_FADINGOUT ) );
}

bool displayContextDef_t::Rect_ContainsPoint( rectDef_t *rect, float x, float y ) {
	if ( rect ) {
		if ( x > rect->x && x < rect->x + rect->w && y > rect->y && y < rect->y + rect->h ) {
			return true;
		}
	}
	return false;
}

int displayContextDef_t::Menu_ItemsMatchingGroup( menuDef_t *menu, const char *name ) {
	int i;
	int count = 0;
	char *pdest;
	int wildcard = -1;  // if wildcard is set, it's value is the number of characters to compare


	pdest = (char*)strstr( name, "*" ); // allow wildcard strings (ex.  "hide nb_*" would translate to "hide nb_pg1; hide nb_extra" etc)
	if ( pdest ) {
		wildcard = pdest - name;
	}

	for ( i = 0; i < menu->itemCount; i++ ) {
		if ( wildcard != -1 ) {
			if ( Q_strncmp( menu->items[i]->window.name, name, wildcard ) == 0 || ( menu->items[i]->window.group && Q_strncmp( menu->items[i]->window.group, name, wildcard ) == 0 ) ) {
				count++;
			}
		} else {
			if ( Q_stricmp( menu->items[i]->window.name, name ) == 0 || ( menu->items[i]->window.group && Q_stricmp( menu->items[i]->window.group, name ) == 0 ) ) {
				count++;
			}
		}
	}

	return count;
}

itemDef_t * displayContextDef_t::Menu_GetMatchingItemByNumber( menuDef_t *menu, int index, const char *name ) {
	int i;
	int count = 0;
	char *pdest;
	int wildcard = -1;  // if wildcard is set, it's value is the number of characters to compare

	pdest = (char*)strstr( name, "*" ); // allow wildcard strings (ex.  "hide nb_*" would translate to "hide nb_pg1; hide nb_extra" etc)
	if ( pdest ) {
		wildcard = pdest - name;
	}

	for ( i = 0; i < menu->itemCount; i++ ) {
		if ( wildcard != -1 ) {
			if ( Q_strncmp( menu->items[i]->window.name, name, wildcard ) == 0 || ( menu->items[i]->window.group && Q_strncmp( menu->items[i]->window.group, name, wildcard ) == 0 ) ) {
				if ( count == index ) {
					return menu->items[i];
				}
				count++;
			}
		} else {
			if ( Q_stricmp( menu->items[i]->window.name, name ) == 0 || ( menu->items[i]->window.group && Q_stricmp( menu->items[i]->window.group, name ) == 0 ) ) {
				if ( count == index ) {
					return menu->items[i];
				}
				count++;
			}
		}
	}
	return NULL;
}



void displayContextDef_t::Script_SetColor( itemDef_t *item, char **args ) {
	const char *name;
	int i;
	float f;
	vec4_t *out;
	// expecting type of color to set and 4 args for the color
	if ( String_Parse( args, &name ) ) {
		out = NULL;
		if ( Q_stricmp( name, "backcolor" ) == 0 ) {
			out = &item->window.backColor;
			item->window.flags |= WINDOW_BACKCOLORSET;
		} else if ( Q_stricmp( name, "forecolor" ) == 0 ) {
			out = &item->window.foreColor;
			item->window.flags |= WINDOW_FORECOLORSET;
		} else if ( Q_stricmp( name, "bordercolor" ) == 0 ) {
			out = &item->window.borderColor;
		}

		if ( out ) {
			for ( i = 0; i < 4; i++ ) {
				if ( !Float_Parse( args, &f ) ) {
					return;
				}
				( *out )[i] = f;
			}
		}
	}
}

void displayContextDef_t::Script_SetAsset( itemDef_t *item, char **args ) {
	const char *name;
	// expecting name to set asset to
	if ( String_Parse( args, &name ) ) {
		// check for a model
		if ( item->type == ITEM_TYPE_MODEL ) {
		}
	}
}

void displayContextDef_t::Script_SetBackground( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *name;
	// expecting name to set asset to
	if ( String_Parse( args, &name ) ) {
		item->window.background = DC->registerShaderNoMip( name );
	}
}




itemDef_t * displayContextDef_t::Menu_FindItemByName( menuDef_t *menu, const char *p ) {
	int i;

	if ( menu == NULL || p == NULL ) {
		return NULL;
	}

	for ( i = 0; i < menu->itemCount; i++ ) {
		if ( Q_stricmp( p, menu->items[i]->window.name ) == 0 ) {
			return menu->items[i];
		}
	}

	return NULL;
}

void displayContextDef_t::Script_SetTeamColor( itemDef_t *item, char **args ) {
	auto&& DC = this;
	if ( DC->getTeamColor ) {
		int i;
		vec4_t color;
		DC->getTeamColor( &color );
		for ( i = 0; i < 4; i++ ) {
			item->window.backColor[i] = color[i];
		}
	}
}

void displayContextDef_t::Script_SetItemColor( itemDef_t *item, char **args ) {
	const char *itemname;
	const char *name;
	vec4_t color;
	int i;
	vec4_t *out;
	// expecting type of color to set and 4 args for the color
	if ( String_Parse( args, &itemname ) && String_Parse( args, &name ) ) {
		itemDef_t *item2;
		int j;
		int count = Menu_ItemsMatchingGroup( (menuDef_t*)item->parent, itemname );

		if ( !Color_Parse( args, &color ) ) {
			return;
		}

		for ( j = 0; j < count; j++ ) {
			item2 = Menu_GetMatchingItemByNumber( (menuDef_t*)item->parent, j, itemname );
			if ( item2 != NULL ) {
				out = NULL;
				if ( Q_stricmp( name, "backcolor" ) == 0 ) {
					out = &item2->window.backColor;
				} else if ( Q_stricmp( name, "forecolor" ) == 0 ) {
					out = &item2->window.foreColor;
					item2->window.flags |= WINDOW_FORECOLORSET;
				} else if ( Q_stricmp( name, "bordercolor" ) == 0 ) {
					out = &item2->window.borderColor;
				}

				if ( out ) {
					for ( i = 0; i < 4; i++ ) {
						( *out )[i] = color[i];
					}
				}
			}
		}
	}
}


void displayContextDef_t::Menu_ShowItemByName( menuDef_t *menu, const char *p, bool bShow ) {
	auto&& DC = this;
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup( menu, p );
	for ( i = 0; i < count; i++ ) {
		item = Menu_GetMatchingItemByNumber( menu, i, p );
		if ( item != NULL ) {
			if ( bShow ) {
				item->window.flags |= WINDOW_VISIBLE;
			} else {
				item->window.flags &= ~WINDOW_VISIBLE;
				// stop cinematics playing in the window
				if ( item->window.cinematic >= 0 ) {
					DC->stopCinematic( item->window.cinematic );
					item->window.cinematic = -1;
				}
			}
		}
	}
}

void displayContextDef_t::Menu_FadeItemByName( menuDef_t *menu, const char *p, bool fadeOut ) {
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup( menu, p );
	for ( i = 0; i < count; i++ ) {
		item = Menu_GetMatchingItemByNumber( menu, i, p );
		if ( item != NULL ) {
			if ( fadeOut ) {
				item->window.flags |= ( WINDOW_FADINGOUT | WINDOW_VISIBLE );
				item->window.flags &= ~WINDOW_FADINGIN;
			} else {
				item->window.flags |= ( WINDOW_VISIBLE | WINDOW_FADINGIN );
				item->window.flags &= ~WINDOW_FADINGOUT;
			}
		}
	}
}

menuDef_t * displayContextDef_t::Menus_FindByName( const char *p ) {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		if ( Q_stricmp( Menus[i].window.name, p ) == 0 ) {
			return &Menus[i];
		}
	}
	return NULL;
}

void displayContextDef_t::Menus_ShowByName( const char *p ) {
	menuDef_t *menu = Menus_FindByName( p );
	if ( menu ) {
		Menus_Activate( menu );
	}
}

void displayContextDef_t::Menus_OpenByName( const char *p ) {
	Menus_ActivateByName( p, true );
}

 void displayContextDef_t::Menu_RunCloseScript( menuDef_t *menu ) {
	if ( menu && menu->window.flags & WINDOW_VISIBLE && menu->onClose ) {
		itemDef_t item;
		item.parent = menu;
		Item_RunScript( &item, menu->onClose );
	}
}

void displayContextDef_t::Menus_CloseByName( const char *p ) {
	menuDef_t *menu = Menus_FindByName( p );
	if ( menu != NULL ) {
		Menu_RunCloseScript( menu );
		menu->window.flags &= ~( WINDOW_VISIBLE | WINDOW_HASFOCUS );
		if ( menu->window.flags & WINDOW_MODAL ) {
			if ( modalMenuCount <= 0 ) {
				Com_Printf( S_COLOR_YELLOW "WARNING: tried closing a modal window with an empty modal stack!\n" );
			} else
			{
				modalMenuCount--;
				// if modal doesn't have a parent, the stack item may be NULL .. just go back to the main menu then
				if ( modalMenuStack[modalMenuCount] ) {
					Menus_ActivateByName( modalMenuStack[modalMenuCount]->window.name, false ); // don't try to push the one we are opening to the stack
				}
			}
		}
	}
}

void displayContextDef_t::Menus_CloseAll() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		Menu_RunCloseScript( &Menus[i] );
		Menus[i].window.flags &= ~( WINDOW_HASFOCUS | WINDOW_VISIBLE );
	}
}


void displayContextDef_t::Script_Show( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menu_ShowItemByName((menuDef_t*)item->parent, name, true );
	}
}

void displayContextDef_t::Script_Hide( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menu_ShowItemByName( (menuDef_t*)item->parent, name, false );
	}
}

void displayContextDef_t::Script_FadeIn( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menu_FadeItemByName( (menuDef_t*)item->parent, name, false );
	}
}

void displayContextDef_t::Script_FadeOut( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menu_FadeItemByName((menuDef_t*)item->parent, name, true );
	}
}



void displayContextDef_t::Script_Open( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menus_OpenByName( name );
	}
}

// DHM - Nerve

void displayContextDef_t::Script_ConditionalOpen( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *cvar;
	const char *name1;
	const char *name2;
	float val;

	if ( String_Parse( args, &cvar ) && String_Parse( args, &name1 ) && String_Parse( args, &name2 ) ) {

		val = DC->getCVarValue( cvar );
		if ( val == 0.f ) {
			Menus_OpenByName( name2 );
		} else {
			Menus_OpenByName( name1 );
		}
	}
}

// DHM - Nerve

void displayContextDef_t::Script_Close( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menus_CloseByName( name );
	}
}




/*
==============
Script_Clipboard
==============
*/
void displayContextDef_t::Script_Clipboard( itemDef_t *item, char **args ) {
	auto&& DC = this;
	char curscript[64];
	DC->getCVarString( "cg_clipboardName", curscript, sizeof( curscript ) ); // grab the string the client set
	Menu_ShowItemByName( (menuDef_t*)item->parent, curscript, true );
}




#define NOTEBOOK_MAX_PAGES 6    // this will not be a define


/*
==============
Script_NotebookShowpage
	hide all notebook pages and show just the active one

	inc == 0	- show current page
	inc == val	- turn inc pages in the notebook (negative numbers are backwards)
	inc == 999	- key number.  +999 is jump to last page, -999 is jump to cover page
==============
*/
void displayContextDef_t::Script_NotebookShowpage( itemDef_t *item, char **args ) {
	auto&& DC = this;
	int i, inc, curpage, newpage = 0, pages;

	pages = DC->getCVarValue( "cg_notebookpages" );

	if ( Int_Parse( args, &inc ) ) {

		curpage = DC->getCVarValue( "ui_notebookCurrentPage" );


		if ( inc == 0 ) {  // opening
			if ( pages && !curpage ) { // only open to cover if no pages exist
				inc = 1;    // otherwise, go to first available page
			}
		}

		if ( inc == 999 ) {            // jump to end
//			newpage = NOTEBOOK_MAX_PAGES;	// = lastpage;
			curpage = 0;
			inc = -1;
		} else if ( inc == -999 ) {    // jump to start
			curpage = 0;
			inc = 0;
		} else if ( inc > 500 ) {
//			curpage = DEBRIEFING_BASE + (inc - 500);
			curpage = inc;
			inc = 0;
		}

		if ( inc ) {
			int dec = 0;

			if ( inc > 0 ) {
				for ( i = 1; i < NOTEBOOK_MAX_PAGES; i++ ) {
					newpage = curpage + i;
					if ( newpage > NOTEBOOK_MAX_PAGES ) {
						newpage = newpage % NOTEBOOK_MAX_PAGES;
					}

					if ( newpage == 0 ) {
						continue;
					}

					if ( pages & ( 1 << ( newpage - 1 ) ) ) {
						dec++;
//						if(dec == inc)
//							break;
						break;
					}
				}
				if ( i < NOTEBOOK_MAX_PAGES ) {  // a valid page was found
					curpage = newpage;
				}

			} else {
				for ( i = 1; i < NOTEBOOK_MAX_PAGES; i++ ) {
					newpage = curpage - i;
					if ( newpage <= 0 ) {
						newpage = newpage + NOTEBOOK_MAX_PAGES;
					}

					if ( pages & ( 1 << ( newpage - 1 ) ) ) {
						break;
					}
				}
				if ( i < NOTEBOOK_MAX_PAGES ) {  // a valid page was found
					curpage = newpage;
				}

			}
		}


		// hide all the pages
//		Menu_ShowItemByName(item->parent, "page_*", false);
		Menu_ShowItemByName( (menuDef_t*)item->parent, "cover", false );
		for ( i = 1; i <= NOTEBOOK_MAX_PAGES; i++ ) {
			Menu_ShowItemByName((menuDef_t*)item->parent, va( "page%d", i ), false );
		}

		// show the visible one

		if ( curpage ) {
			Menu_ShowItemByName((menuDef_t*)item->parent, va( "page%d", curpage ), true );
		} else {
			Menu_ShowItemByName((menuDef_t*)item->parent, "cover", true );
		}

		DC->setCVar( "ui_notebookCurrentPage", va( "%d", curpage ) ); // store new current page

	}
}



void displayContextDef_t::Menu_TransitionItemByName( menuDef_t *menu, const char *p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt ) {
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup( menu, p );
	for ( i = 0; i < count; i++ ) {
		item = Menu_GetMatchingItemByNumber( menu, i, p );
		if ( item != NULL ) {
			item->window.flags |= ( WINDOW_INTRANSITION | WINDOW_VISIBLE );
			item->window.offsetTime = time;
			memcpy( &item->window.rectClient, &rectFrom, sizeof( rectDef_t ) );
			memcpy( &item->window.rectEffects, &rectTo, sizeof( rectDef_t ) );
			item->window.rectEffects2.x = fabs( rectTo.x - rectFrom.x ) / amt;
			item->window.rectEffects2.y = fabs( rectTo.y - rectFrom.y ) / amt;
			item->window.rectEffects2.w = fabs( rectTo.w - rectFrom.w ) / amt;
			item->window.rectEffects2.h = fabs( rectTo.h - rectFrom.h ) / amt;
			Item_UpdatePosition( item );
		}
	}
}


void displayContextDef_t::Script_Transition( itemDef_t *item, char **args ) {
	const char *name;
	rectDef_t rectFrom, rectTo;
	int time;
	float amt;

	if ( String_Parse( args, &name ) ) {
		if ( Rect_Parse( args, &rectFrom ) && Rect_Parse( args, &rectTo ) && Int_Parse( args, &time ) && Float_Parse( args, &amt ) ) {
			Menu_TransitionItemByName((menuDef_t*)item->parent, name, rectFrom, rectTo, time, amt );
		}
	}
}


void displayContextDef_t::Menu_OrbitItemByName( menuDef_t *menu, const char *p, float x, float y, float cx, float cy, int time ) {
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup( menu, p );
	for ( i = 0; i < count; i++ ) {
		item = Menu_GetMatchingItemByNumber( menu, i, p );
		if ( item != NULL ) {
			item->window.flags |= ( WINDOW_ORBITING | WINDOW_VISIBLE );
			item->window.offsetTime = time;
			item->window.rectEffects.x = cx;
			item->window.rectEffects.y = cy;
			item->window.rectClient.x = x;
			item->window.rectClient.y = y;
			Item_UpdatePosition( item );
		}
	}
}


void displayContextDef_t::Script_Orbit( itemDef_t *item, char **args ) {
	const char *name;
	float cx, cy, x, y;
	int time;

	if ( String_Parse( args, &name ) ) {
		if ( Float_Parse( args, &x ) && Float_Parse( args, &y ) && Float_Parse( args, &cx ) && Float_Parse( args, &cy ) && Int_Parse( args, &time ) ) {
			Menu_OrbitItemByName( (menuDef_t*)item->parent, name, x, y, cx, cy, time );
		}
	}
}



void displayContextDef_t::Script_SetFocus( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *name;
	itemDef_t *focusItem;

	if ( String_Parse( args, &name ) ) {
		focusItem = Menu_FindItemByName( (menuDef_t*)item->parent, name );
		if ( focusItem && !( focusItem->window.flags & WINDOW_DECORATION ) && !( focusItem->window.flags & WINDOW_HASFOCUS ) ) {
			Menu_ClearFocus( (menuDef_t*)item->parent );
			focusItem->window.flags |= WINDOW_HASFOCUS;
			if ( focusItem->onFocus ) {
				Item_RunScript( focusItem, focusItem->onFocus );
			}
			if ( DC->Assets.itemFocusSound ) {
				DC->startLocalSound( DC->Assets.itemFocusSound, CHAN_LOCAL_SOUND );
			}
		}
	}
}

void displayContextDef_t::Script_SetPlayerModel( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *name;
	if ( String_Parse( args, &name ) ) {
		DC->setCVar( "team_model", name );
	}
}

void displayContextDef_t::Script_SetPlayerHead( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *name;
	if ( String_Parse( args, &name ) ) {
		DC->setCVar( "team_headmodel", name );
	}
}

// ATVI Wolfenstein Misc #304
// the parser misreads setCvar "bleh" ""
// you have to use clearCvar "bleh"
void displayContextDef_t::Script_ClearCvar( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *cvar;
	if ( String_Parse( args, &cvar ) ) {
		DC->setCVar( cvar, "" );
	}
}

void displayContextDef_t::Script_SetCvar( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *cvar, *val;
	if ( String_Parse( args, &cvar ) && String_Parse( args, &val ) ) {
		DC->setCVar( cvar, val );
	}
}

void displayContextDef_t::Script_Exec( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *val;
	if ( String_Parse( args, &val ) ) {
		DC->executeText( EXEC_APPEND, va( "%s ; ", val ) );
	}
}

void displayContextDef_t::Script_Play( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *val;
	if ( String_Parse( args, &val ) ) {
		DC->startLocalSound( DC->registerSound( val ), CHAN_LOCAL_SOUND );      // all sounds are not 3d
	}
}

void displayContextDef_t::Script_playLooped( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *val;
	if ( String_Parse( args, &val ) ) {
		DC->stopBackgroundTrack();
		DC->startBackgroundTrack( val, val );
	}
}

// NERVE - SMF
void displayContextDef_t::Script_AddListItem( itemDef_t *item, char **args ) {
	auto&& DC = this;
	const char *itemname, *val, *name;
	itemDef_t *t;

	if ( String_Parse( args, &itemname ) && String_Parse( args, &val ) && String_Parse( args, &name ) ) {
		t = Menu_FindItemByName((menuDef_t*)item->parent, itemname );
		if ( t && t->special ) {
			DC->feederAddItem( t->special, name, atoi( val ) );
		}
	}
}
// -NERVE - SMF
// DHM - Nerve
void displayContextDef_t::Script_CheckAutoUpdate( itemDef_t *item, char **args ) {
	auto&& DC = this;
	DC->checkAutoUpdate();
}

void displayContextDef_t::Script_GetAutoUpdate( itemDef_t *item, char **args ) {
	auto&& DC = this;
	DC->getAutoUpdate();
}
// DHM - Nerve

commandDef_t commandList[] =
{
	{"fadein", &displayContextDef_t::Script_FadeIn},                  // group/name
	{"fadeout", &displayContextDef_t::Script_FadeOut},                // group/name
	{"show", &displayContextDef_t::Script_Show},                      // group/name
	{"hide", &displayContextDef_t::Script_Hide},                      // group/name
	{"setcolor", &displayContextDef_t::Script_SetColor},              // works on this
	{"open", &displayContextDef_t::Script_Open},                      // menu

	{"conditionalopen", &displayContextDef_t::Script_ConditionalOpen},    // DHM - Nerve:: cvar menu menu
													 // opens first menu if cvar is true[non-zero], second if false

	{"close", &displayContextDef_t::Script_Close},                    // menu
	{"clipboard", &displayContextDef_t::Script_Clipboard},            // show the current clipboard group by name
	{"showpage", &displayContextDef_t::Script_NotebookShowpage},          //
	{"setasset", &displayContextDef_t::Script_SetAsset},              // works on this
	{"setbackground", &displayContextDef_t::Script_SetBackground},    // works on this
	{"setitemcolor", &displayContextDef_t::Script_SetItemColor},      // group/name
	{"setteamcolor", &displayContextDef_t::Script_SetTeamColor},      // sets this background color to team color
	{"setfocus", &displayContextDef_t::Script_SetFocus},              // sets this background color to team color
	{"setplayermodel", &displayContextDef_t::Script_SetPlayerModel},  // sets this background color to team color
	{"setplayerhead", &displayContextDef_t::Script_SetPlayerHead},    // sets this background color to team color
	{"transition", &displayContextDef_t::Script_Transition},          // group/name
	{"setcvar", &displayContextDef_t::Script_SetCvar},                // group/name
	{"clearcvar", &displayContextDef_t::Script_ClearCvar},
	{"exec", &displayContextDef_t::Script_Exec},                      // group/name
	{"play", &displayContextDef_t::Script_Play},                      // group/name
	{"playlooped", &displayContextDef_t::Script_playLooped},          // group/name
	{"orbit", &displayContextDef_t::Script_Orbit},                    // group/name
	{"addlistitem", &displayContextDef_t::Script_AddListItem},        // NERVE - SMF - special command to add text items to list box
	{"checkautoupdate", &displayContextDef_t::Script_CheckAutoUpdate},    // DHM - Nerve
	{"getautoupdate", &displayContextDef_t::Script_GetAutoUpdate} // DHM - Nerve
};

int scriptCommandCount = sizeof( commandList ) / sizeof( commandDef_t );


void displayContextDef_t::Item_RunScript( itemDef_t *item, const char *s ) {
	auto&& DC = this;
	char script[1024], *p;
	int i;
	bool bRan;
	memset( script, 0, sizeof( script ) );
	if ( item && s && s[0] ) {
		Q_strcat( script, 1024, s );
		p = script;
		while ( 1 ) {
			const char *command;
			// expect command then arguments, ; ends command, NULL ends script
			if ( !String_Parse( &p, &command ) ) {
				return;
			}

			if ( command[0] == ';' && command[1] == '\0' ) {
				continue;
			}

			bRan = false;
			for ( i = 0; i < scriptCommandCount; i++ ) {
				if ( Q_stricmp( command, commandList[i].name ) == 0 ) {
					((this->*commandList[i].handler)(item, &p));
					bRan = true;
					break;
				}
			}
			// not in our auto list, pass to handler
			if ( !bRan ) {
				(this->*(DC->runScript))(&p);
			}
		}
	}
}


bool displayContextDef_t::Item_EnableShowViaCvar( itemDef_t *item, int flag ) {
	auto&& DC = this;
	char script[1024], *p;
	memset( script, 0, sizeof( script ) );
	if ( item && item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest ) {
		char buff[1024];
		DC->getCVarString( item->cvarTest, buff, sizeof( buff ) );

		Q_strcat( script, 1024, item->enableCvar );
		p = script;
		while ( 1 ) {
			const char *val;
			// expect value then ; or NULL, NULL ends list
			if ( !String_Parse( &p, &val ) ) {
				return ( item->cvarFlags & flag ) ? false : true;
			}

			if ( val[0] == ';' && val[1] == '\0' ) {
				continue;
			}

			// enable it if any of the values are true
			if ( item->cvarFlags & flag ) {
				if ( Q_stricmp( buff, val ) == 0 ) {
					return true;
				}
			} else {
				// disable it if any of the values are true
				if ( Q_stricmp( buff, val ) == 0 ) {
					return false;
				}
			}

		}
		return ( item->cvarFlags & flag ) ? false : true;
	}
	return true;
}


// will optionaly set focus to this item
bool displayContextDef_t::Item_SetFocus( itemDef_t *item, float x, float y ) {
	auto&& DC = this;
	int i;
	itemDef_t *oldFocus;
	sfxHandle_t *sfx = &DC->Assets.itemFocusSound;
	bool playSound = false;
	menuDef_t *parent; // bk001206: = (menuDef_t*)item->parent;
	// sanity check, non-null, not a decoration and does not already have the focus
	if ( item == NULL || item->window.flags & WINDOW_DECORATION || item->window.flags & WINDOW_HASFOCUS || !( item->window.flags & WINDOW_VISIBLE ) ) {
		return false;
	}

	// bk001206 - this can be NULL.
	parent = (menuDef_t*)item->parent;

	// items can be enabled and disabled based on cvars
	if ( item->cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !Item_EnableShowViaCvar( item, CVAR_ENABLE ) ) {
		return false;
	}

	if ( item->cvarFlags & ( CVAR_SHOW | CVAR_HIDE ) && !Item_EnableShowViaCvar( item, CVAR_SHOW ) ) {
		return false;
	}

	oldFocus = Menu_ClearFocus( (menuDef_t*)item->parent );

	if ( item->type == ITEM_TYPE_TEXT ) {
		rectDef_t r;
		r = item->textRect;
		r.y -= r.h;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			item->window.flags |= WINDOW_HASFOCUS;
			if ( item->focusSound ) {
				sfx = &item->focusSound;
			}
			playSound = true;
		} else {
			if ( oldFocus ) {
				oldFocus->window.flags |= WINDOW_HASFOCUS;
				if ( oldFocus->onFocus ) {
					Item_RunScript( oldFocus, oldFocus->onFocus );
				}
			}
		}
	} else {
		item->window.flags |= WINDOW_HASFOCUS;
		if ( item->onFocus ) {
			Item_RunScript( item, item->onFocus );
		}
		if ( item->focusSound ) {
			sfx = &item->focusSound;
		}
		playSound = true;
	}

	if ( playSound && sfx ) {
		DC->startLocalSound( *sfx, CHAN_LOCAL_SOUND );
	}

	for ( i = 0; i < parent->itemCount; i++ ) {
		if ( parent->items[i] == item ) {
			parent->cursorItem = i;
			break;
		}
	}

	return true;
}

int displayContextDef_t::Item_ListBox_MaxScroll( itemDef_t *item ) {
	auto&& DC = this;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	int count = DC->feederCount( item->special );
	int max;

	if ( item->window.flags & WINDOW_HORIZONTAL ) {
		max = count - ( item->window.rect.w / listPtr->elementWidth ) + 1;
	} else {
		max = count - ( item->window.rect.h / listPtr->elementHeight ) + 1;
	}
	if ( max < 0 ) {
		return 0;
	}
	return max;
}

int displayContextDef_t::Item_ListBox_ThumbPosition( itemDef_t *item ) {
	float max, pos, size;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

	max = Item_ListBox_MaxScroll( item );
	if ( item->window.flags & WINDOW_HORIZONTAL ) {
		size = item->window.rect.w - ( SCROLLBAR_SIZE * 2 ) - 2;
		if ( max > 0 ) {
			pos = ( size - SCROLLBAR_SIZE ) / (float) max;
		} else {
			pos = 0;
		}
		pos *= listPtr->startPos;
		return item->window.rect.x + 1 + SCROLLBAR_SIZE + pos;
	} else {
		size = item->window.rect.h - ( SCROLLBAR_SIZE * 2 ) - 2;
		if ( max > 0 ) {
			pos = ( size - SCROLLBAR_SIZE ) / (float) max;
		} else {
			pos = 0;
		}
		pos *= listPtr->startPos;
		return item->window.rect.y + 1 + SCROLLBAR_SIZE + pos;
	}
}

int displayContextDef_t::Item_ListBox_ThumbDrawPosition( itemDef_t *item ) {
	auto&& DC = this;
	int min, max;

	if ( itemCapture == item ) {
		if ( item->window.flags & WINDOW_HORIZONTAL ) {
			min = item->window.rect.x + SCROLLBAR_SIZE + 1;
			max = item->window.rect.x + item->window.rect.w - 2 * SCROLLBAR_SIZE - 1;
			if ( DC->cursorx >= min + SCROLLBAR_SIZE / 2 && DC->cursorx <= max + SCROLLBAR_SIZE / 2 ) {
				return DC->cursorx - SCROLLBAR_SIZE / 2;
			} else {
				return Item_ListBox_ThumbPosition( item );
			}
		} else {
			min = item->window.rect.y + SCROLLBAR_SIZE + 1;
			max = item->window.rect.y + item->window.rect.h - 2 * SCROLLBAR_SIZE - 1;
			if ( DC->cursory >= min + SCROLLBAR_SIZE / 2 && DC->cursory <= max + SCROLLBAR_SIZE / 2 ) {
				return DC->cursory - SCROLLBAR_SIZE / 2;
			} else {
				return Item_ListBox_ThumbPosition( item );
			}
		}
	} else {
		return Item_ListBox_ThumbPosition( item );
	}
}

float displayContextDef_t::Item_Slider_ThumbPosition( itemDef_t *item ) {
	auto&& DC = this;
	float value, range, x;
	editFieldDef_t *editDef = (editFieldDef_t*)item->typeData;

	if ( item->text ) {
		x = item->textRect.x + item->textRect.w + 8;
	} else {
		x = item->window.rect.x;
	}

	if ( editDef == NULL && item->cvar ) {
		return x;
	}

	value = DC->getCVarValue( item->cvar );

	if ( value < editDef->minVal ) {
		value = editDef->minVal;
	} else if ( value > editDef->maxVal ) {
		value = editDef->maxVal;
	}

	range = editDef->maxVal - editDef->minVal;
	value -= editDef->minVal;
	value /= range;
	//value /= (editDef->maxVal - editDef->minVal);
	value *= SLIDER_WIDTH;
	x += value;
	// vm fuckage
	//x = x + (((float)value / editDef->maxVal) * SLIDER_WIDTH);
	return x;
}

int displayContextDef_t::Item_Slider_OverSlider( itemDef_t *item, float x, float y ) {
	rectDef_t r;

	r.x = Item_Slider_ThumbPosition( item ) - ( SLIDER_THUMB_WIDTH / 2 );
	r.y = item->window.rect.y - 2;
	r.w = SLIDER_THUMB_WIDTH;
	r.h = SLIDER_THUMB_HEIGHT;

	if ( Rect_ContainsPoint( &r, x, y ) ) {
		return WINDOW_LB_THUMB;
	}
	return 0;
}

int displayContextDef_t::Item_ListBox_OverLB( itemDef_t *item, float x, float y ) {
	auto&& DC = this;
	rectDef_t r;
	listBoxDef_t *listPtr;
	int thumbstart;
	int count;

	count = DC->feederCount( item->special );
	listPtr = (listBoxDef_t*)item->typeData;
	if ( item->window.flags & WINDOW_HORIZONTAL ) {
		// check if on left arrow
		r.x = item->window.rect.x;
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		r.h = r.w = SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_LEFTARROW;
		}
		// check if on right arrow
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_RIGHTARROW;
		}
		// check if on thumb
		thumbstart = Item_ListBox_ThumbPosition( item );
		r.x = thumbstart;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_THUMB;
		}
		r.x = item->window.rect.x + SCROLLBAR_SIZE;
		r.w = thumbstart - r.x;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGUP;
		}
		r.x = thumbstart + SCROLLBAR_SIZE;
		r.w = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGDN;
		}
	} else {
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		r.y = item->window.rect.y;
		r.h = r.w = SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_LEFTARROW;
		}
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_RIGHTARROW;
		}
		thumbstart = Item_ListBox_ThumbPosition( item );
		r.y = thumbstart;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_THUMB;
		}
		r.y = item->window.rect.y + SCROLLBAR_SIZE;
		r.h = thumbstart - r.y;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGUP;
		}
		r.y = thumbstart + SCROLLBAR_SIZE;
		r.h = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGDN;
		}
	}
	return 0;
}


void displayContextDef_t::Item_ListBox_MouseEnter( itemDef_t *item, float x, float y ) {
	rectDef_t r;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

	item->window.flags &= ~( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN );
	item->window.flags |= Item_ListBox_OverLB( item, x, y );

	if ( item->window.flags & WINDOW_HORIZONTAL ) {
		if ( !( item->window.flags & ( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN ) ) ) {
			// check for selection hit as we have exausted buttons and thumb
			if ( listPtr->elementStyle == LISTBOX_IMAGE ) {
				r.x = item->window.rect.x;
				r.y = item->window.rect.y;
				r.h = item->window.rect.h - SCROLLBAR_SIZE;
				r.w = item->window.rect.w - listPtr->drawPadding;
				if ( Rect_ContainsPoint( &r, x, y ) ) {
					listPtr->cursorPos =  (int)( ( x - r.x ) / listPtr->elementWidth )  + listPtr->startPos;
					if ( listPtr->cursorPos >= listPtr->endPos ) {
						listPtr->cursorPos = listPtr->endPos;
					}
				}
			} else {
				// text hit..
			}
		}
	} else if ( !( item->window.flags & ( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN ) ) ) {
		r.x = item->window.rect.x;
		r.y = item->window.rect.y;
		r.w = item->window.rect.w - SCROLLBAR_SIZE;
		r.h = item->window.rect.h - listPtr->drawPadding;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			listPtr->cursorPos =  (int)( ( y - 2 - r.y ) / listPtr->elementHeight )  + listPtr->startPos;
			if ( listPtr->cursorPos > listPtr->endPos ) {
				listPtr->cursorPos = listPtr->endPos;
			}
		}
	}
}

void displayContextDef_t::Item_MouseEnter( itemDef_t *item, float x, float y ) {
	rectDef_t r;
	if ( item ) {
		r = item->textRect;
		r.y -= r.h;
		// in the text rect?

		// items can be enabled and disabled based on cvars
		if ( item->cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !Item_EnableShowViaCvar( item, CVAR_ENABLE ) ) {
			return;
		}

		if ( item->cvarFlags & ( CVAR_SHOW | CVAR_HIDE ) && !Item_EnableShowViaCvar( item, CVAR_SHOW ) ) {
			return;
		}

		if ( Rect_ContainsPoint( &r, x, y ) ) {
			if ( !( item->window.flags & WINDOW_MOUSEOVERTEXT ) ) {
				Item_RunScript( item, item->mouseEnterText );
				item->window.flags |= WINDOW_MOUSEOVERTEXT;
			}
			if ( !( item->window.flags & WINDOW_MOUSEOVER ) ) {
				Item_RunScript( item, item->mouseEnter );
				item->window.flags |= WINDOW_MOUSEOVER;
			}

		} else {
			// not in the text rect
			if ( item->window.flags & WINDOW_MOUSEOVERTEXT ) {
				// if we were
				Item_RunScript( item, item->mouseExitText );
				item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
			}
			if ( !( item->window.flags & WINDOW_MOUSEOVER ) ) {
				Item_RunScript( item, item->mouseEnter );
				item->window.flags |= WINDOW_MOUSEOVER;
			}

			if ( item->type == ITEM_TYPE_LISTBOX ) {
				Item_ListBox_MouseEnter( item, x, y );
			}
		}
	}
}

void displayContextDef_t::Item_MouseLeave( itemDef_t *item ) {
	if ( item ) {
		if ( item->window.flags & WINDOW_MOUSEOVERTEXT ) {
			Item_RunScript( item, item->mouseExitText );
			item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
		}
		Item_RunScript( item, item->mouseExit );
		item->window.flags &= ~( WINDOW_LB_RIGHTARROW | WINDOW_LB_LEFTARROW );
	}
}

itemDef_t * displayContextDef_t::Menu_HitTest( menuDef_t *menu, float x, float y ) {
	int i;
	for ( i = 0; i < menu->itemCount; i++ ) {
		if ( Rect_ContainsPoint( &menu->items[i]->window.rect, x, y ) ) {
			return menu->items[i];
		}
	}
	return NULL;
}

void displayContextDef_t::Item_SetMouseOver( itemDef_t *item, bool focus ) {
	if ( item ) {
		if ( focus ) {
			item->window.flags |= WINDOW_MOUSEOVER;
		} else {
			item->window.flags &= ~WINDOW_MOUSEOVER;
		}
	}
}


bool displayContextDef_t::Item_OwnerDraw_HandleKey( itemDef_t *item, int key ) {
	auto&& DC = this;
	if ( item && DC->ownerDrawHandleKey ) {
		return DC->ownerDrawHandleKey( item->window.ownerDraw, item->window.ownerDrawFlags, &item->special, key );
	}
	return false;
}

bool displayContextDef_t::Item_ListBox_HandleKey( itemDef_t *item, int key, bool down, bool force ) {
	auto&& DC = this;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	int count = DC->feederCount( item->special );
	int max, viewmax;

	if ( force || ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) && item->window.flags & WINDOW_HASFOCUS ) ) {
		max = Item_ListBox_MaxScroll( item );
		if ( item->window.flags & WINDOW_HORIZONTAL ) {
			viewmax = ( item->window.rect.w / listPtr->elementWidth );
			if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) {
				if ( !listPtr->notselectable ) {
					listPtr->cursorPos--;
					if ( listPtr->cursorPos < 0 ) {
						listPtr->cursorPos = 0;
					}
					if ( listPtr->cursorPos < listPtr->startPos ) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					(this->*(DC->feederSelection))(item->special, item->cursorPos);
				} else {
					listPtr->startPos--;
					if ( listPtr->startPos < 0 ) {
						listPtr->startPos = 0;
					}
				}
				return true;
			}
			if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) {
				if ( !listPtr->notselectable ) {
					listPtr->cursorPos++;
					if ( listPtr->cursorPos < listPtr->startPos ) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if ( listPtr->cursorPos >= count ) {
						listPtr->cursorPos = count - 1;
					}
					if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					(this->*(DC->feederSelection))( item->special, item->cursorPos );
				} else {
					listPtr->startPos++;
					if ( listPtr->startPos >= count ) {
						listPtr->startPos = count - 1;
					}
				}
				return true;
			}
		} else {
			viewmax = ( item->window.rect.h / listPtr->elementHeight );
			if ( key == K_UPARROW || key == K_KP_UPARROW ) {
				if ( !listPtr->notselectable ) {
					listPtr->cursorPos--;
					if ( listPtr->cursorPos < 0 ) {
						listPtr->cursorPos = 0;
					}
					if ( listPtr->cursorPos < listPtr->startPos ) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					(this->*(DC->feederSelection))( item->special, item->cursorPos );
				} else {
					listPtr->startPos--;
					if ( listPtr->startPos < 0 ) {
						listPtr->startPos = 0;
					}
				}
				return true;
			}
			if ( key == K_DOWNARROW || key == K_KP_DOWNARROW ) {
				if ( !listPtr->notselectable ) {
					listPtr->cursorPos++;
					if ( listPtr->cursorPos < listPtr->startPos ) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if ( listPtr->cursorPos >= count ) {
						listPtr->cursorPos = count - 1;
					}
					if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					item->cursorPos = listPtr->cursorPos;
					(this->*(DC->feederSelection))( item->special, item->cursorPos );
				} else {
					listPtr->startPos++;
					if ( listPtr->startPos > max ) {
						listPtr->startPos = max;
					}
				}
				return true;
			}
		}
		// mouse hit
		if ( key == K_MOUSE1 || key == K_MOUSE2 ) {
			if ( item->window.flags & WINDOW_LB_LEFTARROW ) {
				listPtr->startPos--;
				if ( listPtr->startPos < 0 ) {
					listPtr->startPos = 0;
				}
			} else if ( item->window.flags & WINDOW_LB_RIGHTARROW ) {
				// one down
				listPtr->startPos++;
				if ( listPtr->startPos > max ) {
					listPtr->startPos = max;
				}
			} else if ( item->window.flags & WINDOW_LB_PGUP ) {
				// page up
				listPtr->startPos -= viewmax;
				if ( listPtr->startPos < 0 ) {
					listPtr->startPos = 0;
				}
			} else if ( item->window.flags & WINDOW_LB_PGDN ) {
				// page down
				listPtr->startPos += viewmax;
				if ( listPtr->startPos > max ) {
					listPtr->startPos = max;
				}
			} else if ( item->window.flags & WINDOW_LB_THUMB ) {
				// Display_SetCaptureItem(item);
			} else {
				// select an item
				if ( DC->realTime < lastListBoxClickTime && listPtr->doubleClick ) {
					Item_RunScript( item, listPtr->doubleClick );
				}
				lastListBoxClickTime = DC->realTime + DOUBLE_CLICK_DELAY;
				if ( item->cursorPos != listPtr->cursorPos ) {
					item->cursorPos = listPtr->cursorPos;
					(this->*(DC->feederSelection))( item->special, item->cursorPos );
				}
			}
			return true;
		}
		if ( key == K_HOME || key == K_KP_HOME ) {
			// home
			listPtr->startPos = 0;
			return true;
		}
		if ( key == K_END || key == K_KP_END ) {
			// end
			listPtr->startPos = max;
			return true;
		}
		if ( key == K_PGUP || key == K_KP_PGUP ) {
			// page up
			if ( !listPtr->notselectable ) {
				listPtr->cursorPos -= viewmax;
				if ( listPtr->cursorPos < 0 ) {
					listPtr->cursorPos = 0;
				}
				if ( listPtr->cursorPos < listPtr->startPos ) {
					listPtr->startPos = listPtr->cursorPos;
				}
				if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}
				item->cursorPos = listPtr->cursorPos;
				(this->*(DC->feederSelection))( item->special, item->cursorPos );
			} else {
				listPtr->startPos -= viewmax;
				if ( listPtr->startPos < 0 ) {
					listPtr->startPos = 0;
				}
			}
			return true;
		}
		if ( key == K_PGDN || key == K_KP_PGDN ) {
			// page down
			if ( !listPtr->notselectable ) {
				listPtr->cursorPos += viewmax;
				if ( listPtr->cursorPos < listPtr->startPos ) {
					listPtr->startPos = listPtr->cursorPos;
				}
				if ( listPtr->cursorPos >= count ) {
					listPtr->cursorPos = count - 1;
				}
				if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}
				item->cursorPos = listPtr->cursorPos;
				(this->*(DC->feederSelection))( item->special, item->cursorPos );
			} else {
				listPtr->startPos += viewmax;
				if ( listPtr->startPos > max ) {
					listPtr->startPos = max;
				}
			}
			return true;
		}
	}
	return false;
}

bool displayContextDef_t::Item_YesNo_HandleKey( itemDef_t *item, int key ) {
	auto&& DC = this;
	if ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) && item->window.flags & WINDOW_HASFOCUS && item->cvar ) {
		if ( key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3 ) {
			// ATVI Wolfenstein Misc #462
			// added the flag to toggle via action script only
			if ( !( item->cvarFlags & CVAR_NOTOGGLE ) ) {
				DC->setCVar( item->cvar, va( "%i", !DC->getCVarValue( item->cvar ) ) );
			}
			return true;
		}
	}
	return false;
}

int displayContextDef_t::Item_Multi_CountSettings( itemDef_t *item ) {
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if ( multiPtr == NULL ) {
		return 0;
	}
	return multiPtr->count;
}

int displayContextDef_t::Item_Multi_FindCvarByValue( itemDef_t *item ) {
	auto&& DC = this;
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if ( multiPtr ) {
		if ( multiPtr->strDef ) {
			DC->getCVarString( item->cvar, buff, sizeof( buff ) );
		} else {
			value = DC->getCVarValue( item->cvar );
		}
		for ( i = 0; i < multiPtr->count; i++ ) {
			if ( multiPtr->strDef ) {
				if ( Q_stricmp( buff, multiPtr->cvarStr[i] ) == 0 ) {
					return i;
				}
			} else {
				if ( multiPtr->cvarValue[i] == value ) {
					return i;
				}
			}
		}
	}
	return 0;
}

const char * displayContextDef_t::Item_Multi_Setting( itemDef_t *item ) {
	auto&& DC = this;
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if ( multiPtr ) {
		if ( multiPtr->strDef ) {
			DC->getCVarString( item->cvar, buff, sizeof( buff ) );
		} else {
			value = DC->getCVarValue( item->cvar );
		}
		for ( i = 0; i < multiPtr->count; i++ ) {
			if ( multiPtr->strDef ) {
				if ( Q_stricmp( buff, multiPtr->cvarStr[i] ) == 0 ) {
					return multiPtr->cvarList[i];
				}
			} else {
				if ( multiPtr->cvarValue[i] == value ) {
					return multiPtr->cvarList[i];
				}
			}
		}
	}
	return "";
}

bool displayContextDef_t::Item_Multi_HandleKey( itemDef_t *item, int key ) {
	auto&& DC = this;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if ( multiPtr ) {
		if ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) && item->window.flags & WINDOW_HASFOCUS && item->cvar ) {
			if ( key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3 ) {
				int current = Item_Multi_FindCvarByValue( item ) + 1;
				int max = Item_Multi_CountSettings( item );
				if ( current < 0 || current >= max ) {
					current = 0;
				}
				if ( multiPtr->strDef ) {
					DC->setCVar( item->cvar, multiPtr->cvarStr[current] );
				} else {
					float value = multiPtr->cvarValue[current];
					if ( ( (float)( (int) value ) ) == value ) {
						DC->setCVar( item->cvar, va( "%i", (int) value ) );
					} else {
						DC->setCVar( item->cvar, va( "%f", value ) );
					}
				}
				return true;
			}
		}
	}
	return false;
}

bool displayContextDef_t::Item_TextField_HandleKey( itemDef_t *item, int key ) {
	auto&& DC = this;
	char buff[1024];
	int len;
	itemDef_t *newItem = NULL;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;

	if ( item->cvar ) {

		memset( buff, 0, sizeof( buff ) );
		DC->getCVarString( item->cvar, buff, sizeof( buff ) );
		len = strlen( buff );
		if ( editPtr->maxChars && len > editPtr->maxChars ) {
			len = editPtr->maxChars;
		}
		if ( key & K_CHAR_FLAG ) {
			key &= ~K_CHAR_FLAG;


			if ( key == 'h' - 'a' + 1 ) {      // ctrl-h is backspace
				if ( item->cursorPos > 0 ) {
					memmove( &buff[item->cursorPos - 1], &buff[item->cursorPos], len + 1 - item->cursorPos );
					item->cursorPos--;
					if ( item->cursorPos < editPtr->paintOffset ) {
						editPtr->paintOffset--;
					}
				}
				DC->setCVar( item->cvar, buff );
				return true;
			}


			//
			// ignore any non printable chars
			//
			if ( key < 32 || !item->cvar ) {
				return true;
			}

			if ( item->type == ITEM_TYPE_NUMERICFIELD ) {
				if ( key < '0' || key > '9' ) {
					return false;
				}
			}

			if ( !DC->getOverstrikeMode() ) {
				if ( ( len == MAX_EDITFIELD - 1 ) || ( editPtr->maxChars && len >= editPtr->maxChars ) ) {
					return true;
				}
				memmove( &buff[item->cursorPos + 1], &buff[item->cursorPos], len + 1 - item->cursorPos );
			} else {
				if ( editPtr->maxChars && item->cursorPos >= editPtr->maxChars ) {
					return true;
				}
			}

			buff[item->cursorPos] = key;

			DC->setCVar( item->cvar, buff );

			if ( item->cursorPos < len + 1 ) {
				item->cursorPos++;
				if ( editPtr->maxPaintChars && item->cursorPos > editPtr->maxPaintChars ) {
					editPtr->paintOffset++;
				}
			}

		} else {

			if ( key == K_DEL || key == K_KP_DEL ) {
				if ( item->cursorPos < len ) {
					memmove( buff + item->cursorPos, buff + item->cursorPos + 1, len - item->cursorPos );
					DC->setCVar( item->cvar, buff );
				}
				return true;
			}

			if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) {
				if ( editPtr->maxPaintChars && item->cursorPos >= editPtr->paintOffset + editPtr->maxPaintChars && item->cursorPos < len ) {
					item->cursorPos++;
					editPtr->paintOffset++;
					return true;
				}
				if ( item->cursorPos < len ) {
					item->cursorPos++;
				}
				return true;
			}

			if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) {
				if ( item->cursorPos > 0 ) {
					item->cursorPos--;
				}
				if ( item->cursorPos < editPtr->paintOffset ) {
					editPtr->paintOffset--;
				}
				return true;
			}

			if ( key == K_HOME || key == K_KP_HOME ) { // || ( tolower(key) == 'a' && trap_Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = 0;
				editPtr->paintOffset = 0;
				return true;
			}

			if ( key == K_END || key == K_KP_END ) { // ( tolower(key) == 'e' && trap_Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = len;
				if ( item->cursorPos > editPtr->maxPaintChars ) {
					editPtr->paintOffset = len - editPtr->maxPaintChars;
				}
				return true;
			}

			if ( key == K_INS || key == K_KP_INS ) {
				DC->setOverstrikeMode( !DC->getOverstrikeMode() );
				return true;
			}
		}

		if ( key == K_TAB || key == K_DOWNARROW || key == K_KP_DOWNARROW ) {
			newItem = Menu_SetNextCursorItem( (menuDef_t*)item->parent );
			if ( newItem && ( newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD ) ) {
				g_editItem = newItem;
			}
		}

		if ( key == K_UPARROW || key == K_KP_UPARROW ) {
			newItem = Menu_SetPrevCursorItem((menuDef_t*)item->parent );
			if ( newItem && ( newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD ) ) {
				g_editItem = newItem;
			}
		}

		// NERVE - SMF
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			if ( item->onAccept ) {
				Item_RunScript( item, item->onAccept );
			}
		}
		// -NERVE - SMF

		if ( key == K_ENTER || key == K_KP_ENTER || key == K_ESCAPE ) {
			return false;
		}

		return true;
	}
	return false;

}

void displayContextDef_t::Scroll_ListBox_AutoFunc( void *p ) {
	auto&& DC = this;
	scrollInfo_t *si = (scrollInfo_t*)p;
	if ( DC->realTime > si->nextScrollTime ) {
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_ListBox_HandleKey( si->item, si->scrollKey, true, false );
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

	if ( DC->realTime > si->nextAdjustTime ) {
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if ( si->adjustValue > SCROLL_TIME_FLOOR ) {
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

void displayContextDef_t::Scroll_ListBox_ThumbFunc( void *p ) {
	auto&& DC = this;
	scrollInfo_t *si = (scrollInfo_t*)p;
	rectDef_t r;
	int pos, max;

	listBoxDef_t *listPtr = (listBoxDef_t*)si->item->typeData;
	if ( si->item->window.flags & WINDOW_HORIZONTAL ) {
		if ( DC->cursorx == si->xStart ) {
			return;
		}
		r.x = si->item->window.rect.x + SCROLLBAR_SIZE + 1;
		r.y = si->item->window.rect.y + si->item->window.rect.h - SCROLLBAR_SIZE - 1;
		r.h = SCROLLBAR_SIZE;
		r.w = si->item->window.rect.w - ( SCROLLBAR_SIZE * 2 ) - 2;
		max = Item_ListBox_MaxScroll( si->item );
		//
		pos = ( DC->cursorx - r.x - SCROLLBAR_SIZE / 2 ) * max / ( r.w - SCROLLBAR_SIZE );
		if ( pos < 0 ) {
			pos = 0;
		} else if ( pos > max )     {
			pos = max;
		}
		listPtr->startPos = pos;
		si->xStart = DC->cursorx;
	} else if ( DC->cursory != si->yStart )     {

		r.x = si->item->window.rect.x + si->item->window.rect.w - SCROLLBAR_SIZE - 1;
		r.y = si->item->window.rect.y + SCROLLBAR_SIZE + 1;
		r.h = si->item->window.rect.h - ( SCROLLBAR_SIZE * 2 ) - 2;
		r.w = SCROLLBAR_SIZE;
		max = Item_ListBox_MaxScroll( si->item );
		//
		pos = ( DC->cursory - r.y - SCROLLBAR_SIZE / 2 ) * max / ( r.h - SCROLLBAR_SIZE );
		if ( pos < 0 ) {
			pos = 0;
		} else if ( pos > max )     {
			pos = max;
		}
		listPtr->startPos = pos;
		si->yStart = DC->cursory;
	}

	if ( DC->realTime > si->nextScrollTime ) {
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_ListBox_HandleKey( si->item, si->scrollKey, true, false );
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

	if ( DC->realTime > si->nextAdjustTime ) {
		si->nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if ( si->adjustValue > SCROLL_TIME_FLOOR ) {
			si->adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

void displayContextDef_t::Scroll_Slider_ThumbFunc( void *p ) {
	auto&& DC = this;
	float x, value, cursorx;
	scrollInfo_t *si = (scrollInfo_t*)p;
	editFieldDef_t *editDef = (editFieldDef_t*)si->item->typeData;

	if ( si->item->text ) {
		x = si->item->textRect.x + si->item->textRect.w + 8;
	} else {
		x = si->item->window.rect.x;
	}

	cursorx = DC->cursorx;

	if ( cursorx < x ) {
		cursorx = x;
	} else if ( cursorx > x + SLIDER_WIDTH ) {
		cursorx = x + SLIDER_WIDTH;
	}
	value = cursorx - x;
	value /= SLIDER_WIDTH;
	value *= ( editDef->maxVal - editDef->minVal );
	value += editDef->minVal;
	DC->setCVar( si->item->cvar, va( "%f", value ) );
}

void displayContextDef_t::Item_StartCapture( itemDef_t *item, int key ) {
	auto&& DC = this;
	int flags;
	switch ( item->type ) {
	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:

	case ITEM_TYPE_LISTBOX:
	{
		flags = Item_ListBox_OverLB( item, DC->cursorx, DC->cursory );
		if ( flags & ( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW ) ) {
			scrollInfo.nextScrollTime = DC->realTime + SCROLL_TIME_START;
			scrollInfo.nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
			scrollInfo.adjustValue = SCROLL_TIME_START;
			scrollInfo.scrollKey = key;
			scrollInfo.scrollDir = ( flags & WINDOW_LB_LEFTARROW ) ? true : false;
			scrollInfo.item = item;
			captureData = &scrollInfo;
			captureFunc = &displayContextDef_t::Scroll_ListBox_AutoFunc;
			itemCapture = item;
		} else if ( flags & WINDOW_LB_THUMB ) {
			scrollInfo.scrollKey = key;
			scrollInfo.item = item;
			scrollInfo.xStart = DC->cursorx;
			scrollInfo.yStart = DC->cursory;
			captureData = &scrollInfo;
			captureFunc = &displayContextDef_t::Scroll_ListBox_ThumbFunc;
			itemCapture = item;
		}
		break;
	}
	case ITEM_TYPE_SLIDER:
	{
		flags = Item_Slider_OverSlider( item, DC->cursorx, DC->cursory );
		if ( flags & WINDOW_LB_THUMB ) {
			scrollInfo.scrollKey = key;
			scrollInfo.item = item;
			scrollInfo.xStart = DC->cursorx;
			scrollInfo.yStart = DC->cursory;
			captureData = &scrollInfo;
			captureFunc = &displayContextDef_t::Scroll_Slider_ThumbFunc;
			itemCapture = item;
		}
		break;
	}
	}
}

void displayContextDef_t::Item_StopCapture( itemDef_t *item ) {

}

bool displayContextDef_t::Item_Slider_HandleKey( itemDef_t *item, int key, bool down ) {
	auto&& DC = this;
	float x, value, width, work;

	//DC->Print("slider handle key\n");
	if ( item->window.flags & WINDOW_HASFOCUS && item->cvar && Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) ) {
		if ( key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3 ) {
			editFieldDef_t *editDef = (editFieldDef_t*)item->typeData;
			if ( editDef ) {
				rectDef_t testRect;
				width = SLIDER_WIDTH;
				if ( item->text ) {
					x = item->textRect.x + item->textRect.w + 8;
				} else {
					x = item->window.rect.x;
				}

				testRect = item->window.rect;
				testRect.x = x;
				value = (float)SLIDER_THUMB_WIDTH / 2;
				testRect.x -= value;
				//DC->Print("slider x: %f\n", testRect.x);
				testRect.w = ( SLIDER_WIDTH + (float)SLIDER_THUMB_WIDTH / 2 );
				//DC->Print("slider w: %f\n", testRect.w);
				if ( Rect_ContainsPoint( &testRect, DC->cursorx, DC->cursory ) ) {
					work = DC->cursorx - x;
					value = work / width;
					value *= ( editDef->maxVal - editDef->minVal );
					// vm fuckage
					// value = (((float)(DC->cursorx - x)/ SLIDER_WIDTH) * (editDef->maxVal - editDef->minVal));
					value += editDef->minVal;
					DC->setCVar( item->cvar, va( "%f", value ) );
					return true;
				}
			}
		}
	}
	DC->Print( "slider handle key exit\n" );
	return false;
}


bool displayContextDef_t::Item_HandleKey( itemDef_t *item, int key, bool down ) {

	if ( itemCapture ) {
		Item_StopCapture( itemCapture );
		itemCapture = NULL;
		captureFunc = NULL;
		captureData = NULL;
	} else {
		// bk001206 - parentheses
		if ( down && ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 ) ) {
			Item_StartCapture( item, key );
		}
	}

	if ( !down ) {
		return false;
	}

	switch ( item->type ) {
	case ITEM_TYPE_BUTTON:
		return false;
		break;
	case ITEM_TYPE_RADIOBUTTON:
		return false;
		break;
	case ITEM_TYPE_CHECKBOX:
		return false;
		break;
	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:
		//return Item_TextField_HandleKey(item, key);
		return false;
		break;
	case ITEM_TYPE_COMBO:
		return false;
		break;
	case ITEM_TYPE_LISTBOX:
		return Item_ListBox_HandleKey( item, key, down, false );
		break;
	case ITEM_TYPE_YESNO:
		return Item_YesNo_HandleKey( item, key );
		break;
	case ITEM_TYPE_MULTI:
		return Item_Multi_HandleKey( item, key );
		break;
	case ITEM_TYPE_OWNERDRAW:
		return Item_OwnerDraw_HandleKey( item, key );
		break;
	case ITEM_TYPE_BIND:
		return Item_Bind_HandleKey( item, key, down );
		break;
	case ITEM_TYPE_SLIDER:
		return Item_Slider_HandleKey( item, key, down );
		break;
		//case ITEM_TYPE_IMAGE:
		//  Item_Image_Paint(item);
		//  break;
	default:
		return false;
		break;
	}

	//return false;
}

void displayContextDef_t::Item_Action( itemDef_t *item ) {
	if ( item ) {
		Item_RunScript( item, item->action );
	}
}

itemDef_t * displayContextDef_t::Menu_SetPrevCursorItem( menuDef_t *menu ) {
	auto&& DC = this;
	bool wrapped = false;
	int oldCursor = menu->cursorItem;

	if ( menu->cursorItem < 0 ) {
		menu->cursorItem = menu->itemCount - 1;
		wrapped = true;
	}

	while ( menu->cursorItem > -1 ) {

		menu->cursorItem--;
		if ( menu->cursorItem < 0 && !wrapped ) {
			wrapped = true;
			menu->cursorItem = menu->itemCount - 1;
		}
		// NERVE - SMF
		if ( menu->cursorItem < 0 ) {
			menu->cursorItem = oldCursor;
			return NULL;
		}
		// -NERVE - SMF

		if ( Item_SetFocus( menu->items[menu->cursorItem], DC->cursorx, DC->cursory ) ) {
			Menu_HandleMouseMove( menu, menu->items[menu->cursorItem]->window.rect.x + 1, menu->items[menu->cursorItem]->window.rect.y + 1 );
			return menu->items[menu->cursorItem];
		}
	}
	menu->cursorItem = oldCursor;
	return NULL;

}

itemDef_t * displayContextDef_t::Menu_SetNextCursorItem( menuDef_t *menu ) {
	auto&& DC = this;
	bool wrapped = false;
	int oldCursor = menu->cursorItem;


	if ( menu->cursorItem == -1 ) {
		menu->cursorItem = 0;
		wrapped = true;
	}

	while ( menu->cursorItem < menu->itemCount ) {

		menu->cursorItem++;
		if ( menu->cursorItem >= menu->itemCount ) {  // (SA) had a problem 'tabbing' in dialogs with only one possible button
			if ( !wrapped ) {
				wrapped = true;
				menu->cursorItem = 0;
			} else {
				return menu->items[oldCursor];
			}
		}

		if ( Item_SetFocus( menu->items[menu->cursorItem], DC->cursorx, DC->cursory ) ) {
			Menu_HandleMouseMove( menu, menu->items[menu->cursorItem]->window.rect.x + 1, menu->items[menu->cursorItem]->window.rect.y + 1 );
			return menu->items[menu->cursorItem];
		}
	}

	menu->cursorItem = oldCursor;
	return NULL;
}


void displayContextDef_t::Window_CloseCinematic( windowDef_t *window ) {
	auto&& DC = this;
	if ( window->style == WINDOW_STYLE_CINEMATIC && window->cinematic >= 0 ) {
		DC->stopCinematic( window->cinematic );
		window->cinematic = -1;
	}
}

void displayContextDef_t::Menu_CloseCinematics( menuDef_t *menu ) {
	auto&& DC = this;
	if ( menu ) {
		int i;
		Window_CloseCinematic( &menu->window );
		for ( i = 0; i < menu->itemCount; i++ ) {
			Window_CloseCinematic( &menu->items[i]->window );
			if ( menu->items[i]->type == ITEM_TYPE_OWNERDRAW ) {
				DC->stopCinematic( 0 - menu->items[i]->window.ownerDraw );
			}
		}
	}
}

void displayContextDef_t::Display_CloseCinematics() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		Menu_CloseCinematics( &Menus[i] );
	}
}

void  displayContextDef_t::Menus_Activate( menuDef_t *menu ) {
	auto&& DC = this;
	menu->window.flags |= ( WINDOW_HASFOCUS | WINDOW_VISIBLE );
	if ( menu->onOpen ) {
		itemDef_t item;
		item.parent = menu;
		Item_RunScript( &item, menu->onOpen );
	}

	if ( menu->soundName && *menu->soundName ) {
//		DC->stopBackgroundTrack();					// you don't want to do this since it will reset s_rawend
		DC->startBackgroundTrack( menu->soundName, menu->soundName );
	}

	Display_CloseCinematics();

}

int displayContextDef_t::Display_VisibleMenuCount() {
	int i, count;
	count = 0;
	for ( i = 0; i < menuCount; i++ ) {
		if ( Menus[i].window.flags & ( WINDOW_FORCED | WINDOW_VISIBLE ) ) {
			count++;
		}
	}
	return count;
}

void displayContextDef_t::Menus_HandleOOBClick( menuDef_t *menu, int key, bool down ) {
	auto&& DC = this;
	if ( menu ) {
		int i;
		// basically the behaviour we are looking for is if there are windows in the stack.. see if
		// the cursor is within any of them.. if not close them otherwise activate them and pass the
		// key on.. force a mouse move to activate focus and script stuff
		if ( down && menu->window.flags & WINDOW_OOB_CLICK ) {
			Menu_RunCloseScript( menu );
			menu->window.flags &= ~( WINDOW_HASFOCUS | WINDOW_VISIBLE );
		}

		for ( i = 0; i < menuCount; i++ ) {
			if ( Menu_OverActiveItem( &Menus[i], DC->cursorx, DC->cursory ) ) {
//				Menu_RunCloseScript(menu);			// NERVE - SMF - why do we close the calling menu instead of just removing the focus?
//				menu->window.flags &= ~(WINDOW_HASFOCUS | WINDOW_VISIBLE);
				menu->window.flags &= ~( WINDOW_HASFOCUS );
				Menus_Activate( &Menus[i] );
				Menu_HandleMouseMove( &Menus[i], DC->cursorx, DC->cursory );
				Menu_HandleKey( &Menus[i], key, down );
			}
		}

		if ( Display_VisibleMenuCount() == 0 ) {
			if ( DC->Pause ) {
				DC->Pause( false );
			}
		}
		Display_CloseCinematics();
	}
}

rectDef_t * displayContextDef_t::Item_CorrectedTextRect( itemDef_t *item ) {
	static rectDef_t rect;
	memset( &rect, 0, sizeof( rectDef_t ) );
	if ( item ) {
		rect = item->textRect;
		if ( rect.w ) {
			rect.y -= rect.h;
		}
	}
	return &rect;
}

void displayContextDef_t::Menu_HandleKey( menuDef_t *menu, int key, bool down ) {
	auto&& DC = this;
	int i;
	itemDef_t *item = NULL;
	bool inHandler = false;

	Menu_HandleMouseMove( menu, DC->cursorx, DC->cursory );     // NERVE - SMF - fix for focus not resetting on unhidden buttons

	if ( inHandler ) {
		return;
	}

	inHandler = true;
	if ( g_waitingForKey && down ) {
		Item_Bind_HandleKey( g_bindItem, key, down );
		inHandler = false;
		return;
	}

	if ( g_editingField && down ) {
		if ( !Item_TextField_HandleKey( g_editItem, key ) ) {
			g_editingField = false;
			g_editItem = NULL;
			inHandler = false;
			return;
		} else if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 ) {
			g_editingField = false;
			g_editItem = NULL;
			Display_MouseMove( NULL, DC->cursorx, DC->cursory );
		} else if ( key == K_TAB || key == K_UPARROW || key == K_DOWNARROW ) {
			return;
		}
	}

	if ( menu == NULL ) {
		inHandler = false;
		return;
	}

	// see if the mouse is within the window bounds and if so is this a mouse click
	if ( down && !( menu->window.flags & WINDOW_POPUP ) && !Rect_ContainsPoint( &menu->window.rect, DC->cursorx, DC->cursory ) ) {
		static bool inHandleKey = false;
		// bk001206 - parentheses
		if ( !inHandleKey && ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 ) ) {
			inHandleKey = true;
			Menus_HandleOOBClick( menu, key, down );
			inHandleKey = false;
			inHandler = false;
			return;
		}
	}

	// get the item with focus
	for ( i = 0; i < menu->itemCount; i++ ) {
		if ( menu->items[i]->window.flags & WINDOW_HASFOCUS ) {
			item = menu->items[i];
		}
	}

	if ( item != NULL ) {
		if ( Item_HandleKey( item, key, down ) ) {
			Item_Action( item );
			inHandler = false;
			return;
		}
	}

	if ( !down ) {
		inHandler = false;
		return;
	}

	// NERVE - SMF
	if ( key > 0 && key <= 255 && menu->onKey[key] ) {
		itemDef_t it;
		it.parent = menu;
		Item_RunScript( &it, menu->onKey[key] );
		return;
	}
	// - NERVE - SMF

	// default handling
	switch ( key ) {

	case K_F11:
		if ( DC->getCVarValue( "developer" ) ) {
			debugMode ^= 1;
		}
		break;

	case K_F12:
		if ( DC->getCVarValue( "developer" ) ) {
			DC->executeText( EXEC_APPEND, "screenshot\n" );
		}
		break;
	case K_KP_UPARROW:
	case K_UPARROW:
		Menu_SetPrevCursorItem( menu );
		break;

	case K_ESCAPE:
		if ( !g_waitingForKey && menu->onESC ) {
			itemDef_t it;
			it.parent = menu;
			Item_RunScript( &it, menu->onESC );
		}
		break;

	case K_TAB:
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		Menu_SetNextCursorItem( menu );
		break;

	case K_MOUSE1:
	case K_MOUSE2:
		if ( item ) {
			if ( item->type == ITEM_TYPE_TEXT ) {
				if ( Rect_ContainsPoint( Item_CorrectedTextRect( item ), DC->cursorx, DC->cursory ) ) {
					Item_Action( item );
				}
			} else if ( item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD ) {
				if ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) ) {
					editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;

					// NERVE - SMF - reset scroll offset so we can see what we're editing
					if ( editPtr ) {
						editPtr->paintOffset = 0;
					}

					item->cursorPos = 0;
					g_editingField = true;
					g_editItem = item;

					DC->setOverstrikeMode( true );
				}
			} else {
				if ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) ) {
					Item_Action( item );
				}
			}
		}
		break;

	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
	case K_AUX1:
	case K_AUX2:
	case K_AUX3:
	case K_AUX4:
	case K_AUX5:
	case K_AUX6:
	case K_AUX7:
	case K_AUX8:
	case K_AUX9:
	case K_AUX10:
	case K_AUX11:
	case K_AUX12:
	case K_AUX13:
	case K_AUX14:
	case K_AUX15:
	case K_AUX16:
		break;
	case K_KP_ENTER:
	case K_ENTER:
	case K_MOUSE3:
		if ( item ) {
			if ( item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD ) {
				item->cursorPos = 0;
				g_editingField = true;
				g_editItem = item;
				DC->setOverstrikeMode( true );
			} else {
				Item_Action( item );
			}
		}
		break;
	}
	inHandler = false;
}

void displayContextDef_t::ToWindowCoords( float *x, float *y, windowDef_t *window ) {
	if ( window->border != 0 ) {
		*x += window->borderSize;
		*y += window->borderSize;
	}
	*x += window->rect.x;
	*y += window->rect.y;
}

void displayContextDef_t::Rect_ToWindowCoords( rectDef_t *rect, windowDef_t *window ) {
	ToWindowCoords( &rect->x, &rect->y, window );
}

void displayContextDef_t::Item_SetTextExtents( itemDef_t *item, int *width, int *height, const char *text ) {
	auto&& DC = this;
	const char *textPtr = ( text ) ? text : item->text;

	if ( textPtr == NULL ) {
		return;
	}

	*width = item->textRect.w;
	*height = item->textRect.h;

	// keeps us from computing the widths and heights more than once
	if ( *width == 0 || ( item->type == ITEM_TYPE_OWNERDRAW && item->textalignment == ITEM_ALIGN_CENTER ) || item->textalignment == ITEM_ALIGN_CENTER2 ) {
		int originalWidth = DC->textWidth( item->text, item->textscale, 0 );

		if ( item->type == ITEM_TYPE_OWNERDRAW && ( item->textalignment == ITEM_ALIGN_CENTER || item->textalignment == ITEM_ALIGN_RIGHT ) ) {
			originalWidth += DC->ownerDrawWidth( item->window.ownerDraw, item->textscale );
		} else if ( item->type == ITEM_TYPE_EDITFIELD && item->textalignment == ITEM_ALIGN_CENTER && item->cvar ) {
			char buff[256];
			DC->getCVarString( item->cvar, buff, 256 );
			originalWidth += DC->textWidth( buff, item->textscale, 0 );
		} else if ( item->textalignment == ITEM_ALIGN_CENTER2 )   {
			// NERVE - SMF - default centering case
			originalWidth += DC->textWidth( text, item->textscale, 0 );
		}

		*width = DC->textWidth( textPtr, item->textscale, 0 );
		*height = DC->textHeight( textPtr, item->textscale, 0 );
		item->textRect.w = *width;
		item->textRect.h = *height;
		item->textRect.x = item->textalignx;
		item->textRect.y = item->textaligny;
		if ( item->textalignment == ITEM_ALIGN_RIGHT ) {
			item->textRect.x = item->textalignx - originalWidth;
		} else if ( item->textalignment == ITEM_ALIGN_CENTER || item->textalignment == ITEM_ALIGN_CENTER2 ) {
			// NERVE - SMF - default centering case
			item->textRect.x = item->textalignx - originalWidth / 2;
		}

		ToWindowCoords( &item->textRect.x, &item->textRect.y, &item->window );
	}
}

void displayContextDef_t::Item_TextColor( itemDef_t *item, vec4_t *newColor ) {
	auto&& DC = this;
	vec4_t lowLight;
	menuDef_t *parent = (menuDef_t*)item->parent;

	Fade( &item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, true, parent->fadeAmount );

	if ( item->window.flags & WINDOW_HASFOCUS ) {
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor( parent->focusColor,lowLight,*newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else if ( item->textStyle == ITEM_TEXTSTYLE_BLINK && !( ( DC->realTime / BLINK_DIVISOR ) & 1 ) ) {
		lowLight[0] = 0.8 * item->window.foreColor[0];
		lowLight[1] = 0.8 * item->window.foreColor[1];
		lowLight[2] = 0.8 * item->window.foreColor[2];
		lowLight[3] = 0.8 * item->window.foreColor[3];
		LerpColor( item->window.foreColor,lowLight,*newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( newColor, &item->window.foreColor, sizeof( vec4_t ) );
		// items can be enabled and disabled based on cvars
	}

	if ( item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest ) {
		if ( item->cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !Item_EnableShowViaCvar( item, CVAR_ENABLE ) ) {
			memcpy( newColor, &parent->disableColor, sizeof( vec4_t ) );
		}
	}
}

void displayContextDef_t::Item_Text_AutoWrapped_Paint( itemDef_t *item ) {
	auto&& DC = this;
	char text[1024];
	const char *p, *textPtr, *newLinePtr;
	char buff[1024];
	int width, height, len, textWidth, newLine, newLineWidth;
	float y;
	vec4_t color;

	textWidth = 0;
	newLinePtr = NULL;

	if ( item->text == NULL ) {
		if ( item->cvar == NULL ) {
			return;
		} else {
			DC->getCVarString( item->cvar, text, sizeof( text ) );
			textPtr = text;
		}
	} else {
		textPtr = item->text;
	}
	if ( *textPtr == '\0' ) {
		return;
	}
	Item_TextColor( item, &color );
	Item_SetTextExtents( item, &width, &height, textPtr );

	y = item->textaligny;
	len = 0;
	buff[0] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;
	while ( p ) {
		if ( *p == ' ' || *p == '\t' || *p == '\n' || *p == '\0' ) {
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
		}
		textWidth = DC->textWidth( buff, item->textscale, 0 );
		if ( ( newLine && textWidth > item->window.rect.w ) || *p == '\n' || *p == '\0' ) {
			if ( len ) {
				if ( item->textalignment == ITEM_ALIGN_LEFT ) {
					item->textRect.x = item->textalignx;
				} else if ( item->textalignment == ITEM_ALIGN_RIGHT ) {
					item->textRect.x = item->textalignx - newLineWidth;
				} else if ( item->textalignment == ITEM_ALIGN_CENTER ) {
					item->textRect.x = item->textalignx - newLineWidth / 2;
				}
				item->textRect.y = y;
				ToWindowCoords( &item->textRect.x, &item->textRect.y, &item->window );
				//
				buff[newLine] = '\0';
				DC->drawText( item->textRect.x, item->textRect.y, item->textscale, color, buff, 0, 0, item->textStyle );
			}
			if ( *p == '\0' ) {
				break;
			}
			//
			y += height + 5;
			p = newLinePtr;
			len = 0;
			newLine = 0;
			newLineWidth = 0;
			continue;
		}
		buff[len++] = *p++;

		if ( buff[len - 1] == 13 ) {
			buff[len - 1] = ' ';
		}

		buff[len] = '\0';
	}
}

void displayContextDef_t::Item_Text_Wrapped_Paint( itemDef_t *item ) {
	auto&& DC = this;
	char text[1024];
	const char *p, *start, *textPtr;
	char buff[1024];
	int width, height;
	float x, y;
	vec4_t color;

	// now paint the text and/or any optional images
	// default to left

	if ( item->text == NULL ) {
		if ( item->cvar == NULL ) {
			return;
		} else {
			DC->getCVarString( item->cvar, text, sizeof( text ) );
			textPtr = text;
		}
	} else {
		textPtr = item->text;
	}
	if ( *textPtr == '\0' ) {
		return;
	}

	Item_TextColor( item, &color );
	Item_SetTextExtents( item, &width, &height, textPtr );

	x = item->textRect.x;
	y = item->textRect.y;
	start = textPtr;
	p = strchr( textPtr, '\r' );
	while ( p && *p ) {
		strncpy( buff, start, p - start + 1 );
		buff[p - start] = '\0';
		DC->drawText( x, y, item->textscale, color, buff, 0, 0, item->textStyle );
		y += height + 5;
		start += p - start + 1;
		p = strchr( p + 1, '\r' );
	}
	DC->drawText( x, y, item->textscale, color, start, 0, 0, item->textStyle );
}

void displayContextDef_t::Item_Text_Paint( itemDef_t *item ) {
	auto&& DC = this;
	char text[1024];
	const char *textPtr;
	int height, width;
	vec4_t color;

	if ( item->window.flags & WINDOW_WRAPPED ) {
		Item_Text_Wrapped_Paint( item );
		return;
	}
	if ( item->window.flags & WINDOW_AUTOWRAPPED ) {
		Item_Text_AutoWrapped_Paint( item );
		return;
	}

	if ( item->text == NULL ) {
		if ( item->cvar == NULL ) {
			return;
		} else {
			DC->getCVarString( item->cvar, text, sizeof( text ) );
			if ( item->window.flags & CG_SHOW_TEXTASINT ) {
				COM_StripExtension( text, text );
			}
			textPtr = text;
		}
	} else {
		textPtr = item->text;
	}

	// this needs to go here as it sets extents for cvar types as well
	Item_SetTextExtents( item, &width, &height, textPtr );

	if ( *textPtr == '\0' ) {
		return;
	}


	Item_TextColor( item, &color );

	//FIXME: this is a fucking mess
/*
	adjust = 0;
	if (item->textStyle == ITEM_TEXTSTYLE_OUTLINED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
		adjust = 0.5;
	}

	if (item->textStyle == ITEM_TEXTSTYLE_SHADOWED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
		Fade(&item->window.flags, &DC->Assets.shadowColor[3], DC->Assets.fadeClamp, &item->window.nextTime, DC->Assets.fadeCycle, false);
		DC->drawText(item->textRect.x + DC->Assets.shadowX, item->textRect.y + DC->Assets.shadowY, item->textscale, DC->Assets.shadowColor, textPtr, adjust);
	}
*/


//	if (item->textStyle == ITEM_TEXTSTYLE_OUTLINED || item->textStyle == ITEM_TEXTSTYLE_OUTLINESHADOWED) {
//		Fade(&item->window.flags, &item->window.outlineColor[3], DC->Assets.fadeClamp, &item->window.nextTime, DC->Assets.fadeCycle, false);
//		/*
//		Text_Paint(item->textRect.x-1, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x+1, item->textRect.y-1, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x-1, item->textRect.y, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x+1, item->textRect.y, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x-1, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
//		Text_Paint(item->textRect.x+1, item->textRect.y+1, item->textscale, item->window.foreColor, textPtr, adjust);
//		*/
//		DC->drawText(item->textRect.x - 1, item->textRect.y + 1, item->textscale * 1.02, item->window.outlineColor, textPtr, adjust);
//	}

	DC->drawText( item->textRect.x, item->textRect.y, item->textscale, color, textPtr, 0, 0, item->textStyle );
}



void displayContextDef_t::Item_TextField_Paint( itemDef_t *item ) {
	auto&& DC = this;
	char buff[1024];
	vec4_t newColor, lowLight;
	int offset;
	int text_len = 0; // screen length of the editfield text that will be printed
	int field_offset; // character offset in the editfield string
	int screen_offset; // offset on screen for precise placement
	menuDef_t *parent = (menuDef_t*)item->parent;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;

	Item_Text_Paint( item );

	buff[0] = '\0';

	if ( item->cvar ) {
		DC->getCVarString( item->cvar, buff, sizeof( buff ) );
	}

	parent = (menuDef_t*)item->parent;

	if ( item->window.flags & WINDOW_HASFOCUS ) {
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor( parent->focusColor,lowLight,newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( &newColor, &item->window.foreColor, sizeof( vec4_t ) );
	}

	// NOTE: offset from the editfield prefix (like "Say: " in limbo menu)
	offset = ( item->text && *item->text ) ? 8 : 0;

	// TTimo
	// text length control
	// if the edit field goes beyond the available width, drop some characters at the beginning of the string and apply some offseting
	// FIXME: we could cache the text length and offseting, but given the low count of edit fields, I abstained for now
	// FIXME: this won't handle going back into the line of the editfield to the hidden area
	// start of text painting: item->textRect.x + item->textRect.w + offset
	// our window limit: item->window.rect.x + item->window.rect.w
	field_offset = -1;
	do
	{
		field_offset++;
		if ( buff + editPtr->paintOffset + field_offset == '\0' ) {
			break;                                                   // keep it safe
		}
		text_len = DC->textWidth( buff + editPtr->paintOffset + field_offset, item->textscale, 0 );
	} while ( text_len + item->textRect.x + item->textRect.w + offset > item->window.rect.x + item->window.rect.w );
	if ( field_offset ) {
		// we had to take out some chars to make it fit in, there is an additional screen offset to compute
		screen_offset = item->window.rect.x + item->window.rect.w - ( text_len + item->textRect.x + item->textRect.w + offset );
	} else {
		screen_offset = 0;
	}

	if ( item->window.flags & WINDOW_HASFOCUS && g_editingField ) {
		char cursor = DC->getOverstrikeMode() ? '_' : '|';
		DC->drawTextWithCursor( item->textRect.x + item->textRect.w + offset + screen_offset, item->textRect.y, item->textscale, newColor, buff + editPtr->paintOffset + field_offset, item->cursorPos - editPtr->paintOffset - field_offset, cursor, editPtr->maxPaintChars, item->textStyle );
	} else {
		DC->drawText( item->textRect.x + item->textRect.w + offset + screen_offset, item->textRect.y, item->textscale, newColor, buff + editPtr->paintOffset + field_offset, 0, editPtr->maxPaintChars, item->textStyle );
	}

}

void displayContextDef_t::Item_YesNo_Paint( itemDef_t *item ) {
	auto&& DC = this;
	vec4_t newColor, lowLight;
	float value;
	menuDef_t *parent = (menuDef_t*)item->parent;

	value = ( item->cvar ) ? DC->getCVarValue( item->cvar ) : 0;

	if ( item->window.flags & WINDOW_HASFOCUS ) {
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor( parent->focusColor,lowLight,newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( &newColor, &item->window.foreColor, sizeof( vec4_t ) );
	}

	if ( item->text ) {
		Item_Text_Paint( item );
		DC->drawText( item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor,
					  ( value != 0 ) ? DC->translateString( "Yes" ) : DC->translateString( "No" ), 0, 0, item->textStyle );
	} else {
		DC->drawText( item->textRect.x, item->textRect.y, item->textscale, newColor, ( value != 0 ) ? "Yes" : "No", 0, 0, item->textStyle );
	}
}

void displayContextDef_t::Item_Multi_Paint( itemDef_t *item ) {
	auto&& DC = this;
	vec4_t newColor, lowLight;
	const char *text = "";
	menuDef_t *parent = (menuDef_t*)item->parent;

	if ( item->window.flags & WINDOW_HASFOCUS ) {
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor( parent->focusColor,lowLight,newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( &newColor, &item->window.foreColor, sizeof( vec4_t ) );
	}

	text = Item_Multi_Setting( item );

	if ( item->text ) {
		Item_Text_Paint( item );
		DC->drawText( item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, text, 0, 0, item->textStyle );
	} else {
		DC->drawText( item->textRect.x, item->textRect.y, item->textscale, newColor, text, 0, 0, item->textStyle );
	}
}


typedef struct {
	char    *command;
	int id;
	int defaultbind1;
	int defaultbind2;
	int bind1;
	int bind2;
} bind_t;

typedef struct
{
	char*   name;
	float defaultvalue;
	float value;
} configcvar_t;


static bind_t g_bindings[] =
{
	{"+scores",      -1,             -1, -1, -1},
	{"+speed",           K_SHIFT,        -1, -1, -1},
	{"+forward",     K_UPARROW,      -1, -1, -1},
	{"+back",            K_DOWNARROW,    -1, -1, -1},
	{"+moveleft",        ',',         -1, -1, -1},
	{"+moveright",       '.',         -1, -1, -1},
	{"+moveup",      K_SPACE,        -1, -1, -1},
	{"+movedown",        'c',         -1, -1, -1},
	{"+left",            K_LEFTARROW,    -1, -1, -1},
	{"+right",           K_RIGHTARROW,   -1, -1, -1},
	{"+strafe",      K_ALT,          -1, -1, -1},
	{"+lookup",      K_PGDN,         -1, -1, -1},
	{"+lookdown",        K_DEL,          -1, -1, -1},
	{"+mlook",           '/',         -1, -1, -1},
	{"centerview",       K_END,          -1, -1, -1},
	{"+zoom",            'z',             -1, -1, -1},
	{"weaponbank 1", '1',         -1, -1, -1},
	{"weaponbank 2", '2',         -1, -1, -1},
	{"weaponbank 3", '3',         -1, -1, -1},
	{"weaponbank 4", '4',         -1, -1, -1},
	{"weaponbank 5", '5',         -1, -1, -1},
	{"weaponbank 6", '6',         -1, -1, -1},
	{"weaponbank 7", '7',         -1, -1, -1},
	{"weaponbank 8", '8',         -1, -1, -1},
	{"weaponbank 9", '9',         -1, -1, -1},
	{"weaponbank 10",    '0',         -1, -1, -1},
	{"+attack",      K_CTRL,         -1, -1, -1},
	{"weapprev",     K_MWHEELDOWN,   -1, -1, -1},
	{"weapnext",     K_MWHEELUP,     -1, -1, -1},
	{"weapalt",          -1,             -1, -1, -1},
	{"weaplastused", -1,             -1, -1, -1}, //----(SA)	added
	{"weapnextinbank",   -1,             -1, -1, -1}, //----(SA)	added
	{"weapprevinbank",   -1,             -1, -1, -1}, //----(SA)	added
	{"+useitem",     K_ENTER,        -1, -1, -1},
	{"itemprev",     '[',         -1, -1, -1},
	{"itemnext",     ']',         -1, -1, -1},
	{"+button3",     K_MOUSE3,       -1, -1, -1},

/*
	{"prevTeamMember",	-1,				-1, -1, -1},
	{"nextTeamMember",	-1,				-1, -1, -1},
	{"nextOrder",		-1,				-1, -1, -1},
	{"confirmOrder",	-1,				-1, -1, -1},
	{"denyOrder",		-1,				-1, -1, -1},
	{"taskOffense",     -1,				-1, -1, -1},
	{"taskDefense",     -1,				-1, -1, -1},
	{"taskPatrol",		-1,				-1, -1, -1},
	{"taskCamp",		-1,				-1, -1, -1},
	{"taskFollow",		-1,				-1, -1, -1},
	{"taskRetrieve",	-1,				-1, -1, -1},
	{"taskEscort",		-1,				-1, -1, -1},
	{"taskOwnFlag",     -1,				-1, -1, -1},
	{"taskSuicide",     -1,				-1, -1, -1},
	{"tauntKillInsult", -1,				-1, -1, -1},
	{"tauntPraise",     -1,				-1, -1, -1},
	{"tauntTaunt",		-1,				-1, -1, -1},
	{"tauntDeathInsult",-1,				-1, -1, -1},
	{"tauntGauntlet",	-1,				-1, -1, -1},
*/
	{"scoresUp",     -1,             -1, -1, -1},
	{"scoresDown",       -1,             -1, -1, -1},
	{"messagemode",  -1,             -1, -1, -1},
	{"messagemode2", -1,             -1, -1, -1},
	{"messagemode3", -1,             -1, -1, -1},
	{"messagemode4", -1,             -1, -1, -1},

	{"+activate",        -1,             -1, -1, -1},
	{"zoomin",           -1,             -1, -1, -1},
	{"zoomout",          -1,             -1, -1, -1},
	{"+kick",            -1,             -1, -1, -1},
	{"+reload",      -1,             -1, -1, -1},
	{"+sprint",      -1,             -1, -1, -1},
	{"notebook",     K_TAB,          -1, -1, -1},
	{"help",         K_F1,           -1, -1, -1},
	{"+leanleft",        -1,             -1, -1, -1},
	{"+leanright",       -1,             -1, -1, -1},

	// DHM - Nerve
	{"vote yes",     -1,             -1, -1, -1},
	{"vote no",          -1,             -1, -1, -1},
	// dhm
	// NERVE - SMF
	{"OpenLimboMenu",    -1,             -1, -1, -1},
	{"mp_QuickMessage",  -1,             -1, -1, -1},
	{"+dropweapon",  -1,             -1, -1, -1},
	// -NERVE - SMF

	{"weapon 1",     -1,             -1, -1, -1},
	{"weapon 2",     -1,             -1, -1, -1},
	{"weapon 3",     -1,             -1, -1, -1},
	{"weapon 4",     -1,             -1, -1, -1},
	{"weapon 5",     -1,             -1, -1, -1},
	{"weapon 6",     -1,             -1, -1, -1},
	{"weapon 7",     -1,             -1, -1, -1},
	{"weapon 8",     -1,             -1, -1, -1},
	{"weapon 9",     -1,             -1, -1, -1},
	{"weapon 10",        -1,             -1, -1, -1},
	{"weapon 11",        -1,             -1, -1, -1},
	{"weapon 12",        -1,             -1, -1, -1},
	{"weapon 13",        -1,             -1, -1, -1},
	{"weapon 14",        -1,             -1, -1, -1},
	{"weapon 15",        -1,             -1, -1, -1},
	{"weapon 16",        -1,             -1, -1, -1},
	{"weapon 17",        -1,             -1, -1, -1},
	{"weapon 18",        -1,             -1, -1, -1},
	{"weapon 19",        -1,             -1, -1, -1},
	{"weapon 20",        -1,             -1, -1, -1},
	{"weapon 21",        -1,             -1, -1, -1},
	{"weapon 22",        -1,             -1, -1, -1},
	{"weapon 23",        -1,             -1, -1, -1},
	{"weapon 24",        -1,             -1, -1, -1},
	{"weapon 25",        -1,             -1, -1, -1},
	{"weapon 26",        -1,             -1, -1, -1},
	{"weapon 27",        -1,             -1, -1, -1},
	{"weapon 28",        -1,             -1, -1, -1},
	{"weapon 29",        -1,             -1, -1, -1},
	{"weapon 30",        -1,             -1, -1, -1},
	{"weapon 31",        -1,             -1, -1, -1},
	{"weapon 32",        -1,             -1, -1, -1}
};


static const int g_bindCount = sizeof( g_bindings ) / sizeof( bind_t );

/*
// TTimo unused
static configcvar_t g_configcvars[] =
{
	{"cl_run",			0,					0},
	{"m_pitch",			0,					0},
	{"cg_autoswitch",	0,					0},
	{"sensitivity",		0,					0},
	{"in_joystick",		0,					0},
	{"joy_threshold",	0,					0},
	{"m_filter",		0,					0},
	{"cl_freelook",		0,					0},
	{"cg_quickMessageAlt",	0,				0},			// NERVE - SMF
	{NULL,				0,					0}
};
*/

/*
=================
Controls_GetKeyAssignment
=================
*/
void displayContextDef_t::Controls_GetKeyAssignment( char *command, int *twokeys ) {
	auto&& DC = this;
	int count;
	int j;
	char b[256];

	twokeys[0] = twokeys[1] = -1;
	count = 0;

	for ( j = 0; j < 256; j++ )
	{
		DC->getBindingBuf( j, b, 256 );
		if ( *b == 0 ) {
			continue;
		}
		if ( !Q_stricmp( b, command ) ) {
			twokeys[count] = j;
			count++;
			if ( count == 2 ) {
				break;
			}
		}
	}
}

/*
=================
Controls_GetConfig
=================
*/
void displayContextDef_t::Controls_GetConfig( void ) {
	int i;
	int twokeys[2];

	// iterate each command, get its numeric binding
	for ( i = 0; i < g_bindCount; i++ )
	{

		Controls_GetKeyAssignment( g_bindings[i].command, twokeys );

		g_bindings[i].bind1 = twokeys[0];
		g_bindings[i].bind2 = twokeys[1];
	}

	//s_controls.invertmouse.curvalue  = DC->getCVarValue( "m_pitch" ) < 0;
	//s_controls.smoothmouse.curvalue  = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "m_filter" ) );
	//s_controls.alwaysrun.curvalue    = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cl_run" ) );
	//s_controls.autoswitch.curvalue   = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cg_autoswitch" ) );
	//s_controls.sensitivity.curvalue  = UI_ClampCvar( 2, 30, Controls_GetCvarValue( "sensitivity" ) );
	//s_controls.joyenable.curvalue    = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "in_joystick" ) );
	//s_controls.joythreshold.curvalue = UI_ClampCvar( 0.05, 0.75, Controls_GetCvarValue( "joy_threshold" ) );
	//s_controls.freelook.curvalue     = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "cl_freelook" ) );
}

/*
=================
Controls_SetConfig
=================
*/
void displayContextDef_t::Controls_SetConfig( bool restart ) {
	auto&& DC = this;
	int i;

	// iterate each command, get its numeric binding
	for ( i = 0; i < g_bindCount; i++ )
	{

		if ( g_bindings[i].bind1 != -1 ) {
			DC->setBinding( g_bindings[i].bind1, g_bindings[i].command );

			if ( g_bindings[i].bind2 != -1 ) {
				DC->setBinding( g_bindings[i].bind2, g_bindings[i].command );
			}
		}
	}

	//if ( s_controls.invertmouse.curvalue )
	//	DC->setCVar("m_pitch", va("%f),-fabs( DC->getCVarValue( "m_pitch" ) ) );
	//else
	//	trap_Cvar_SetValue( "m_pitch", fabs( trap_Cvar_VariableValue( "m_pitch" ) ) );

	//trap_Cvar_SetValue( "m_filter", s_controls.smoothmouse.curvalue );
	//trap_Cvar_SetValue( "cl_run", s_controls.alwaysrun.curvalue );
	//trap_Cvar_SetValue( "cg_autoswitch", s_controls.autoswitch.curvalue );
	//trap_Cvar_SetValue( "sensitivity", s_controls.sensitivity.curvalue );
	//trap_Cvar_SetValue( "in_joystick", s_controls.joyenable.curvalue );
	//trap_Cvar_SetValue( "joy_threshold", s_controls.joythreshold.curvalue );
	//trap_Cvar_SetValue( "cl_freelook", s_controls.freelook.curvalue );
#if !defined( __MACOS__ )
	DC->executeText( EXEC_APPEND, "in_restart\n" );
#endif
	//trap_Cmd_ExecuteText( EXEC_APPEND, "in_restart\n" );
}

/*
=================
Controls_SetDefaults
=================
*/
void displayContextDef_t::Controls_SetDefaults( void ) {
	int i;

	// iterate each command, set its default binding
	for ( i = 0; i < g_bindCount; i++ )
	{
		g_bindings[i].bind1 = g_bindings[i].defaultbind1;
		g_bindings[i].bind2 = g_bindings[i].defaultbind2;
	}

	//s_controls.invertmouse.curvalue  = Controls_GetCvarDefault( "m_pitch" ) < 0;
	//s_controls.smoothmouse.curvalue  = Controls_GetCvarDefault( "m_filter" );
	//s_controls.alwaysrun.curvalue    = Controls_GetCvarDefault( "cl_run" );
	//s_controls.autoswitch.curvalue   = Controls_GetCvarDefault( "cg_autoswitch" );
	//s_controls.sensitivity.curvalue  = Controls_GetCvarDefault( "sensitivity" );
	//s_controls.joyenable.curvalue    = Controls_GetCvarDefault( "in_joystick" );
	//s_controls.joythreshold.curvalue = Controls_GetCvarDefault( "joy_threshold" );
	//s_controls.freelook.curvalue     = Controls_GetCvarDefault( "cl_freelook" );
}

int displayContextDef_t::BindingIDFromName( const char *name ) {
	int i;
	for ( i = 0; i < g_bindCount; i++ )
	{
		if ( Q_stricmp( name, g_bindings[i].command ) == 0 ) {
			return i;
		}
	}
	return -1;
}

char g_nameBind1[32];
char g_nameBind2[32];

char* displayContextDef_t::BindingFromName( const char *cvar ) {
	auto&& DC = this;
	int i, b1, b2;

	// iterate each command, set its default binding
	for ( i = 0; i < g_bindCount; i++ )
	{
		if ( Q_stricmp( cvar, g_bindings[i].command ) == 0 ) {
			b1 = g_bindings[i].bind1;
			if ( b1 == -1 ) {
				break;
			}
			DC->keynumToStringBuf( b1, g_nameBind1, 32 );
			Q_strupr( g_nameBind1 );

			b2 = g_bindings[i].bind2;
			if ( b2 != -1 ) {
				DC->keynumToStringBuf( b2, g_nameBind2, 32 );
				Q_strupr( g_nameBind2 );
				strcat( g_nameBind1, DC->translateString( " or " ) );
				strcat( g_nameBind1, g_nameBind2 );
			}
			return g_nameBind1;         // NERVE - SMF
		}
	}
	strcpy( g_nameBind1, "???" );
	return g_nameBind1;         // NERVE - SMF
}

void displayContextDef_t::Item_Slider_Paint( itemDef_t *item ) {
	auto&& DC = this;
	vec4_t newColor, lowLight;
	float x, y, value;
	menuDef_t *parent = (menuDef_t*)item->parent;

	value = ( item->cvar ) ? DC->getCVarValue( item->cvar ) : 0;

	if ( item->window.flags & WINDOW_HASFOCUS ) {
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor( parent->focusColor,lowLight,newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( &newColor, &item->window.foreColor, sizeof( vec4_t ) );
	}

	y = item->window.rect.y;
	if ( item->text ) {
		Item_Text_Paint( item );
		x = item->textRect.x + item->textRect.w + 8;
	} else {
		x = item->window.rect.x;
	}
	DC->setColor( newColor );
	DC->drawHandlePic( x, y, SLIDER_WIDTH, SLIDER_HEIGHT, DC->Assets.sliderBar );

	x = Item_Slider_ThumbPosition( item );
	DC->drawHandlePic( x - ( SLIDER_THUMB_WIDTH / 2 ), y - 2, SLIDER_THUMB_WIDTH, SLIDER_THUMB_HEIGHT, DC->Assets.sliderThumb );
}

void displayContextDef_t::Item_Bind_Paint( itemDef_t *item ) {
	auto&& DC = this;
	vec4_t newColor, lowLight;
	float value;
	int maxChars = 0;
	menuDef_t *parent = (menuDef_t*)item->parent;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;
	if ( editPtr ) {
		maxChars = editPtr->maxPaintChars;
	}

	value = ( item->cvar ) ? DC->getCVarValue( item->cvar ) : 0;

	if ( item->window.flags & WINDOW_HASFOCUS ) {
		if ( g_bindItem == item ) {
			lowLight[0] = 0.8f * 1.0f;
			lowLight[1] = 0.8f * 0.0f;
			lowLight[2] = 0.8f * 0.0f;
			lowLight[3] = 0.8f * 1.0f;
		} else {
			lowLight[0] = 0.8f * parent->focusColor[0];
			lowLight[1] = 0.8f * parent->focusColor[1];
			lowLight[2] = 0.8f * parent->focusColor[2];
			lowLight[3] = 0.8f * parent->focusColor[3];
		}
		LerpColor( parent->focusColor,lowLight,newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( &newColor, &item->window.foreColor, sizeof( vec4_t ) );
	}

	if ( item->text ) {
		Item_Text_Paint( item );
		BindingFromName( item->cvar );
		DC->drawText( item->textRect.x + item->textRect.w + 8, item->textRect.y, item->textscale, newColor, g_nameBind1, 0, maxChars, item->textStyle );
	} else {
		DC->drawText( item->textRect.x, item->textRect.y, item->textscale, newColor, ( value != 0 ) ? "FIXME" : "FIXME", 0, maxChars, item->textStyle );
	}
}

bool displayContextDef_t::Display_KeyBindPending() {
	return g_waitingForKey;
}

bool displayContextDef_t::Item_Bind_HandleKey( itemDef_t *item, int key, bool down ) {
	auto&& DC = this;
	int id;
	int i;

	if ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) && !g_waitingForKey ) {
		if ( down && ( key == K_MOUSE1 || key == K_ENTER ) ) {
			g_waitingForKey = true;
			g_bindItem = item;
		}
		return true;
	} else
	{
		if ( !g_waitingForKey || g_bindItem == NULL ) {
			return true;
		}

		if ( key & K_CHAR_FLAG ) {
			return true;
		}

		switch ( key )
		{
		case K_ESCAPE:
			g_waitingForKey = false;
			return true;

		case K_BACKSPACE:
			id = BindingIDFromName( item->cvar );
			if ( id != -1 ) {
				g_bindings[id].bind1 = -1;
				g_bindings[id].bind2 = -1;
			}
			Controls_SetConfig( true );
			g_waitingForKey = false;
			g_bindItem = NULL;
			return true;

		case '`':
			return true;
		}
	}

	if ( key != -1 ) {

		for ( i = 0; i < g_bindCount; i++ )
		{

			if ( g_bindings[i].bind2 == key ) {
				g_bindings[i].bind2 = -1;
			}

			if ( g_bindings[i].bind1 == key ) {
				g_bindings[i].bind1 = g_bindings[i].bind2;
				g_bindings[i].bind2 = -1;
			}
		}
	}


	id = BindingIDFromName( item->cvar );

	if ( id != -1 ) {
		if ( key == -1 ) {
			if ( g_bindings[id].bind1 != -1 ) {
				DC->setBinding( g_bindings[id].bind1, "" );
				g_bindings[id].bind1 = -1;
			}
			if ( g_bindings[id].bind2 != -1 ) {
				DC->setBinding( g_bindings[id].bind2, "" );
				g_bindings[id].bind2 = -1;
			}
		} else if ( g_bindings[id].bind1 == -1 )     {
			g_bindings[id].bind1 = key;
		} else if ( g_bindings[id].bind1 != key && g_bindings[id].bind2 == -1 )     {
			g_bindings[id].bind2 = key;
		} else {
			DC->setBinding( g_bindings[id].bind1, "" );
			DC->setBinding( g_bindings[id].bind2, "" );
			g_bindings[id].bind1 = key;
			g_bindings[id].bind2 = -1;
		}
	}

	Controls_SetConfig( true );
	g_waitingForKey = false;

	return true;
}



void displayContextDef_t::AdjustFrom640( float *x, float *y, float *w, float *h ) {
	auto&& DC = this;
	//*x = *x * DC->scale + DC->bias;
	*x *= DC->xscale;
	*y *= DC->yscale;
	*w *= DC->xscale;
	*h *= DC->yscale;
}

void displayContextDef_t::Item_Model_Paint( itemDef_t *item ) {
	auto&& DC = this;
	float x, y, w, h;   //,xx;
	refdef_t refdef;
	qhandle_t hModel;
	refEntity_t ent;
	vec3_t mins, maxs, origin;
	vec3_t angles;
	modelDef_t *modelPtr = (modelDef_t*)item->typeData;
	int backLerpWhole;

	if ( modelPtr == NULL ) {
		return;
	}

	if ( !item->asset ) {
		return;
	}

	hModel = item->asset;

	// setup the refdef
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear( refdef.viewaxis );
	x = item->window.rect.x + 1;
	y = item->window.rect.y + 1;
	w = item->window.rect.w - 2;
	h = item->window.rect.h - 2;

	AdjustFrom640( &x, &y, &w, &h );

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	DC->modelBounds( hModel, mins, maxs );

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

	// calculate distance so the model nearly fills the box
	if ( true ) {
		float len = 0.5 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;    // len / tan( fov/2 )
		//origin[0] = len / tan(w/2);
	} else {
		origin[0] = item->textscale;
	}

#define NEWWAY
#ifdef NEWWAY
	refdef.fov_x = ( modelPtr->fov_x ) ? modelPtr->fov_x : w;
	refdef.fov_y = ( modelPtr->fov_y ) ? modelPtr->fov_y : h;
#else
	refdef.fov_x = (int)( (float)refdef.width / 640.0f * 90.0f );
	xx = refdef.width / tan( refdef.fov_x / 360 * M_PI );
	refdef.fov_y = atan2( refdef.height, xx );
	refdef.fov_y *= ( 360 / M_PI );
#endif
	DC->clearScene();

	refdef.time = DC->realTime;

	// add the model

	memset( &ent, 0, sizeof( ent ) );

	//adjust = 5.0 * sin( (float)uis.realtime / 500 );
	//adjust = 360 % (int)((float)uis.realtime / 1000);
	//VectorSet( angles, 0, 0, 1 );

	// use item storage to track
	if ( modelPtr->rotationSpeed ) {
		if ( DC->realTime > item->window.nextTime ) {
			item->window.nextTime = DC->realTime + modelPtr->rotationSpeed;
			modelPtr->angle = (int)( modelPtr->angle + 1 ) % 360;
		}
	}
	VectorSet( angles, 0, modelPtr->angle, 0 );
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;


	if ( modelPtr->frameTime ) { // don't advance on the first frame
		modelPtr->backlerp += ( ( ( DC->realTime - modelPtr->frameTime ) / 1000.0f ) * (float)modelPtr->fps );
	}

	if ( modelPtr->backlerp > 1 ) {
		backLerpWhole = floor( modelPtr->backlerp );

		modelPtr->frame += ( backLerpWhole );
		if ( ( modelPtr->frame - modelPtr->startframe ) > modelPtr->numframes ) {
			modelPtr->frame = modelPtr->startframe + modelPtr->frame % modelPtr->numframes; // todo: ignoring loopframes

		}
		modelPtr->oldframe += ( backLerpWhole );
		if ( ( modelPtr->oldframe - modelPtr->startframe ) > modelPtr->numframes ) {
			modelPtr->oldframe = modelPtr->startframe + modelPtr->oldframe % modelPtr->numframes;   // todo: ignoring loopframes

		}
		modelPtr->backlerp = modelPtr->backlerp - backLerpWhole;
	}

	modelPtr->frameTime = DC->realTime;

	ent.frame       = modelPtr->frame;
	ent.oldframe    = modelPtr->oldframe;
	ent.backlerp    = 1.0f - modelPtr->backlerp;

	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	DC->addRefEntityToScene( &ent );
	DC->renderScene( &refdef );

}


void displayContextDef_t::Item_Image_Paint( itemDef_t *item ) {
	auto&& DC = this;
	if ( item == NULL ) {
		return;
	}
	DC->drawHandlePic( item->window.rect.x + 1, item->window.rect.y + 1, item->window.rect.w - 2, item->window.rect.h - 2, item->asset );
}

void displayContextDef_t::Item_ListBox_Paint( itemDef_t *item ) {
	auto&& DC = this;
	float x, y, size, count, i, thumb;
	qhandle_t image;
	qhandle_t optionalImage;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

	// the listbox is horizontal or vertical and has a fixed size scroll bar going either direction
	// elements are enumerated from the DC and either text or image handles are acquired from the DC as well
	// textscale is used to size the text, textalignx and textaligny are used to size image elements
	// there is no clipping available so only the last completely visible item is painted
	count = DC->feederCount( item->special );
	// default is vertical if horizontal flag is not here
	if ( item->window.flags & WINDOW_HORIZONTAL ) {
		// draw scrollbar in bottom of the window
		// bar
		x = item->window.rect.x + 1;
		y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE - 1;
		DC->drawHandlePic( x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowLeft );
		x += SCROLLBAR_SIZE - 1;
		size = item->window.rect.w - ( SCROLLBAR_SIZE * 2 );
		DC->drawHandlePic( x, y, size + 1, SCROLLBAR_SIZE, DC->Assets.scrollBar );
		x += size - 1;
		DC->drawHandlePic( x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowRight );
		// thumb
		thumb = Item_ListBox_ThumbDrawPosition( item ); //Item_ListBox_ThumbPosition(item);
		if ( thumb > x - SCROLLBAR_SIZE - 1 ) {
			thumb = x - SCROLLBAR_SIZE - 1;
		}
		DC->drawHandlePic( thumb, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb );
		//
		listPtr->endPos = listPtr->startPos;
		size = item->window.rect.w - 2;
		// items
		// size contains max available space
		if ( listPtr->elementStyle == LISTBOX_IMAGE ) {
			// fit = 0;
			x = item->window.rect.x + 1;
			y = item->window.rect.y + 1;
			for ( i = listPtr->startPos; i < count; i++ ) {
				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage( item->special, i );
				if ( image ) {
					DC->drawHandlePic( x + 1, y + 1, listPtr->elementWidth - 2, listPtr->elementHeight - 2, image );
				}

				if ( i == item->cursorPos ) {
					DC->drawRect( x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, item->window.borderSize, item->window.borderColor );
				}

				size -= listPtr->elementWidth;
				if ( size < listPtr->elementWidth ) {
					listPtr->drawPadding = size; //listPtr->elementWidth - size;
					break;
				}
				x += listPtr->elementWidth;
				listPtr->endPos++;
				// fit++;
			}
		} else {
			//
		}
	} else {
		// draw scrollbar to right side of the window
		x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE - 1;
		y = item->window.rect.y + 1;
		DC->drawHandlePic( x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowUp );
		y += SCROLLBAR_SIZE - 1;

		listPtr->endPos = listPtr->startPos;
		size = item->window.rect.h - ( SCROLLBAR_SIZE * 2 );
		DC->drawHandlePic( x, y, SCROLLBAR_SIZE, size + 1, DC->Assets.scrollBar );
		y += size - 1;
		DC->drawHandlePic( x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowDown );
		// thumb
		thumb = Item_ListBox_ThumbDrawPosition( item ); //Item_ListBox_ThumbPosition(item);
		if ( thumb > y - SCROLLBAR_SIZE - 1 ) {
			thumb = y - SCROLLBAR_SIZE - 1;
		}
		DC->drawHandlePic( x, thumb, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb );

		// adjust size for item painting
		size = item->window.rect.h - 2;
		if ( listPtr->elementStyle == LISTBOX_IMAGE ) {
			// fit = 0;
			x = item->window.rect.x + 1;
			y = item->window.rect.y + 1;
			for ( i = listPtr->startPos; i < count; i++ ) {
				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage( item->special, i );
				if ( image ) {
					DC->drawHandlePic( x + 1, y + 1, listPtr->elementWidth - 2, listPtr->elementHeight - 2, image );
				}

				if ( i == item->cursorPos ) {
					DC->drawRect( x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, item->window.borderSize, item->window.borderColor );
				}

				listPtr->endPos++;
				size -= listPtr->elementWidth;
				if ( size < listPtr->elementHeight ) {
					listPtr->drawPadding = listPtr->elementHeight - size;
					break;
				}
				y += listPtr->elementHeight;
				// fit++;
			}
		} else {
			x = item->window.rect.x + 1;
			y = item->window.rect.y + 1;
			for ( i = listPtr->startPos; i < count; i++ ) {
				const char *text;
				// always draw at least one
				// which may overdraw the box if it is too small for the element

				if ( listPtr->numColumns > 0 ) {
					int j;
					for ( j = 0; j < listPtr->numColumns; j++ ) {
						text = DC->feederItemText( item->special, i, j, &optionalImage );
						if ( optionalImage >= 0 ) {
							DC->drawHandlePic( x + 4 + listPtr->columnInfo[j].pos, y - 1 + listPtr->elementHeight / 2, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage );
						} else if ( text ) {
							DC->drawText( x + 4 + listPtr->columnInfo[j].pos + item->textalignx,
										  y + listPtr->elementHeight + item->textaligny, item->textscale, item->window.foreColor, text, 0, listPtr->columnInfo[j].maxChars, item->textStyle );
						}
					}
				} else {
					text = DC->feederItemText( item->special, i, 0, &optionalImage );
					if ( optionalImage >= 0 ) {
						//DC->drawHandlePic(x + 4 + listPtr->elementHeight, y, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage);
					} else if ( text ) {
						DC->drawText( x + 4, y + listPtr->elementHeight, item->textscale, item->window.foreColor, text, 0, 0, item->textStyle );
					}
				}

				if ( i == item->cursorPos ) {
					DC->fillRect( x, y, item->window.rect.w - SCROLLBAR_SIZE - 4, listPtr->elementHeight - 1, item->window.outlineColor );
				}

				size -= listPtr->elementHeight;
				if ( size < listPtr->elementHeight ) {
					listPtr->drawPadding = listPtr->elementHeight - size;
					break;
				}
				listPtr->endPos++;
				y += listPtr->elementHeight;
				// fit++;
			}
		}
	}
}


void displayContextDef_t::Item_OwnerDraw_Paint( itemDef_t *item ) {
	auto&& DC = this;
	menuDef_t *parent;

	if ( item == NULL ) {
		return;
	}

	parent = (menuDef_t*)item->parent;

	if ( DC->ownerDrawItem ) {
		vec4_t color, lowLight;
		menuDef_t *parent = (menuDef_t*)item->parent;
		Fade( &item->window.flags, &item->window.foreColor[3], parent->fadeClamp, &item->window.nextTime, parent->fadeCycle, true, parent->fadeAmount );
		memcpy( &color, &item->window.foreColor, sizeof( color ) );
		if ( item->numColors > 0 && DC->getValue ) {
			// if the value is within one of the ranges then set color to that, otherwise leave at default
			int i;
			float f = DC->getValue( item->window.ownerDraw, item->colorRangeType );
			for ( i = 0; i < item->numColors; i++ ) {
				if ( f >= item->colorRanges[i].low && f <= item->colorRanges[i].high ) {
					memcpy( &color, &item->colorRanges[i].color, sizeof( color ) );
					break;
				}
			}
		}

		// take hudalpha into account unless explicitly ignoring
		if ( !( item->window.flags & WINDOW_IGNORE_HUDALPHA ) ) {
			color[3] *= DC->getCVarValue( "cg_hudAlpha" );;
		}


		if ( item->window.flags & WINDOW_HASFOCUS ) {
			lowLight[0] = 0.8 * parent->focusColor[0];
			lowLight[1] = 0.8 * parent->focusColor[1];
			lowLight[2] = 0.8 * parent->focusColor[2];
			lowLight[3] = 0.8 * parent->focusColor[3];
			LerpColor( parent->focusColor,lowLight,color,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
		} else if ( item->textStyle == ITEM_TEXTSTYLE_BLINK && !( ( DC->realTime / BLINK_DIVISOR ) & 1 ) ) {
			lowLight[0] = 0.8 * item->window.foreColor[0];
			lowLight[1] = 0.8 * item->window.foreColor[1];
			lowLight[2] = 0.8 * item->window.foreColor[2];
			lowLight[3] = 0.8 * item->window.foreColor[3];
			LerpColor( item->window.foreColor,lowLight,color,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
		}

		if ( item->cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !Item_EnableShowViaCvar( item, CVAR_ENABLE ) ) {
			memcpy( color, parent->disableColor, sizeof( vec4_t ) );
		}

		if ( item->text ) {
			Item_Text_Paint( item );
			if ( item->text[0] ) {
				// +8 is an offset kludge to properly align owner draw items that have text combined with them
				DC->ownerDrawItem( item->textRect.x + item->textRect.w + 8, item->window.rect.y, item->window.rect.w, item->window.rect.h, 0, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle );
			} else {
				DC->ownerDrawItem( item->textRect.x + item->textRect.w, item->window.rect.y, item->window.rect.w, item->window.rect.h, 0, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle );
			}
		} else {
			DC->ownerDrawItem( item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h, item->textalignx, item->textaligny, item->window.ownerDraw, item->window.ownerDrawFlags, item->alignment, item->special, item->textscale, color, item->window.background, item->textStyle );
		}
	}
}


void displayContextDef_t::Item_Paint( itemDef_t *item ) {
	auto&& DC = this;
	vec4_t red;
	menuDef_t *parent = (menuDef_t*)item->parent;
	red[0] = red[3] = 1;
	red[1] = red[2] = 0;

	if ( item == NULL ) {
		return;
	}

	// NERVE - SMF
	if ( DC->textFont ) {
		DC->textFont( item->font );
	}

	if ( item->window.flags & WINDOW_ORBITING ) {
		if ( DC->realTime > item->window.nextTime ) {
			float rx, ry, a, c, s, w, h;

			item->window.nextTime = DC->realTime + item->window.offsetTime;
			// translate
			w = item->window.rectClient.w / 2;
			h = item->window.rectClient.h / 2;
			rx = item->window.rectClient.x + w - item->window.rectEffects.x;
			ry = item->window.rectClient.y + h - item->window.rectEffects.y;
			a = 3 * M_PI / 180;
			c = cos( a );
			s = sin( a );
			item->window.rectClient.x = ( rx * c - ry * s ) + item->window.rectEffects.x - w;
			item->window.rectClient.y = ( rx * s + ry * c ) + item->window.rectEffects.y - h;
			Item_UpdatePosition( item );

		}
	}


	if ( item->window.flags & WINDOW_INTRANSITION ) {
		if ( DC->realTime > item->window.nextTime ) {
			int done = 0;
			item->window.nextTime = DC->realTime + item->window.offsetTime;
			// transition the x,y
			if ( item->window.rectClient.x == item->window.rectEffects.x ) {
				done++;
			} else {
				if ( item->window.rectClient.x < item->window.rectEffects.x ) {
					item->window.rectClient.x += item->window.rectEffects2.x;
					if ( item->window.rectClient.x > item->window.rectEffects.x ) {
						item->window.rectClient.x = item->window.rectEffects.x;
						done++;
					}
				} else {
					item->window.rectClient.x -= item->window.rectEffects2.x;
					if ( item->window.rectClient.x < item->window.rectEffects.x ) {
						item->window.rectClient.x = item->window.rectEffects.x;
						done++;
					}
				}
			}
			if ( item->window.rectClient.y == item->window.rectEffects.y ) {
				done++;
			} else {
				if ( item->window.rectClient.y < item->window.rectEffects.y ) {
					item->window.rectClient.y += item->window.rectEffects2.y;
					if ( item->window.rectClient.y > item->window.rectEffects.y ) {
						item->window.rectClient.y = item->window.rectEffects.y;
						done++;
					}
				} else {
					item->window.rectClient.y -= item->window.rectEffects2.y;
					if ( item->window.rectClient.y < item->window.rectEffects.y ) {
						item->window.rectClient.y = item->window.rectEffects.y;
						done++;
					}
				}
			}
			if ( item->window.rectClient.w == item->window.rectEffects.w ) {
				done++;
			} else {
				if ( item->window.rectClient.w < item->window.rectEffects.w ) {
					item->window.rectClient.w += item->window.rectEffects2.w;
					if ( item->window.rectClient.w > item->window.rectEffects.w ) {
						item->window.rectClient.w = item->window.rectEffects.w;
						done++;
					}
				} else {
					item->window.rectClient.w -= item->window.rectEffects2.w;
					if ( item->window.rectClient.w < item->window.rectEffects.w ) {
						item->window.rectClient.w = item->window.rectEffects.w;
						done++;
					}
				}
			}
			if ( item->window.rectClient.h == item->window.rectEffects.h ) {
				done++;
			} else {
				if ( item->window.rectClient.h < item->window.rectEffects.h ) {
					item->window.rectClient.h += item->window.rectEffects2.h;
					if ( item->window.rectClient.h > item->window.rectEffects.h ) {
						item->window.rectClient.h = item->window.rectEffects.h;
						done++;
					}
				} else {
					item->window.rectClient.h -= item->window.rectEffects2.h;
					if ( item->window.rectClient.h < item->window.rectEffects.h ) {
						item->window.rectClient.h = item->window.rectEffects.h;
						done++;
					}
				}
			}

			Item_UpdatePosition( item );

			if ( done == 4 ) {
				item->window.flags &= ~WINDOW_INTRANSITION;
			}

		}
	}

	if ( item->window.ownerDrawFlags && DC->ownerDrawVisible ) {
		if ( !DC->ownerDrawVisible( item->window.ownerDrawFlags ) ) {
			item->window.flags &= ~WINDOW_VISIBLE;
		} else {
			item->window.flags |= WINDOW_VISIBLE;
		}
	}

	if ( item->cvarFlags & ( CVAR_SHOW | CVAR_HIDE ) ) {
		if ( !Item_EnableShowViaCvar( item, CVAR_SHOW ) ) {
			return;
		}
	}

	if ( item->window.flags & WINDOW_TIMEDVISIBLE ) {
	}

	if ( !( item->window.flags & WINDOW_VISIBLE ) ) {
		return;
	}

	// paint the rect first..
	Window_Paint( &item->window, parent->fadeAmount, parent->fadeClamp, parent->fadeCycle );

	if ( debugMode ) {
		vec4_t color;
		rectDef_t *r = Item_CorrectedTextRect( item );
		color[1] = color[3] = 1;
		color[0] = color[2] = 0;
		DC->drawRect( r->x, r->y, r->w, r->h, 1, color );
	}

	//DC->drawRect(item->window.rect.x, item->window.rect.y, item->window.rect.w, item->window.rect.h, 1, red);

	switch ( item->type ) {
	case ITEM_TYPE_OWNERDRAW:
		Item_OwnerDraw_Paint( item );
		break;
	case ITEM_TYPE_TEXT:
	case ITEM_TYPE_BUTTON:
		Item_Text_Paint( item );
		break;
	case ITEM_TYPE_RADIOBUTTON:
		break;
	case ITEM_TYPE_CHECKBOX:
		break;
	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:
		Item_TextField_Paint( item );
		break;
	case ITEM_TYPE_COMBO:
		break;
	case ITEM_TYPE_LISTBOX:
		Item_ListBox_Paint( item );
		break;
//		case ITEM_TYPE_IMAGE:
//			Item_Image_Paint(item);
//			break;
	case ITEM_TYPE_MENUMODEL:
		Item_Model_Paint( item );
		break;
	case ITEM_TYPE_MODEL:
		Item_Model_Paint( item );
		break;
	case ITEM_TYPE_YESNO:
		Item_YesNo_Paint( item );
		break;
	case ITEM_TYPE_MULTI:
		Item_Multi_Paint( item );
		break;
	case ITEM_TYPE_BIND:
		Item_Bind_Paint( item );
		break;
	case ITEM_TYPE_SLIDER:
		Item_Slider_Paint( item );
		break;
	default:
		break;
	}
}

void displayContextDef_t::Menu_Init( menuDef_t *menu ) {
	auto&& DC = this;
	memset( menu, 0, sizeof( menuDef_t ) );
	menu->cursorItem = -1;
	menu->fadeAmount = DC->Assets.fadeAmount;
	menu->fadeClamp = DC->Assets.fadeClamp;
	menu->fadeCycle = DC->Assets.fadeCycle;
	Window_Init( &menu->window );
}

itemDef_t * displayContextDef_t::Menu_GetFocusedItem( menuDef_t *menu ) {
	int i;
	if ( menu ) {
		for ( i = 0; i < menu->itemCount; i++ ) {
			if ( menu->items[i]->window.flags & WINDOW_HASFOCUS ) {
				return menu->items[i];
			}
		}
	}
	return NULL;
}

menuDef_t * displayContextDef_t::Menu_GetFocused() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		if ( Menus[i].window.flags & WINDOW_HASFOCUS && Menus[i].window.flags & WINDOW_VISIBLE ) {
			return &Menus[i];
		}
	}
	return NULL;
}

void displayContextDef_t::Menu_ScrollFeeder( menuDef_t *menu, int feeder, bool down ) {
	if ( menu ) {
		int i;
		for ( i = 0; i < menu->itemCount; i++ ) {
			if ( menu->items[i]->special == feeder ) {
				Item_ListBox_HandleKey( menu->items[i], ( down ) ? K_DOWNARROW : K_UPARROW, true, true );
				return;
			}
		}
	}
}



void displayContextDef_t::Menu_SetFeederSelection( menuDef_t *menu, int feeder, int index, const char *name ) {
	auto&& DC = this;
	if ( menu == NULL ) {
		if ( name == NULL ) {
			menu = Menu_GetFocused();
		} else {
			menu = Menus_FindByName( name );
		}
	}

	if ( menu ) {
		int i;
		for ( i = 0; i < menu->itemCount; i++ ) {
			if ( menu->items[i]->special == feeder ) {
				if ( index == 0 ) {
					listBoxDef_t *listPtr = (listBoxDef_t*)menu->items[i]->typeData;
					listPtr->cursorPos = 0;
					listPtr->startPos = 0;
				}
				menu->items[i]->cursorPos = index;
				(this->*(DC->feederSelection))( menu->items[i]->special, menu->items[i]->cursorPos );
				return;
			}
		}
	}
}

bool displayContextDef_t::Menus_AnyFullScreenVisible() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		if ( Menus[i].window.flags & WINDOW_VISIBLE && Menus[i].fullScreen ) {
			return true;
		}
	}
	return false;
}

menuDef_t * displayContextDef_t::Menus_ActivateByName( const char *p, bool modalStack ) {
	int i;
	menuDef_t *m = NULL;
	menuDef_t *focus = Menu_GetFocused();
	for ( i = 0; i < menuCount; i++ ) {
		if ( Q_stricmp( Menus[i].window.name, p ) == 0 ) {
			m = &Menus[i];
			Menus_Activate( m );
			if ( modalStack && m->window.flags & WINDOW_MODAL ) {
				if ( modalMenuCount >= MAX_MODAL_MENUS ) {
					Com_Error( ERR_DROP, "MAX_MODAL_MENUS exceeded\n" );
				}
				modalMenuStack[modalMenuCount++] = focus;
			}
		} else {
			Menus[i].window.flags &= ~WINDOW_HASFOCUS;
		}
	}
	Display_CloseCinematics();
	return m;
}


void displayContextDef_t::Item_Init( itemDef_t *item ) {
	memset( item, 0, sizeof( itemDef_t ) );
	item->textscale = 0.55f;
	Window_Init( &item->window );
}

void displayContextDef_t::Menu_HandleMouseMove( menuDef_t *menu, float x, float y ) {
	int i, pass;
	bool focusSet = false;

	itemDef_t *overItem;
	if ( menu == NULL ) {
		return;
	}

	if ( !( menu->window.flags & ( WINDOW_VISIBLE | WINDOW_FORCED ) ) ) {
		return;
	}

	if ( itemCapture ) {
		if ( itemCapture->type == ITEM_TYPE_LISTBOX ) {
			// NERVE - SMF - lose capture if out of client rect
			if ( !Rect_ContainsPoint( &itemCapture->window.rect, x, y ) ) {
				Item_StopCapture( itemCapture );
				itemCapture = NULL;
				captureFunc = NULL;
				captureData = NULL;
			}

		}
		//Item_MouseMove(itemCapture, x, y);
		return;
	}

	if ( g_waitingForKey || g_editingField ) {
		return;
	}

	// FIXME: this is the whole issue of focus vs. mouse over..
	// need a better overall solution as i don't like going through everything twice
	for ( pass = 0; pass < 2; pass++ ) {
		for ( i = 0; i < menu->itemCount; i++ ) {
			// turn off focus each item
			// menu->items[i].window.flags &= ~WINDOW_HASFOCUS;

			if ( !( menu->items[i]->window.flags & ( WINDOW_VISIBLE | WINDOW_FORCED ) ) ) {
				continue;
			}

			// items can be enabled and disabled based on cvars
			if ( menu->items[i]->cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !Item_EnableShowViaCvar( menu->items[i], CVAR_ENABLE ) ) {
				continue;
			}

			if ( menu->items[i]->cvarFlags & ( CVAR_SHOW | CVAR_HIDE ) && !Item_EnableShowViaCvar( menu->items[i], CVAR_SHOW ) ) {
				continue;
			}



			if ( Rect_ContainsPoint( &menu->items[i]->window.rect, x, y ) ) {
				if ( pass == 1 ) {
					overItem = menu->items[i];
					if ( overItem->type == ITEM_TYPE_TEXT && overItem->text ) {
						if ( !Rect_ContainsPoint( Item_CorrectedTextRect( overItem ), x, y ) ) {
							continue;
						}
					}
					// if we are over an item
					if ( IsVisible( overItem->window.flags ) ) {
						// different one
						Item_MouseEnter( overItem, x, y );
						// Item_SetMouseOver(overItem, true);

						// if item is not a decoration see if it can take focus
						if ( !focusSet ) {
							focusSet = Item_SetFocus( overItem, x, y );
						}
					}
				}
			} else if ( menu->items[i]->window.flags & WINDOW_MOUSEOVER ) {
				Item_MouseLeave( menu->items[i] );
				Item_SetMouseOver( menu->items[i], false );
			}
		}
	}

}

void displayContextDef_t::Menu_Paint( menuDef_t *menu, bool forcePaint ) {
	auto&& DC = this;
	int i;

	if ( menu == NULL ) {
		return;
	}

	if ( !( menu->window.flags & WINDOW_VISIBLE ) &&  !forcePaint ) {
		return;
	}

	if ( menu->window.ownerDrawFlags && DC->ownerDrawVisible && !DC->ownerDrawVisible( menu->window.ownerDrawFlags ) ) {
		return;
	}

	if ( forcePaint ) {
		menu->window.flags |= WINDOW_FORCED;
	}

	// draw the background if necessary
	if ( menu->fullScreen ) {
		// implies a background shader
		// FIXME: make sure we have a default shader if fullscreen is set with no background
		DC->drawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, menu->window.background );
	} else if ( menu->window.background ) {
		// this allows a background shader without being full screen
		//UI_DrawHandlePic(menu->window.rect.x, menu->window.rect.y, menu->window.rect.w, menu->window.rect.h, menu->backgroundShader);
	}

	// paint the background and or border
	Window_Paint( &menu->window, menu->fadeAmount, menu->fadeClamp, menu->fadeCycle );

	for ( i = 0; i < menu->itemCount; i++ ) {
		Item_Paint( menu->items[i] );
	}

	if ( debugMode ) {
		vec4_t color;
		color[0] = color[2] = color[3] = 1;
		color[1] = 0;
		DC->drawRect( menu->window.rect.x, menu->window.rect.y, menu->window.rect.w, menu->window.rect.h, 1, color );
	}
}

/*
===============
Item_ValidateTypeData
===============
*/
void displayContextDef_t::Item_ValidateTypeData( itemDef_t *item ) {
	if ( item->typeData ) {
		return;
	}

	if ( item->type == ITEM_TYPE_LISTBOX ) {
		item->typeData = UI_Alloc( sizeof( listBoxDef_t ) );
		memset( item->typeData, 0, sizeof( listBoxDef_t ) );
	} else if ( item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD || item->type == ITEM_TYPE_YESNO || item->type == ITEM_TYPE_BIND || item->type == ITEM_TYPE_SLIDER || item->type == ITEM_TYPE_TEXT ) {
		item->typeData = UI_Alloc( sizeof( editFieldDef_t ) );
		memset( item->typeData, 0, sizeof( editFieldDef_t ) );
		if ( item->type == ITEM_TYPE_EDITFIELD ) {
			if ( !( (editFieldDef_t *) item->typeData )->maxPaintChars ) {
				( (editFieldDef_t *) item->typeData )->maxPaintChars = MAX_EDITFIELD;
			}
		}
	} else if ( item->type == ITEM_TYPE_MULTI ) {
		item->typeData = UI_Alloc( sizeof( multiDef_t ) );
	} else if ( item->type == ITEM_TYPE_MODEL ) {
		item->typeData = UI_Alloc( sizeof( modelDef_t ) );
	} else if ( item->type == ITEM_TYPE_MENUMODEL ) {
		item->typeData = UI_Alloc( sizeof( modelDef_t ) );
	}
}

/*
===============
Keyword Hash
===============
*/

#define KEYWORDHASH_SIZE    512

typedef struct keywordHash_s
{
	char *keyword;
	bool (displayContextDef_t::*func )( itemDef_t *item, int handle );
	struct keywordHash_s *next;
} keywordHash_t;

int KeywordHash_Key( char *keyword ) {
	int register hash, i;

	hash = 0;
	for ( i = 0; keyword[i] != '\0'; i++ ) {
		if ( keyword[i] >= 'A' && keyword[i] <= 'Z' ) {
			hash += ( keyword[i] + ( 'a' - 'A' ) ) * ( 119 + i );
		} else {
			hash += keyword[i] * ( 119 + i );
		}
	}
	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) ) & ( KEYWORDHASH_SIZE - 1 );
	return hash;
}

void KeywordHash_Add( keywordHash_t *table[], keywordHash_t *key ) {
	int hash;

	hash = KeywordHash_Key( key->keyword );
/*
	if (table[hash]) {
		int collision = true;
	}
*/
	key->next = table[hash];
	table[hash] = key;
}

keywordHash_t *KeywordHash_Find( keywordHash_t *table[], char *keyword ) {
	keywordHash_t *key;
	int hash;

	hash = KeywordHash_Key( keyword );
	for ( key = table[hash]; key; key = key->next ) {
		if ( !Q_stricmp( key->keyword, keyword ) ) {
			return key;
		}
	}
	return NULL;
}

/*
===============
Item Keyword Parse functions
===============
*/

// name <string>
bool displayContextDef_t::ItemParse_name( itemDef_t *item, int handle ) {
	if ( !PC_String_Parse( handle, &item->window.name ) ) {
		return false;
	}
	return true;
}

// name <string>
bool displayContextDef_t::ItemParse_focusSound( itemDef_t *item, int handle ) {
	auto&& DC = this;
	const char *temp;
	if ( !PC_String_Parse( handle, &temp ) ) {
		return false;
	}
	item->focusSound = DC->registerSound( temp );
	return true;
}


// text <string>
bool displayContextDef_t::ItemParse_text( itemDef_t *item, int handle ) {
	if ( !PC_String_Parse( handle, &item->text ) ) {
		return false;
	}
	return true;
}

//----(SA)	added

// textfile <string>
// read an external textfile into item->text
bool displayContextDef_t::ItemParse_textfile( itemDef_t *item, int handle ) {
	auto&& DC = this;
	const char  *newtext;
	pc_token_t token;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}

	newtext = DC->fileText( token.string );
	item->text = String_Alloc( newtext );

	return true;
}
//----(SA)

// group <string>
bool displayContextDef_t::ItemParse_group( itemDef_t *item, int handle ) {
	if ( !PC_String_Parse( handle, &item->window.group ) ) {
		return false;
	}
	return true;
}


// asset_model <string>
bool displayContextDef_t::ItemParse_asset_model( itemDef_t *item, int handle ) {
	auto&& DC = this;
	const char *temp;
	modelDef_t *modelPtr;
	Item_ValidateTypeData( item );
	modelPtr = (modelDef_t*)item->typeData;

	if ( !PC_String_Parse( handle, &temp ) ) {
		return false;
	}
	if ( !( item->asset ) ) {
		item->asset = DC->registerModel( temp );
//		modelPtr->angle = rand() % 360;
	}
	return true;
}

// asset_shader <string>
bool displayContextDef_t::ItemParse_asset_shader( itemDef_t *item, int handle ) {
	auto&& DC = this;
	const char *temp;

	if ( !PC_String_Parse( handle, &temp ) ) {
		return false;
	}
	item->asset = DC->registerShaderNoMip( temp );
	return true;
}

// model_origin <number> <number> <number>
bool displayContextDef_t::ItemParse_model_origin( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData( item );
	modelPtr = (modelDef_t*)item->typeData;

	if ( PC_Float_Parse( handle, &modelPtr->origin[0] ) ) {
		if ( PC_Float_Parse( handle, &modelPtr->origin[1] ) ) {
			if ( PC_Float_Parse( handle, &modelPtr->origin[2] ) ) {
				return true;
			}
		}
	}
	return false;
}

// model_fovx <number>
bool displayContextDef_t::ItemParse_model_fovx( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData( item );
	modelPtr = (modelDef_t*)item->typeData;

	if ( !PC_Float_Parse( handle, &modelPtr->fov_x ) ) {
		return false;
	}
	return true;
}

// model_fovy <number>
bool displayContextDef_t::ItemParse_model_fovy( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData( item );
	modelPtr = (modelDef_t*)item->typeData;

	if ( !PC_Float_Parse( handle, &modelPtr->fov_y ) ) {
		return false;
	}
	return true;
}

// model_rotation <integer>
bool displayContextDef_t::ItemParse_model_rotation( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData( item );
	modelPtr = (modelDef_t*)item->typeData;

	if ( !PC_Int_Parse( handle, &modelPtr->rotationSpeed ) ) {
		return false;
	}
	return true;
}

// model_angle <integer>
bool displayContextDef_t::ItemParse_model_angle( itemDef_t *item, int handle ) {
	modelDef_t *modelPtr;
	Item_ValidateTypeData( item );
	modelPtr = (modelDef_t*)item->typeData;

	if ( !PC_Int_Parse( handle, &modelPtr->angle ) ) {
		return false;
	}
	return true;
}

// model_animplay <int(startframe)> <int(numframes)> <int(loopframes)> <int(fps)>
bool displayContextDef_t::ItemParse_model_animplay( itemDef_t *item, int handle ) {
	auto&& DC = this;
	modelDef_t *modelPtr;
	Item_ValidateTypeData( item );
	modelPtr = (modelDef_t*)item->typeData;

	modelPtr->animated = 1;

	if ( !PC_Int_Parse( handle, &modelPtr->startframe ) ) {
		return false;
	}
	if ( !PC_Int_Parse( handle, &modelPtr->numframes ) ) {
		return false;
	}
	if ( !PC_Int_Parse( handle, &modelPtr->loopframes ) ) {
		return false;
	}
	if ( !PC_Int_Parse( handle, &modelPtr->fps ) ) {
		return false;
	}

	modelPtr->frame     = modelPtr->startframe + 1;
	modelPtr->oldframe  = modelPtr->startframe;
	modelPtr->backlerp  = 0.0f;
	modelPtr->frameTime = DC->realTime;
	return true;
}


// rect <rectangle>
bool displayContextDef_t::ItemParse_rect( itemDef_t *item, int handle ) {
	if ( !PC_Rect_Parse( handle, &item->window.rectClient ) ) {
		return false;
	}
	return true;
}

// NERVE - SMF
// origin <integer, integer>
bool displayContextDef_t::ItemParse_origin( itemDef_t *item, int handle ) {
	int x, y;

	if ( !PC_Int_Parse( handle, &x ) ) {
		return false;
	}
	if ( !PC_Int_Parse( handle, &y ) ) {
		return false;
	}

	item->window.rectClient.x += x;
	item->window.rectClient.y += y;

	return true;
}
// -NERVE - SMF

// style <integer>
bool displayContextDef_t::ItemParse_style( itemDef_t *item, int handle ) {
	if ( !PC_Int_Parse( handle, &item->window.style ) ) {
		return false;
	}
	return true;
}

// decoration
bool displayContextDef_t::ItemParse_decoration( itemDef_t *item, int handle ) {
	item->window.flags |= WINDOW_DECORATION;
	return true;
}

// notselectable
bool displayContextDef_t::ItemParse_notselectable( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;
	Item_ValidateTypeData( item );
	listPtr = (listBoxDef_t*)item->typeData;
	if ( item->type == ITEM_TYPE_LISTBOX && listPtr ) {
		listPtr->notselectable = true;
	}
	return true;
}

// manually wrapped
bool displayContextDef_t::ItemParse_wrapped( itemDef_t *item, int handle ) {
	item->window.flags |= WINDOW_WRAPPED;
	return true;
}

// auto wrapped
bool displayContextDef_t::ItemParse_autowrapped( itemDef_t *item, int handle ) {
	item->window.flags |= WINDOW_AUTOWRAPPED;
	return true;
}


// horizontalscroll
bool displayContextDef_t::ItemParse_horizontalscroll( itemDef_t *item, int handle ) {
	item->window.flags |= WINDOW_HORIZONTAL;
	return true;
}

// type <integer>
bool displayContextDef_t::ItemParse_type( itemDef_t *item, int handle ) {
	if ( !PC_Int_Parse( handle, &item->type ) ) {
		return false;
	}
	Item_ValidateTypeData( item );
	return true;
}

// elementwidth, used for listbox image elements
// uses textalignx for storage
bool displayContextDef_t::ItemParse_elementwidth( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	Item_ValidateTypeData( item );
	listPtr = (listBoxDef_t*)item->typeData;
	if ( !PC_Float_Parse( handle, &listPtr->elementWidth ) ) {
		return false;
	}
	return true;
}

// elementheight, used for listbox image elements
// uses textaligny for storage
bool displayContextDef_t::ItemParse_elementheight( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	Item_ValidateTypeData( item );
	listPtr = (listBoxDef_t*)item->typeData;
	if ( !PC_Float_Parse( handle, &listPtr->elementHeight ) ) {
		return false;
	}
	return true;
}

// feeder <float>
bool displayContextDef_t::ItemParse_feeder( itemDef_t *item, int handle ) {
	if ( !PC_Float_Parse( handle, &item->special ) ) {
		return false;
	}
	return true;
}

// elementtype, used to specify what type of elements a listbox contains
// uses textstyle for storage
bool displayContextDef_t::ItemParse_elementtype( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return false;
	}
	listPtr = (listBoxDef_t*)item->typeData;
	if ( !PC_Int_Parse( handle, &listPtr->elementStyle ) ) {
		return false;
	}
	return true;
}

// columns sets a number of columns and an x pos and width per..
bool displayContextDef_t::ItemParse_columns( itemDef_t *item, int handle ) {
	int num, i;
	listBoxDef_t *listPtr;

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return false;
	}
	listPtr = (listBoxDef_t*)item->typeData;
	if ( PC_Int_Parse( handle, &num ) ) {
		if ( num > MAX_LB_COLUMNS ) {
			num = MAX_LB_COLUMNS;
		}
		listPtr->numColumns = num;
		for ( i = 0; i < num; i++ ) {
			int pos, width, maxChars;

			if ( PC_Int_Parse( handle, &pos ) && PC_Int_Parse( handle, &width ) && PC_Int_Parse( handle, &maxChars ) ) {
				listPtr->columnInfo[i].pos = pos;
				listPtr->columnInfo[i].width = width;
				listPtr->columnInfo[i].maxChars = maxChars;
			} else {
				return false;
			}
		}
	} else {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_border( itemDef_t *item, int handle ) {
	if ( !PC_Int_Parse( handle, &item->window.border ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_bordersize( itemDef_t *item, int handle ) {
	if ( !PC_Float_Parse( handle, &item->window.borderSize ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_visible( itemDef_t *item, int handle ) {
	int i;

	if ( !PC_Int_Parse( handle, &i ) ) {
		return false;
	}
	if ( i ) {
		item->window.flags |= WINDOW_VISIBLE;
	}
	return true;
}

bool displayContextDef_t::ItemParse_ownerdraw( itemDef_t *item, int handle ) {
	if ( !PC_Int_Parse( handle, &item->window.ownerDraw ) ) {
		return false;
	}
	item->type = ITEM_TYPE_OWNERDRAW;
	return true;
}

bool displayContextDef_t::ItemParse_align( itemDef_t *item, int handle ) {
	if ( !PC_Int_Parse( handle, &item->alignment ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_textalign( itemDef_t *item, int handle ) {
	if ( !PC_Int_Parse( handle, &item->textalignment ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_textalignx( itemDef_t *item, int handle ) {
	if ( !PC_Float_Parse( handle, &item->textalignx ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_textaligny( itemDef_t *item, int handle ) {
	if ( !PC_Float_Parse( handle, &item->textaligny ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_textscale( itemDef_t *item, int handle ) {
	if ( !PC_Float_Parse( handle, &item->textscale ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_textstyle( itemDef_t *item, int handle ) {
	if ( !PC_Int_Parse( handle, &item->textStyle ) ) {
		return false;
	}
	return true;
}

//----(SA)	added for forcing a font for a given item
bool displayContextDef_t::ItemParse_textfont( itemDef_t *item, int handle ) {
	if ( !PC_Int_Parse( handle, &item->font ) ) {
		return false;
	}
	return true;
}
//----(SA)	end

bool displayContextDef_t::ItemParse_backcolor( itemDef_t *item, int handle ) {
	int i;
	float f;

	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return false;
		}
		item->window.backColor[i]  = f;
	}
	return true;
}

bool displayContextDef_t::ItemParse_forecolor( itemDef_t *item, int handle ) {
	int i;
	float f;

	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return false;
		}
		item->window.foreColor[i]  = f;
		item->window.flags |= WINDOW_FORECOLORSET;
	}
	return true;
}

bool displayContextDef_t::ItemParse_bordercolor( itemDef_t *item, int handle ) {
	int i;
	float f;

	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return false;
		}
		item->window.borderColor[i]  = f;
	}
	return true;
}

bool displayContextDef_t::ItemParse_outlinecolor( itemDef_t *item, int handle ) {
	if ( !PC_Color_Parse( handle, &item->window.outlineColor ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_background( itemDef_t *item, int handle ) {
	auto&& DC = this;
	const char *temp;

	if ( !PC_String_Parse( handle, &temp ) ) {
		return false;
	}
	item->window.background = DC->registerShaderNoMip( temp );
	return true;
}

bool displayContextDef_t::ItemParse_cinematic( itemDef_t *item, int handle ) {
	if ( !PC_String_Parse( handle, &item->window.cinematicName ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_doubleClick( itemDef_t *item, int handle ) {
	listBoxDef_t *listPtr;

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return false;
	}

	listPtr = (listBoxDef_t*)item->typeData;

	if ( !PC_Script_Parse( handle, &listPtr->doubleClick ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_onFocus( itemDef_t *item, int handle ) {
	if ( !PC_Script_Parse( handle, &item->onFocus ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_leaveFocus( itemDef_t *item, int handle ) {
	if ( !PC_Script_Parse( handle, &item->leaveFocus ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_mouseEnter( itemDef_t *item, int handle ) {
	if ( !PC_Script_Parse( handle, &item->mouseEnter ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_mouseExit( itemDef_t *item, int handle ) {
	if ( !PC_Script_Parse( handle, &item->mouseExit ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_mouseEnterText( itemDef_t *item, int handle ) {
	if ( !PC_Script_Parse( handle, &item->mouseEnterText ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_mouseExitText( itemDef_t *item, int handle ) {
	if ( !PC_Script_Parse( handle, &item->mouseExitText ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_action( itemDef_t *item, int handle ) {
	if ( !PC_Script_Parse( handle, &item->action ) ) {
		return false;
	}
	return true;
}

// NERVE - SMF
bool displayContextDef_t::ItemParse_accept( itemDef_t *item, int handle ) {
	if ( !PC_Script_Parse( handle, &item->onAccept ) ) {
		return false;
	}
	return true;
}
// -NERVE - SMF

bool displayContextDef_t::ItemParse_special( itemDef_t *item, int handle ) {
	if ( !PC_Float_Parse( handle, &item->special ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_cvarTest( itemDef_t *item, int handle ) {
	if ( !PC_String_Parse( handle, &item->cvarTest ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::ItemParse_cvar( itemDef_t *item, int handle ) {
	editFieldDef_t *editPtr;

	Item_ValidateTypeData( item );
	if ( !PC_String_Parse( handle, &item->cvar ) ) {
		return false;
	}
	if ( item->typeData ) {
		editPtr = (editFieldDef_t*)item->typeData;
		editPtr->minVal = -1;
		editPtr->maxVal = -1;
		editPtr->defVal = -1;
	}
	return true;
}

bool displayContextDef_t::ItemParse_maxChars( itemDef_t *item, int handle ) {
	editFieldDef_t *editPtr;
	int maxChars;

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return false;
	}

	if ( !PC_Int_Parse( handle, &maxChars ) ) {
		return false;
	}
	editPtr = (editFieldDef_t*)item->typeData;
	editPtr->maxChars = maxChars;
	return true;
}

bool displayContextDef_t::ItemParse_maxPaintChars( itemDef_t *item, int handle ) {
	editFieldDef_t *editPtr;
	int maxChars;

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return false;
	}

	if ( !PC_Int_Parse( handle, &maxChars ) ) {
		return false;
	}
	editPtr = (editFieldDef_t*)item->typeData;
	editPtr->maxPaintChars = maxChars;
	return true;
}



bool displayContextDef_t::ItemParse_cvarFloat( itemDef_t *item, int handle ) {
	editFieldDef_t *editPtr;

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return false;
	}
	editPtr = (editFieldDef_t*)item->typeData;
	if ( PC_String_Parse( handle, &item->cvar ) &&
		 PC_Float_Parse( handle, &editPtr->defVal ) &&
		 PC_Float_Parse( handle, &editPtr->minVal ) &&
		 PC_Float_Parse( handle, &editPtr->maxVal ) ) {
		return true;
	}
	return false;
}

bool displayContextDef_t::ItemParse_cvarStrList( itemDef_t *item, int handle ) {
	pc_token_t token;
	multiDef_t *multiPtr;
	int pass;

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return false;
	}
	multiPtr = (multiDef_t*)item->typeData;
	multiPtr->count = 0;
	multiPtr->strDef = true;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}
	if ( *token.string != '{' ) {
		return false;
	}

	pass = 0;
	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			PC_SourceError( handle, "end of file inside menu item\n" );
			return false;
		}

		if ( *token.string == '}' ) {
			return true;
		}

		if ( *token.string == ',' || *token.string == ';' ) {
			continue;
		}

		if ( pass == 0 ) {
			multiPtr->cvarList[multiPtr->count] = String_Alloc( token.string );
			pass = 1;
		} else {
			multiPtr->cvarStr[multiPtr->count] = String_Alloc( token.string );
			pass = 0;
			multiPtr->count++;
			if ( multiPtr->count >= MAX_MULTI_CVARS ) {
				return false;
			}
		}

	}
	return false;  // bk001205 - LCC missing return value
}

bool displayContextDef_t::ItemParse_cvarFloatList( itemDef_t *item, int handle ) {
	pc_token_t token;
	multiDef_t *multiPtr;

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return false;
	}
	multiPtr = (multiDef_t*)item->typeData;
	multiPtr->count = 0;
	multiPtr->strDef = false;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}
	if ( *token.string != '{' ) {
		return false;
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			PC_SourceError( handle, "end of file inside menu item\n" );
			return false;
		}

		if ( *token.string == '}' ) {
			return true;
		}

		if ( *token.string == ',' || *token.string == ';' ) {
			continue;
		}

		multiPtr->cvarList[multiPtr->count] = String_Alloc( token.string );
		if ( !PC_Float_Parse( handle, &multiPtr->cvarValue[multiPtr->count] ) ) {
			return false;
		}

		multiPtr->count++;
		if ( multiPtr->count >= MAX_MULTI_CVARS ) {
			return false;
		}

	}
	return false;  // bk001205 - LCC missing return value
}


bool displayContextDef_t::ParseColorRange( itemDef_t *item, int handle, int type ) {
	colorRangeDef_t color;

	if ( item->numColors && type != item->colorRangeType ) {
		PC_SourceError( handle, "both addColorRange and addColorRangeRel - set within same itemdef\n" );
		return false;
	}

	item->colorRangeType = type;

	if ( PC_Float_Parse( handle, &color.low ) &&
		 PC_Float_Parse( handle, &color.high ) &&
		 PC_Color_Parse( handle, &color.color ) ) {
		if ( item->numColors < MAX_COLOR_RANGES ) {
			memcpy( &item->colorRanges[item->numColors], &color, sizeof( color ) );
			item->numColors++;
		}
		return true;
	}
	return false;
}

bool displayContextDef_t::ItemParse_addColorRangeRel( itemDef_t *item, int handle ) {
	return ParseColorRange( item, handle, RANGETYPE_RELATIVE );
}

bool displayContextDef_t::ItemParse_addColorRange( itemDef_t *item, int handle ) {
	return ParseColorRange( item, handle, RANGETYPE_ABSOLUTE );
}



bool displayContextDef_t::ItemParse_ownerdrawFlag( itemDef_t *item, int handle ) {
	int i;
	if ( !PC_Int_Parse( handle, &i ) ) {
		return false;
	}
	item->window.ownerDrawFlags |= i;
	return true;
}

bool displayContextDef_t::ItemParse_enableCvar( itemDef_t *item, int handle ) {
	if ( PC_Script_Parse( handle, &item->enableCvar ) ) {
		item->cvarFlags = CVAR_ENABLE;
		return true;
	}
	return false;
}

bool displayContextDef_t::ItemParse_disableCvar( itemDef_t *item, int handle ) {
	if ( PC_Script_Parse( handle, &item->enableCvar ) ) {
		item->cvarFlags = CVAR_DISABLE;
		return true;
	}
	return false;
}

bool displayContextDef_t::ItemParse_noToggle( itemDef_t *item, int handle ) {
	item->cvarFlags |= CVAR_NOTOGGLE;
	return true;
}

bool displayContextDef_t::ItemParse_showCvar( itemDef_t *item, int handle ) {
	if ( PC_Script_Parse( handle, &item->enableCvar ) ) {
		item->cvarFlags = CVAR_SHOW;
		return true;
	}
	return false;
}

bool displayContextDef_t::ItemParse_hideCvar( itemDef_t *item, int handle ) {
	if ( PC_Script_Parse( handle, &item->enableCvar ) ) {
		item->cvarFlags = CVAR_HIDE;
		return true;
	}
	return false;
}


keywordHash_t itemParseKeywords[] = {
	{"name", &displayContextDef_t::ItemParse_name, NULL},
	{"text", &displayContextDef_t::ItemParse_text, NULL},
	{"textfile", &displayContextDef_t::ItemParse_textfile, NULL},  //----(SA)	added
	{"group", &displayContextDef_t::ItemParse_group, NULL},
	{"asset_model", &displayContextDef_t::ItemParse_asset_model, NULL},
	{"asset_shader", &displayContextDef_t::ItemParse_asset_shader, NULL},
	{"model_origin", &displayContextDef_t::ItemParse_model_origin, NULL},
	{"model_fovx", &displayContextDef_t::ItemParse_model_fovx, NULL},
	{"model_fovy", &displayContextDef_t::ItemParse_model_fovy, NULL},
	{"model_rotation", &displayContextDef_t::ItemParse_model_rotation, NULL},
	{"model_angle", &displayContextDef_t::ItemParse_model_angle, NULL},
	{"model_animplay", &displayContextDef_t::ItemParse_model_animplay, NULL},
	{"rect", &displayContextDef_t::ItemParse_rect, NULL},
	{"origin", &displayContextDef_t::ItemParse_origin, NULL},              // NERVE - SMF
	{"style", &displayContextDef_t::ItemParse_style, NULL},
	{"decoration", &displayContextDef_t::ItemParse_decoration, NULL},
	{"notselectable", &displayContextDef_t::ItemParse_notselectable, NULL},
	{"wrapped", &displayContextDef_t::ItemParse_wrapped, NULL},
	{"autowrapped", &displayContextDef_t::ItemParse_autowrapped, NULL},
	{"horizontalscroll", &displayContextDef_t::ItemParse_horizontalscroll, NULL},
	{"type", &displayContextDef_t::ItemParse_type, NULL},
	{"elementwidth", &displayContextDef_t::ItemParse_elementwidth, NULL},
	{"elementheight", &displayContextDef_t::ItemParse_elementheight, NULL},
	{"feeder", &displayContextDef_t::ItemParse_feeder, NULL},
	{"elementtype",&displayContextDef_t::ItemParse_elementtype, NULL},
	{"columns", &displayContextDef_t::ItemParse_columns, NULL},
	{"border", &displayContextDef_t::ItemParse_border, NULL},
	{"bordersize", &displayContextDef_t::ItemParse_bordersize, NULL},
	{"visible", &displayContextDef_t::ItemParse_visible, NULL},
	{"ownerdraw", &displayContextDef_t::ItemParse_ownerdraw, NULL},
	{"align", &displayContextDef_t::ItemParse_align, NULL},
	{"textalign", &displayContextDef_t::ItemParse_textalign, NULL},
	{"textalignx", &displayContextDef_t::ItemParse_textalignx, NULL},
	{"textaligny", &displayContextDef_t::ItemParse_textaligny, NULL},
	{"textscale", &displayContextDef_t::ItemParse_textscale, NULL},
	{"textstyle", &displayContextDef_t::ItemParse_textstyle, NULL},
	{"textfont", &displayContextDef_t::ItemParse_textfont, NULL},              // (SA)
	{"backcolor", &displayContextDef_t::ItemParse_backcolor, NULL},
	{"forecolor", &displayContextDef_t::ItemParse_forecolor, NULL},
	{"bordercolor", &displayContextDef_t::ItemParse_bordercolor, NULL},
	{"outlinecolor", &displayContextDef_t::ItemParse_outlinecolor, NULL},
	{"background", &displayContextDef_t::ItemParse_background, NULL},
	{"onFocus", &displayContextDef_t::ItemParse_onFocus, NULL},
	{"leaveFocus", &displayContextDef_t::ItemParse_leaveFocus, NULL},
	{"mouseEnter", &displayContextDef_t::ItemParse_mouseEnter, NULL},
	{"mouseExit", &displayContextDef_t::ItemParse_mouseExit, NULL},
	{"mouseEnterText", &displayContextDef_t::ItemParse_mouseEnterText, NULL},
	{"mouseExitText", &displayContextDef_t::ItemParse_mouseExitText, NULL},
	{"action", &displayContextDef_t::ItemParse_action, NULL},
	{"accept", &displayContextDef_t::ItemParse_accept, NULL},              // NERVE - SMF
	{"special", &displayContextDef_t::ItemParse_special, NULL},
	{"cvar", &displayContextDef_t::ItemParse_cvar, NULL},
	{"maxChars", &displayContextDef_t::ItemParse_maxChars, NULL},
	{"maxPaintChars", &displayContextDef_t::ItemParse_maxPaintChars, NULL},
	{"focusSound", &displayContextDef_t::ItemParse_focusSound, NULL},
	{"cvarFloat", &displayContextDef_t::ItemParse_cvarFloat, NULL},
	{"cvarStrList", &displayContextDef_t::ItemParse_cvarStrList, NULL},
	{"cvarFloatList", &displayContextDef_t::ItemParse_cvarFloatList, NULL},
	{"addColorRange", &displayContextDef_t::ItemParse_addColorRange, NULL},
	{"addColorRangeRel", &displayContextDef_t::ItemParse_addColorRangeRel, NULL},
	{"ownerdrawFlag", &displayContextDef_t::ItemParse_ownerdrawFlag, NULL},
	{"enableCvar", &displayContextDef_t::ItemParse_enableCvar, NULL},
	{"cvarTest", &displayContextDef_t::ItemParse_cvarTest, NULL},
	{"disableCvar", &displayContextDef_t::ItemParse_disableCvar, NULL},
	{"showCvar", &displayContextDef_t::ItemParse_showCvar, NULL},
	{"hideCvar", &displayContextDef_t::ItemParse_hideCvar, NULL},
	{"cinematic", &displayContextDef_t::ItemParse_cinematic, NULL},
	{"doubleclick", &displayContextDef_t::ItemParse_doubleClick, NULL},
	{"noToggle", &displayContextDef_t::ItemParse_noToggle, NULL}, // TTimo: use with ITEM_TYPE_YESNO and an action script (see sv_punkbuster)
	{NULL, NULL, NULL}
};

keywordHash_t *itemParseKeywordHash[KEYWORDHASH_SIZE];

/*
===============
Item_SetupKeywordHash
===============
*/
void displayContextDef_t::Item_SetupKeywordHash( void ) {
	int i;

	memset( itemParseKeywordHash, 0, sizeof( itemParseKeywordHash ) );
	for ( i = 0; itemParseKeywords[i].keyword; i++ ) {
		KeywordHash_Add( itemParseKeywordHash, &itemParseKeywords[i] );
	}
}

/*
===============
Item_Parse
===============
*/
bool displayContextDef_t::Item_Parse(int handle, itemDef_t * item) {
	pc_token_t token;
	keywordHash_t *key;


	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}
	if ( *token.string != '{' ) {
		return false;
	}
	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			PC_SourceError( handle, "end of file inside menu item\n" );
			return false;
		}

		if ( *token.string == '}' ) {
			return true;
		}

		key = KeywordHash_Find( itemParseKeywordHash, token.string );
		if ( !key ) {
			PC_SourceError( handle, "unknown menu item keyword %s", token.string );
			continue;
		}
		if (!(this->*(key->func))(item, handle)) {
			PC_SourceError( handle, "couldn't parse menu item keyword %s", token.string );
			return false;
		}
	}
	return false;  // bk001205 - LCC missing return value
}


// Item_InitControls
// init's special control types
void displayContextDef_t::Item_InitControls( itemDef_t *item ) {
	if ( item == NULL ) {
		return;
	}
	if ( item->type == ITEM_TYPE_LISTBOX ) {
		listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
		item->cursorPos = 0;
		if ( listPtr ) {
			listPtr->cursorPos = 0;
			listPtr->startPos = 0;
			listPtr->endPos = 0;
			listPtr->cursorPos = 0;
		}
	}
}

/*
===============
Menu Keyword Parse functions
===============
*/

bool displayContextDef_t::MenuParse_font(itemDef_t* item, int handle) {
	auto&& DC = this;
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_String_Parse( handle, &menu->font ) ) {
		return false;
	}
	if ( !DC->Assets.fontRegistered ) {
		DC->registerFont( menu->font, 48, &DC->Assets.textFont );
		DC->Assets.fontRegistered = true;
	}
	return true;
}

bool displayContextDef_t::MenuParse_name( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_String_Parse( handle, &menu->window.name ) ) {
		return false;
	}
	if ( Q_stricmp( menu->window.name, "main" ) == 0 ) {
		// default main as having focus
		//menu->window.flags |= WINDOW_HASFOCUS;
	}
	return true;
}

bool displayContextDef_t::MenuParse_fullscreen( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_Int_Parse( handle, (int*)&menu->fullScreen ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_rect( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_Rect_Parse( handle, &menu->window.rect ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_style( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_Int_Parse( handle, &menu->window.style ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_visible( itemDef_t *item, int handle ) {
	int i;
	menuDef_t *menu = (menuDef_t*)item;

	if ( !PC_Int_Parse( handle, &i ) ) {
		return false;
	}
	if ( i ) {
		menu->window.flags |= WINDOW_VISIBLE;
	}
	return true;
}

bool displayContextDef_t::MenuParse_onOpen( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_Script_Parse( handle, &menu->onOpen ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_onClose( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_Script_Parse( handle, &menu->onClose ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_onESC( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_Script_Parse( handle, &menu->onESC ) ) {
		return false;
	}
	return true;
}



bool displayContextDef_t::MenuParse_border( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_Int_Parse( handle, &menu->window.border ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_borderSize( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_Float_Parse( handle, &menu->window.borderSize ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_backcolor( itemDef_t *item, int handle ) {
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return false;
		}
		menu->window.backColor[i]  = f;
	}
	return true;
}

bool displayContextDef_t::MenuParse_forecolor( itemDef_t *item, int handle ) {
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return false;
		}
		menu->window.foreColor[i]  = f;
		menu->window.flags |= WINDOW_FORECOLORSET;
	}
	return true;
}

bool displayContextDef_t::MenuParse_bordercolor( itemDef_t *item, int handle ) {
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return false;
		}
		menu->window.borderColor[i]  = f;
	}
	return true;
}

bool displayContextDef_t::MenuParse_focuscolor( itemDef_t *item, int handle ) {
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;

	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return false;
		}
		menu->focusColor[i]  = f;
	}
	return true;
}

bool displayContextDef_t::MenuParse_disablecolor( itemDef_t *item, int handle ) {
	int i;
	float f;
	menuDef_t *menu = (menuDef_t*)item;
	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return false;
		}
		menu->disableColor[i]  = f;
	}
	return true;
}


bool  displayContextDef_t::MenuParse_outlinecolor( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( !PC_Color_Parse( handle, &menu->window.outlineColor ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_background( itemDef_t *item, int handle ) {
	auto&& DC = this;
	const char *buff;
	menuDef_t *menu = (menuDef_t*)item;

	if ( !PC_String_Parse( handle, &buff ) ) {
		return false;
	}
	menu->window.background = DC->registerShaderNoMip( buff );
	return true;
}

bool displayContextDef_t::MenuParse_cinematic( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if ( !PC_String_Parse( handle, &menu->window.cinematicName ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_ownerdrawFlag( itemDef_t *item, int handle ) {
	int i;
	menuDef_t *menu = (menuDef_t*)item;

	if ( !PC_Int_Parse( handle, &i ) ) {
		return false;
	}
	menu->window.ownerDrawFlags |= i;
	return true;
}

bool displayContextDef_t::MenuParse_ownerdraw( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if ( !PC_Int_Parse( handle, &menu->window.ownerDraw ) ) {
		return false;
	}
	return true;
}


// decoration
bool displayContextDef_t::MenuParse_popup( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	menu->window.flags |= WINDOW_POPUP;
	return true;
}


bool displayContextDef_t::MenuParse_outOfBounds( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	menu->window.flags |= WINDOW_OOB_CLICK;
	return true;
}

bool displayContextDef_t::MenuParse_soundLoop( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if ( !PC_String_Parse( handle, &menu->soundName ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_fadeClamp( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if ( !PC_Float_Parse( handle, &menu->fadeClamp ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_fadeAmount( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if ( !PC_Float_Parse( handle, &menu->fadeAmount ) ) {
		return false;
	}
	return true;
}


bool displayContextDef_t::MenuParse_fadeCycle( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;

	if ( !PC_Int_Parse( handle, &menu->fadeCycle ) ) {
		return false;
	}
	return true;
}


bool displayContextDef_t::MenuParse_itemDef( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	if ( menu->itemCount < MAX_MENUITEMS ) {
		menu->items[menu->itemCount] = (itemDef_t*)UI_Alloc( sizeof( itemDef_t ) );
		Item_Init( menu->items[menu->itemCount] );
		if ( !Item_Parse( handle, menu->items[menu->itemCount] ) ) {
			return false;
		}
		Item_InitControls( menu->items[menu->itemCount] );
		menu->items[menu->itemCount++]->parent = menu;
	}
	return true;
}

// NERVE - SMF
bool displayContextDef_t::MenuParse_execKey( itemDef_t *item, int handle ) {
	menuDef_t *menu = ( menuDef_t* )item;
	char keyname;
	short int keyindex;

	if ( !PC_Char_Parse( handle, &keyname ) ) {
		return false;
	}
	keyindex = keyname;

	if ( !PC_Script_Parse( handle, &menu->onKey[keyindex] ) ) {
		return false;
	}
	return true;
}

bool displayContextDef_t::MenuParse_execKeyInt( itemDef_t *item, int handle ) {
	menuDef_t *menu = ( menuDef_t* )item;
	int keyname;

	if ( !PC_Int_Parse( handle, &keyname ) ) {
		return false;
	}

	if ( !PC_Script_Parse( handle, &menu->onKey[keyname] ) ) {
		return false;
	}
	return true;
}
// -NERVE - SMF

// TTimo
bool displayContextDef_t::MenuParse_modal( itemDef_t *item, int handle ) {
	menuDef_t *menu = (menuDef_t*)item;
	menu->window.flags |= WINDOW_MODAL;
	return true;
}


keywordHash_t menuParseKeywords[] = {
	{"font", &displayContextDef_t::MenuParse_font, NULL},
	{"name", &displayContextDef_t::MenuParse_name, NULL},
	{"fullscreen", &displayContextDef_t::MenuParse_fullscreen, NULL},
	{"rect", &displayContextDef_t::MenuParse_rect, NULL},
	{"style", &displayContextDef_t::MenuParse_style, NULL},
	{"visible", &displayContextDef_t::MenuParse_visible, NULL},
	{"onOpen",&displayContextDef_t::MenuParse_onOpen, NULL},
	{"onClose", &displayContextDef_t::MenuParse_onClose, NULL},
	{"onESC", &displayContextDef_t::MenuParse_onESC, NULL},
	{"border", &displayContextDef_t::MenuParse_border, NULL},
	{"borderSize", &displayContextDef_t::MenuParse_borderSize, NULL},
	{"backcolor", &displayContextDef_t::MenuParse_backcolor, NULL},
	{"forecolor", &displayContextDef_t::MenuParse_forecolor, NULL},
	{"bordercolor", &displayContextDef_t::MenuParse_bordercolor, NULL},
	{"focuscolor", &displayContextDef_t::MenuParse_focuscolor, NULL},
	{"disablecolor", &displayContextDef_t::MenuParse_disablecolor, NULL},
	{"outlinecolor", &displayContextDef_t::MenuParse_outlinecolor, NULL},
	{"background", &displayContextDef_t::MenuParse_background, NULL},
	{"ownerdraw", &displayContextDef_t::MenuParse_ownerdraw, NULL},
	{"ownerdrawFlag",&displayContextDef_t::MenuParse_ownerdrawFlag, NULL},
	{"outOfBoundsClick", &displayContextDef_t::MenuParse_outOfBounds, NULL},
	{"soundLoop", &displayContextDef_t::MenuParse_soundLoop, NULL},
	{"itemDef", &displayContextDef_t::MenuParse_itemDef, NULL},
	{"cinematic", &displayContextDef_t::MenuParse_cinematic, NULL},
	{"popup", &displayContextDef_t::MenuParse_popup, NULL},
	{"fadeClamp", &displayContextDef_t::MenuParse_fadeClamp, NULL},
	{"fadeCycle", &displayContextDef_t::MenuParse_fadeCycle, NULL},
	{"fadeAmount", &displayContextDef_t::MenuParse_fadeAmount, NULL},
	{"execKey", &displayContextDef_t::MenuParse_execKey, NULL},                // NERVE - SMF
	{"execKeyInt", &displayContextDef_t::MenuParse_execKeyInt, NULL},          // NERVE - SMF
	{"modal", &displayContextDef_t::MenuParse_modal, NULL },
	{NULL, NULL, NULL}
};

keywordHash_t *menuParseKeywordHash[KEYWORDHASH_SIZE];

/*
===============
Menu_SetupKeywordHash
===============
*/
void displayContextDef_t::Menu_SetupKeywordHash( void ) {
	int i;

	memset( menuParseKeywordHash, 0, sizeof( menuParseKeywordHash ) );
	for ( i = 0; menuParseKeywords[i].keyword; i++ ) {
		KeywordHash_Add( menuParseKeywordHash, &menuParseKeywords[i] );
	}
}

/*
===============
Menu_Parse
===============
*/
bool displayContextDef_t::Menu_Parse( int handle, menuDef_t *menu ) {
	pc_token_t token;
	keywordHash_t *key;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return false;
	}
	if ( *token.string != '{' ) {
		return false;
	}

	while ( 1 ) {

		memset( &token, 0, sizeof( pc_token_t ) );
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			PC_SourceError( handle, "end of file inside menu\n" );
			return false;
		}

		if ( *token.string == '}' ) {
			return true;
		}

		key = KeywordHash_Find( menuParseKeywordHash, token.string );
		if ( !key ) {
			PC_SourceError( handle, "unknown menu keyword %s", token.string );
			continue;
		}
		if ( !(this->*(key->func))( (itemDef_t*)menu, handle ) ) {
			PC_SourceError( handle, "couldn't parse menu keyword %s", token.string );
			return false;
		}
	}
	return false;  // bk001205 - LCC missing return value
}

/*
===============
Menu_New
===============
*/
void displayContextDef_t::Menu_New( int handle ) {
	menuDef_t *menu = &Menus[menuCount];

	if ( menuCount < MAX_MENUS ) {
		Menu_Init( menu );
		if ( Menu_Parse( handle, menu ) ) {
			Menu_PostParse( menu );
			menuCount++;
		}
	}
}

int displayContextDef_t::Menu_Count() {
	return menuCount;
}

void displayContextDef_t::Menu_PaintAll() {
	auto&& DC = this;
	int i;
	if ( captureFunc ) {
		(this->*captureFunc)( captureData );
	}

	for ( i = 0; i < Menu_Count(); i++ ) {
		Menu_Paint( &Menus[i], false );
	}

	if ( debugMode ) {
		vec4_t v = {1, 1, 1, 1};
		DC->drawText( 5, 25, .5, v, va( "fps: %f", DC->FPS ), 0, 0, 0 );
	}
}

void displayContextDef_t::Menu_Reset() {
	menuCount = 0;
}

displayContextDef_t * displayContextDef_t::Display_GetContext() {
	auto&& DC = this;
	return DC;
}

//static float captureX; // TTimo: unused
//static float captureY; // TTimo: unused

void * displayContextDef_t::Display_CaptureItem( int x, int y ) {
	int i;

	for ( i = 0; i < menuCount; i++ ) {
		// turn off focus each item
		// menu->items[i].window.flags &= ~WINDOW_HASFOCUS;
		if ( Rect_ContainsPoint( &Menus[i].window.rect, x, y ) ) {
			return &Menus[i];
		}
	}
	return NULL;
}


// FIXME:
bool displayContextDef_t::Display_MouseMove( void *p, int x, int y ) {
	int i;
	menuDef_t *menu = (menuDef_t*)p;

	if ( menu == NULL ) {
		menu = Menu_GetFocused();
		if ( menu ) {
			if ( menu->window.flags & WINDOW_POPUP ) {
				Menu_HandleMouseMove( menu, x, y );
				return true;
			}
		}
		for ( i = 0; i < menuCount; i++ ) {
			Menu_HandleMouseMove( &Menus[i], x, y );
		}
	} else {
		menu->window.rect.x += x;
		menu->window.rect.y += y;
		Menu_UpdatePosition( menu );
	}
	return true;

}

int displayContextDef_t::Display_CursorType( int x, int y ) {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		rectDef_t r2;
		r2.x = Menus[i].window.rect.x - 3;
		r2.y = Menus[i].window.rect.y - 3;
		r2.w = r2.h = 7;
		if ( Rect_ContainsPoint( &r2, x, y ) ) {
			return CURSOR_SIZER;
		}
	}
	return CURSOR_ARROW;
}


void displayContextDef_t::Display_HandleKey( int key, bool down, int x, int y ) {
	menuDef_t *menu = (menuDef_t*)Display_CaptureItem( x, y );
	if ( menu == NULL ) {
		menu = Menu_GetFocused();
	}
	if ( menu ) {
		Menu_HandleKey( menu, key, down );
	}
}

void displayContextDef_t::Window_CacheContents( windowDef_t *window ) {//static 
	auto&& DC = this;
	if ( window ) {
		if ( window->cinematicName ) {
			int cin = DC->playCinematic( window->cinematicName, 0, 0, 0, 0 );
			DC->stopCinematic( cin );
		}
	}
}


void  displayContextDef_t::Item_CacheContents( itemDef_t *item ) {//
	if ( item ) {
		Window_CacheContents( &item->window );
	}

}

void displayContextDef_t::Menu_CacheContents( menuDef_t *menu ) {//static 
	auto&& DC = this;
	if ( menu ) {
		int i;
		Window_CacheContents( &menu->window );
		for ( i = 0; i < menu->itemCount; i++ ) {
			Item_CacheContents( menu->items[i] );
		}

		if ( menu->soundName && *menu->soundName ) {
			DC->registerSound( menu->soundName );
		}
	}

}

void displayContextDef_t::Display_CacheAll() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		Menu_CacheContents( &Menus[i] );
	}
}


bool displayContextDef_t::Menu_OverActiveItem( menuDef_t *menu, float x, float y ) {
	if ( menu && menu->window.flags & ( WINDOW_VISIBLE | WINDOW_FORCED ) ) {
		if ( Rect_ContainsPoint( &menu->window.rect, x, y ) ) {
			int i;
			for ( i = 0; i < menu->itemCount; i++ ) {
				// turn off focus each item
				// menu->items[i]->window.flags &= ~WINDOW_HASFOCUS;

				if ( !( menu->items[i]->window.flags & ( WINDOW_VISIBLE | WINDOW_FORCED ) ) ) {
					continue;
				}

				if ( menu->items[i]->window.flags & WINDOW_DECORATION ) {
					continue;
				}

				if ( Rect_ContainsPoint( &menu->items[i]->window.rect, x, y ) ) {
					itemDef_t *overItem = menu->items[i];
					if ( overItem->type == ITEM_TYPE_TEXT && overItem->text ) {
						if ( Rect_ContainsPoint( Item_CorrectedTextRect( overItem ), x, y ) ) {
							return true;
						} else {
							continue;
						}
					} else {
						return true;
					}
				}
			}

		}
	}
	return false;
}

