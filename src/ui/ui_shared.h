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

#ifndef __UI_SHARED_H
#define __UI_SHARED_H


#include "../game/q_shared.h"
#include "../cgame/tr_types.h"
#include "keycodes.h"

// TTimo case sensitivity
#include "../../MAIN/ui_mp/menudef.h"

#define MAX_MENUNAME 32
#define MAX_ITEMTEXT 64
#define MAX_ITEMACTION 64
#define MAX_MENUDEFFILE 4096
#define MAX_MENUFILE 32768
#define MAX_MENUS 64
//#define MAX_MENUITEMS 256
#define MAX_MENUITEMS 128 // JPW NERVE q3ta was 96
#define MAX_COLOR_RANGES 10
#define MAX_MODAL_MENUS 16

#define WINDOW_MOUSEOVER        0x00000001  // mouse is over it, non exclusive
#define WINDOW_HASFOCUS         0x00000002  // has cursor focus, exclusive
#define WINDOW_VISIBLE          0x00000004  // is visible
#define WINDOW_GREY             0x00000008  // is visible but grey ( non-active )
#define WINDOW_DECORATION       0x00000010  // for decoration only, no mouse, keyboard, etc..
#define WINDOW_FADINGOUT        0x00000020  // fading out, non-active
#define WINDOW_FADINGIN         0x00000040  // fading in
#define WINDOW_MOUSEOVERTEXT    0x00000080  // mouse is over it, non exclusive
#define WINDOW_INTRANSITION     0x00000100  // window is in transition
#define WINDOW_FORECOLORSET     0x00000200  // forecolor was explicitly set ( used to color alpha images or not )
#define WINDOW_HORIZONTAL       0x00000400  // for list boxes and sliders, vertical is default this is set of horizontal
#define WINDOW_LB_LEFTARROW     0x00000800  // mouse is over left/up arrow
#define WINDOW_LB_RIGHTARROW    0x00001000  // mouse is over right/down arrow
#define WINDOW_LB_THUMB         0x00002000  // mouse is over thumb
#define WINDOW_LB_PGUP          0x00004000  // mouse is over page up
#define WINDOW_LB_PGDN          0x00008000  // mouse is over page down
#define WINDOW_ORBITING         0x00010000  // item is in orbit
#define WINDOW_OOB_CLICK        0x00020000  // close on out of bounds click
#define WINDOW_WRAPPED          0x00040000  // manually wrap text
#define WINDOW_AUTOWRAPPED      0x00080000  // auto wrap text
#define WINDOW_FORCED           0x00100000  // forced open
#define WINDOW_POPUP            0x00200000  // popup
#define WINDOW_BACKCOLORSET     0x00400000  // backcolor was explicitly set
#define WINDOW_TIMEDVISIBLE     0x00800000  // visibility timing ( NOT implemented )
#define WINDOW_IGNORE_HUDALPHA  0x01000000  // window will apply cg_hudAlpha value to colors unless this flag is set
#define WINDOW_MODAL                        0x02000000 // window is modal, the window to go back to is stored in a stack

// CGAME cursor type bits
#define CURSOR_NONE             0x00000001
#define CURSOR_ARROW            0x00000002
#define CURSOR_SIZER            0x00000004

#ifdef CGAME
#define STRING_POOL_SIZE    128 * 1024
#else
#define STRING_POOL_SIZE    384 * 1024
#endif

#define MAX_STRING_HANDLES  4096
#define MAX_SCRIPT_ARGS     12
#define MAX_EDITFIELD       256

#define ART_FX_BASE         "menu/art/fx_base"
#define ART_FX_BLUE         "menu/art/fx_blue"
#define ART_FX_CYAN         "menu/art/fx_cyan"
#define ART_FX_GREEN        "menu/art/fx_grn"
#define ART_FX_RED          "menu/art/fx_red"
#define ART_FX_TEAL         "menu/art/fx_teal"
#define ART_FX_WHITE        "menu/art/fx_white"
#define ART_FX_YELLOW       "menu/art/fx_yel"

#define ASSET_GRADIENTBAR           "ui_mp/assets/gradientbar2.tga"
#define ASSET_SCROLLBAR             "ui_mp/assets/scrollbar.tga"
#define ASSET_SCROLLBAR_ARROWDOWN   "ui_mp/assets/scrollbar_arrow_dwn_a.tga"
#define ASSET_SCROLLBAR_ARROWUP     "ui_mp/assets/scrollbar_arrow_up_a.tga"
#define ASSET_SCROLLBAR_ARROWLEFT   "ui_mp/assets/scrollbar_arrow_left.tga"
#define ASSET_SCROLLBAR_ARROWRIGHT  "ui_mp/assets/scrollbar_arrow_right.tga"
#define ASSET_SCROLL_THUMB          "ui_mp/assets/scrollbar_thumb.tga"
#define ASSET_SLIDER_BAR            "ui_mp/assets/slider2.tga"
#define ASSET_SLIDER_THUMB          "ui_mp/assets/sliderbutt_1.tga"

