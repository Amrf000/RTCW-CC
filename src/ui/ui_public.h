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

#ifndef __UI_PUBLIC_H__
#define __UI_PUBLIC_H__

#define UI_API_VERSION  4

typedef struct {
	connstate_t connState;
	int connectPacketCount;
	int clientNum;
	char servername[MAX_STRING_CHARS];
	char updateInfoString[MAX_STRING_CHARS];
	char messageString[MAX_STRING_CHARS];
} uiClientState_t;

typedef enum {
	UI_ERROR,
	UI_PRINT,
	UI_MILLISECONDS,
	UI_CVAR_SET,
	UI_CVAR_VARIABLEVALUE,
	UI_CVAR_VARIABLESTRINGBUFFER,
	UI_CVAR_SETVALUE,
	UI_CVAR_RESET,
	UI_CVAR_CREATE,
	UI_CVAR_INFOSTRINGBUFFER,
	UI_ARGC,
	UI_ARGV,
	UI_CMD_EXECUTETEXT,
	UI_FS_FOPENFILE,
	UI_FS_READ,
	UI_FS_WRITE,
	UI_FS_FCLOSEFILE,
	UI_FS_GETFILELIST,
	UI_FS_DELETEFILE,
	UI_R_REGISTERMODEL,
	UI_R_REGISTERSKIN,
	UI_R_REGISTERSHADERNOMIP,
	UI_R_CLEARSCENE,
	UI_R_ADDREFENTITYTOSCENE,
	UI_R_ADDPOLYTOSCENE,
	UI_R_ADDPOLYSTOSCENE,
	// JOSEPH 12-6-99
	UI_R_ADDLIGHTTOSCENE,
	// END JOSEPH
	//----(SA)
	UI_R_ADDCORONATOSCENE,
	//----(SA)
	UI_R_RENDERSCENE,
	UI_R_SETCOLOR,
	UI_R_DRAWSTRETCHPIC,
	UI_UPDATESCREEN,        // 30
	UI_CM_LERPTAG,
	UI_CM_LOADMODEL,
	UI_S_REGISTERSOUND,
	UI_S_STARTLOCALSOUND,
	UI_KEY_KEYNUMTOSTRINGBUF,
	UI_KEY_GETBINDINGBUF,
	UI_KEY_SETBINDING,
	UI_KEY_ISDOWN,
	UI_KEY_GETOVERSTRIKEMODE,
	UI_KEY_SETOVERSTRIKEMODE,
	UI_KEY_CLEARSTATES,
	UI_KEY_GETCATCHER,
	UI_KEY_SETCATCHER,
	UI_GETCLIPBOARDDATA,
	UI_GETGLCONFIG,
	UI_GETCLIENTSTATE,
	UI_GETCONFIGSTRING,
	UI_LAN_GETLOCALSERVERCOUNT,
	UI_LAN_GETLOCALSERVERADDRESSSTRING,
	UI_LAN_GETGLOBALSERVERCOUNT,        // 50
	UI_LAN_GETGLOBALSERVERADDRESSSTRING,
	UI_LAN_GETPINGQUEUECOUNT,
	UI_LAN_CLEARPING,
	UI_LAN_GETPING,
	UI_LAN_GETPINGINFO,
	UI_CVAR_REGISTER,
	UI_CVAR_UPDATE,
	UI_MEMORY_REMAINING,

	UI_GET_CDKEY,
	UI_SET_CDKEY,
	UI_R_REGISTERFONT,
	UI_R_MODELBOUNDS,
	UI_PC_ADD_GLOBAL_DEFINE,
	UI_PC_LOAD_SOURCE,
	UI_PC_FREE_SOURCE,
	UI_PC_READ_TOKEN,
	UI_PC_SOURCE_FILE_AND_LINE,
	UI_S_STOPBACKGROUNDTRACK,
	UI_S_STARTBACKGROUNDTRACK,
	UI_REAL_TIME,
	UI_LAN_GETSERVERCOUNT,
	UI_LAN_GETSERVERADDRESSSTRING,
	UI_LAN_GETSERVERINFO,
	UI_LAN_MARKSERVERVISIBLE,
	UI_LAN_UPDATEVISIBLEPINGS,
	UI_LAN_RESETPINGS,
	UI_LAN_LOADCACHEDSERVERS,
	UI_LAN_SAVECACHEDSERVERS,
	UI_LAN_ADDSERVER,
	UI_LAN_REMOVESERVER,
	UI_CIN_PLAYCINEMATIC,
	UI_CIN_STOPCINEMATIC,
	UI_CIN_RUNCINEMATIC,
	UI_CIN_DRAWCINEMATIC,
	UI_CIN_SETEXTENTS,
	UI_R_REMAP_SHADER,
	UI_VERIFY_CDKEY,
	UI_LAN_SERVERSTATUS,
	UI_LAN_GETSERVERPING,
	UI_LAN_SERVERISVISIBLE,
	UI_LAN_COMPARESERVERS,
	UI_CL_GETLIMBOSTRING,           // NERVE - SMF
	UI_SET_PBCLSTATUS,              // DHM - Nerve
	UI_CHECKAUTOUPDATE,             // DHM - Nerve
	UI_GET_AUTOUPDATE,              // DHM - Nerve
	UI_CL_TRANSLATE_STRING,
	UI_OPENURL,
	UI_SET_PBSVSTATUS,              // TTimo

	UI_MEMSET = 100,
	UI_MEMCPY,
	UI_STRNCPY,
	UI_SIN,
	UI_COS,
	UI_ATAN2,
	UI_SQRT,
	UI_FLOOR,
	UI_CEIL

} uiImport_t;

