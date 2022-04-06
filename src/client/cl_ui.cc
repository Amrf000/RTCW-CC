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


#include "client.h"

#include "../game/botlib.h"

extern botlib_export_t* botlib_export;

bool uivm;

/*
====================
GetClientState
====================
*/
static void GetClientState(uiClientState_t* state) {
	state->connectPacketCount = clc.connectPacketCount;
	state->connState = cls.state;
	Q_strncpyz(state->servername, cls.servername, sizeof(state->servername));
	Q_strncpyz(state->updateInfoString, cls.updateInfoString, sizeof(state->updateInfoString));
	Q_strncpyz(state->messageString, clc.serverMessage, sizeof(state->messageString));
	state->clientNum = cl.snap.ps.clientNum;
}

/*
====================
LAN_LoadCachedServers
====================
*/
void LAN_LoadCachedServers() {
	int size;
	fileHandle_t fileIn;
	cls.numglobalservers = cls.nummplayerservers = cls.numfavoriteservers = 0;
	cls.numGlobalServerAddresses = 0;
	if (FS_SV_FOpenFileRead("servercache.dat", &fileIn)) {
		FS_Read(&cls.numglobalservers, sizeof(int), fileIn);
		FS_Read(&cls.nummplayerservers, sizeof(int), fileIn);
		FS_Read(&cls.numfavoriteservers, sizeof(int), fileIn);
		FS_Read(&size, sizeof(int), fileIn);
		if (size == sizeof(cls.globalServers) + sizeof(cls.favoriteServers) + sizeof(cls.mplayerServers)) {
			FS_Read(&cls.globalServers, sizeof(cls.globalServers), fileIn);
			FS_Read(&cls.mplayerServers, sizeof(cls.mplayerServers), fileIn);
			FS_Read(&cls.favoriteServers, sizeof(cls.favoriteServers), fileIn);
		}
		else {
			cls.numglobalservers = cls.nummplayerservers = cls.numfavoriteservers = 0;
			cls.numGlobalServerAddresses = 0;
		}
		FS_FCloseFile(fileIn);
	}
}

/*
====================
LAN_SaveServersToCache
====================
*/
void LAN_SaveServersToCache() {
	int size;
	fileHandle_t fileOut;
#ifdef __MACOS__    //DAJ MacOS file typing
	{
		extern _MSL_IMP_EXP_C long _fcreator, _ftype;
		_ftype = 'WlfB';
		_fcreator = 'WlfM';
	}
#endif
	fileOut = FS_SV_FOpenFileWrite("servercache.dat");
	FS_Write(&cls.numglobalservers, sizeof(int), fileOut);
	FS_Write(&cls.nummplayerservers, sizeof(int), fileOut);
	FS_Write(&cls.numfavoriteservers, sizeof(int), fileOut);
	size = sizeof(cls.globalServers) + sizeof(cls.favoriteServers) + sizeof(cls.mplayerServers);
	FS_Write(&size, sizeof(int), fileOut);
	FS_Write(&cls.globalServers, sizeof(cls.globalServers), fileOut);
	FS_Write(&cls.mplayerServers, sizeof(cls.mplayerServers), fileOut);
	FS_Write(&cls.favoriteServers, sizeof(cls.favoriteServers), fileOut);
	FS_FCloseFile(fileOut);
}


/*
====================
LAN_ResetPings
====================
*/
static void LAN_ResetPings(int source) {
	int count, i;
	serverInfo_t* servers = NULL;
	count = 0;

	switch (source) {
	case AS_LOCAL:
		servers = &cls.localServers[0];
		count = MAX_OTHER_SERVERS;
		break;
	case AS_MPLAYER:
		servers = &cls.mplayerServers[0];
		count = MAX_OTHER_SERVERS;
		break;
	case AS_GLOBAL:
		servers = &cls.globalServers[0];
		count = MAX_GLOBAL_SERVERS;
		break;
	case AS_FAVORITES:
		servers = &cls.favoriteServers[0];
		count = MAX_OTHER_SERVERS;
		break;
	}
	if (servers) {
		for (i = 0; i < count; i++) {
			servers[i].ping = -1;
		}
	}
}