#define SCROLLBAR_SIZE      16.0
#define SLIDER_WIDTH        96.0
#define SLIDER_HEIGHT       16.0
#define SLIDER_THUMB_WIDTH  12.0
#define SLIDER_THUMB_HEIGHT 20.0
#define NUM_CROSSHAIRS      10

typedef struct {
	const char *command;
	const char *args[MAX_SCRIPT_ARGS];
} scriptDef_t;


typedef struct {
	float x;    // horiz position
	float y;    // vert position
	float w;    // width
	float h;    // height;
} rectDef_t;

typedef rectDef_t Rectangle;

// FIXME: do something to separate text vs window stuff
typedef struct {
	Rectangle rect;                 // client coord rectangle
	Rectangle rectClient;           // screen coord rectangle
	const char *name;               //
	const char *model;              //
	const char *group;              // if it belongs to a group
	const char *cinematicName;      // cinematic name
	int cinematic;                  // cinematic handle
	int style;                      //
	int border;                     //
	int ownerDraw;                  // ownerDraw style
	int ownerDrawFlags;             // show flags for ownerdraw items
	float borderSize;               //
	int flags;                      // visible, focus, mouseover, cursor
	Rectangle rectEffects;          // for various effects
	Rectangle rectEffects2;         // for various effects
	int offsetTime;                 // time based value for various effects
	int nextTime;                   // time next effect should cycle
	vec4_t foreColor;               // text color
	vec4_t backColor;               // border color
	vec4_t borderColor;             // border color
	vec4_t outlineColor;            // border color
	qhandle_t background;           // background asset
} windowDef_t;

typedef windowDef_t Window;


typedef struct {
	vec4_t color;
	int type;
	float low;
	float high;
} colorRangeDef_t;

// FIXME: combine flags into bitfields to save space
// FIXME: consolidate all of the common stuff in one structure for menus and items
// THINKABOUTME: is there any compelling reason not to have items contain items
// and do away with a menu per say.. major issue is not being able to dynamically allocate
// and destroy stuff.. Another point to consider is adding an alloc free call for vm's and have
// the engine just allocate the pool for it based on a cvar
// many of the vars are re-used for different item types, as such they are not always named appropriately
// the benefits of c++ in DOOM will greatly help crap like this
// FIXME: need to put a type ptr that points to specific type info per type
//
#define MAX_LB_COLUMNS 16

typedef struct columnInfo_s {
	int pos;
	int width;
	int maxChars;
} columnInfo_t;

typedef struct listBoxDef_s {
	int startPos;
	int endPos;
	int drawPadding;
	int cursorPos;
	float elementWidth;
	float elementHeight;
	int elementStyle;
	int numColumns;
	columnInfo_t columnInfo[MAX_LB_COLUMNS];
	const char *doubleClick;
	bool notselectable;
} listBoxDef_t;

typedef struct editFieldDef_s {
	float minVal;                   //	edit field limits
	float maxVal;                   //
	float defVal;                   //
	float range;                    //
	int maxChars;                   // for edit fields
	int maxPaintChars;              // for edit fields
	int paintOffset;                //
} editFieldDef_t;

#define MAX_MULTI_CVARS 32

typedef struct multiDef_s {
	const char *cvarList[MAX_MULTI_CVARS];
	const char *cvarStr[MAX_MULTI_CVARS];
	float cvarValue[MAX_MULTI_CVARS];
	int count;
	bool strDef;
} multiDef_t;

typedef struct modelDef_s {
	int angle;
	vec3_t origin;
	float fov_x;
	float fov_y;
	int rotationSpeed;

	int animated;
	int startframe;
	int numframes;
	int loopframes;
	int fps;

	int frame;
	int oldframe;
	float backlerp;
	int frameTime;
} modelDef_t;

#define CVAR_ENABLE     0x00000001
#define CVAR_DISABLE    0x00000002
#define CVAR_SHOW       0x00000004
#define CVAR_HIDE       0x00000008
#define CVAR_NOTOGGLE   0x00000010

#define UI_MAX_TEXT_LINES 64

