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

// sv_game.c -- interface to the game dll

#include "server.h"

#include "../game/botlib.h"

botlib_export_t* botlib_export;

void SV_GameError(const char* string) {
	Com_Error(ERR_DROP, "%s", string);
}

void SV_GamePrint(const char* string) {
	Com_Printf("%s", string);
}

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int SV_NumForGentity(sharedEntity_t* ent) {
	int num;

	num = ((byte*)ent - (byte*)sv.gentities) / sv.gentitySize;

	return num;
}

sharedEntity_t* SV_GentityNum(int num) {
	sharedEntity_t* ent;

	ent = (sharedEntity_t*)((byte*)sv.gentities + sv.gentitySize * (num));

	return ent;
}

playerState_t* SV_GameClientNum(int num) {
	playerState_t* ps;

	ps = (playerState_t*)((byte*)sv.gameClients + sv.gameClientSize * (num));

	return ps;
}

svEntity_t* SV_SvEntityForGentity(sharedEntity_t* gEnt) {
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES) {
		Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt");
	}
	return &sv.svEntities[gEnt->s.number];
}

sharedEntity_t* SV_GEntityForSvEntity(svEntity_t* svEnt) {
	int num;

	num = svEnt - sv.svEntities;
	return SV_GentityNum(num);
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand(int clientNum, const char* text) {
	if (clientNum == -1) {
		SV_SendServerCommand(NULL, "%s", text);
	}
	else {
		if (clientNum < 0 || clientNum >= sv_maxclients->integer) {
			return;
		}
		SV_SendServerCommand(svs.clients + clientNum, "%s", text);
	}
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient(int clientNum, const char* reason) {
	if (clientNum < 0 || clientNum >= sv_maxclients->integer) {
		return;
	}
	SV_DropClient(svs.clients + clientNum, reason);
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel(sharedEntity_t* ent, const char* name) {
	clipHandle_t h;
	vec3_t mins, maxs;

	if (!name) {
		Com_Error(ERR_DROP, "SV_SetBrushModel: NULL");
	}

	if (name[0] != '*') {
		Com_Error(ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name);
	}


	ent->s.modelindex = atoi(name + 1);

	h = CM_InlineModel(ent->s.modelindex);
	CM_ModelBounds(h, mins, maxs);
	VectorCopy(mins, ent->r.mins);
	VectorCopy(maxs, ent->r.maxs);
	ent->r.bmodel = true;

	ent->r.contents = -1;       // we don't know exactly what is in the brushes

	SV_LinkEntity(ent);       // FIXME: remove
}



/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
bool SV_inPVS(const vec3_t p1, const vec3_t p2) {
	int leafnum;
	int cluster;
	int area1, area2;
	byte* mask;

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	area1 = CM_LeafArea(leafnum);
	mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	area2 = CM_LeafArea(leafnum);
	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7))))) {
		return false;
	}
	if (!CM_AreasConnected(area1, area2)) {
		return false;      // a door blocks sight
	}
	return true;
}


/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
bool SV_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2) {
	int leafnum;
	int cluster;
	int area1, area2;
	byte* mask;

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	area1 = CM_LeafArea(leafnum);
	mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	area2 = CM_LeafArea(leafnum);

	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7))))) {
		return false;
	}

	return true;
}


/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState(sharedEntity_t* ent, bool open_) {
	svEntity_t* svEnt;

	svEnt = SV_SvEntityForGentity(ent);
	if (svEnt->areanum2 == -1) {
		return;
	}
	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open_);
}