/*
====================
LAN_AddServer
====================
*/
static int LAN_AddServer(int source, const char* name, const char* address) {
	int max, * count, i;
	netadr_t adr;
	serverInfo_t* servers = NULL;
	max = MAX_OTHER_SERVERS;
	count = 0;

	switch (source) {
	case AS_LOCAL:
		count = &cls.numlocalservers;
		servers = &cls.localServers[0];
		break;
	case AS_MPLAYER:
		count = &cls.nummplayerservers;
		servers = &cls.mplayerServers[0];
		break;
	case AS_GLOBAL:
		max = MAX_GLOBAL_SERVERS;
		count = &cls.numglobalservers;
		servers = &cls.globalServers[0];
		break;
	case AS_FAVORITES:
		count = &cls.numfavoriteservers;
		servers = &cls.favoriteServers[0];
		break;
	}
	if (servers && *count < max) {
		NET_StringToAdr(address, &adr);
		for (i = 0; i < *count; i++) {
			if (NET_CompareAdr(servers[i].adr, adr)) {
				break;
			}
		}
		if (i >= *count) {
			servers[*count].adr = adr;
			Q_strncpyz(servers[*count].hostName, name, sizeof(servers[*count].hostName));
			servers[*count].visible = true;
			(*count)++;
			return 1;
		}
		return 0;
	}
	return -1;
}

/*
====================
LAN_RemoveServer
====================
*/
static void LAN_RemoveServer(int source, const char* addr) {
	int* count, i;
	serverInfo_t* servers = NULL;
	count = 0;
	switch (source) {
	case AS_LOCAL:
		count = &cls.numlocalservers;
		servers = &cls.localServers[0];
		break;
	case AS_MPLAYER:
		count = &cls.nummplayerservers;
		servers = &cls.mplayerServers[0];
		break;
	case AS_GLOBAL:
		count = &cls.numglobalservers;
		servers = &cls.globalServers[0];
		break;
	case AS_FAVORITES:
		count = &cls.numfavoriteservers;
		servers = &cls.favoriteServers[0];
		break;
	}
	if (servers) {
		netadr_t comp;
		NET_StringToAdr(addr, &comp);
		for (i = 0; i < *count; i++) {
			if (NET_CompareAdr(comp, servers[i].adr)) {
				int j = i;
				while (j < *count - 1) {
					Com_Memcpy(&servers[j], &servers[j + 1], sizeof(servers[j]));
					j++;
				}
				(*count)--;
				break;
			}
		}
	}
}


/*
====================
LAN_GetServerCount
====================
*/
static int LAN_GetServerCount(int source) {
	switch (source) {
	case AS_LOCAL:
		return cls.numlocalservers;
		break;
	case AS_MPLAYER:
		return cls.nummplayerservers;
		break;
	case AS_GLOBAL:
		return cls.numglobalservers;
		break;
	case AS_FAVORITES:
		return cls.numfavoriteservers;
		break;
	}
	return 0;
}

/*
====================
LAN_GetLocalServerAddressString
====================
*/
static void LAN_GetServerAddressString(int source, int n, char* buf, int buflen) {
	switch (source) {
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			Q_strncpyz(buf, NET_AdrToString(cls.localServers[n].adr), buflen);
			return;
		}
		break;
	case AS_MPLAYER:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			Q_strncpyz(buf, NET_AdrToString(cls.mplayerServers[n].adr), buflen);
			return;
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
			Q_strncpyz(buf, NET_AdrToString(cls.globalServers[n].adr), buflen);
			return;
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			Q_strncpyz(buf, NET_AdrToString(cls.favoriteServers[n].adr), buflen);
			return;
		}
		break;
	}
	buf[0] = '\0';
}

/*
====================
LAN_GetServerInfo
====================
*/
static void LAN_GetServerInfo(int source, int n, char* buf, int buflen) {
	char info[MAX_STRING_CHARS];
	serverInfo_t* server = NULL;
	info[0] = '\0';
	switch (source) {
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			server = &cls.localServers[n];
		}
		break;
	case AS_MPLAYER:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			server = &cls.mplayerServers[n];
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
			server = &cls.globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			server = &cls.favoriteServers[n];
		}
		break;
	}
	if (server && buf) {
		buf[0] = '\0';
		Info_SetValueForKey(info, "hostname", server->hostName);
		Info_SetValueForKey(info, "mapname", server->mapName);
		Info_SetValueForKey(info, "clients", va("%i", server->clients));
		Info_SetValueForKey(info, "sv_maxclients", va("%i", server->maxClients));
		Info_SetValueForKey(info, "ping", va("%i", server->ping));
		Info_SetValueForKey(info, "minping", va("%i", server->minPing));
		Info_SetValueForKey(info, "maxping", va("%i", server->maxPing));
		Info_SetValueForKey(info, "game", server->game);
		Info_SetValueForKey(info, "gametype", va("%i", server->gameType));
		Info_SetValueForKey(info, "nettype", va("%i", server->netType));
		Info_SetValueForKey(info, "addr", NET_AdrToString(server->adr));
		Info_SetValueForKey(info, "sv_allowAnonymous", va("%i", server->allowAnonymous));
		Info_SetValueForKey(info, "friendlyFire", va("%i", server->friendlyFire));               // NERVE - SMF
		Info_SetValueForKey(info, "maxlives", va("%i", server->maxlives));                       // NERVE - SMF
		Info_SetValueForKey(info, "tourney", va("%i", server->tourney));                     // NERVE - SMF
		Info_SetValueForKey(info, "punkbuster", va("%i", server->punkbuster));                   // DHM - Nerve
		Info_SetValueForKey(info, "gamename", server->gameName);                                // Arnout
		Info_SetValueForKey(info, "g_antilag", va("%i", server->antilag)); // TTimo
		Q_strncpyz(buf, info, buflen);
	}
	else {
		if (buf) {
			buf[0] = '\0';
		}
	}
}

