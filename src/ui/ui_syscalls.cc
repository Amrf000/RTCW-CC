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

#include "ui_local.h"

// this file is only included when building a dll
// syscalls.asm is included instead when building a qvm

static int ( QDECL * syscall )( int arg, ... ) = ( int ( QDECL * )( int, ... ) ) - 1;

//#if defined( __MACOS__ )
//#pragma export on
//#endif
//extern "C" __declspec(dllexport) void dllEntry( int ( QDECL *syscallptr )( int arg,... ) ) {
//	syscall = syscallptr;
//}
//#if defined( __MACOS__ )
//#pragma export off
//#endif

//int PASSFLOAT(float x);
//int  float x ) {
//	float floatTemp;
//	floatTemp = x;
//	return *(int *)&floatTemp;
//}

void trap_Print( const char *string ) {
	syscall_UI_PRINT( string );
}

//void trap_Error( const char *string ) {
//	syscall_UI_ERROR( string );
//}

//int trap_Milliseconds( void ) {
//	return syscall_UI_MILLISECONDS ();
//}

//void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
//	syscall_UI_CVAR_REGISTER( cvar, var_name, value, flags );
//}

//void trap_Cvar_Update( vmCvar_t *cvar ) {
//	syscall_UI_CVAR_UPDATE( cvar );
//}

//void trap_Cvar_Set( const char *var_name, const char *value ) {
//	syscall_UI_CVAR_SET( var_name, value );
//}

float trap_Cvar_VariableValue( const char *var_name ) {
	int temp;
	temp = syscall_UI_CVAR_VARIABLEVALUE( var_name );
	return ( *(float*)&temp );
}

//void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
//	syscall_UI_CVAR_VARIABLESTRINGBUFFER( var_name, buffer, bufsize );
//}

void trap_Cvar_SetValue( const char *var_name, float value ) {
	syscall_UI_CVAR_SETVALUE( var_name,  value );
}

//void trap_Cvar_Reset( const char *name ) {
//	syscall_UI_CVAR_RESET( name );
//}

//void trap_Cvar_Create( const char *var_name, const char *var_value, int flags ) {
//	syscall_UI_CVAR_CREATE( var_name, var_value, flags );
//}

//void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize ) {
//	syscall_UI_CVAR_INFOSTRINGBUFFER( bit, buffer, bufsize );
//}

//int trap_Argc( void ) {
//	return syscall_UI_ARGC ();
//}

//void trap_Argv( int n, char *buffer, int bufferLength ) {
//	syscall_UI_ARGV( n, buffer, bufferLength );
//}

void trap_Cmd_ExecuteText( int exec_when, const char *text ) {
	syscall_UI_CMD_EXECUTETEXT( exec_when, text );
}

//int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
//	return syscall_UI_FS_FOPENFILE( qpath, f, mode );
//}

//void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
//	syscall_UI_FS_READ( buffer, len, f );
//}

void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	syscall_UI_FS_WRITE( buffer, len, f );
}

//void trap_FS_FCloseFile( fileHandle_t f ) {
//	syscall_UI_FS_FCLOSEFILE( f );
//}

int trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	return syscall_UI_FS_GETFILELIST( path, extension, listbuf, bufsize );
}

int trap_FS_Delete( const char *filename ) {
	return syscall_UI_FS_DELETEFILE( filename );
}

qhandle_t trap_R_RegisterModel( const char *name ) {
	return syscall_UI_R_REGISTERMODEL( name );
}

qhandle_t trap_R_RegisterSkin( const char *name ) {
	return syscall_UI_R_REGISTERSKIN( name );
}

void trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font ) {
	syscall_UI_R_REGISTERFONT( fontName, pointSize, font );
}

qhandle_t trap_R_RegisterShaderNoMip( const char *name ) {
	return syscall_UI_R_REGISTERSHADERNOMIP( name );
}

void trap_R_ClearScene( void ) {
	syscall_UI_R_CLEARSCENE ();
}

void trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	syscall_UI_R_ADDREFENTITYTOSCENE( re );
}

void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	syscall_UI_R_ADDPOLYTOSCENE( hShader, numVerts, verts );
}

void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw ) {
	syscall_UI_R_ADDLIGHTTOSCENE( org, intensity , r ,  g ,  b , overdraw );
}

void trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, bool visible ) {
	syscall_UI_R_ADDCORONATOSCENE( org,  r ,  g ,  b ,  scale , id, visible  );
}

void trap_R_RenderScene( const refdef_t *fd ) {
	syscall_UI_R_RENDERSCENE( fd );
}

