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

// cl_cgame.c  -- client system interaction with client game

#include "client.h"

#include "../game/botlib.h"

extern botlib_export_t *botlib_export;

extern bool loadCamera( int camNum, const char *name );
extern void startCamera( int camNum, int time );
extern bool getCameraInfo( int camNum, int time, float* origin, float* angles, float *fov );

// RF, this is only used when running a local server
extern void SV_SendMoveSpeedsToGame( int entnum, char *text );

// NERVE - SMF
void Key_GetBindingBuf( int keynum, char *buf, int buflen );
void Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
// -NERVE - SMF



/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState( gameState_t *gs ) {
	*gs = cl.gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
void CL_GetGlconfig( glconfig_t *glconfig ) {
	*glconfig = cls.glconfig;
}


/*
====================
CL_GetUserCmd
====================
*/
bool CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cmdNumber > cl.cmdNumber ) {
		Com_Error( ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cmdNumber <= cl.cmdNumber - CMD_BACKUP ) {
		return false;
	}

	*ucmd = cl.cmds[ cmdNumber & CMD_MASK ];

	return true;
}

int CL_GetCurrentCmdNumber( void ) {
	return cl.cmdNumber;
}


/*
====================
CL_GetParseEntityState
====================
*/
bool    CL_GetParseEntityState( int parseEntityNumber, entityState_t *state ) {
	// can't return anything that hasn't been parsed yet
	if ( parseEntityNumber >= cl.parseEntitiesNum ) {
		Com_Error( ERR_DROP, "CL_GetParseEntityState: %i >= %i",
				   parseEntityNumber, cl.parseEntitiesNum );
	}

	// can't return anything that has been overwritten in the circular buffer
	if ( parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES ) {
		return false;
	}

	*state = cl.parseEntities[ parseEntityNumber & ( MAX_PARSE_ENTITIES - 1 ) ];
	return true;
}

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void    CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	*snapshotNumber = cl.snap.messageNum;
	*serverTime = cl.snap.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
bool    CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	clSnapshot_t    *clSnap;
	int i, count;

	if ( snapshotNumber > cl.snap.messageNum ) {
		Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP ) {
		return false;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
	if ( !clSnap->valid ) {
		return false;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES ) {
		return false;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->ps = clSnap->ps;
	count = clSnap->numEntities;
	if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
		Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
	for ( i = 0 ; i < count ; i++ ) {
		snapshot->entities[i] =
			cl.parseEntities[ ( clSnap->parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 ) ];
	}

	// FIXME: configstring changes and server commands!!!

	return true;
}

/*
==============
CL_SetUserCmdValue
==============
*/
void CL_SetUserCmdValue( int userCmdValue, int holdableValue, float sensitivityScale, int mpSetup, int mpIdentClient ) {
	cl.cgameUserCmdValue        = userCmdValue;
	cl.cgameUserHoldableValue   = holdableValue;
	cl.cgameSensitivity         = sensitivityScale;
	cl.cgameMpSetup             = mpSetup;              // NERVE - SMF
	cl.cgameMpIdentClient       = mpIdentClient;        // NERVE - SMF
}

/*
==================
CL_SetClientLerpOrigin
==================
*/
void CL_SetClientLerpOrigin( float x, float y, float z ) {
	cl.cgameClientLerpOrigin[0] = x;
	cl.cgameClientLerpOrigin[1] = y;
	cl.cgameClientLerpOrigin[2] = z;
}

/*
==============
CL_AddCgameCommand
==============
*/
void CL_AddCgameCommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}

/*
==============
CL_CgameError
==============
*/
void CL_CgameError( const char *string ) {
	Com_Error( ERR_DROP, "%s", string );
}


/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( void ) {
	char        *old, *s;
	int i, index;
	char        *dup;
	gameState_t oldGs;
	int len;

	index = atoi( Cmd_Argv( 1 ) );
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "configstring > MAX_CONFIGSTRINGS" );
	}
//	s = Cmd_Argv(2);
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom( 2 );

	old = cl.gameState.stringData + cl.gameState.stringOffsets[ index ];
	if ( !strcmp( old, s ) ) {
		return;     // unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	memset( &cl.gameState, 0, sizeof( cl.gameState ) );

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( i == index ) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}
		if ( !dup[0] ) {
			continue;       // leave with the default empty string
		}

		len = strlen( dup );

		if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
			Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
		memcpy( cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1 );
		cl.gameState.dataCount += len + 1;
	}

	if ( index == CS_SYSTEMINFO ) {
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}