/*
====================
LAN_GetServerPing
====================
*/
static int LAN_GetServerPing(int source, int n) {
	serverInfo_t* server = NULL;
	switch (source) {
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			server = &cls.localServers[n];
		}
		break;
	case AS_MPLAYER:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			server = &cls.mplayerServers[n];
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
			server = &cls.globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			server = &cls.favoriteServers[n];
		}
		break;
	}
	if (server) {
		return server->ping;
	}
	return -1;
}

/*
====================
LAN_GetServerPtr
====================
*/
static serverInfo_t* LAN_GetServerPtr(int source, int n) {
	switch (source) {
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			return &cls.localServers[n];
		}
		break;
	case AS_MPLAYER:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			return &cls.mplayerServers[n];
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
			return &cls.globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			return &cls.favoriteServers[n];
		}
		break;
	}
	return NULL;
}

/*
====================
LAN_CompareServers
====================
*/
static int LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2) {
	int res;
	serverInfo_t* server1, * server2;

	server1 = LAN_GetServerPtr(source, s1);
	server2 = LAN_GetServerPtr(source, s2);
	if (!server1 || !server2) {
		return 0;
	}

	res = 0;
	switch (sortKey) {
	case SORT_HOST:
		res = Q_stricmp(server1->hostName, server2->hostName);
		break;

	case SORT_MAP:
		res = Q_stricmp(server1->mapName, server2->mapName);
		break;
	case SORT_CLIENTS:
		if (server1->clients < server2->clients) {
			res = -1;
		}
		else if (server1->clients > server2->clients) {
			res = 1;
		}
		else {
			res = 0;
		}
		break;
	case SORT_GAME:
		if (server1->gameType < server2->gameType) {
			res = -1;
		}
		else if (server1->gameType > server2->gameType) {
			res = 1;
		}
		else {
			res = 0;
		}
		break;
	case SORT_PING:
		if (server1->ping < server2->ping) {
			res = -1;
		}
		else if (server1->ping > server2->ping) {
			res = 1;
		}
		else {
			res = 0;
		}
		break;
	case SORT_PUNKBUSTER:
		if (server1->punkbuster < server2->punkbuster) {
			res = -1;
		}
		else if (server1->punkbuster > server2->punkbuster) {
			res = 1;
		}
		else {
			res = 0;
		}
	}

	if (sortDir) {
		if (res < 0) {
			return 1;
		}
		if (res > 0) {
			return -1;
		}
		return 0;
	}
	return res;
}

/*
====================
LAN_GetPingQueueCount
====================
*/
static int LAN_GetPingQueueCount(void) {
	return (CL_GetPingQueueCount());
}

/*
====================
LAN_ClearPing
====================
*/
static void LAN_ClearPing(int n) {
	CL_ClearPing(n);
}

/*
====================
LAN_GetPing
====================
*/
static void LAN_GetPing(int n, char* buf, int buflen, int* pingtime) {
	CL_GetPing(n, buf, buflen, pingtime);
}

/*
====================
LAN_GetPingInfo
====================
*/
static void LAN_GetPingInfo(int n, char* buf, int buflen) {
	CL_GetPingInfo(n, buf, buflen);
}