enum uiMenuCommand_t {
	UIMENU_NONE,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_NEED_CD,
	UIMENU_BAD_CD_KEY,
	UIMENU_TEAM,
	UIMENU_POSTGAME,
	UIMENU_NOTEBOOK,
	UIMENU_CLIPBOARD,
	UIMENU_HELP,
	UIMENU_BOOK1,           //----(SA)	added
	UIMENU_BOOK2,           //----(SA)	added
	UIMENU_BOOK3,           //----(SA)	added
	UIMENU_WM_PICKTEAM,         // NERVE - SMF
	UIMENU_WM_PICKPLAYER,       // NERVE - SMF
	UIMENU_WM_QUICKMESSAGE,     // NERVE - SMF
	UIMENU_WM_QUICKMESSAGEALT,  // NERVE - SMF
	UIMENU_WM_LIMBO,            // NERVE - SMF
	UIMENU_WM_AUTOUPDATE        // NERVE - DHM
} ;

#define SORT_HOST           0
#define SORT_MAP            1
#define SORT_CLIENTS        2
#define SORT_GAME           3
#define SORT_PING           4
#define SORT_PUNKBUSTER     5



int VM_Call_UI_GETAPIVERSION();
int VM_Call_UI_INIT(bool inGameLoad);
int VM_Call_UI_SHUTDOWN();
int VM_Call_UI_KEY_EVENT(int key, bool down);
int VM_Call_UI_MOUSE_EVENT(int dx, int dy);
int VM_Call_UI_REFRESH(int time);
bool VM_Call_UI_IS_FULLSCREEN();
int VM_Call_UI_SET_ACTIVE_MENU(uiMenuCommand_t menu);
int VM_Call_UI_GET_ACTIVE_MENU();
bool VM_Call_UI_CONSOLE_COMMAND(int realTime);
int VM_Call_UI_DRAW_CONNECT_SCREEN(bool overlay);
bool VM_Call_UI_HASUNIQUECDKEY();
bool VM_Call_UI_CHECKEXECKEY(int key);