/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
bool CL_GetServerCommand( int serverCommandNumber ) {
	char    *s;
	char    *cmd;
	static char bigConfigString[BIG_INFO_STRING];
	int argc;

	// if we have irretrievably lost a reliable command, drop the connection
	if ( serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS ) {
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if ( clc.demoplaying ) {
			return false;
		}
		Com_Error( ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out" );
		return false;
	}

	if ( serverCommandNumber > clc.serverCommandSequence ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: requested a command not received" );
		return false;
	}

	s = clc.serverCommands[ serverCommandNumber & ( MAX_RELIABLE_COMMANDS - 1 ) ];
	clc.lastExecutedServerCommand = serverCommandNumber;

	if ( cl_showServerCommands->integer ) {         // NERVE - SMF
		Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );
	}

rescan:
	Cmd_TokenizeString( s );
	cmd = Cmd_Argv( 0 );
	argc = Cmd_Argc();

	if ( !strcmp( cmd, "disconnect" ) ) {
		// NERVE - SMF - allow server to indicate why they were disconnected
		if ( argc >= 2 ) {
			Com_Error( ERR_SERVERDISCONNECT, va( "Server Disconnected - %s", Cmd_Argv( 1 ) ) );
		} else {
			Com_Error( ERR_SERVERDISCONNECT,"Server disconnected\n" );
		}
	}

	if ( !strcmp( cmd, "bcs0" ) ) {
		Com_sprintf( bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv( 1 ), Cmd_Argv( 2 ) );
		return false;
	}

	if ( !strcmp( cmd, "bcs1" ) ) {
		s = Cmd_Argv( 2 );
		if ( strlen( bigConfigString ) + strlen( s ) >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		return false;
	}

	if ( !strcmp( cmd, "bcs2" ) ) {
		s = Cmd_Argv( 2 );
		if ( strlen( bigConfigString ) + strlen( s ) + 1 >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		strcat( bigConfigString, "\"" );
		s = bigConfigString;
		goto rescan;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		return true;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		memset( cl.cmds, 0, sizeof( cl.cmds ) );
		return true;
	}

	if ( !strcmp( cmd, "popup" ) ) { // direct server to client popup request, bypassing cgame
//		trap_UI_Popup(Cmd_Argv(1));
//		if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
//			VM_Call_UI_SET_ACTIVE_MENU( UIMENU_CLIPBOARD);
//			Menus_OpenByName(Cmd_Argv(1));
//		}
		return false;
	}


	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make apropriate adjustments,
	// but we also clear the console and notify lines here
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if ( !com_sv_running->integer ) {
			return false;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText( "wait ; wait ; wait ; wait ; screenshot levelshot\n" );
		return true;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return true;
}

// DHM - Nerve :: Copied from server to here
/*
====================
CL_SetExpectedHunkUsage

  Sets com_expectedhunkusage, so the client knows how to draw the percentage bar
====================
*/
void CL_SetExpectedHunkUsage( const char *mapname ) {
	int handle;
	char *memlistfile = "hunkusage.dat";
	char *buf;
	char *buftrav;
	char *token;
	int len;

	len = FS_FOpenFileByMode( memlistfile, &handle, FS_READ );
	if ( len >= 0 ) { // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value

		buf = (char *)Z_Malloc( len + 1 );
		memset( buf, 0, len + 1 );

		FS_Read( (void *)buf, len, handle );
		FS_FCloseFile( handle );

		// now parse the file, filtering out the current map
		buftrav = buf;
		while ( ( token = COM_Parse( &buftrav ) ) && token[0] ) {
			if ( !Q_strcasecmp( token, (char *)mapname ) ) {
				// found a match
				token = COM_Parse( &buftrav );  // read the size
				if ( token && token[0] ) {
					// this is the usage
					Cvar_Set( "com_expectedhunkusage", token );
					Z_Free( buf );
					return;
				}
			}
		}

		Z_Free( buf );
	}
	// just set it to a negative number,so the cgame knows not to draw the percent bar
	Cvar_Set( "com_expectedhunkusage", "-1" );
}

// dhm - nerve

/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap( const char *mapname ) {
	int checksum;

	// DHM - Nerve :: If we are not running the server, then set expected usage here
	if ( !com_sv_running->integer ) {
		CL_SetExpectedHunkUsage( mapname );
	} else
	{
		// TTimo
		// catch here when a local server is started to avoid outdated com_errorDiagnoseIP
		Cvar_Set( "com_errorDiagnoseIP", "" );
	}

	CM_LoadMap( mapname, true, &checksum );
}

/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame( void ) {
	cls.keyCatchers &= ~KEYCATCH_CGAME;
	cls.cgameStarted = false;
	if ( !cgvm ) {
		return;
	}
	VM_Call_CG_SHUTDOWN();
	//VM_Free( cgvm );
	//cgvm = NULL;
	cgvm = false;
}

static int  FloatAsInt( float f ) {
	int temp;

	*(float *)&temp = f;

	return temp;
}

/*
====================
CL_CgameSystemCalls

The cgame module is making a system call
====================
*/
//#define VMA( x ) VM_ArgPtr( args[x] )
//#define VMF( x )  ( (float *)args )[x]
//int CL_CgameSystemCalls( int *args ) {
//	switch ( args[0] ) {
//		// - NERVE - SMF
//	default:
//		Com_Error(ERR_DROP, "Bad cgame system trap: %i", args[0]);
//	}
//	return 0;
//}
	int syscall_CG_PRINT(const char* fmt) {
		Com_Printf( "%s", fmt);
		return 0;
	}
	int syscall_CG_ERROR(const char* fmt) {
		Com_Error( ERR_DROP, "%s", fmt);
		return 0;
	}
	int syscall_CG_MILLISECONDS() {
		return Sys_Milliseconds();
	}
	int syscall_CG_CVAR_REGISTER(vmCvar_t* vmCvar, const char* varName, const char* defaultValue, int flags) {
		Cvar_Register(vmCvar, varName, defaultValue, flags);
		return 0;
	}
	int syscall_CG_CVAR_UPDATE(vmCvar_t* vmCvar) {
		Cvar_Update(vmCvar);
		return 0;
	}
	int syscall_CG_CVAR_SET(const char* var_name, const char* value) {
		Cvar_Set(var_name, value);
		return 0;
	}
	int syscall_CG_CVAR_VARIABLESTRINGBUFFER(const char* var_name, char* buffer, int bufsize) {
		Cvar_VariableStringBuffer(var_name, buffer, bufsize);
		return 0;
	}
	int syscall_CG_ARGC() {
		return Cmd_Argc();
	}
	int syscall_CG_ARGV(int arg, char* buffer, int bufferLength) {
		Cmd_ArgvBuffer(arg, buffer, bufferLength);
		return 0;
	}
	int syscall_CG_ARGS(char* buffer, int bufferLength) {
		Cmd_ArgsBuffer(buffer, bufferLength);
		return 0;
	}
	int syscall_CG_FS_FOPENFILE(const char* qpath, fileHandle_t* f, fsMode_t mode) {
		return FS_FOpenFileByMode(qpath, f, mode);
	}
	int syscall_CG_FS_READ(void* buffer, int len, fileHandle_t f) {
		FS_Read(buffer, len, f);
		return 0;
	}
	int syscall_CG_FS_WRITE(const void* buffer, int len, fileHandle_t h) {
		return FS_Write(buffer, len, h);
	}
	int syscall_CG_FS_FCLOSEFILE(fileHandle_t f) {
		FS_FCloseFile(f);
		return 0;
	}
	int syscall_CG_SENDCONSOLECOMMAND(const char* text) {
		Cbuf_AddText(text);
		return 0;
	}
	int syscall_CG_ADDCOMMAND(const char* cmdName) {
		CL_AddCgameCommand(cmdName);
		return 0;
	}
	int syscall_CG_REMOVECOMMAND(const char* cmd_name) {
		Cmd_RemoveCommand(cmd_name);
		return 0;
	}
	int syscall_CG_SENDCLIENTCOMMAND(const char* cmd) {
		CL_AddReliableCommand(cmd);
		return 0;
	}
	int syscall_CG_UPDATESCREEN() {
		// this is used during lengthy level loading, so pump message loop
//		Com_EventLoop();	// FIXME() { if a server restarts here, BAD THINGS HAPPEN!
// We can't call Com_EventLoop here, a restart will crash and this _does_ happen
// if there is a map change while we are downloading at pk3.
// ZOID
		SCR_UpdateScreen();
		return 0;
	}
	int syscall_CG_CM_LOADMAP(const char* mapname) {
		CL_CM_LoadMap(mapname);
		return 0;
	}
	int syscall_CG_CM_NUMINLINEMODELS() {
		return CM_NumInlineModels();
	}
	clipHandle_t syscall_CG_CM_INLINEMODEL(int index) {
		return CM_InlineModel(index);
	}
	clipHandle_t syscall_CG_CM_TEMPBOXMODEL(const vec3_t mins, const vec3_t maxs) {
		return CM_TempBoxModel(mins, maxs, false );
	}
	int syscall_CG_CM_TEMPCAPSULEMODEL(const vec3_t mins, const vec3_t maxs) {
		return CM_TempBoxModel(mins, maxs, true );
	}
	int syscall_CG_CM_POINTCONTENTS(const vec3_t p, clipHandle_t model) {
		return CM_PointContents(p, model);
	}
	int syscall_CG_CM_TRANSFORMEDPOINTCONTENTS(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles) {
		return CM_TransformedPointContents(p, model, origin, angles);
	}
	int syscall_CG_CM_BOXTRACE(trace_t* results, const vec3_t start, const vec3_t end,
		const vec3_t mins, const vec3_t maxs,
		clipHandle_t model, int brushmask) {
		CM_BoxTrace(results, start, end, mins, maxs, model, brushmask, /*int capsule*/ false );
		return 0;
	}
	int syscall_CG_CM_TRANSFORMEDBOXTRACE(trace_t* results, const vec3_t start, const vec3_t end,
		const vec3_t mins, const vec3_t maxs,
		clipHandle_t model, int brushmask,
		const vec3_t origin, const vec3_t angles) {
		CM_TransformedBoxTrace(results, start, end, mins, maxs, model , brushmask, origin, angles, /*int capsule*/ false );
		return 0;
	}
	int syscall_CG_CM_CAPSULETRACE(trace_t* results, const vec3_t start, const vec3_t end,
		const vec3_t mins, const vec3_t maxs,
		clipHandle_t model, int brushmask) {
		CM_BoxTrace(results, start, end, mins, maxs, model, brushmask, /*int capsule*/ true );
		return 0;
	}
	int syscall_CG_CM_TRANSFORMEDCAPSULETRACE(trace_t* results, const vec3_t start, const vec3_t end,
		const vec3_t mins, const vec3_t maxs,
		clipHandle_t model, int brushmask,
		const vec3_t origin, const vec3_t angles) {
		CM_TransformedBoxTrace(results, start, end, mins, maxs, model, brushmask, origin, angles, /*int capsule*/ true );
		return 0;
	}
	int syscall_CG_CM_MARKFRAGMENTS(int numPoints, const vec3_t* points, const vec3_t projection,
		int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t* fragmentBuffer) {
		return re.MarkFragments(numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer);
	}
	int syscall_CG_S_STARTSOUND(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle) {
		S_StartSound(origin, entityNum, entchannel, sfxHandle);
		return 0;
//----(SA)	added
	}
	int syscall_CG_S_STARTSOUNDEX(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle, int flags) {
		S_StartSoundEx(origin, entityNum, entchannel, sfxHandle, flags);
		return 0;
//----(SA)	end
	}
	int syscall_CG_S_STARTLOCALSOUND(sfxHandle_t sfxHandle, int channelNum) {
		S_StartLocalSound(sfxHandle, channelNum);
		return 0;
	}
	int syscall_CG_S_CLEARLOOPINGSOUNDS(bool killall) {
		S_ClearLoopingSounds(); // (SA) modified so no_pvs sounds can function
		return 0;
	}
	int syscall_CG_S_ADDLOOPINGSOUND(int entityNum, const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfxHandle, int volume) {
		// FIXME MrE() { handling of looping sounds changed
		S_AddLoopingSound(entityNum, origin, velocity, range, sfxHandle, volume);
		return 0;
	}
	int syscall_CG_S_ADDREALLOOPINGSOUND(int entityNum, const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfxHandle, int volume) {
		S_AddLoopingSound(entityNum, origin, velocity, range, sfxHandle, volume);
		//S_AddRealLoopingSound( args[1], VMA(2), VMA(3), args[4], args[5] );
		return 0;
	}
	int syscall_CG_S_STOPLOOPINGSOUND(int entityNum) {
		// RF, not functional anymore, since we reverted to old looping code
		//S_StopLoopingSound( entityNum );
		return 0;
	}
	int syscall_CG_S_UPDATEENTITYPOSITION(int entityNum, const vec3_t origin) {
		S_UpdateEntityPosition(entityNum, origin);
		return 0;
// Ridah, talking animations
	}
	int syscall_CG_S_GETVOICEAMPLITUDE(int entityNum) {
		return S_GetVoiceAmplitude(entityNum);
// done.
	}
	int syscall_CG_S_RESPATIALIZE(int entityNum, const vec3_t head, vec3_t axis[3], int inwater) {
		S_Respatialize(entityNum, head, axis, inwater);
		return 0;
	}
	int syscall_CG_S_REGISTERSOUND(const char* name) {
#ifdef DOOMSOUND    ///// (SA) DOOMSOUND
		return S_RegisterSound(name );
#else
		return S_RegisterSound(name, false );
#endif  ///// (SA) DOOMSOUND
	}

	int syscall_CG_S_STARTBACKGROUNDTRACK(const char* intro, const char* loop) {
		S_StartBackgroundTrack(intro, loop);
		return 0;
	}
	int syscall_CG_S_STARTSTREAMINGSOUND(const char* intro, const char* loop, int entnum, int channel, int attenuation) {
		S_StartStreamingSound(intro, loop, entnum, channel, attenuation);
		return 0;
	}



	int syscall_CG_R_LOADWORLDMAP(const char* name) {
		re.LoadWorld(name);
		return 0;
	}
	int syscall_CG_R_REGISTERMODEL(const char* name) {
		return re.RegisterModel(name);
	}
	int syscall_CG_R_REGISTERSKIN(const char* name) {
		return re.RegisterSkin(name);

		//----(SA)	added
	}

	int syscall_CG_R_GETSKINMODEL(qhandle_t skinid, const char* type, char* name) {
		return re.GetSkinModel(skinid, type, name);
	}
	int syscall_CG_R_GETMODELSHADER(qhandle_t modelid, int surfnum, int withlightmap) {
		return re.GetShaderFromModel(modelid, surfnum, withlightmap);
		//----(SA)	end

	}
	int syscall_CG_R_REGISTERSHADER(const char* name) {
		return re.RegisterShader(name);
	}

	int syscall_CG_R_REGISTERFONT(const char* fontName, int pointSize, fontInfo_t* font) {
		re.RegisterFont(fontName, pointSize, font);
		return 0;
	}
	int syscall_CG_R_REGISTERSHADERNOMIP(const char* name) {
		return re.RegisterShaderNoMip(name);
	}



	int syscall_CG_R_CLEARSCENE() {
		re.ClearScene();
		return 0;
	}
	int syscall_CG_R_ADDREFENTITYTOSCENE(const refEntity_t* re1) {
		re.AddRefEntityToScene( re1);
		return 0;
	}
	int syscall_CG_R_ADDPOLYTOSCENE(qhandle_t hShader, int numVerts, const polyVert_t* verts) {
		re.AddPolyToScene(hShader, numVerts, verts);
		return 0;
		// Ridah
	}

	int syscall_CG_R_ADDPOLYSTOSCENE(qhandle_t hShader, int numVerts, const polyVert_t* verts, int numPolys) {
		re.AddPolysToScene(hShader, numVerts, verts, numPolys);
		return 0;
		// done.
	}
	int syscall_CG_R_LIGHTFORPOINT(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir) {
//		return re.LightForPoint( VMA(1), VMA(2), VMA(3), VMA(4) );
		return 0;
	}
	int syscall_CG_R_ADDLIGHTTOSCENE(const vec3_t org, float intensity, float r, float g, float b, int overdraw) {
		re.AddLightToScene( org, intensity, r, g, b, overdraw);
		return 0;
	}
	int syscall_CG_R_ADDADDITIVELIGHTTOSCENE() {
//		re.AddAdditiveLightToScene( VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		return 0;
	}


	int syscall_CG_R_ADDCORONATOSCENE(const vec3_t org, float r, float g, float b, float scale, int id, bool visible) {
		re.AddCoronaToScene(org, r, g, b, scale, id, visible);
		return 0;
	}
	int syscall_CG_R_SETFOG(int fogvar, int var1, int var2, float r, float g, float b, float density) {
		re.SetFog(fogvar, var1, var2, r, g, b, density);
		return 0;
	}
	int syscall_CG_R_RENDERSCENE(const refdef_t* fd) {
		re.RenderScene(fd);
		return 0;
	}
	int syscall_CG_R_SETCOLOR(const float* rgba) {
		re.SetColor(rgba);
		return 0;
	}


	int syscall_CG_R_DRAWSTRETCHPIC(float x, float y, float w, float h,
		float s1, float t1, float s2, float t2, qhandle_t hShader) {
		re.DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hShader);
		return 0;
	}
	int syscall_CG_R_DRAWROTATEDPIC(float x, float y, float w, float h,
		float s1, float t1, float s2, float t2, qhandle_t hShader, float angle) {
		re.DrawRotatedPic(x, y, w, h, s1, t1, s2, t2, hShader, angle);
		return 0;
	}
	int syscall_CG_R_DRAWSTRETCHPIC_GRADIENT(float x, float y, float w, float h,
		float s1, float t1, float s2, float t2, qhandle_t hShader, const float* gradientColor, int gradientType) {
		re.DrawStretchPicGradient(x, y, w, h, s1, t1, s2, t2, hShader, gradientColor, gradientType);
		return 0;
	}
	int syscall_CG_R_MODELBOUNDS(qhandle_t model, vec3_t mins, vec3_t maxs) {
		re.ModelBounds(model, mins, maxs);
		return 0;
	}


	int syscall_CG_R_LERPTAG(orientation_t* tag, const refEntity_t* refent, const char* tagName, int startIndex) {
		return re.LerpTag(tag, refent, tagName, startIndex);
	}
	int syscall_CG_GETGLCONFIG(glconfig_t* glconfig) {
		CL_GetGlconfig(glconfig);
		return 0;
	}



	int syscall_CG_GETGAMESTATE(gameState_t* gs) {
		CL_GetGameState(gs);
		return 0;
	}
	int syscall_CG_GETCURRENTSNAPSHOTNUMBER(int* snapshotNumber, int* serverTime) {
		CL_GetCurrentSnapshotNumber(snapshotNumber, serverTime);
		return 0;
	}
	int syscall_CG_GETSNAPSHOT(int snapshotNumber, snapshot_t* snapshot) {
		return CL_GetSnapshot(snapshotNumber, snapshot);
	}
	int syscall_CG_GETSERVERCOMMAND(int serverCommandNumber) {
		return CL_GetServerCommand(serverCommandNumber);
	}
	int syscall_CG_GETCURRENTCMDNUMBER() {
		return CL_GetCurrentCmdNumber();
	}
	int syscall_CG_GETUSERCMD(int cmdNumber, usercmd_t* ucmd) {
		return CL_GetUserCmd(cmdNumber, ucmd);
	}



	int syscall_CG_SETUSERCMDVALUE(int userCmdValue, int holdableValue, float sensitivityScale, int mpSetup, int mpIdentClient) {
		CL_SetUserCmdValue(userCmdValue, holdableValue, sensitivityScale, mpSetup, mpIdentClient);
		return 0;
	}
	int syscall_CG_SETCLIENTLERPORIGIN(float x, float y, float z) {
		CL_SetClientLerpOrigin(x, y, z);
		return 0;
	}
	int syscall_CG_MEMORY_REMAINING() {
		return Hunk_MemoryRemaining();
	}
	int syscall_CG_KEY_ISDOWN(int keynum) {
		return Key_IsDown(keynum);
	}
	int syscall_CG_KEY_GETCATCHER() {
		return Key_GetCatcher();
	}
	

	int syscall_CG_KEY_SETCATCHER(int catcher) {
		Key_SetCatcher(catcher);
		return 0;
	}
	int syscall_CG_KEY_GETKEY(const char* binding) {
		return Key_GetKey(binding);
	}
	int syscall_CG_MEMSET(void* _Dst, int    _Val, size_t _Size) {
		return (int)memset(_Dst, _Val, _Size);
	}
	int syscall_CG_MEMCPY(void* _Dst, void const* _Src, size_t      _Size) {
		return (int)memcpy(_Dst, _Src, _Size);
	}
	int syscall_CG_STRNCPY(char*  _Destination, char const* _Source, size_t _Count) {
		return (int)strncpy(_Destination, _Source, _Count);
	}
	int syscall_CG_SIN(float f) {
		return FloatAsInt( sin(f) );
	}
	int syscall_CG_COS(float f) {
		return FloatAsInt( cos(f) );
	}
	int syscall_CG_ATAN2(double _Y, double _X) {
		return FloatAsInt( atan2(_Y, _X) );
	}
	int syscall_CG_SQRT(float f) {
		return FloatAsInt( sqrt(f) );
	}


	int syscall_CG_FLOOR(float f) {
		return FloatAsInt( floor(f) );
	}
	int syscall_CG_CEIL(float f) {
		return FloatAsInt( ceil(f) );
	}
	int syscall_CG_ACOS(float f) {
		return FloatAsInt( Q_acos(f) );

	}
	int syscall_CG_PC_ADD_GLOBAL_DEFINE(char* string) {
		return botlib_export->PC_AddGlobalDefine(string);
	}
	int syscall_CG_PC_LOAD_SOURCE(const char* filename) {
		return botlib_export->PC_LoadSourceHandle(filename);
	}
	int syscall_CG_PC_FREE_SOURCE(int handle) {
		return botlib_export->PC_FreeSourceHandle(handle);
	}
	int syscall_CG_PC_READ_TOKEN(int handle, pc_token_t* pc_token) {
		return botlib_export->PC_ReadTokenHandle(handle, pc_token);
	}


	int syscall_CG_PC_SOURCE_FILE_AND_LINE(int handle, char* filename, int* line) {
		return botlib_export->PC_SourceFileAndLine(handle, filename, line);

	}
	int syscall_CG_S_STOPBACKGROUNDTRACK() {
		S_StopBackgroundTrack();
		return 0;

	}
	int syscall_CG_REAL_TIME(qtime_t* qtime) {
		return Com_RealTime(qtime);
	}
	int syscall_CG_SNAPVECTOR(float* v) {
		Sys_SnapVector(v);
		return 0;

	}
	int syscall_CG_SENDMOVESPEEDSTOGAME(int entnum, char* text) {
		SV_SendMoveSpeedsToGame(entnum, text);
		return 0;

	}


	int syscall_CG_CIN_PLAYCINEMATIC(const char* arg, int x, int y, int w, int h, int systemBits) {
		return CIN_PlayCinematic(arg, x, y, w, h, systemBits);

	}
	e_status syscall_CG_CIN_STOPCINEMATIC(int handle) {
		return CIN_StopCinematic(handle);

	}
	e_status syscall_CG_CIN_RUNCINEMATIC(int handle) {
		return CIN_RunCinematic(handle);
	}


	int syscall_CG_CIN_DRAWCINEMATIC(int handle) {
		CIN_DrawCinematic(handle);
		return 0;

	}
	int syscall_CG_CIN_SETEXTENTS(int handle, int x, int y, int w, int h) {
		CIN_SetExtents(handle, x, y, w, h);
		return 0;

	}
	int syscall_CG_R_REMAP_SHADER(const char* oldShader, const char* newShader, const char* offsetTim) {
		re.RemapShader(oldShader, newShader, offsetTim);
		return 0;

	}
	int syscall_CG_TESTPRINTINT(char* string, int i) {
		Com_Printf( "%s%i\n", string, i);
		return 0;
	}
	int syscall_CG_TESTPRINTFLOAT(char* string, float f) {
		Com_Printf( "%s%f\n", string, f);
		return 0;
	}

	int syscall_CG_LOADCAMERA(int camNum, const char* name) {
		return loadCamera(camNum, name);

	}
	int syscall_CG_STARTCAMERA(int camNum, int time) {
		startCamera(camNum, time);
		return 0;

	}


	bool syscall_CG_GETCAMERAINFO(int camNum, int time, float* origin, float* angles, float* fov) {
		return getCameraInfo(camNum, time, origin, angles, fov);

	}
	int syscall_CG_GET_ENTITY_TOKEN(char* buffer, int size) {
		return re.GetEntityToken(buffer, size);

	}
	int syscall_CG_INGAME_POPUP(const char* arg0) {
		if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
			// NERVE - SMF
			if (arg0 && !Q_stricmp(arg0, "UIMENU_WM_PICKTEAM" ) ) {
				VM_Call_UI_SET_ACTIVE_MENU(UIMENU_WM_PICKTEAM );
			} else if (arg0 && !Q_stricmp(arg0, "UIMENU_WM_PICKPLAYER" ) )    {
				VM_Call_UI_SET_ACTIVE_MENU( UIMENU_WM_PICKPLAYER );
			} else if (arg0 && !Q_stricmp(arg0, "UIMENU_WM_QUICKMESSAGE" ) )    {
				VM_Call_UI_SET_ACTIVE_MENU(UIMENU_WM_QUICKMESSAGE );
			} else if (arg0 && !Q_stricmp(arg0, "UIMENU_WM_QUICKMESSAGEALT" ) )    {
				VM_Call_UI_SET_ACTIVE_MENU(UIMENU_WM_QUICKMESSAGEALT );
			} else if (arg0 && !Q_stricmp(arg0, "UIMENU_WM_LIMBO" ) )    {
				VM_Call_UI_SET_ACTIVE_MENU(UIMENU_WM_LIMBO );
			} else if (arg0 && !Q_stricmp(arg0, "UIMENU_WM_AUTOUPDATE" ) )    {
				VM_Call_UI_SET_ACTIVE_MENU(UIMENU_WM_AUTOUPDATE );
			}
			// -NERVE - SMF
			else if (arg0 && !Q_stricmp(arg0, "hbook1" ) ) {   //----(SA)
				VM_Call_UI_SET_ACTIVE_MENU(UIMENU_BOOK1 );
			} else if (arg0 && !Q_stricmp(arg0, "hbook2" ) )    { //----(SA)
				VM_Call_UI_SET_ACTIVE_MENU(UIMENU_BOOK2 );
			} else if (arg0 && !Q_stricmp(arg0, "hbook3" ) )    { //----(SA)
				VM_Call_UI_SET_ACTIVE_MENU(UIMENU_BOOK3 );
			} else {
				VM_Call_UI_SET_ACTIVE_MENU(UIMENU_CLIPBOARD );
			}
		}
		return 0;

		// NERVE - SMF
	}


	int syscall_CG_INGAME_CLOSEPOPUP(const char*  arg0) {
		// if popup menu is up, then close it
		if (arg0 && !Q_stricmp(arg0, "UIMENU_WM_LIMBO" ) ) {
			if ( VM_Call_UI_GET_ACTIVE_MENU() == UIMENU_WM_LIMBO ) {
				VM_Call_UI_KEY_EVENT(K_ESCAPE, true );
				VM_Call_UI_KEY_EVENT(K_ESCAPE, true );
			}
		}
		return 0;

	}
	int syscall_CG_LIMBOCHAT(const char* str) {
		if (str) {
			CL_AddToLimboChat(str);
		}
		return 0;

	}
	int syscall_CG_KEY_GETBINDINGBUF(int keynum, char* buf, int buflen) {
		Key_GetBindingBuf(keynum, buf, buflen);
		return 0;

	}
	int syscall_CG_KEY_SETBINDING(int keynum, const char* binding) {
		Key_SetBinding(keynum, binding);
		return 0;

	}

	int syscall_CG_KEY_KEYNUMTOSTRINGBUF(int keynum, char* buf, int buflen) {
		Key_KeynumToStringBuf(keynum, buf, buflen);
		return 0;

	}
	int syscall_CG_TRANSLATE_STRING(const char* string, char* dest_buffer) {
		CL_TranslateString(string, dest_buffer);
		return 0;
	}


/*
====================
CL_UpdateLevelHunkUsage

  This updates the "hunkusage.dat" file with the current map and it's hunk usage count

  This is used for level loading, so we can show a percentage bar dependant on the amount
  of hunk memory allocated so far

  This will be slightly inaccurate if some settings like sound quality are changed, but these
  things should only account for a small variation (hopefully)
====================
*/
void CL_UpdateLevelHunkUsage( void ) {
	int handle;
	char *memlistfile = "hunkusage.dat";
	char *buf, *outbuf;
	char *buftrav, *outbuftrav;
	char *token;
	char outstr[256];
	int len, memusage;

	memusage = Cvar_VariableIntegerValue( "com_hunkused" ) + Cvar_VariableIntegerValue( "hunk_soundadjust" );

	len = FS_FOpenFileByMode( memlistfile, &handle, FS_READ );
	if ( len >= 0 ) { // the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value

		buf = (char *)Z_Malloc( len + 1 );
		memset( buf, 0, len + 1 );
		outbuf = (char *)Z_Malloc( len + 1 );
		memset( outbuf, 0, len + 1 );

		FS_Read( (void *)buf, len, handle );
		FS_FCloseFile( handle );

		// now parse the file, filtering out the current map
		buftrav = buf;
		outbuftrav = outbuf;
		outbuftrav[0] = '\0';
		while ( ( token = COM_Parse( &buftrav ) ) && token[0] ) {
			if ( !Q_strcasecmp( token, cl.mapname ) ) {
				// found a match
				token = COM_Parse( &buftrav );  // read the size
				if ( token && token[0] ) {
					if ( atoi( token ) == memusage ) {  // if it is the same, abort this process
						Z_Free( buf );
						Z_Free( outbuf );
						return;
					}
				}
			} else {    // send it to the outbuf
				Q_strcat( outbuftrav, len + 1, token );
				Q_strcat( outbuftrav, len + 1, " " );
				token = COM_Parse( &buftrav );  // read the size
				if ( token && token[0] ) {
					Q_strcat( outbuftrav, len + 1, token );
					Q_strcat( outbuftrav, len + 1, "\n" );
				} else {
					Com_Error( ERR_DROP, "hunkusage.dat file is corrupt\n" );
				}
			}
		}

#ifdef __MACOS__    //DAJ MacOS file typing
		{
			extern _MSL_IMP_EXP_C long _fcreator, _ftype;
			_ftype = 'TEXT';
			_fcreator = 'WlfS';
		}
#endif
		handle = FS_FOpenFileWrite( memlistfile );
		if ( handle < 0 ) {
			Com_Error( ERR_DROP, "cannot create %s\n", memlistfile );
		}
		// input file is parsed, now output to the new file
		len = strlen( outbuf );
		if ( FS_Write( (void *)outbuf, len, handle ) != len ) {
			Com_Error( ERR_DROP, "cannot write to %s\n", memlistfile );
		}
		FS_FCloseFile( handle );

		Z_Free( buf );
		Z_Free( outbuf );
	}
	// now append the current map to the current file
	FS_FOpenFileByMode( memlistfile, &handle, FS_APPEND );
	if ( handle < 0 ) {
		Com_Error( ERR_DROP, "cannot write to hunkusage.dat, check disk full\n" );
	}
	Com_sprintf( outstr, sizeof( outstr ), "%s %i\n", cl.mapname, memusage );
	FS_Write( outstr, strlen( outstr ), handle );
	FS_FCloseFile( handle );

	// now just open it and close it, so it gets copied to the pak dir
	len = FS_FOpenFileByMode( memlistfile, &handle, FS_READ );
	if ( len >= 0 ) {
		FS_FCloseFile( handle );
	}
}

/*
====================
CL_InitCGame

Should only by called by CL_StartHunkUsers
====================
*/
void CL_InitCGame( void ) {
	const char          *info;
	const char          *mapname;
	int t1, t2;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl.mapname, sizeof( cl.mapname ), "maps/%s.bsp", mapname );

	cgvm = true;
	// load the dll
	//cgvm = VM_Create( "cgame", CL_CgameSystemCalls, VMI_NATIVE );
	//if ( !cgvm ) {
	//	Com_Error( ERR_DROP, "VM_Create on cgame failed" );
	//}
	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	VM_Call_CG_INIT( clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum );

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_Printf( "CL_InitCGame: %5.2f seconds\n", ( t2 - t1 ) / 1000.0 );

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
	if ( !Sys_LowPhysicalMemory() ) {
		Com_TouchMemory();
	}

	// clear anything that got printed
	Con_ClearNotify();

	// Ridah, update the memory usage file
	CL_UpdateLevelHunkUsage();
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
bool CL_GameCommand( void ) {
	if ( !cgvm ) {
		return false;
	}

	return VM_Call_CG_CONSOLE_COMMAND();
}



/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo ) {
	VM_Call_CG_DRAW_ACTIVE_FRAME(cl.serverTime, stereo, clc.demoplaying );
	// VM_Debug( 0 );
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define RESET_TIME  500

void CL_AdjustTimeDelta( void ) {
	int resetTime;
	int newDelta;
	int deltaDelta;

	cl.newSnapshots = false;

	// the delta never drifts when replaying a demo
	if ( clc.demoplaying ) {
		return;
	}

	// if the current time is WAY off, just correct to the current value
	if ( com_sv_running->integer ) {
		resetTime = 100;
	} else {
		resetTime = RESET_TIME;
	}

	newDelta = cl.snap.serverTime - cls.realtime;
	deltaDelta = abs( newDelta - cl.serverTimeDelta );

	if ( deltaDelta > RESET_TIME ) {
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snap.serverTime;  // FIXME: is this a problem for cgame?
		cl.serverTime = cl.snap.serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	} else if ( deltaDelta > 100 ) {
		// fast adjust, cut the difference in half
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<FAST> " );
		}
		cl.serverTimeDelta = ( cl.serverTimeDelta + newDelta ) >> 1;
	} else {
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if ( com_timescale->value == 0 || com_timescale->value == 1 ) {
			if ( cl.extrapolatedSnapshot ) {
				cl.extrapolatedSnapshot = false;
				cl.serverTimeDelta -= 2;
			} else {
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl.serverTimeDelta );
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot( void ) {
	// ignore snapshots that don't have entities
	if ( cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE ) {
		return;
	}
	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime = cl.snap.serverTime;

	clc.timeDemoBaseTime = cl.snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if ( cl_activeAction->string[0] ) {
		Cbuf_AddText( cl_activeAction->string );
		Cvar_Set( "activeAction", "" );
	}

	Sys_BeginProfiling();
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime( void ) {
	// getting a valid frame message ends the connection process
	if ( cls.state != CA_ACTIVE ) {
		if ( cls.state != CA_PRIMED ) {
			return;
		}
		if ( clc.demoplaying ) {
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if ( !clc.firstDemoFrameSkipped ) {
				clc.firstDemoFrameSkipped = true;
				return;
			}
			CL_ReadDemoMessage();
		}
		if ( cl.newSnapshots ) {
			cl.newSnapshots = false;
			CL_FirstSnapshot();
		}
		if ( cls.state != CA_ACTIVE ) {
			return;
		}
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if ( !cl.snap.valid ) {
		Com_Error( ERR_DROP, "CL_SetCGameTime: !cl.snap.valid" );
	}

	// allow pause in single player
	if ( sv_paused->integer && cl_paused->integer && com_sv_running->integer ) {
		// paused
		return;
	}

	if ( cl.snap.serverTime < cl.oldFrameServerTime ) {
		// Ridah, if this is a localhost, then we are probably loading a savegame
		if ( !Q_stricmp( cls.servername, "localhost" ) ) {
			// do nothing?
			CL_FirstSnapshot();
		} else {
			Com_Error( ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime" );
		}
	}
	cl.oldFrameServerTime = cl.snap.serverTime;


	// get our current view of time

	if ( clc.demoplaying && cl_freezeDemo->integer ) {
		// cl_freezeDemo is used to lock a demo in place for single frame advances

	} else {
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		int tn;

		tn = cl_timeNudge->integer;
		if ( tn < -30 ) {
			tn = -30;
		} else if ( tn > 30 ) {
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if ( cl.serverTime < cl.oldServerTime ) {
			cl.serverTime = cl.oldServerTime;
		}
		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if ( cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5 ) {
			cl.extrapolatedSnapshot = true;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if ( cl.newSnapshots ) {
		CL_AdjustTimeDelta();
	}

	if ( !clc.demoplaying ) {
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if ( cl_timedemo->integer ) {
		if ( !clc.timeDemoStart ) {
			clc.timeDemoStart = Sys_Milliseconds();
		}
		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	while ( cl.serverTime >= cl.snap.serverTime ) {
		// feed another messag, which should change
		// the contents of cl.snap
		CL_ReadDemoMessage();
		if ( cls.state != CA_ACTIVE ) {
			return;     // end of demo
		}
	}

}

/*
====================
CL_GetTag
====================
*/
bool CL_GetTag( int clientNum, char *tagname, orientation_t *orx ) {
	if ( !cgvm ) {
		return false;
	}

    return VM_Call_CG_GET_TAG(clientNum, tagname, orx );
}