/*
====================
LAN_MarkServerVisible
====================
*/
static void LAN_MarkServerVisible(int source, int n, bool visible) {
	if (n == -1) {
		int count = MAX_OTHER_SERVERS;
		serverInfo_t* server = NULL;
		switch (source) {
		case AS_LOCAL:
			server = &cls.localServers[0];
			break;
		case AS_MPLAYER:
			server = &cls.mplayerServers[0];
			break;
		case AS_GLOBAL:
			server = &cls.globalServers[0];
			count = MAX_GLOBAL_SERVERS;
			break;
		case AS_FAVORITES:
			server = &cls.favoriteServers[0];
			break;
		}
		if (server) {
			for (n = 0; n < count; n++) {
				server[n].visible = visible;
			}
		}

	}
	else {
		switch (source) {
		case AS_LOCAL:
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				cls.localServers[n].visible = visible;
			}
			break;
		case AS_MPLAYER:
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				cls.mplayerServers[n].visible = visible;
			}
			break;
		case AS_GLOBAL:
			if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
				cls.globalServers[n].visible = visible;
			}
			break;
		case AS_FAVORITES:
			if (n >= 0 && n < MAX_OTHER_SERVERS) {
				cls.favoriteServers[n].visible = visible;
			}
			break;
		}
	}
}


/*
=======================
LAN_ServerIsVisible
=======================
*/
static int LAN_ServerIsVisible(int source, int n) {
	switch (source) {
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			return cls.localServers[n].visible;
		}
		break;
	case AS_MPLAYER:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			return cls.mplayerServers[n].visible;
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS) {
			return cls.globalServers[n].visible;
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_OTHER_SERVERS) {
			return cls.favoriteServers[n].visible;
		}
		break;
	}
	return false;
}

/*
=======================
LAN_UpdateVisiblePings
=======================
*/
bool LAN_UpdateVisiblePings(int source) {
	return CL_UpdateVisiblePings_f(source);
}

/*
====================
LAN_GetServerStatus
====================
*/
int LAN_GetServerStatus(char* serverAddress, char* serverStatus, int maxLen) {
	return CL_ServerStatus(serverAddress, serverStatus, maxLen);
}

/*
====================
CL_GetGlConfig
====================
*/
static void CL_GetGlconfig(glconfig_t* config) {
	*config = cls.glconfig;
}

/*
====================
GetClipboardData
====================
*/
static void GetClipboardData(char* buf, int buflen) {
	char* cbd;

	cbd = Sys_GetClipboardData();

	if (!cbd) {
		*buf = 0;
		return;
	}

	Q_strncpyz(buf, cbd, buflen);

	Z_Free(cbd);
}

/*
====================
Key_KeynumToStringBuf
====================
*/
void Key_KeynumToStringBuf(int keynum, char* buf, int buflen) {
	Q_strncpyz(buf, Key_KeynumToString(keynum, true), buflen);
}

/*
====================
Key_GetBindingBuf
====================
*/
void Key_GetBindingBuf(int keynum, char* buf, int buflen) {
	char* value;

	value = Key_GetBinding(keynum);
	if (value) {
		Q_strncpyz(buf, value, buflen);
	}
	else {
		*buf = 0;
	}
}

/*
====================
Key_GetCatcher
====================
*/
int Key_GetCatcher(void) {
	return cls.keyCatchers;
}

/*
====================
Ket_SetCatcher
====================
*/
void Key_SetCatcher(int catcher) {
	// NERVE - SMF - console overrides everything
	if (cls.keyCatchers & KEYCATCH_CONSOLE) {
		cls.keyCatchers = catcher | KEYCATCH_CONSOLE;
	}
	else {
		cls.keyCatchers = catcher;
	}

}


/*
====================
CLUI_GetCDKey
====================
*/
static void CLUI_GetCDKey(char* buf, int buflen) {
	cvar_t* fs;
	fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (UI_usesUniqueCDKey() && fs && fs->string[0] != 0) {
		memcpy(buf, &cl_cdkey[16], 16);
		buf[16] = 0;
	}
	else {
		memcpy(buf, cl_cdkey, 16);
		buf[16] = 0;
	}
}


/*
====================
CLUI_SetCDKey
====================
*/
static void CLUI_SetCDKey(char* buf) {
	cvar_t* fs;
	fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (UI_usesUniqueCDKey() && fs && fs->string[0] != 0) {
		memcpy(&cl_cdkey[16], buf, 16);
		cl_cdkey[32] = 0;
		// set the flag so the fle will be written at the next opportunity
		cvar_modifiedFlags |= CVAR_ARCHIVE;
	}
	else {
		memcpy(cl_cdkey, buf, 16);
		// set the flag so the fle will be written at the next opportunity
		cvar_modifiedFlags |= CVAR_ARCHIVE;
	}
}