int syscall_UI_ERROR(const char* fmt);
int syscall_UI_PRINT(const char* fmt);
int syscall_UI_MILLISECONDS();
int syscall_UI_CVAR_REGISTER(vmCvar_t* vmCvar, const char* varName, const char* defaultValue, int flags);
int syscall_UI_CVAR_UPDATE(vmCvar_t* vmCvar);
int syscall_UI_CVAR_SET(const char* var_name, const char* value);
int syscall_UI_CVAR_VARIABLEVALUE(const char* var_name);
int syscall_UI_CVAR_VARIABLESTRINGBUFFER(const char* var_name, char* buffer, int bufsize);
int syscall_UI_CVAR_SETVALUE(const char* var_name, float value);
int syscall_UI_CVAR_RESET(const char* var_name);
int syscall_UI_CVAR_CREATE(const char* var_name, const char* var_value, int flags);
int syscall_UI_CVAR_INFOSTRINGBUFFER(int bit, char* buff, int buffsize);
int syscall_UI_ARGC();
int syscall_UI_ARGV(int arg, char* buffer, int bufferLength);
int syscall_UI_CMD_EXECUTETEXT(int exec_when, const char* text);
int syscall_UI_FS_FOPENFILE(const char* qpath, fileHandle_t* f, fsMode_t mode);
int syscall_UI_FS_READ(void* buffer, int len, fileHandle_t f);
int syscall_UI_FS_WRITE(const void* buffer, int len, fileHandle_t h);
int syscall_UI_FS_FCLOSEFILE(fileHandle_t f);
int syscall_UI_FS_DELETEFILE(const char* filename);
int syscall_UI_FS_GETFILELIST(const char* path, const char* extension, char* listbuf, int bufsize);
int syscall_UI_R_REGISTERMODEL(const char* name);
int syscall_UI_R_REGISTERSKIN(const char* name);
int syscall_UI_R_REGISTERSHADERNOMIP(const char* name);
int syscall_UI_R_CLEARSCENE();
int syscall_UI_R_ADDREFENTITYTOSCENE(const refEntity_t* re1);
int syscall_UI_R_ADDPOLYTOSCENE(qhandle_t hShader, int numVerts, const polyVert_t* verts);
int syscall_UI_R_ADDPOLYSTOSCENE(qhandle_t hShader, int numVerts, const polyVert_t* verts, int numPolys);
int syscall_UI_R_ADDLIGHTTOSCENE(const vec3_t org, float intensity, float r, float g, float b, int overdraw);
int syscall_UI_R_ADDCORONATOSCENE(const vec3_t org, float r, float g, float b, float scale, int id, bool visible);
int syscall_UI_R_RENDERSCENE(const refdef_t* fd);
int syscall_UI_R_SETCOLOR(const float* rgba);
int syscall_UI_R_DRAWSTRETCHPIC(float x, float y, float w, float h,
	float s1, float t1, float s2, float t2, qhandle_t hShader);