typedef struct itemDef_s {
	Window window;                  // common positional, border, style, layout info
	Rectangle textRect;             // rectangle the text ( if any ) consumes
	int type;                       // text, button, radiobutton, checkbox, textfield, listbox, combo
	int alignment;                  // left center right
	int textalignment;              // ( optional ) alignment for text within rect based on text width
	float textalignx;               // ( optional ) text alignment x coord
	float textaligny;               // ( optional ) text alignment x coord
	float textscale;                // scale percentage from 72pts
	int font;                       // (SA)
	int textStyle;                  // ( optional ) style, normal and shadowed are it for now
	const char *text;   // display text
	void *parent;                   // menu owner
	qhandle_t asset;                // handle to asset
	const char *mouseEnterText;     // mouse enter script
	const char *mouseExitText;      // mouse exit script
	const char *mouseEnter;         // mouse enter script
	const char *mouseExit;          // mouse exit script
	const char *action;             // select script
	const char *onAccept;           // NERVE - SMF - run when the users presses the enter key
	const char *onFocus;            // select script
	const char *leaveFocus;         // select script
	const char *cvar;               // associated cvar
	const char *cvarTest;           // associated cvar for enable actions
	const char *enableCvar;         // enable, disable, show, or hide based on value, this can contain a list
	int cvarFlags;                  //	what type of action to take on cvarenables
	sfxHandle_t focusSound;
	int numColors;                  // number of color ranges
	colorRangeDef_t colorRanges[MAX_COLOR_RANGES];
	int colorRangeType;             // either
	float special;                  // used for feeder id's etc.. diff per type
	int cursorPos;                  // cursor position in characters
	void *typeData;                 // type specific data ptr's
} itemDef_t;

typedef struct {
	Window window;
	const char  *font;              // font
	bool fullScreen;            // covers entire screen
	int itemCount;                  // number of items;
	int fontIndex;                  //
	int cursorItem;                 // which item as the cursor
	int fadeCycle;                  //
	float fadeClamp;                //
	float fadeAmount;               //
	const char *onOpen;             // run when the menu is first opened
	const char *onClose;            // run when the menu is closed
	const char *onESC;              // run when the menu is closed
	const char *onKey[255];         // NERVE - SMF - execs commands when a key is pressed
	const char *soundName;          // background loop sound for menu

	vec4_t focusColor;              // focus color for items
	vec4_t disableColor;            // focus color for items
	itemDef_t *items[MAX_MENUITEMS]; // items this menu contains
} menuDef_t;