/*
====================
GetConfigString
====================
*/
static int GetConfigString(int index, char* buf, int size) {
	int offset;

	if (index < 0 || index >= MAX_CONFIGSTRINGS) {
		return false;
	}

	offset = cl.gameState.stringOffsets[index];
	if (!offset) {
		if (size) {
			buf[0] = 0;
		}
		return false;
	}

	Q_strncpyz(buf, cl.gameState.stringData + offset, size);

	return true;
}

/*
====================
FloatAsInt
====================
*/
static int FloatAsInt(float f) {
	int temp;

	*(float*)&temp = f;

	return temp;
}

//void* VM_ArgPtr(int intValue);
//#define VMA( x ) VM_ArgPtr( args[x] )
//#define VMF( x )  ( (float *)args )[x]

/*
====================
CL_UISystemCalls

The ui module is making a system call
====================
*/
//int CL_UISystemCalls( int *args ) {
//	switch ( args[0] ) {
//	default:
//		Com_Error(ERR_DROP, "Bad UI system trap: %i", args[0]);
//
//	}
//
//	return 0;
//}

int syscall_UI_ERROR(const char* fmt) {
	Com_Error(ERR_DROP, "%s", fmt);
	return 0;

}

int syscall_UI_PRINT(const char* fmt) {
	Com_Printf("%s", fmt);
	return 0;

}


int syscall_UI_MILLISECONDS() {
	return Sys_Milliseconds();

}
int syscall_UI_CVAR_REGISTER(vmCvar_t* vmCvar, const char* varName, const char* defaultValue, int flags) {
	Cvar_Register(vmCvar, varName, defaultValue, flags);
	return 0;

}
int syscall_UI_CVAR_UPDATE(vmCvar_t* vmCvar) {
	Cvar_Update(vmCvar);
	return 0;

}
int syscall_UI_CVAR_SET(const char* var_name, const char* value) {
	Cvar_Set(var_name, value);
	return 0;

}


int syscall_UI_CVAR_VARIABLEVALUE(const char* var_name) {
	return FloatAsInt(Cvar_VariableValue(var_name));

}
int syscall_UI_CVAR_VARIABLESTRINGBUFFER(const char* var_name, char* buffer, int bufsize) {
	Cvar_VariableStringBuffer(var_name, buffer, bufsize);
	return 0;

}
int syscall_UI_CVAR_SETVALUE(const char* var_name, float value) {
	Cvar_SetValue(var_name, value);
	return 0;

}
int syscall_UI_CVAR_RESET(const char* var_name) {
	Cvar_Reset(var_name);
	return 0;

}


int syscall_UI_CVAR_CREATE(const char* var_name, const char* var_value, int flags) {
	Cvar_Get(var_name, var_value, flags);
	return 0;

}
int syscall_UI_CVAR_INFOSTRINGBUFFER(int bit, char* buff, int buffsize) {
	Cvar_InfoStringBuffer(bit, buff, buffsize);
	return 0;

}
int syscall_UI_ARGC() {
	return Cmd_Argc();

}
int syscall_UI_ARGV(int arg, char* buffer, int bufferLength) {
	Cmd_ArgvBuffer(arg, buffer, bufferLength);
	return 0;

}



int syscall_UI_CMD_EXECUTETEXT(int exec_when, const char* text) {
	Cbuf_ExecuteText(exec_when, text);
	return 0;

}
int syscall_UI_FS_FOPENFILE(const char* qpath, fileHandle_t* f, fsMode_t mode) {
	return FS_FOpenFileByMode(qpath, f, mode);

}


int syscall_UI_FS_READ(void* buffer, int len, fileHandle_t f) {
	FS_Read(buffer, len, f);
	return 0;

}
int syscall_UI_FS_WRITE(const void* buffer, int len, fileHandle_t h) {
	FS_Write(buffer, len, h);
	return 0;

}
int syscall_UI_FS_FCLOSEFILE(fileHandle_t f) {
	FS_FCloseFile(f);
	return 0;

}
int syscall_UI_FS_DELETEFILE(const char* filename) {
	return FS_Delete((char*)filename);

}
int syscall_UI_FS_GETFILELIST(const char* path, const char* extension, char* listbuf, int bufsize) {
	return FS_GetFileList(path, extension, listbuf, bufsize);

}
int syscall_UI_R_REGISTERMODEL(const char* name) {
	return re.RegisterModel(name);

}