int syscall_UI_R_MODELBOUNDS(qhandle_t model, vec3_t mins, vec3_t maxs);
int syscall_UI_UPDATESCREEN();
int syscall_UI_CM_LERPTAG(orientation_t* tag, const refEntity_t* refent, const char* tagName, int startIndex);
int syscall_UI_S_REGISTERSOUND(const char* name);
int syscall_UI_S_STARTLOCALSOUND(sfxHandle_t sfxHandle, int channelNum);
int syscall_UI_KEY_KEYNUMTOSTRINGBUF(int keynum, char* buf, int bufle);
int syscall_UI_KEY_GETBINDINGBUF(int keynum, char* buf, int buflen);
int syscall_UI_KEY_SETBINDING(int keynum, const char* binding);
int syscall_UI_KEY_ISDOWN(int keynum);
int syscall_UI_KEY_GETOVERSTRIKEMODE();
int syscall_UI_KEY_SETOVERSTRIKEMODE(bool state);
int syscall_UI_KEY_CLEARSTATES();
int syscall_UI_KEY_GETCATCHER();
int syscall_UI_KEY_SETCATCHER(int catcher);
int syscall_UI_GETCLIPBOARDDATA(char* buf, int buflen);
int syscall_UI_GETCLIENTSTATE(uiClientState_t* state);
int syscall_UI_GETGLCONFIG(glconfig_t* config);
int syscall_UI_GETCONFIGSTRING(int index, char* buf, int size);
int syscall_UI_LAN_LOADCACHEDSERVERS();
int syscall_UI_LAN_SAVECACHEDSERVERS();
int syscall_UI_LAN_ADDSERVER(int source, const char* name, const char* address);
int syscall_UI_LAN_REMOVESERVER(int source, const char* addr);
int syscall_UI_LAN_GETPINGQUEUECOUNT();
int syscall_UI_LAN_CLEARPING(int n);
int syscall_UI_LAN_GETPING(int n, char* buf, int buflen, int* pingtime);
int syscall_UI_LAN_GETPINGINFO(int n, char* buf, int buflen);
int syscall_UI_LAN_GETSERVERCOUNT(int source);
int syscall_UI_LAN_GETSERVERADDRESSSTRING(int source, int n, char* buf, int buflen);
int syscall_UI_LAN_GETSERVERINFO(int source, int n, char* buf, int buflen);
int syscall_UI_LAN_GETSERVERPING(int source, int n);
int syscall_UI_LAN_MARKSERVERVISIBLE(int source, int n, bool visible);
int syscall_UI_LAN_SERVERISVISIBLE(int source, int n);
int syscall_UI_LAN_UPDATEVISIBLEPINGS(int source);
int syscall_UI_LAN_RESETPINGS(int source);
int syscall_UI_LAN_SERVERSTATUS(const char* serverAddress, char* serverStatus, int maxLen);
int syscall_UI_SET_PBCLSTATUS(int status);
int syscall_UI_SET_PBSVSTATUS(int status);
int syscall_UI_LAN_COMPARESERVERS(int source, int sortKey, int sortDir, int s1, int s2);
int syscall_UI_MEMORY_REMAINING();
int syscall_UI_GET_CDKEY(char* buf, int buflen);
int syscall_UI_SET_CDKEY(char* buf);
int syscall_UI_R_REGISTERFONT(const char* fontName, int pointSize, fontInfo_t* font);
int syscall_UI_MEMSET(void* _Dst, int    _Val, size_t _Size);
int syscall_UI_MEMCPY(void* _Dst, void const* _Src, size_t      _Size);
int syscall_UI_STRNCPY(char* _Destination, char const* _Source, size_t _Count);
int syscall_UI_SIN(double _X);
int syscall_UI_COS(double _X);
int syscall_UI_ATAN2(double _Y, double _X);
int syscall_UI_SQRT(double _X);
int syscall_UI_FLOOR(double _X);
int syscall_UI_CEIL(double _X);

int syscall_UI_PC_ADD_GLOBAL_DEFINE(char* string);
int syscall_UI_PC_LOAD_SOURCE(const char* filename);
int syscall_UI_PC_FREE_SOURCE(int handle);
int syscall_UI_PC_READ_TOKEN(int handle, pc_token_t* pc_token);
int syscall_UI_PC_SOURCE_FILE_AND_LINE(int handle, char* filename, int* line);
int syscall_UI_S_STOPBACKGROUNDTRACK();
int syscall_UI_S_STARTBACKGROUNDTRACK(const char* intro, const char* loop);
int syscall_UI_REAL_TIME(qtime_t* qtime);
int syscall_UI_CIN_PLAYCINEMATIC(const char* arg, int x, int y, int w, int h, int systemBits);
int syscall_UI_CIN_STOPCINEMATIC(int handle);
int syscall_UI_CIN_RUNCINEMATIC(int handle);
int syscall_UI_CIN_DRAWCINEMATIC(int handle);
int syscall_UI_CIN_SETEXTENTS(int handle, int x, int y, int w, int h);
int syscall_UI_R_REMAP_SHADER(const char* oldShader, const char* newShader, const char* offsetTime);
int syscall_UI_VERIFY_CDKEY(const char* key, const char* checksum);
int syscall_UI_CL_GETLIMBOSTRING(int index, char* buf);
int syscall_UI_CL_TRANSLATE_STRING(const char* string, char* dest_buffer);
int syscall_UI_CHECKAUTOUPDATE();
int syscall_UI_GET_AUTOUPDATE();
int syscall_UI_OPENURL(const char* url);

#endif