//void trap_R_SetColor( const float *rgba ) {
//	syscall_UI_R_SETCOLOR( rgba );
//}

void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	syscall_UI_R_DRAWSTRETCHPIC(  x ,  y ,  w , h ,  s1 , t1 ,  s2 , t2 , hShader );
}

void    trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	syscall_UI_R_MODELBOUNDS( model, mins, maxs );
}

void trap_UpdateScreen( void ) {
	syscall_UI_UPDATESCREEN ();
}

int trap_CM_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex ) {
	return syscall_UI_CM_LERPTAG( tag, refent, tagName, 0 );           // NEFVE - SMF - fixed
}

void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	syscall_UI_S_STARTLOCALSOUND( sfx, channelNum );
}

sfxHandle_t trap_S_RegisterSound( const char *sample ) {
	return syscall_UI_S_REGISTERSOUND( sample );
}

void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	syscall_UI_KEY_KEYNUMTOSTRINGBUF( keynum, buf, buflen );
}

void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	syscall_UI_KEY_GETBINDINGBUF( keynum, buf, buflen );
}

void trap_Key_SetBinding( int keynum, const char *binding ) {
	syscall_UI_KEY_SETBINDING( keynum, binding );
}

bool trap_Key_IsDown( int keynum ) {
	return syscall_UI_KEY_ISDOWN( keynum );
}

bool trap_Key_GetOverstrikeMode( void ) {
	return syscall_UI_KEY_GETOVERSTRIKEMODE ();
}

void trap_Key_SetOverstrikeMode( bool state ) {
	syscall_UI_KEY_SETOVERSTRIKEMODE( state );
}

void trap_Key_ClearStates( void ) {
	syscall_UI_KEY_CLEARSTATES ();
}

int trap_Key_GetCatcher( void ) {
	return syscall_UI_KEY_GETCATCHER ();
}

void trap_Key_SetCatcher( int catcher ) {
	syscall_UI_KEY_SETCATCHER( catcher );
}

void trap_GetClipboardData( char *buf, int bufsize ) {
	syscall_UI_GETCLIPBOARDDATA( buf, bufsize );
}

void trap_GetClientState( uiClientState_t *state ) {
	syscall_UI_GETCLIENTSTATE( state );
}

void trap_GetGlconfig( glconfig_t *glconfig ) {
	syscall_UI_GETGLCONFIG( glconfig );
}

int trap_GetConfigString( int index, char* buff, int buffsize ) {
	return syscall_UI_GETCONFIGSTRING( index, buff, buffsize );
}

//int trap_LAN_GetLocalServerCount( void ) {
//	return syscall_UI_LAN_GETLOCALSERVERCOUNT ();
//}
//
//void trap_LAN_GetLocalServerAddressString( int n, char *buf, int buflen ) {
//	syscall_UI_LAN_GETLOCALSERVERADDRESSSTRING( n, buf, buflen );
//}
//
//int trap_LAN_GetGlobalServerCount( void ) {
//	return syscall_UI_LAN_GETGLOBALSERVERCOUNT ();
//}
//
//void trap_LAN_GetGlobalServerAddressString( int n, char *buf, int buflen ) {
//	syscall_UI_LAN_GETGLOBALSERVERADDRESSSTRING( n, buf, buflen );
//}

int trap_LAN_GetPingQueueCount( void ) {
	return syscall_UI_LAN_GETPINGQUEUECOUNT ();
}

void trap_LAN_ClearPing( int n ) {
	syscall_UI_LAN_CLEARPING( n );
}

void trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	syscall_UI_LAN_GETPING( n, buf, buflen, pingtime );
}

void trap_LAN_GetPingInfo( int n, char *buf, int buflen ) {
	syscall_UI_LAN_GETPINGINFO( n, buf, buflen );
}

// NERVE - SMF
bool trap_LAN_UpdateVisiblePings( int source ) {
	return syscall_UI_LAN_UPDATEVISIBLEPINGS( source );
}

int trap_LAN_GetServerCount( int source ) {
	return syscall_UI_LAN_GETSERVERCOUNT( source );
}

int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	return syscall_UI_LAN_COMPARESERVERS( source, sortKey, sortDir, s1, s2 );
}

void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	syscall_UI_LAN_GETSERVERADDRESSSTRING( source, n, buf, buflen );
}

void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	syscall_UI_LAN_GETSERVERINFO( source, n, buf, buflen );
}

int trap_LAN_AddServer( int source, const char *name, const char *addr ) {
	return syscall_UI_LAN_ADDSERVER( source, name, addr );
}