int syscall_UI_R_REGISTERSKIN(const char* name) {
	return re.RegisterSkin(name);

}
int syscall_UI_R_REGISTERSHADERNOMIP(const char* name) {
	return re.RegisterShaderNoMip(name);

}
int syscall_UI_R_CLEARSCENE() {
	re.ClearScene();
	return 0;

}
int syscall_UI_R_ADDREFENTITYTOSCENE(const refEntity_t* re1) {
	re.AddRefEntityToScene(re1);
	return 0;

}
int syscall_UI_R_ADDPOLYTOSCENE(qhandle_t hShader, int numVerts, const polyVert_t* verts) {
	re.AddPolyToScene(hShader, numVerts, verts);
	return 0;

	// Ridah
}
int syscall_UI_R_ADDPOLYSTOSCENE(qhandle_t hShader, int numVerts, const polyVert_t* verts, int numPolys) {
	re.AddPolysToScene(hShader, numVerts, verts, numPolys);
	return 0;
	// done.

}


int syscall_UI_R_ADDLIGHTTOSCENE(const vec3_t org, float intensity, float r, float g, float b, int overdraw) {
	re.AddLightToScene(org, intensity, r, g, b, overdraw);
	return 0;

}
int syscall_UI_R_ADDCORONATOSCENE(const vec3_t org, float r, float g, float b, float scale, int id, bool visible) {
	re.AddCoronaToScene(org, r, g, b, scale, id, visible);
	return 0;

}
int syscall_UI_R_RENDERSCENE(const refdef_t* fd) {
	re.RenderScene(fd);
	return 0;

}
int syscall_UI_R_SETCOLOR(const float* rgba) {
	re.SetColor(rgba);
	return 0;

}
int syscall_UI_R_DRAWSTRETCHPIC(float x, float y, float w, float h,
	float s1, float t1, float s2, float t2, qhandle_t hShader) {
	re.DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hShader);
	return 0;

}
int syscall_UI_R_MODELBOUNDS(qhandle_t model, vec3_t mins, vec3_t maxs) {
	re.ModelBounds(model, mins, maxs);
	return 0;

}


int syscall_UI_UPDATESCREEN() {
	SCR_UpdateScreen();
	return 0;

}
int syscall_UI_CM_LERPTAG(orientation_t* tag, const refEntity_t* refent, const char* tagName, int startIndex) {
	return re.LerpTag(tag, refent, tagName, startIndex);

}
int syscall_UI_S_REGISTERSOUND(const char* name) {
#ifdef DOOMSOUND    ///// (SA) DOOMSOUND
	return S_RegisterSound(name);
#else
	return S_RegisterSound(name, false);
#endif  ///// (SA) DOOMSOUND

}
int syscall_UI_S_STARTLOCALSOUND(sfxHandle_t sfxHandle, int channelNum) {
	S_StartLocalSound(sfxHandle, channelNum);
	return 0;

}
int syscall_UI_KEY_KEYNUMTOSTRINGBUF(int keynum, char* buf, int bufle) {
	Key_KeynumToStringBuf(keynum, buf, bufle);
	return 0;

}
int syscall_UI_KEY_GETBINDINGBUF(int keynum, char* buf, int buflen) {
	Key_GetBindingBuf(keynum, buf, buflen);
	return 0;

}
int syscall_UI_KEY_SETBINDING(int keynum, const char* binding) {
	Key_SetBinding(keynum, binding);
	return 0;

}
int syscall_UI_KEY_ISDOWN(int keynum) {
	return Key_IsDown(keynum);

}
int syscall_UI_KEY_GETOVERSTRIKEMODE() {
	return Key_GetOverstrikeMode();

}



int syscall_UI_KEY_SETOVERSTRIKEMODE(bool state) {
	Key_SetOverstrikeMode(state);
	return 0;

}
int syscall_UI_KEY_CLEARSTATES() {
	Key_ClearStates();
	return 0;

}
int syscall_UI_KEY_GETCATCHER() {
	return Key_GetCatcher();

}
int syscall_UI_KEY_SETCATCHER(int catcher) {
	Key_SetCatcher(catcher);
	return 0;

}
int syscall_UI_GETCLIPBOARDDATA(char* buf, int buflen) {
	GetClipboardData(buf, buflen);
	return 0;

}



int syscall_UI_GETCLIENTSTATE(uiClientState_t* state) {
	GetClientState(state);
	return 0;

}
int syscall_UI_GETGLCONFIG(glconfig_t* config) {
	CL_GetGlconfig(config);
	return 0;

}
int syscall_UI_GETCONFIGSTRING(int index, char* buf, int size) {
	return GetConfigString(index, buf, size);

}
int syscall_UI_LAN_LOADCACHEDSERVERS() {
	LAN_LoadCachedServers();
	return 0;

}
int syscall_UI_LAN_SAVECACHEDSERVERS() {
	LAN_SaveServersToCache();
	return 0;

}
int syscall_UI_LAN_ADDSERVER(int source, const char* name, const char* address) {
	return LAN_AddServer(source, name, address);

}