typedef struct {
	const char *fontStr;
	const char *cursorStr;
	const char *gradientStr;
	fontInfo_t textFont;
	fontInfo_t smallFont;
	fontInfo_t bigFont;
	qhandle_t cursor;
	qhandle_t gradientBar;
	qhandle_t scrollBarArrowUp;
	qhandle_t scrollBarArrowDown;
	qhandle_t scrollBarArrowLeft;
	qhandle_t scrollBarArrowRight;
	qhandle_t scrollBar;
	qhandle_t scrollBarThumb;
	qhandle_t buttonMiddle;
	qhandle_t buttonInside;
	qhandle_t solidBox;
	qhandle_t sliderBar;
	qhandle_t sliderThumb;
	sfxHandle_t menuEnterSound;
	sfxHandle_t menuExitSound;
	sfxHandle_t menuBuzzSound;
	sfxHandle_t itemFocusSound;
	float fadeClamp;
	int fadeCycle;
	float fadeAmount;
	float shadowX;
	float shadowY;
	vec4_t shadowColor;
	float shadowFadeClamp;
	bool fontRegistered;

	// player settings
	qhandle_t fxBasePic;
	qhandle_t fxPic[7];
	qhandle_t crosshairShader[NUM_CROSSHAIRS];

} cachedAssets_t;
enum uiMenuCommand_t;
struct displayContextDef_t {
	qhandle_t ( *registerShaderNoMip )( const char *p );
	void ( *setColor )( const vec4_t v );
	void ( *drawHandlePic )( float x, float y, float w, float h, qhandle_t asset );
	void ( *drawStretchPic )( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
	void ( *drawText )( float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style );
	int ( *textWidth )( const char *text, float scale, int limit );
	int ( *textHeight )( const char *text, float scale, int limit );
	void ( *textFont )( int font );          // NERVE - SMF
	qhandle_t ( *registerModel )( const char *p );
	void ( *modelBounds )( qhandle_t model, vec3_t min, vec3_t max );
	void ( *fillRect )( float x, float y, float w, float h, const vec4_t color );
	void ( *drawRect )( float x, float y, float w, float h, float size, const vec4_t color );
	void ( *drawSides )( float x, float y, float w, float h, float size );
	void ( *drawTopBottom )( float x, float y, float w, float h, float size );
	void ( *clearScene )();
	void ( *addRefEntityToScene )( const refEntity_t *re );
	void ( *renderScene )( const refdef_t *fd );
	void ( *registerFont )( const char *pFontname, int pointSize, fontInfo_t *font );
	void ( *ownerDrawItem )( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle );
	float ( *getValue )( int ownerDraw, int type );
	bool ( *ownerDrawVisible )( int flags );
	void CG_RunMenuScript(char** args);
	void (displayContextDef_t::*runScript )( char **p );
	void ( *getTeamColor )( vec4_t *color );
	void ( *getCVarString )( const char *cvar, char *buffer, int bufsize );
	float ( *getCVarValue )( const char *cvar );
	void ( *setCVar )( const char *cvar, const char *value );
	void ( *drawTextWithCursor )( float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style );
	void ( *setOverstrikeMode )( bool b );
	bool ( *getOverstrikeMode )();
	void ( *startLocalSound )( sfxHandle_t sfx, int channelNum );
	bool ( *ownerDrawHandleKey )( int ownerDraw, int flags, float *special, int key );
	int ( *feederCount )( float feederID );
	const char *( *feederItemText )( float feederID, int index, int column, qhandle_t * handle );
	const char *( *fileText )( char *flieName );
	qhandle_t ( *feederItemImage )( float feederID, int index );
	void (displayContextDef_t::*feederSelection )( float feederID, int index );
	void ( *feederAddItem )( float feederID, const char *name, int index );           // NERVE - SMF
	char* ( *translateString )( const char *string );                                 // NERVE - SMF
	void ( *checkAutoUpdate )();                                         // DHM - Nerve
	void ( *getAutoUpdate )();                                           // DHM - Nerve

	void ( *keynumToStringBuf )( int keynum, char *buf, int buflen );
	void ( *getBindingBuf )( int keynum, char *buf, int buflen );
	void ( *setBinding )( int keynum, const char *binding );
	void ( *executeText )( int exec_when, const char *text );
	void ( *Error )( int level, const char *error, ... );
	void ( *Print )( const char *msg, ... );
	void ( *Pause )( bool b );
	int ( *ownerDrawWidth )( int ownerDraw, float scale );
//	sfxHandle_t (*registerSound)(const char *name, bool compressed);
	sfxHandle_t ( *registerSound )( const char *name );
	void ( *startBackgroundTrack )( const char *intro, const char *loop );
	void ( *stopBackgroundTrack )();
	int ( *playCinematic )( const char *name, float x, float y, float w, float h );
	void ( *stopCinematic )( int handle );
	void ( *drawCinematic )( int handle, float x, float y, float w, float h );
	void ( *runCinematicFrame )( int handle );

	float yscale;
	float xscale;
	float bias;
	int realTime;
	int frameTime;
	int cursorx;
	int cursory;
	bool debug;

	cachedAssets_t Assets;

	glconfig_t glconfig;
	qhandle_t whiteShader;
	qhandle_t gradientImage;
	qhandle_t cursor;
	float FPS;
	void Fade(int* flags, float* f, float clamp, int* nextTime, int offsetTime, bool bFlags, float fadeAmount);
	void Window_Paint(Window* w, float fadeAmount, float fadeClamp, float fadeCycle);
	void Script_SetBackground(itemDef_t* item, char** args);
	 void Menu_CacheContents(menuDef_t* menu);//static
	 void Item_CacheContents(itemDef_t* item);
	 void Window_CacheContents(windowDef_t* window);
	 void Display_HandleKey(int key, bool down, int x, int y);
	 int Display_CursorType(int x, int y);
	 bool Display_MouseMove(void* p, int x, int y);
	 void* Display_CaptureItem(int x, int y);
	 displayContextDef_t* Display_GetContext();
	 void Menu_Reset();
	 void Menu_PaintAll();
	 void Menu_New(int handle);
	 int  Menu_Count();
	 bool Menu_Parse(int handle, menuDef_t* menu);
	 void Menu_SetupKeywordHash(void);
	 bool MenuParse_modal(itemDef_t* item, int handle);
	 bool MenuParse_execKeyInt(itemDef_t* item, int handle);
	 bool MenuParse_execKey(itemDef_t* item, int handle);
	 bool MenuParse_itemDef(itemDef_t* item, int handle);
	 bool MenuParse_fadeCycle(itemDef_t* item, int handle);
	 bool MenuParse_fadeAmount(itemDef_t* item, int handle);
	 bool MenuParse_fadeClamp(itemDef_t* item, int handle);
	 bool MenuParse_soundLoop(itemDef_t* item, int handle);
	 bool MenuParse_outOfBounds(itemDef_t* item, int handle);
	 bool MenuParse_popup(itemDef_t* item, int handle);
	 bool MenuParse_ownerdraw(itemDef_t* item, int handle);
	 bool MenuParse_ownerdrawFlag(itemDef_t* item, int handle);
	 bool MenuParse_cinematic(itemDef_t* item, int handle);
	 bool MenuParse_background(itemDef_t* item, int handle);
	 bool MenuParse_outlinecolor(itemDef_t* item, int handle);
	 bool MenuParse_disablecolor(itemDef_t* item, int handle); 
	 bool MenuParse_focuscolor(itemDef_t* item, int handle);
	 bool MenuParse_bordercolor(itemDef_t* item, int handle);
	 bool MenuParse_forecolor(itemDef_t* item, int handle);
	 bool MenuParse_backcolor(itemDef_t* item, int handle);
	 bool MenuParse_borderSize(itemDef_t* item, int handle);
	 bool MenuParse_border(itemDef_t* item, int handle);
	 bool MenuParse_onESC(itemDef_t* item, int handle);
	 bool MenuParse_onClose(itemDef_t* item, int handle);
	 bool MenuParse_onOpen(itemDef_t* item, int handle);
	 bool MenuParse_visible(itemDef_t* item, int handle);
	 bool MenuParse_style(itemDef_t* item, int handle);
	 bool MenuParse_rect(itemDef_t* item, int handle);
	 bool MenuParse_fullscreen(itemDef_t* item, int handle);
	 bool MenuParse_name(itemDef_t* item, int handle);
	 bool MenuParse_font(itemDef_t* item, int handle);
	 void Item_InitControls(itemDef_t* item);
	 bool Item_Parse(int handle, itemDef_t* item);
	 void GradientBar_Paint(rectDef_t* rect, vec4_t color);
	 void Window_Init(Window* w);
	 void* UI_Alloc(int size);
	 void UI_InitMemory(void);
	 bool UI_OutOfMemory();
	 void String_Report();
	 void String_Init();
	 void PC_SourceWarning(int handle, char* format, ...);
	 void PC_SourceError(int handle, char* format, ...);
	 void LerpColor(vec4_t a, vec4_t b, vec4_t c, float t);
	 bool Float_Parse(char** p, float* f);
	 bool PC_Float_Parse(int handle, float* f);
	 bool Color_Parse(char** p, vec4_t* c);
	 bool PC_Color_Parse(int handle, vec4_t* c);
	 bool Int_Parse(char** p, int* i);
	 bool PC_Int_Parse(int handle, int* i);
	 bool Rect_Parse(char** p, rectDef_t* r);
	 bool PC_Rect_Parse(int handle, rectDef_t* r);
	 bool String_Parse(char** p, const char** out);
	 bool PC_String_Parse(int handle, const char** out);
	 bool PC_String_Parse_Trans(int handle, const char** out);
	 bool PC_Char_Parse(int handle, char* out);
	 bool PC_Script_Parse(int handle, const char** out);
	 void Init_Display(displayContextDef_t* dc);
	 void Script_Clipboard(itemDef_t* item, char** args);
	 void Menu_ShowItemByName(menuDef_t* menu, const char* p, bool bShow);
	 bool IsVisible(int flags);
	 bool Rect_ContainsPoint(rectDef_t* rect, float x, float y);
	 int Menu_ItemsMatchingGroup(menuDef_t* menu, const char* name);
	 itemDef_t* Menu_GetMatchingItemByNumber(menuDef_t* menu, int index, const char* name);
	 void Script_SetColor(itemDef_t* item, char** args);
	 void Script_SetAsset(itemDef_t* item, char** args);
	 itemDef_t* Menu_FindItemByName(menuDef_t* menu, const char* p);
	 void Script_SetTeamColor(itemDef_t* item, char** args);
	 void Script_SetItemColor(itemDef_t* item, char** args);
	 void Menu_FadeItemByName(menuDef_t* menu, const char* p, bool fadeOut);
	 menuDef_t* Menus_FindByName(const char* p);
	 void Menus_ShowByName(const char* p);
	 void Menus_OpenByName(const char* p);
	 void Menu_RunCloseScript(menuDef_t* menu);
	 void Menus_CloseByName(const char* p);
	 void Menus_CloseAll();
	 void Script_Show(itemDef_t* item, char** args);
	 void Script_Hide(itemDef_t* item, char** args);
	 void Script_FadeIn(itemDef_t* item, char** args);
	 void Script_FadeOut(itemDef_t* item, char** args);
	 void Script_Open(itemDef_t* item, char** args);
	 void Script_ConditionalOpen(itemDef_t* item, char** args);
	 void Script_Close(itemDef_t* item, char** args);
	 void Script_NotebookShowpage(itemDef_t* item, char** args);
	 void Menu_TransitionItemByName(menuDef_t* menu, const char* p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt);
	 void Script_Transition(itemDef_t* item, char** args);
	 void Menu_OrbitItemByName(menuDef_t* menu, const char* p, float x, float y, float cx, float cy, int time);
	 void Script_Orbit(itemDef_t* item, char** args);
	 void Script_SetFocus(itemDef_t* item, char** args);
	 void Script_SetPlayerModel(itemDef_t* item, char** args);
	 void Script_SetPlayerHead(itemDef_t* item, char** args);
	 void Script_ClearCvar(itemDef_t* item, char** args);
	 void Script_SetCvar(itemDef_t* item, char** args);
	 void Script_Exec(itemDef_t* item, char** args);
	 void Script_Play(itemDef_t* item, char** args);
	 void Script_playLooped(itemDef_t* item, char** args);
	 void Script_AddListItem(itemDef_t* item, char** args);
	 void Script_CheckAutoUpdate(itemDef_t* item, char** args);
	 void Script_GetAutoUpdate(itemDef_t* item, char** args);
	 void Item_RunScript(itemDef_t* item, const char* s);
	 bool Item_EnableShowViaCvar(itemDef_t* item, int flag);
	 bool Item_SetFocus(itemDef_t* item, float x, float y);
	 int Item_ListBox_MaxScroll(itemDef_t* item);
	 int Item_ListBox_ThumbPosition(itemDef_t* item);
	 int Item_ListBox_ThumbDrawPosition(itemDef_t* item);
	 float Item_Slider_ThumbPosition(itemDef_t* item);
	 int Item_Slider_OverSlider(itemDef_t* item, float x, float y);
	 int Item_ListBox_OverLB(itemDef_t* item, float x, float y);
	 void Item_ListBox_MouseEnter(itemDef_t* item, float x, float y);
	 void Item_MouseEnter(itemDef_t* item, float x, float y);
	 void Item_MouseLeave(itemDef_t* item);
	 itemDef_t* Menu_HitTest(menuDef_t* menu, float x, float y);
	 void Item_SetMouseOver(itemDef_t* item, bool focus);
	 bool Item_OwnerDraw_HandleKey(itemDef_t* item, int key);
	 bool Item_ListBox_HandleKey(itemDef_t* item, int key, bool down, bool force);
	 bool Item_YesNo_HandleKey(itemDef_t* item, int key);
	 int Item_Multi_CountSettings(itemDef_t* item);
	 int Item_Multi_FindCvarByValue(itemDef_t* item);
	 const char* Item_Multi_Setting(itemDef_t* item);
	 bool Item_Multi_HandleKey(itemDef_t* item, int key);
	 bool Item_TextField_HandleKey(itemDef_t* item, int key);
	 void Scroll_ListBox_AutoFunc(void* p);
	 void Scroll_ListBox_ThumbFunc(void* p);
	 void Scroll_Slider_ThumbFunc(void* p);
	 void Item_StartCapture(itemDef_t* item, int key);
	 void Item_StopCapture(itemDef_t* item);
	 bool Item_Slider_HandleKey(itemDef_t* item, int key, bool down);
	 bool Item_HandleKey(itemDef_t* item, int key, bool down);
	 void Item_Action(itemDef_t* item);
	 itemDef_t* Menu_SetPrevCursorItem(menuDef_t* menu);
	 itemDef_t* Menu_SetNextCursorItem(menuDef_t* menu);
	 void Window_CloseCinematic(windowDef_t* window);
	 void Menu_CloseCinematics(menuDef_t* menu);
	 void Display_CloseCinematics();
	 void Menus_Activate(menuDef_t* menu);
	 int Display_VisibleMenuCount();
	 void Menus_HandleOOBClick(menuDef_t* menu, int key, bool down);
	 rectDef_t* Item_CorrectedTextRect(itemDef_t* item);
	 void Menu_HandleKey(menuDef_t* menu, int key, bool down);
	 void ToWindowCoords(float* x, float* y, windowDef_t* window);
	 void Rect_ToWindowCoords(rectDef_t* rect, windowDef_t* window);
	 void Item_SetTextExtents(itemDef_t* item, int* width, int* height, const char* text);
	 void Item_TextColor(itemDef_t* item, vec4_t* newColor);
	 void Item_Text_AutoWrapped_Paint(itemDef_t* item);
	 void Item_Text_Wrapped_Paint(itemDef_t* item);
	 void Item_Text_Paint(itemDef_t* item);
	 void Item_TextField_Paint(itemDef_t* item);
	 void Item_YesNo_Paint(itemDef_t* item);
	 void Item_Multi_Paint(itemDef_t* item);
	 void Controls_GetKeyAssignment(char* command, int* twokeys);
	 void Controls_GetConfig(void);
	 void Controls_SetConfig(bool restart);
	 void Controls_SetDefaults(void);
	 int BindingIDFromName(const char* name);
	 char* BindingFromName(const char* cvar);
	 void Item_Slider_Paint(itemDef_t* item);
	 void Item_Bind_Paint(itemDef_t* item);
	 bool Display_KeyBindPending();
	 bool Item_Bind_HandleKey(itemDef_t* item, int key, bool down);
	 void AdjustFrom640(float* x, float* y, float* w, float* h);
	 void Item_Model_Paint(itemDef_t* item);
	 void Item_Image_Paint(itemDef_t* item);
	 void Item_ListBox_Paint(itemDef_t* item);
	 void Item_OwnerDraw_Paint(itemDef_t* item);
	 void Item_Paint(itemDef_t* item);
	 void Menu_Init(menuDef_t* menu);
	 itemDef_t* Menu_GetFocusedItem(menuDef_t* menu);
	 menuDef_t* Menu_GetFocused();
	 void Menu_ScrollFeeder(menuDef_t* menu, int feeder, bool down);
	 void Menu_SetFeederSelection(menuDef_t* menu, int feeder, int index, const char* name);
	 bool Menus_AnyFullScreenVisible();
	 menuDef_t* Menus_ActivateByName(const char* p, bool modalStack);
	 void Item_Init(itemDef_t* item);
	 void Menu_HandleMouseMove(menuDef_t* menu, float x, float y);
	 void Menu_Paint(menuDef_t* menu, bool forcePaint);
	 void Item_ValidateTypeData(itemDef_t* item);
	 bool ItemParse_name(itemDef_t* item, int handle);
	 bool ItemParse_focusSound(itemDef_t* item, int handle);
	 bool ItemParse_text(itemDef_t* item, int handle);
	 bool ItemParse_textfile(itemDef_t* item, int handle);
	 bool ItemParse_group(itemDef_t* item, int handle);
	 bool ItemParse_asset_model(itemDef_t* item, int handle);
	 bool ItemParse_asset_shader(itemDef_t* item, int handle);
	 bool ItemParse_model_origin(itemDef_t* item, int handle);
	 bool ItemParse_model_fovx(itemDef_t* item, int handle);
	 bool ItemParse_model_fovy(itemDef_t* item, int handle);
	 bool ItemParse_model_rotation(itemDef_t* item, int handle);
	 bool ItemParse_model_angle(itemDef_t* item, int handle);
	 bool ItemParse_model_animplay(itemDef_t* item, int handle);
	 bool ItemParse_rect(itemDef_t* item, int handle);
	 bool ItemParse_origin(itemDef_t* item, int handle);
	 bool ItemParse_style(itemDef_t* item, int handle);
	 bool ItemParse_decoration(itemDef_t* item, int handle);
	 bool ItemParse_notselectable(itemDef_t* item, int handle);
	 bool ItemParse_wrapped(itemDef_t* item, int handle);
	 bool ItemParse_autowrapped(itemDef_t* item, int handle);
	 bool ItemParse_horizontalscroll(itemDef_t* item, int handle);
	 bool ItemParse_type(itemDef_t* item, int handle);
	 bool ItemParse_elementwidth(itemDef_t* item, int handle);
	 bool ItemParse_elementheight(itemDef_t* item, int handle);
	 bool ItemParse_feeder(itemDef_t* item, int handle);
	 bool ItemParse_elementtype(itemDef_t* item, int handle);
	 bool ItemParse_columns(itemDef_t* item, int handle);
	 bool ItemParse_border(itemDef_t* item, int handle);
	 bool ItemParse_bordersize(itemDef_t* item, int handle);
	 bool ItemParse_visible(itemDef_t* item, int handle);
	 bool ItemParse_ownerdraw(itemDef_t* item, int handle);
	 bool ItemParse_align(itemDef_t* item, int handle);
	 bool ItemParse_textalign(itemDef_t* item, int handle);
	 bool ItemParse_textalignx(itemDef_t* item, int handle);
	 bool ItemParse_textaligny(itemDef_t* item, int handle);
	 bool ItemParse_textscale(itemDef_t* item, int handle);
	 bool ItemParse_textstyle(itemDef_t* item, int handle);
	 bool ItemParse_textfont(itemDef_t* item, int handle);
	 bool ItemParse_backcolor(itemDef_t* item, int handle);
	 bool ItemParse_forecolor(itemDef_t* item, int handle);
	 bool ItemParse_bordercolor(itemDef_t* item, int handle);
	 bool ItemParse_outlinecolor(itemDef_t* item, int handle);
	 bool ItemParse_background(itemDef_t* item, int handle);
	 bool ItemParse_cinematic(itemDef_t* item, int handle);
	 bool ItemParse_doubleClick(itemDef_t* item, int handle);
	 bool ItemParse_onFocus(itemDef_t* item, int handle);
	 bool ItemParse_leaveFocus(itemDef_t* item, int handle);
	 bool ItemParse_mouseEnter(itemDef_t* item, int handle);
	 bool ItemParse_mouseExit(itemDef_t* item, int handle);
	 bool	 ItemParse_mouseEnterText(itemDef_t* item, int handle);
	 bool ItemParse_mouseExitText(itemDef_t* item, int handle);
	 bool ItemParse_action(itemDef_t* item, int handle);
	 bool ItemParse_accept(itemDef_t* item, int handle);
	 bool ItemParse_special(itemDef_t* item, int handle);
	 bool ItemParse_cvarTest(itemDef_t* item, int handle);
	 bool ItemParse_cvar(itemDef_t* item, int handle);
	 bool ItemParse_maxChars(itemDef_t* item, int handle);
	 bool ItemParse_maxPaintChars(itemDef_t* item, int handle);
	 bool ItemParse_cvarFloat(itemDef_t* item, int handle);
	 bool ItemParse_cvarStrList(itemDef_t* item, int handle);
	 bool ItemParse_cvarFloatList(itemDef_t* item, int handle);
	 bool ParseColorRange(itemDef_t* item, int handle, int type);
	 bool ItemParse_addColorRangeRel(itemDef_t* item, int handle);
	 bool ItemParse_addColorRange(itemDef_t* item, int handle);
	 bool ItemParse_ownerdrawFlag(itemDef_t* item, int handle);
	 bool ItemParse_enableCvar(itemDef_t* item, int handle);
	 bool ItemParse_disableCvar(itemDef_t* item, int handle);
	 bool  ItemParse_noToggle(itemDef_t* item, int handle);
	 bool ItemParse_showCvar(itemDef_t* item, int handle);
	 bool ItemParse_hideCvar(itemDef_t* item, int handle);
	 void Item_SetupKeywordHash(void);
	 void Display_CacheAll();
	 bool Menu_OverActiveItem(menuDef_t* menu, float x, float y);



	 void WM_setWeaponPics();
	 void WM_SetObjective(int objectiveIndex);
	 void _UI_SetActiveMenu(uiMenuCommand_t menu);
	 void WM_setItemPic(char* name, const char* shader);
	 void WM_setVisibility(char* name, bool show);
	 void _UI_Init(bool inGameLoad);
	 void UI_DrawConnectScreen(bool overlay);
	 void WM_ChangePlayerType();
	 void WM_ActivateLimboChat();
	 void UI_FeederSelection(float feederID, int index);
	 void WM_PickItem(int selectionType, int itemIndex);
	 void WM_SetDefaultWeapon();
	 void UI_RunMenuScript(char** args);
	 void CG_FeederSelection(float feederID, int index);
	 void UI_BuildFindPlayerList(bool force);
	 void _UI_Refresh(int realtime);
	 const char* String_Alloc(const char* p);

	 void Item_SetScreenCoords(itemDef_t* item, float x, float y);
	 void Item_UpdatePosition(itemDef_t* item);
	 void Menu_UpdatePosition(menuDef_t* menu);
	 void Menu_PostParse(menuDef_t* menu);
	 itemDef_t* Menu_ClearFocus(menuDef_t* menu);

};
struct commandDef_t {
	const char* name;
	void (displayContextDef_t::* handler)(itemDef_t* item, char** args);
};


int         trap_PC_AddGlobalDefine( char *define );
int         trap_PC_LoadSource( const char *filename );
int         trap_PC_FreeSource( int handle );
int         trap_PC_ReadToken( int handle, pc_token_t *pc_token );
int         trap_PC_SourceFileAndLine( int handle, char *filename, int *line );

#endif