void trap_LAN_RemoveServer( int source, const char *addr ) {
	syscall_UI_LAN_REMOVESERVER( source, addr );
}

int trap_LAN_GetServerPing( int source, int n ) {
	return syscall_UI_LAN_GETSERVERPING( source, n );
}

int trap_LAN_ServerIsVisible( int source, int n ) {
	return syscall_UI_LAN_SERVERISVISIBLE( source, n );
}

int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen ) {
	return syscall_UI_LAN_SERVERSTATUS( serverAddress, serverStatus, maxLen );
}

void trap_LAN_SaveCachedServers() {
	syscall_UI_LAN_SAVECACHEDSERVERS ();
}

void trap_LAN_LoadCachedServers() {
	syscall_UI_LAN_LOADCACHEDSERVERS ();
}

void trap_LAN_MarkServerVisible( int source, int n, bool visible ) {
	syscall_UI_LAN_MARKSERVERVISIBLE( source, n, visible );
}

// DHM - Nerve :: PunkBuster
void trap_SetPbClStatus( int status ) {
	syscall_UI_SET_PBCLSTATUS( status );
}
// DHM - Nerve

// TTimo: also for Sv
void trap_SetPbSvStatus( int status ) {
	syscall_UI_SET_PBSVSTATUS( status );
}

void trap_LAN_ResetPings( int n ) {
	syscall_UI_LAN_RESETPINGS( n );
}
// -NERVE - SMF

int trap_MemoryRemaining( void ) {
	return syscall_UI_MEMORY_REMAINING ();
}

void trap_GetCDKey( char *buf, int buflen ) {
	syscall_UI_GET_CDKEY( buf, buflen );
}

void trap_SetCDKey( char *buf ) {
	syscall_UI_SET_CDKEY( buf );
}

int trap_PC_AddGlobalDefine( char *define ) {
	return syscall_UI_PC_ADD_GLOBAL_DEFINE( define );
}

int trap_PC_LoadSource( const char *filename ) {
	return syscall_UI_PC_LOAD_SOURCE( filename );
}

int trap_PC_FreeSource( int handle ) {
	return syscall_UI_PC_FREE_SOURCE( handle );
}

int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return syscall_UI_PC_READ_TOKEN( handle, pc_token );
}

int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return syscall_UI_PC_SOURCE_FILE_AND_LINE( handle, filename, line );
}

void trap_S_StopBackgroundTrack( void ) {
	syscall_UI_S_STOPBACKGROUNDTRACK ();
}

void trap_S_StartBackgroundTrack( const char *intro, const char *loop ) {
	syscall_UI_S_STARTBACKGROUNDTRACK( intro, loop );
}

//int trap_RealTime( qtime_t *qtime ) {
//	return syscall_UI_REAL_TIME( qtime );
//}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to false (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits ) {
	return syscall_UI_CIN_PLAYCINEMATIC( arg0, xpos, ypos, width, height, bits );
}

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic( int handle ) {
	return (e_status)syscall_UI_CIN_STOPCINEMATIC( handle );
}


// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic( int handle ) {
	return (e_status)syscall_UI_CIN_RUNCINEMATIC( handle );
}


// draws the current frame
void trap_CIN_DrawCinematic( int handle ) {
	syscall_UI_CIN_DRAWCINEMATIC( handle );
}


// allows you to resize the animation dynamically
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	syscall_UI_CIN_SETEXTENTS( handle, x, y, w, h );
}


void    trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {
	syscall_UI_R_REMAP_SHADER( oldShader, newShader, timeOffset );
}

bool trap_VerifyCDKey( const char *key, const char *chksum ) {
	return syscall_UI_VERIFY_CDKEY( key, chksum );
}

// NERVE - SMF
bool trap_GetLimboString( int index, char *buf ) {
	return syscall_UI_CL_GETLIMBOSTRING( index, buf );
}

#define MAX_VA_STRING       32000

char* trap_TranslateString( const char *string ) {
	static char staticbuf[2][MAX_VA_STRING];
	static int bufcount = 0;
	char *buf;

	buf = staticbuf[bufcount++ % 2];

	syscall_UI_CL_TRANSLATE_STRING( string, buf );

	return buf;
}
// -NERVE - SMF

// DHM - Nerve
void trap_CheckAutoUpdate( void ) {
	syscall_UI_CHECKAUTOUPDATE ();
}

void trap_GetAutoUpdate( void ) {
	syscall_UI_GET_AUTOUPDATE ();
}
// DHM - Nerve

void trap_openURL( const char *s ) {
	syscall_UI_OPENURL( s );
}