int syscall_UI_LAN_REMOVESERVER(int source, const char* addr) {
	LAN_RemoveServer(source, addr);
	return 0;

}
int syscall_UI_LAN_GETPINGQUEUECOUNT() {
	return LAN_GetPingQueueCount();

}
int syscall_UI_LAN_CLEARPING(int n) {
	LAN_ClearPing(n);
	return 0;

}
int syscall_UI_LAN_GETPING(int n, char* buf, int buflen, int* pingtime) {
	LAN_GetPing(n, buf, buflen, pingtime);
	return 0;

}
int syscall_UI_LAN_GETPINGINFO(int n, char* buf, int buflen) {
	LAN_GetPingInfo(n, buf, buflen);
	return 0;

}
int syscall_UI_LAN_GETSERVERCOUNT(int source) {
	return LAN_GetServerCount(source);
}





int syscall_UI_LAN_GETSERVERADDRESSSTRING(int source, int n, char* buf, int buflen) {
	LAN_GetServerAddressString(source, n, buf, buflen);
	return 0;

}
int syscall_UI_LAN_GETSERVERINFO(int source, int n, char* buf, int buflen) {
	LAN_GetServerInfo(source, n, buf, buflen);
	return 0;

}
int syscall_UI_LAN_GETSERVERPING(int source, int n) {
	return LAN_GetServerPing(source, n);

}
int syscall_UI_LAN_MARKSERVERVISIBLE(int source, int n, bool visible) {
	LAN_MarkServerVisible(source, n, visible);
	return 0;

}
int syscall_UI_LAN_SERVERISVISIBLE(int source, int n) {
	return LAN_ServerIsVisible(source, n);

}
int syscall_UI_LAN_UPDATEVISIBLEPINGS(int source) {
	return LAN_UpdateVisiblePings(source);

}
int syscall_UI_LAN_RESETPINGS(int source) {
	LAN_ResetPings(source);
	return 0;

}


int syscall_UI_LAN_SERVERSTATUS(const char* serverAddress, char* serverStatus, int maxLen) {
	return LAN_GetServerStatus((char*)serverAddress, serverStatus, maxLen);

}
int syscall_UI_SET_PBCLSTATUS(int status) {
	return 0;

}
int syscall_UI_SET_PBSVSTATUS(int status) {
	return 0;

}
int syscall_UI_LAN_COMPARESERVERS(int source, int sortKey, int sortDir, int s1, int s2) {
	return LAN_CompareServers(source, sortKey, sortDir, s1, s2);

}
int syscall_UI_MEMORY_REMAINING() {
	return Hunk_MemoryRemaining();

}
int syscall_UI_GET_CDKEY(char* buf, int buflen) {
	CLUI_GetCDKey(buf, buflen);
	return 0;

}


int syscall_UI_SET_CDKEY(char* buf) {
	CLUI_SetCDKey(buf);
	return 0;

}
int syscall_UI_R_REGISTERFONT(const char* fontName, int pointSize, fontInfo_t* font) {
	re.RegisterFont(fontName, pointSize, font);
	return 0;

}
int syscall_UI_MEMSET(void* _Dst, int    _Val, size_t _Size) {
	return (int)memset(_Dst, _Val, _Size);

}
int syscall_UI_MEMCPY(void* _Dst, void const* _Src, size_t      _Size) {
	return (int)memcpy(_Dst, _Src, _Size);

}
int syscall_UI_STRNCPY(char* _Destination, char const* _Source, size_t _Count) {
	return (int)strncpy(_Destination, _Source, _Count);

}
int syscall_UI_SIN(double _X) {
	return FloatAsInt(sin(_X));

}
int syscall_UI_COS(double _X) {
	return FloatAsInt(cos(_X));

}



int syscall_UI_ATAN2(double _Y, double _X) {
	return FloatAsInt(atan2(_Y, _X));

}
int syscall_UI_SQRT(double _X) {
	return FloatAsInt(sqrt(_X));

}
int syscall_UI_FLOOR(double _X) {
	return FloatAsInt(floor(_X));

}
int syscall_UI_CEIL(double _X) {
	return FloatAsInt(ceil(_X));

}
int syscall_UI_PC_ADD_GLOBAL_DEFINE(char* string) {
	return botlib_export->PC_AddGlobalDefine(string);
}
int syscall_UI_PC_LOAD_SOURCE(const char* filename) {
	return botlib_export->PC_LoadSourceHandle(filename);
}
int syscall_UI_PC_FREE_SOURCE(int handle) {
	return botlib_export->PC_FreeSourceHandle(handle);
}
int syscall_UI_PC_READ_TOKEN(int handle, pc_token_t* pc_token) {
	return botlib_export->PC_ReadTokenHandle(handle, pc_token);
}
int syscall_UI_PC_SOURCE_FILE_AND_LINE(int handle, char* filename, int* line) {
	return botlib_export->PC_SourceFileAndLine(handle, filename, line);
}