/*
==================
SV_GameAreaEntities
==================
*/
bool    SV_EntityContact(const vec3_t mins, const vec3_t maxs, const sharedEntity_t* gEnt, const int capsule) {
	const float* origin, * angles;
	clipHandle_t ch;
	trace_t trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SV_ClipHandleForEntity(gEnt);
	CM_TransformedBoxTrace(&trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles, capsule);

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo(char* buffer, int bufferSize) {
	if (bufferSize < 1) {
		Com_Error(ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize);
	}
	Q_strncpyz(buffer, Cvar_InfoString(CVAR_SERVERINFO), bufferSize);
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t,
	playerState_t* clients, int sizeofGameClient) {
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}


/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd(int clientNum, usercmd_t* cmd) {
	if (clientNum < 0 || clientNum >= sv_maxclients->integer) {
		Com_Error(ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum);
	}
	*cmd = svs.clients[clientNum].lastUsercmd;
}

//==============================================

static int  FloatAsInt(float f) {
	int temp;

	*(float*)&temp = f;

	return temp;
}

/*
====================
SV_GameSystemCalls

The module is making a system call
====================
*/
//rcg010207 - see my comments in VM_DllSyscall(), in qcommon/vm.c ...
//#if ( ( defined __linux__ ) && ( defined __powerpc__ ) ) || ( defined MACOS_X )
//#define VMA( x ) ( (void *) args[x] )
//#else
//#define VMA( x ) VM_ArgPtr( args[x] )
//#endif
//
//#define VMF( x )  ( (float *)args )[x]

//int SV_GameSystemCalls( int *args ) {
//	switch ( args[0] ) {
//	default:
//		Com_Error(ERR_DROP, "Bad game system trap: %i", args[0]);
//	}
//	return -1;
//}



int syscall_G_PRINT(const char* fmt) {
	Com_Printf("%s", fmt);
	return 0;
}

int syscall_G_ERROR(const char* fmt) {
	Com_Error(ERR_DROP, "%s", fmt);
	return 0;
}


int syscall_G_MILLISECONDS() {
	return Sys_Milliseconds();
}
int syscall_G_CVAR_REGISTER(vmCvar_t* vmCvar, const char* varName, const char* defaultValue, int flags) {
	Cvar_Register(vmCvar, varName, defaultValue, flags);
	return 0;
}
int syscall_G_CVAR_UPDATE(vmCvar_t* vmCvar) {
	Cvar_Update(vmCvar);
	return 0;
}
int syscall_G_CVAR_SET(const char* var_name, const char* value) {
	Cvar_Set(var_name, value);
	return 0;
}


int syscall_G_CVAR_VARIABLE_INTEGER_VALUE(const char* var_name) {
	return Cvar_VariableIntegerValue(var_name);
}
int syscall_G_CVAR_VARIABLE_STRING_BUFFER(const char* var_name, char* buffer, int bufsize) {
	Cvar_VariableStringBuffer(var_name, buffer, bufsize);
	return 0;
}
int syscall_G_ARGC() {
	return Cmd_Argc();
}
int syscall_G_ARGV(int arg, char* buffer, int bufferLength) {
	Cmd_ArgvBuffer(arg, buffer, bufferLength);
	return 0;
}
int syscall_G_SEND_CONSOLE_COMMAND(int exec_when, const char* text) {
	Cbuf_ExecuteText(exec_when, text);
	return 0;
}
int syscall_G_FS_FOPEN_FILE(const char* qpath, fileHandle_t* f, fsMode_t mode) {
	return FS_FOpenFileByMode(qpath, f, mode);
}

int syscall_G_FS_READ(void* buffer, int len, fileHandle_t f) {
	FS_Read(buffer, len, f);
	return 0;
}
int syscall_G_FS_WRITE(const void* buffer, int len, fileHandle_t h) {
	return FS_Write(buffer, len, h);
}
int syscall_G_FS_RENAME(const char* from, const char* to) {
	FS_Rename(from, to);
	return 0;
}

int syscall_G_FS_FCLOSE_FILE(fileHandle_t f) {
	FS_FCloseFile(f);
	return 0;
}
int syscall_G_FS_GETFILELIST(const char* path, const char* extension, char* listbuf, int bufsize) {
	return FS_GetFileList(path, extension, listbuf, bufsize);
}
int syscall_G_LOCATE_GAME_DATA(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t,
	playerState_t* clients, int sizeofGameClient) {
	SV_LocateGameData(gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGameClient);
	return 0;
}
int syscall_G_DROP_CLIENT(int clientNum, const char* reason) {
	SV_GameDropClient(clientNum, reason);
	return 0;
}
int syscall_G_SEND_SERVER_COMMAND(int clientNum, const char* text) {
	SV_GameSendServerCommand(clientNum, text);
	return 0;
}


int syscall_G_LINKENTITY(sharedEntity_t* gEnt) {
	SV_LinkEntity(gEnt);
	return 0;
}
int syscall_G_UNLINKENTITY(sharedEntity_t* gEnt) {
	SV_UnlinkEntity(gEnt);
	return 0;
}
int syscall_G_ENTITIES_IN_BOX(const vec3_t mins, const vec3_t maxs, int* entityList, int maxcount) {
	return SV_AreaEntities(mins, maxs, entityList, maxcount);
}



int syscall_G_ENTITY_CONTACT(const vec3_t mins, const vec3_t maxs, const sharedEntity_t* gEnt) {
	return SV_EntityContact(mins, maxs, gEnt, /* int capsule */ false);
}
int syscall_G_ENTITY_CONTACTCAPSULE(const vec3_t mins, const vec3_t maxs, const sharedEntity_t* gEnt) {
	return SV_EntityContact(mins, maxs, gEnt, /* int capsule */ true);
}
int syscall_G_TRACE(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	SV_Trace(results, start, mins, maxs, end, passEntityNum, contentmask, /* int capsule */ false);
	return 0;
}
int syscall_G_TRACECAPSULE(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	SV_Trace(results, start, mins, maxs, end, passEntityNum, contentmask, /* int capsule */ true);
	return 0;
}
int syscall_G_POINT_CONTENTS(const vec3_t p, int passEntityNum) {
	return SV_PointContents(p, passEntityNum);
}
int syscall_G_SET_BRUSH_MODEL(sharedEntity_t* ent, const char* nam) {
	SV_SetBrushModel(ent, nam);
	return 0;
}


int syscall_G_IN_PVS(const vec3_t p1, const vec3_t p2) {
	return SV_inPVS(p1, p2);
}
int syscall_G_IN_PVS_IGNORE_PORTALS(const vec3_t p1, const vec3_t p2) {
	return SV_inPVSIgnorePortals(p1, p2);
}
int syscall_G_SET_CONFIGSTRING(int index, const char* val) {
	SV_SetConfigstring(index, val);
	return 0;
}
int syscall_G_GET_CONFIGSTRING(int index, char* buffer, int bufferSize) {
	SV_GetConfigstring(index, buffer, bufferSize);
	return 0;
}
int syscall_G_SET_USERINFO(int index, const char* val) {
	SV_SetUserinfo(index, val);
	return 0;
}
int syscall_G_GET_USERINFO(int index, char* buffer, int bufferSize) {
	SV_GetUserinfo(index, buffer, bufferSize);
	return 0;
}
int syscall_G_GET_SERVERINFO(char* buffer, int bufferSize) {
	SV_GetServerinfo(buffer, bufferSize);
	return 0;
}
int syscall_G_ADJUST_AREA_PORTAL_STATE(sharedEntity_t* ent, bool open_) {
	SV_AdjustAreaPortalState(ent, open_);
	return 0;
}


int syscall_G_AREAS_CONNECTED(int area1, int area2) {
	return CM_AreasConnected(area1, area2);
}
int syscall_G_BOT_ALLOCATE_CLIENT() {
	return SV_BotAllocateClient();
}
int syscall_G_BOT_FREE_CLIENT(int clientNum) {
	SV_BotFreeClient(clientNum);
	return 0;
}
int syscall_G_GET_USERCMD(int clientNum, usercmd_t* cmd) {
	SV_GetUsercmd(clientNum, cmd);
	return 0;
}
int syscall_G_GET_ENTITY_TOKEN(char* dest, int destsize) {
	const char* s;

	s = COM_Parse(&sv.entityParsePoint);
	Q_strncpyz(dest, s, destsize);
	if (!sv.entityParsePoint && !s[0]) {
		return false;
	}
	else {
		return true;
	}
}
int syscall_G_DEBUG_POLYGON_CREATE(int color, int numPoints, vec3_t* points) {
	return BotImport_DebugPolygonCreate(color, numPoints, points);
}
int syscall_G_DEBUG_POLYGON_DELETE(int id) {
	BotImport_DebugPolygonDelete(id);
	return 0;
}
int syscall_G_REAL_TIME(qtime_t* qtime) {
	return Com_RealTime(qtime);
}
int syscall_G_SNAPVECTOR(float* v) {
	Sys_SnapVector(v);
	return 0;
}
int syscall_G_GETTAG(int clientNum, char* tagname, orientation_t* orx) {
	return SV_GetTag(clientNum, tagname, orx);
}
//====================================

int syscall_BOTLIB_SETUP() {
	return SV_BotLibSetup();
}
int syscall_BOTLIB_SHUTDOWN() {
	return SV_BotLibShutdown();
}



int syscall_BOTLIB_LIBVAR_SET(char* var_name, char* value) {
	return botlib_export->BotLibVarSet(var_name, value);
}
int syscall_BOTLIB_LIBVAR_GET(char* var_name, char* value, int size) {
	return botlib_export->BotLibVarGet(var_name, value, size);
}
int syscall_BOTLIB_PC_ADD_GLOBAL_DEFINE(char* string) {
	return botlib_export->PC_AddGlobalDefine(string);
}
int syscall_BOTLIB_PC_LOAD_SOURCE(const char* filename) {
	return botlib_export->PC_LoadSourceHandle(filename);
}
int syscall_BOTLIB_PC_FREE_SOURCE(int handle) {
	return botlib_export->PC_FreeSourceHandle(handle);
}
int syscall_BOTLIB_PC_READ_TOKEN(int handle, pc_token_t* pc_token) {
	return botlib_export->PC_ReadTokenHandle(handle, pc_token);
}
int syscall_BOTLIB_PC_SOURCE_FILE_AND_LINE(int handle, char* filename, int* line) {
	return botlib_export->PC_SourceFileAndLine(handle, filename, line);
}



int syscall_BOTLIB_START_FRAME(float time) {
	return botlib_export->BotLibStartFrame(time);
}
int syscall_BOTLIB_LOAD_MAP(const char* mapname) {
	return botlib_export->BotLibLoadMap(mapname);
}
int syscall_BOTLIB_UPDATENTITY(int ent, bot_entitystate_t* state) {
	return botlib_export->BotLibUpdateEntity(ent, state);
}
int syscall_BOTLIB_TEST(int parm0, char* parm1, vec3_t parm2, vec3_t parm3) {
	return botlib_export->Test(parm0, parm1, parm2, parm3);
}
int syscall_BOTLIB_GET_SNAPSHOT_ENTITY(int client, int sequenc) {
	return SV_BotGetSnapshotEntity(client, sequenc);
}
int syscall_BOTLIB_GET_CONSOLE_MESSAGE(int client, char* buf, int size) {
	return SV_BotGetConsoleMessage(client, buf, size);
}
int syscall_BOTLIB_USER_COMMAND(int clientNum, usercmd_t* ucmd) {
	SV_ClientThink(&svs.clients[clientNum], ucmd);
	return 0;
}
int syscall_BOTLIB_AAS_ENTITY_INFO(int entnum, aas_entityinfo_t* info) {
	botlib_export->aas.AAS_EntityInfo(entnum, info);
	return 0;
}


int syscall_BOTLIB_AAS_INITIALIZED() {
	return botlib_export->aas.AAS_Initialized();
}
int syscall_BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX(int presencetype, vec3_t mins, vec3_t maxs) {
	botlib_export->aas.AAS_PresenceTypeBoundingBox(presencetype, mins, maxs);
	return 0;
}
int syscall_BOTLIB_AAS_TIME() {
	return FloatAsInt(botlib_export->aas.AAS_Time());
}
// Ridah
int syscall_BOTLIB_AAS_SETCURRENTWORLD(int index) {
	botlib_export->aas.AAS_SetCurrentWorld(index);
	return 0;
	// done.
}
int syscall_BOTLIB_AAS_POINT_AREA_NUM(vec3_t point) {
	return botlib_export->aas.AAS_PointAreaNum(point);
}
int syscall_BOTLIB_AAS_TRACE_AREAS(vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas) {
	return botlib_export->aas.AAS_TraceAreas(start, end, areas, points, maxareas);
}
int syscall_BOTLIB_AAS_POINT_CONTENTS(vec3_t point) {
	return botlib_export->aas.AAS_PointContents(point);
}
int syscall_BOTLIB_AAS_NEXT_BSP_ENTITY(int ent) {
	return botlib_export->aas.AAS_NextBSPEntity(ent);
}


int syscall_BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY(int ent, char* key, char* value, int size) {
	return botlib_export->aas.AAS_ValueForBSPEpairKey(ent, key, value, size);
}
int syscall_BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY(int ent, char* key, vec3_t v) {
	return botlib_export->aas.AAS_VectorForBSPEpairKey(ent, key, v);
}
int syscall_BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY(int ent, char* key, float* value) {
	return botlib_export->aas.AAS_FloatForBSPEpairKey(ent, key, value);
}
int syscall_BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY(int ent, char* key, int* value) {
	return botlib_export->aas.AAS_IntForBSPEpairKey(ent, key, value);
}
int syscall_BOTLIB_AAS_AREA_REACHABILITY(int areanum) {
	return botlib_export->aas.AAS_AreaReachability(areanum);
}
int syscall_BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA(int areanum, vec3_t origin, int goalareanum, int travelflag) {
	return botlib_export->aas.AAS_AreaTravelTimeToGoalArea(areanum, origin, goalareanum, travelflag);
}
int syscall_BOTLIB_AAS_SWIMMING(vec3_t origin) {
	return botlib_export->aas.AAS_Swimming(origin);
}


int syscall_BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT(struct aas_clientmove_s* move,
	int entnum, vec3_t origin,
	int presencetype, int onground,
	vec3_t velocity, vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, int visualize) {
	return botlib_export->aas.AAS_PredictClientMovement(move, entnum, origin, presencetype, onground,
		velocity, cmdmove, cmdframes, maxframes, frametime, stopevent, stopareanum, visualize);
}



// Ridah, route-tables
int syscall_BOTLIB_AAS_RT_SHOWROUTE(vec3_t srcpos, int srcnum, int destnum) {
	botlib_export->aas.AAS_RT_ShowRoute(srcpos, srcnum, destnum);
	return 0;
}
int syscall_BOTLIB_AAS_RT_GETHIDEPOS(vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos) {
	return botlib_export->aas.AAS_RT_GetHidePos(srcpos, srcnum, srcarea, destpos, destnum, destarea, returnPos);
}
int syscall_BOTLIB_AAS_FINDATTACKSPOTWITHINRANGE(int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float* outpos) {
	return botlib_export->aas.AAS_FindAttackSpotWithinRange(srcnum, rangenum, enemynum, rangedist, travelflags, outpos);
}
int syscall_BOTLIB_AAS_SETAASBLOCKINGENTITY(vec3_t absmin, vec3_t absmax, bool blocking) {
	botlib_export->aas.AAS_SetAASBlockingEntity(absmin, absmax, blocking);
	return 0;
	// done.
}
int syscall_BOTLIB_EA_SAY(int client, char* str) {
	botlib_export->ea.EA_Say(client, str);
	return 0;
}


int syscall_BOTLIB_EA_SAY_TEAM(int client, char* str) {
	botlib_export->ea.EA_SayTeam(client, str);
	return 0;
}
int syscall_BOTLIB_EA_USE_ITEM(int client, char* it) {
	botlib_export->ea.EA_UseItem(client, it);
	return 0;
}
int syscall_BOTLIB_EA_DROP_ITEM(int client, char* it) {
	botlib_export->ea.EA_DropItem(client, it);
	return 0;
}
int syscall_BOTLIB_EA_USE_INV(int client, char* inv) {
	botlib_export->ea.EA_UseInv(client, inv);
	return 0;
}
int syscall_BOTLIB_EA_DROP_INV(int client, char* inv) {
	botlib_export->ea.EA_DropInv(client, inv);
	return 0;
}
int syscall_BOTLIB_EA_GESTURE(int client) {
	botlib_export->ea.EA_Gesture(client);
	return 0;
}


int syscall_BOTLIB_EA_COMMAND(int client, char* command) {
	botlib_export->ea.EA_Command(client, command);
	return 0;
}
int syscall_BOTLIB_EA_SELECT_WEAPON(int client, int weapon) {
	botlib_export->ea.EA_SelectWeapon(client, weapon);
	return 0;
}
int syscall_BOTLIB_EA_TALK(int client) {
	botlib_export->ea.EA_Talk(client);
	return 0;
}
int syscall_BOTLIB_EA_ATTACK(int client) {
	botlib_export->ea.EA_Attack(client);
	return 0;
}
int syscall_BOTLIB_EA_RELOAD(int client) {
	botlib_export->ea.EA_Reload(client);
	return 0;
}
int syscall_BOTLIB_EA_USE(int client) {
	botlib_export->ea.EA_Use(client);
	return 0;
}


int syscall_BOTLIB_EA_RESPAWN(int client) {
	botlib_export->ea.EA_Respawn(client);
	return 0;
}
int syscall_BOTLIB_EA_JUMP(int client) {
	botlib_export->ea.EA_Jump(client);
	return 0;
}
int syscall_BOTLIB_EA_DELAYED_JUMP(int client) {
	botlib_export->ea.EA_DelayedJump(client);
	return 0;
}


int syscall_BOTLIB_EA_CROUCH(int client) {
	botlib_export->ea.EA_Crouch(client);
	return 0;
}
int syscall_BOTLIB_EA_MOVE_UP(int client) {
	botlib_export->ea.EA_MoveUp(client);
	return 0;
}
int syscall_BOTLIB_EA_MOVE_DOWN(int client) {
	botlib_export->ea.EA_MoveDown(client);
	return 0;
}
int syscall_BOTLIB_EA_MOVE_FORWARD(int client) {
	botlib_export->ea.EA_MoveForward(client);
	return 0;
}
int syscall_BOTLIB_EA_MOVE_BACK(int client) {
	botlib_export->ea.EA_MoveBack(client);
	return 0;
}
int syscall_BOTLIB_EA_MOVE_LEFT(int client) {
	botlib_export->ea.EA_MoveLeft(client);
	return 0;
}
int syscall_BOTLIB_EA_MOVE_RIGHT(int client) {
	botlib_export->ea.EA_MoveRight(client);
	return 0;
}


int syscall_BOTLIB_EA_MOVE(int client, vec3_t dir, float speed) {
	botlib_export->ea.EA_Move(client, dir, speed);
	return 0;
}
int syscall_BOTLIB_EA_VIEW(int client, vec3_t viewangles) {
	botlib_export->ea.EA_View(client, viewangles);
	return 0;
}
int syscall_BOTLIB_EA_END_REGULAR(int client, float thinktime) {
	botlib_export->ea.EA_EndRegular(client, thinktime);
	return 0;
}
int syscall_BOTLIB_EA_GET_INPUT(int client, float thinktime, bot_input_t* input) {
	botlib_export->ea.EA_GetInput(client, thinktime, input);
	return 0;
}
int syscall_BOTLIB_EA_RESET_INPUT(int client, bot_input_t* init) {
	botlib_export->ea.EA_ResetInput(client, init);
	return 0;
}
int syscall_BOTLIB_AI_LOAD_CHARACTER(char* charfile, int skill) {
	return botlib_export->ai.BotLoadCharacter(charfile, skill);
}
int syscall_BOTLIB_AI_FREE_CHARACTER(int handle) {
	botlib_export->ai.BotFreeCharacter(handle);
	return 0;
}
int syscall_BOTLIB_AI_CHARACTERISTIC_FLOAT(int character, int index) {
	return FloatAsInt(botlib_export->ai.Characteristic_Float(character, index));
}


int syscall_BOTLIB_AI_CHARACTERISTIC_BFLOAT(int character, int index, float minv, float maxv) {
	return FloatAsInt(botlib_export->ai.Characteristic_BFloat(character, index, minv, maxv));
}
int syscall_BOTLIB_AI_CHARACTERISTIC_INTEGER(int character, int index) {
	return botlib_export->ai.Characteristic_Integer(character, index);
}
int syscall_BOTLIB_AI_CHARACTERISTIC_BINTEGER(int character, int index, int minv, int maxv) {
	return botlib_export->ai.Characteristic_BInteger(character, index, minv, maxv);
}
int syscall_BOTLIB_AI_CHARACTERISTIC_STRING(int character, int index, char* buf, int size) {
	botlib_export->ai.Characteristic_String(character, index, buf, size);
	return 0;
}
int syscall_BOTLIB_AI_ALLOC_CHAT_STATE() {
	return botlib_export->ai.BotAllocChatState();
}
int syscall_BOTLIB_AI_FREE_CHAT_STATE(int handle) {
	botlib_export->ai.BotFreeChatState(handle);
	return 0;
}



int syscall_BOTLIB_AI_QUEUE_CONSOLE_MESSAGE(int chatstate, int type, char* message) {
	botlib_export->ai.BotQueueConsoleMessage(chatstate, type, message);
	return 0;
}
int syscall_BOTLIB_AI_REMOVE_CONSOLE_MESSAGE(int chatstate, int handle) {
	botlib_export->ai.BotRemoveConsoleMessage(chatstate, handle);
	return 0;
}
int syscall_BOTLIB_AI_NEXT_CONSOLE_MESSAGE(int chatstate, bot_consolemessage_t* cm) {
	return botlib_export->ai.BotNextConsoleMessage(chatstate, cm);
}
int syscall_BOTLIB_AI_NUM_CONSOLE_MESSAGE(int chatstate) {
	return botlib_export->ai.BotNumConsoleMessages(chatstate);
}
int syscall_BOTLIB_AI_INITIAL_CHAT(int chatstate, char* type, int mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7) {
	botlib_export->ai.BotInitialChat(chatstate, type, mcontext, var0, var1, var2, var3, var4, var5, var6, var7);
	return 0;
}
int syscall_BOTLIB_AI_NUM_INITIAL_CHATS(int chatstate, char* type) {
	return botlib_export->ai.BotNumInitialChats(chatstate, type);
}
int syscall_BOTLIB_AI_REPLY_CHAT(int chatstate, char* message, int mcontext, int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7) {
	return botlib_export->ai.BotReplyChat(chatstate, message, mcontext, vcontext, var0, var1, var2, var3, var4, var5, var6, var7);
}




int syscall_BOTLIB_AI_CHAT_LENGTH(int chatstate) {
	return botlib_export->ai.BotChatLength(chatstate);
}
int syscall_BOTLIB_AI_ENTER_CHAT(int chatstate, int client, int sendto) {
	botlib_export->ai.BotEnterChat(chatstate, client, sendto);
	return 0;
}
int syscall_BOTLIB_AI_GET_CHAT_MESSAGE(int chatstate, char* buf, int size) {
	botlib_export->ai.BotGetChatMessage(chatstate, buf, size);
	return 0;
}
int syscall_BOTLIB_AI_STRING_CONTAINS(char* str1, char* str2, int casesensitive) {
	return botlib_export->ai.StringContains(str1, str2, casesensitive);
}
int syscall_BOTLIB_AI_FIND_MATCH(char* str, bot_match_t* match, unsigned long int context) {
	return botlib_export->ai.BotFindMatch(str, match, context);
}
int syscall_BOTLIB_AI_MATCH_VARIABLE(bot_match_t* match, int variable, char* buf, int size) {
	botlib_export->ai.BotMatchVariable(match, variable, buf, size);
	return 0;
}



int syscall_BOTLIB_AI_UNIFY_WHITE_SPACES(char* string) {
	botlib_export->ai.UnifyWhiteSpaces(string);
	return 0;
}
int syscall_BOTLIB_AI_REPLACE_SYNONYMS(char* string, unsigned long int context) {
	botlib_export->ai.BotReplaceSynonyms(string, context);
	return 0;
}
int syscall_BOTLIB_AI_LOAD_CHAT_FILE(int chatstate, char* chatfile, char* chatname) {
	return botlib_export->ai.BotLoadChatFile(chatstate, chatfile, chatname);
}
int syscall_BOTLIB_AI_SET_CHAT_GENDER(int chatstate, int gender) {
	botlib_export->ai.BotSetChatGender(chatstate, gender);
	return 0;
}
int syscall_BOTLIB_AI_SET_CHAT_NAME(int chatstate, char* name) {
	botlib_export->ai.BotSetChatName(chatstate, name);
	return 0;
}
int syscall_BOTLIB_AI_RESET_GOAL_STATE(int goalstate) {
	botlib_export->ai.BotResetGoalState(goalstate);
	return 0;
}

int syscall_BOTLIB_AI_RESET_AVOID_GOALS(int goalstate) {
	botlib_export->ai.BotResetAvoidGoals(goalstate);
	return 0;
}
int syscall_BOTLIB_AI_REMOVE_FROM_AVOID_GOALS(int goalstate, int number) {
	botlib_export->ai.BotRemoveFromAvoidGoals(goalstate, number);
	return 0;
}
int syscall_BOTLIB_AI_PUSH_GOAL(int goalstate, bot_goal_t* goal) {
	botlib_export->ai.BotPushGoal(goalstate, goal);
	return 0;
}
int syscall_BOTLIB_AI_POP_GOAL(int goalstate) {
	botlib_export->ai.BotPopGoal(goalstate);
	return 0;
}
int syscall_BOTLIB_AI_EMPTY_GOAL_STACK(int goalstate) {
	botlib_export->ai.BotEmptyGoalStack(goalstate);
	return 0;
}


int syscall_BOTLIB_AI_DUMP_AVOID_GOALS(int goalstate) {
	botlib_export->ai.BotDumpAvoidGoals(goalstate);
	return 0;
}
int syscall_BOTLIB_AI_DUMP_GOAL_STACK(int goalstate) {
	botlib_export->ai.BotDumpGoalStack(goalstate);
	return 0;
}


int syscall_BOTLIB_AI_GOAL_NAME(int number, char* name, int size) {
	botlib_export->ai.BotGoalName(number, name, size);
	return 0;
}
int syscall_BOTLIB_AI_GET_TOP_GOAL(int goalstate, bot_goal_t* goal) {
	return botlib_export->ai.BotGetTopGoal(goalstate, goal);
}
int syscall_BOTLIB_AI_GET_SECOND_GOAL(int goalstate, bot_goal_t* goal) {
	return botlib_export->ai.BotGetSecondGoal(goalstate, goal);
}
int syscall_BOTLIB_AI_CHOOSE_LTG_ITEM(int goalstate, vec3_t origin, int* inventory, int travelflags) {
	return botlib_export->ai.BotChooseLTGItem(goalstate, origin, inventory, travelflags);
}
int syscall_BOTLIB_AI_CHOOSE_NBG_ITEM(int goalstate, vec3_t origin, int* inventory, int travelflags,
	bot_goal_t* ltg, float maxtime) {
	return botlib_export->ai.BotChooseNBGItem(goalstate, origin, inventory, travelflags, ltg, maxtime);
}
int syscall_BOTLIB_AI_TOUCHING_GOAL(vec3_t origin, bot_goal_t* goal) {
	return botlib_export->ai.BotTouchingGoal(origin, goal);
}
int syscall_BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE(int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t* goal) {
	return botlib_export->ai.BotItemGoalInVisButNotVisible(viewer, eye, viewangles, goal);
}

int syscall_BOTLIB_AI_GET_LEVEL_ITEM_GOAL(int index, char* name, bot_goal_t* goal) {
	return botlib_export->ai.BotGetLevelItemGoal(index, name, goal);
}
int syscall_BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL(int num, bot_goal_t* goal) {
	return botlib_export->ai.BotGetNextCampSpotGoal(num, goal);
}
int syscall_BOTLIB_AI_GET_MAP_LOCATION_GOAL(char* name, bot_goal_t* goal) {
	return botlib_export->ai.BotGetMapLocationGoal(name, goal);
}
int syscall_BOTLIB_AI_AVOID_GOAL_TIME(int goalstate, int number) {
	return FloatAsInt(botlib_export->ai.BotAvoidGoalTime(goalstate, number));
}
int syscall_BOTLIB_AI_INIT_LEVEL_ITEMS() {
	botlib_export->ai.BotInitLevelItems();
	return 0;
}
int syscall_BOTLIB_AI_UPDATE_ENTITY_ITEMS() {
	botlib_export->ai.BotUpdateEntityItems();
	return 0;
}
int syscall_BOTLIB_AI_LOAD_ITEM_WEIGHTS(int goalstate, char* filename) {
	return botlib_export->ai.BotLoadItemWeights(goalstate, filename);
}

int syscall_BOTLIB_AI_FREE_ITEM_WEIGHTS(int goalstate) {
	botlib_export->ai.BotFreeItemWeights(goalstate);
	return 0;
}
int syscall_BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC(int parent1, int parent2, int child) {
	botlib_export->ai.BotInterbreedGoalFuzzyLogic(parent1, parent2, child);
	return 0;
}
int syscall_BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC(int goalstate, char* filename) {
	botlib_export->ai.BotSaveGoalFuzzyLogic(goalstate, filename);
	return 0;
}
int syscall_BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC(int goalstate, float range) {
	botlib_export->ai.BotMutateGoalFuzzyLogic(goalstate, range);
	return 0;
}
int syscall_BOTLIB_AI_ALLOC_GOAL_STATE(int client) {
	return botlib_export->ai.BotAllocGoalState(client);
}
int syscall_BOTLIB_AI_FREE_GOAL_STATE(int handle) {
	botlib_export->ai.BotFreeGoalState(handle);
	return 0;
}


int syscall_BOTLIB_AI_RESET_MOVE_STATE(int movestate) {
	botlib_export->ai.BotResetMoveState(movestate);
	return 0;
}
int syscall_BOTLIB_AI_MOVE_TO_GOAL(bot_moveresult_t* result, int movestate, bot_goal_t* goal, int travelflags) {
	botlib_export->ai.BotMoveToGoal(result, movestate, goal, travelflags);
	return 0;
}
int syscall_BOTLIB_AI_MOVE_IN_DIRECTION(int movestate, vec3_t dir, float speed, int type) {
	return botlib_export->ai.BotMoveInDirection(movestate, dir, speed, type);
}
int syscall_BOTLIB_AI_RESET_AVOID_REACH(int movestate) {
	botlib_export->ai.BotResetAvoidReach(movestate);
	return 0;
}
int syscall_BOTLIB_AI_RESET_LAST_AVOID_REACH(int movestate) {
	botlib_export->ai.BotResetLastAvoidReach(movestate);
	return 0;
}
int syscall_BOTLIB_AI_REACHABILITY_AREA(vec3_t origin, int client) {
	return botlib_export->ai.BotReachabilityArea(origin, client);
}


int syscall_BOTLIB_AI_MOVEMENT_VIEW_TARGET(int movestate, bot_goal_t* goal, int travelflags, float lookahead, vec3_t target) {
	return botlib_export->ai.BotMovementViewTarget(movestate, goal, travelflags, lookahead, target);
}
int syscall_BOTLIB_AI_PREDICT_VISIBLE_POSITION(vec3_t origin, int areanum, bot_goal_t* goal, int travelflags, vec3_t target) {
	return botlib_export->ai.BotPredictVisiblePosition(origin, areanum, goal, travelflags, target);
}
int syscall_BOTLIB_AI_ALLOC_MOVE_STATE() {
	return botlib_export->ai.BotAllocMoveState();
}
int syscall_BOTLIB_AI_FREE_MOVE_STATE(int handle) {
	botlib_export->ai.BotFreeMoveState(handle);
	return 0;
}
int syscall_BOTLIB_AI_INIT_MOVE_STATE(int handle, bot_initmove_t* initmove) {
	botlib_export->ai.BotInitMoveState(handle, initmove);
	return 0;
}
// Ridah
int syscall_BOTLIB_AI_INIT_AVOID_REACH(int handle) {
	botlib_export->ai.BotInitAvoidReach(handle);
	return 0;
	// done.
}


int syscall_BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON(int weaponstate, int* inventory) {
	return botlib_export->ai.BotChooseBestFightWeapon(weaponstate, inventory);
}
int syscall_BOTLIB_AI_GET_WEAPON_INFO(int weaponstate, int weapon, weaponinfo_t* weaponinfo) {
	botlib_export->ai.BotGetWeaponInfo(weaponstate, weapon, weaponinfo);
	return 0;
}
int syscall_BOTLIB_AI_LOAD_WEAPON_WEIGHTS(int weaponstate, char* filename) {
	return botlib_export->ai.BotLoadWeaponWeights(weaponstate, filename);
}
int syscall_BOTLIB_AI_ALLOC_WEAPON_STATE() {
	return botlib_export->ai.BotAllocWeaponState();
}
int syscall_BOTLIB_AI_FREE_WEAPON_STATE(int handle) {
	botlib_export->ai.BotFreeWeaponState(handle);
	return 0;
}
int syscall_BOTLIB_AI_RESET_WEAPON_STATE(int weaponstate) {
	botlib_export->ai.BotResetWeaponState(weaponstate);
	return 0;
}
int syscall_BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION(int numranks, float* ranks, int* parent1, int* parent2, int* child) {
	return botlib_export->ai.GeneticParentsAndChildSelection(numranks, ranks, parent1, parent2, child);
}
int syscall_TRAP_MEMSET(void* _Dst, int    _Val, size_t _Size) {
	memset(_Dst, _Val, _Size);
	return 0;
}


int syscall_TRAP_MEMCPY(void* _Dst, void const* _Src, size_t      _Size) {
	memcpy(_Dst, _Src, _Size);
	return 0;
}
int syscall_TRAP_STRNCPY(char* _Destination, char const* _Source, size_t _Count) {
	return (int)strncpy(_Destination, _Source, _Count);
}
int syscall_TRAP_SIN(double _X) {
	return FloatAsInt(sin(_X));
}
int syscall_TRAP_COS(double _X) {
	return FloatAsInt(cos(_X));
}
int syscall_TRAP_ATAN2(double _Y, double _X) {
	return FloatAsInt(atan2(_Y, _X));
}
int syscall_TRAP_SQRT(double _X) {
	return FloatAsInt(sqrt(_X));
}
int syscall_TRAP_MATRIXMULTIPLY(float in1[3][3], float in2[3][3], float out[3][3]) {
	MatrixMultiply((float(*)[3])in1, (float(*)[3])in2, (float(*)[3]) out);
	return 0;
}
int syscall_TRAP_ANGLEVECTORS(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	AngleVectors(angles, forward, right, up);
	return 0;
}
int syscall_TRAP_PERPENDICULARVECTOR(vec3_t dst, const vec3_t src) {
	PerpendicularVector(dst, src);
	return 0;
}
int syscall_TRAP_FLOOR(double _X) {
	return FloatAsInt(floor(_X));
}
int syscall_TRAP_CEIL(double _X) {
	return FloatAsInt(ceil(_X));
}



/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs(void) {
	if (!gvm) {
		return;
	}
	VM_Call_GAME_SHUTDOWN(false);
	//VM_Free(gvm);
	//gvm = NULL;
	gvm = false;
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
void SV_InitGameVM(bool restart) {
	int i;

	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	for (i = 0; i < sv_maxclients->integer; i++) {
		svs.clients[i].gentity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	VM_Call_GAME_INIT(svs.time, Com_Milliseconds(), restart);
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs(void) {
	if (!gvm) {
		return;
	}
	VM_Call_GAME_SHUTDOWN(true);

	gvm = true;
	// do a restart instead of a free
	//gvm = VM_Restart( gvm );
	//if ( !gvm ) { // bk001212 - as done below
	//	Com_Error( ERR_FATAL, "VM_Restart on game failed" );
	//}

	SV_InitGameVM(true);
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs(void) {
	gvm = true;
	// load the dll
	//gvm = VM_Create( "qagame", SV_GameSystemCalls, VMI_NATIVE );
	//if ( !gvm ) {
	//	Com_Error( ERR_FATAL, "VM_Create on game failed" );
	//}

	SV_InitGameVM(false);
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
bool SV_GameCommand(void) {
	if (sv.state != SS_GAME) {
		return false;
	}

	return VM_Call_GAME_CONSOLE_COMMAND();
}


/*
====================
SV_SendMoveSpeedsToGame
====================
*/
void SV_SendMoveSpeedsToGame(int entnum, char* text) {
	if (!gvm) {
		return;
	}
	VM_Call_GAME_RETRIEVE_MOVESPEEDS_FROM_CLIENT(entnum, text);
}

/*
====================
SV_GetTag

  return false if unable to retrieve tag information for this client
====================
*/
extern bool CL_GetTag(int clientNum, char* tagname, orientation_t* orx);

bool SV_GetTag(int clientNum, char* tagname, orientation_t* orx) {
#ifndef DEDICATED // TTimo: dedicated only binary defines DEDICATED
	if (com_dedicated->integer) {
		return false;
	}

	return CL_GetTag(clientNum, tagname, orx);
#else
	return false;
#endif
}