int syscall_UI_S_STOPBACKGROUNDTRACK() {
	S_StopBackgroundTrack();
	return 0;
}
int syscall_UI_S_STARTBACKGROUNDTRACK(const char* intro, const char* loop) {
	S_StartBackgroundTrack(intro, loop);
	return 0;

}
int syscall_UI_REAL_TIME(qtime_t* qtime) {
	return Com_RealTime(qtime);
}


int syscall_UI_CIN_PLAYCINEMATIC(const char* arg, int x, int y, int w, int h, int systemBits) {
	Com_DPrintf("UI_CIN_PlayCinematic\n");
	return CIN_PlayCinematic(arg, x, y, w, h, systemBits);

}
int syscall_UI_CIN_STOPCINEMATIC(int handle) {
	return CIN_StopCinematic(handle);

}
int syscall_UI_CIN_RUNCINEMATIC(int handle) {
	return CIN_RunCinematic(handle);
}



int syscall_UI_CIN_DRAWCINEMATIC(int handle) {
	CIN_DrawCinematic(handle);
	return 0;

}
int syscall_UI_CIN_SETEXTENTS(int handle, int x, int y, int w, int h) {
	CIN_SetExtents(handle, x, y, w, h);
	return 0;

}


int syscall_UI_R_REMAP_SHADER(const char* oldShader, const char* newShader, const char* offsetTime) {
	re.RemapShader(oldShader, newShader, offsetTime);
	return 0;

}
int syscall_UI_VERIFY_CDKEY(const char* key, const char* checksum) {
	return CL_CDKeyValidate(key, checksum);

	// NERVE - SMF
}
int syscall_UI_CL_GETLIMBOSTRING(int index, char* buf) {
	return CL_GetLimboString(index, buf);

}


int syscall_UI_CL_TRANSLATE_STRING(const char* string, char* dest_buffer) {
	CL_TranslateString(string, dest_buffer);
	return 0;
	// -NERVE - SMF

	// DHM - Nerve
}
int syscall_UI_CHECKAUTOUPDATE() {
	CL_CheckAutoUpdate();
	return 0;

}




int syscall_UI_GET_AUTOUPDATE() {
	CL_GetAutoUpdate();
	return 0;
	// DHM - Nerve

}
int syscall_UI_OPENURL(const char* url) {
	CL_OpenURL(url);
	return 0;
}


/*
====================
CL_ShutdownUI
====================
*/
void CL_ShutdownUI(void) {
	cls.keyCatchers &= ~KEYCATCH_UI;
	cls.uiStarted = false;
	if (!uivm) {
		return;
	}
	VM_Call_UI_SHUTDOWN();
	//VM_Free( uivm );
	//uivm = NULL;
	uivm = false;
}

/*
====================
CL_InitUI
====================
*/

void CL_InitUI(void) {
	int v;

	//uivm = VM_Create( "ui", CL_UISystemCalls, VMI_NATIVE );
	//if ( !uivm ) {
	//	Com_Error( ERR_FATAL, "VM_Create on UI failed" );
	//}
	uivm = true;

	// sanity check
	v = VM_Call_UI_GETAPIVERSION();
	if (v != UI_API_VERSION) {
		Com_Error(ERR_FATAL, "User Interface is version %d, expected %d", v, UI_API_VERSION);
		cls.uiStarted = false;
	}

	// init for this gamestate
	VM_Call_UI_INIT((cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE));
}


bool UI_usesUniqueCDKey() {
	if (uivm) {
		return (VM_Call_UI_HASUNIQUECDKEY() == true);
	}
	else {
		return false;
	}
}

bool UI_checkKeyExec(int key) {
	if (uivm) {
		return VM_Call_UI_CHECKEXECKEY(key);
	}
	else {
		return false;
	}
}

/*
====================
UI_GameCommand

See if the current console command is claimed by the ui
====================
*/
bool UI_GameCommand(void) {
	if (!uivm) {
		return false;
	}

	return VM_Call_UI_CONSOLE_COMMAND(cls.realtime);
}
