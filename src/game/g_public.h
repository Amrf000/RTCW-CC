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

#include "../game/be_aas.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_weap.h"
#include "../game/botlib.h"
// g_public.h -- game module information visible to server

#define GAME_API_VERSION    8

// entity->svFlags
// the server does not know how to interpret most of the values
// in entityStates (level eType), so the game must explicitly flag
// special server behaviors
#define SVF_NOCLIENT            0x00000001  // don't send entity to clients, even if it has effects
#define SVF_VISDUMMY            0x00000004  // this ent is a "visibility dummy" and needs it's master to be sent to clients that can see it even if they can't see the master ent
#define SVF_BOT                 0x00000008
// Wolfenstein
#define SVF_CASTAI              0x00000010
// done.
#define SVF_BROADCAST           0x00000020  // send to all connected clients
#define SVF_PORTAL              0x00000040  // merge a second pvs at origin2 into snapshots
#define SVF_USE_CURRENT_ORIGIN  0x00000080  // entity->r.currentOrigin instead of entity->s.origin
											// for link position (missiles and movers)
// Ridah
#define SVF_NOFOOTSTEPS         0x00000100
// done.
// MrE:
#define SVF_CAPSULE             0x00000200  // use capsule for collision detection

#define SVF_VISDUMMY_MULTIPLE   0x00000400  // so that one vis dummy can add to snapshot multiple speakers

// recent id changes
#define SVF_SINGLECLIENT        0x00000800  // only send to a single client (entityShared_t->singleClient)
#define SVF_NOSERVERINFO        0x00001000  // don't send CS_SERVERINFO updates to this client
											// so that it can be updated for ping tools without
											// lagging clients
#define SVF_NOTSINGLECLIENT     0x00002000  // send entity to everyone but one client
											// (entityShared_t->singleClient)

//===============================================================


typedef struct {
	entityState_t s;                // communicated by server to clients

	bool linked;                // false if not in any good cluster
	int linkcount;

	int svFlags;                    // SVF_NOCLIENT, SVF_BROADCAST, etc
	int singleClient;               // only send to this client when SVF_SINGLECLIENT is set

	bool bmodel;                // if false, assume an explicit mins / maxs bounding box
									// only set by trap_SetBrushModel
	vec3_t mins, maxs;
	int contents;                   // CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vec3_t absmin, absmax;          // derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

	// when a trace call is made and passEntityNum != ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
	// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
	int ownerNum;
	int eventTime;

	int worldflags;             // DHM - Nerve
} entityShared_t;



// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
typedef struct {
	entityState_t s;                // communicated by server to clients
	entityShared_t r;               // shared by both the server system and game
} sharedEntity_t;



//===============================================================

//
// system traps provided by the main engine
//
typedef enum {
	//============== general Quake services ==================

	G_PRINT,        // ( const char *string );
	// print message on the local console

	G_ERROR,        // ( const char *string );
	// abort the game

	G_MILLISECONDS, // ( void );
	// get current time for profiling reasons
	// this should NOT be used for any game related tasks,
	// because it is not journaled

	// console variable interaction
	G_CVAR_REGISTER,    // ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	G_CVAR_UPDATE,  // ( vmCvar_t *vmCvar );
	G_CVAR_SET,     // ( const char *var_name, const char *value );
	G_CVAR_VARIABLE_INTEGER_VALUE,  // ( const char *var_name );

	G_CVAR_VARIABLE_STRING_BUFFER,  // ( const char *var_name, char *buffer, int bufsize );

	G_ARGC,         // ( void );
	// ClientCommand and ServerCommand parameter access

	G_ARGV,         // ( int n, char *buffer, int bufferLength );

	G_FS_FOPEN_FILE,    // ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	G_FS_READ,      // ( void *buffer, int len, fileHandle_t f );
	G_FS_WRITE,     // ( const void *buffer, int len, fileHandle_t f );
	G_FS_RENAME,
	G_FS_FCLOSE_FILE,       // ( fileHandle_t f );

	G_SEND_CONSOLE_COMMAND, // ( const char *text );
	// add commands to the console as if they were typed in
	// for map changing, etc


	//=========== server specific functionality =============

	G_LOCATE_GAME_DATA,     // ( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	//							playerState_t *clients, int sizeofGameClient );
	// the game needs to let the server system know where and how big the gentities
	// are, so it can look at them directly without going through an interface

	G_DROP_CLIENT,      // ( int clientNum, const char *reason );
	// kick a client off the server with a message

	G_SEND_SERVER_COMMAND,  // ( int clientNum, const char *fmt, ... );
	// reliably sends a command string to be interpreted by the given
	// client.  If clientNum is -1, it will be sent to all clients

	G_SET_CONFIGSTRING, // ( int num, const char *string );
	// config strings hold all the index strings, and various other information
	// that is reliably communicated to all clients
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	// All confgstrings are cleared at each level start.

	G_GET_CONFIGSTRING, // ( int num, char *buffer, int bufferSize );

	G_GET_USERINFO,     // ( int num, char *buffer, int bufferSize );
	// userinfo strings are maintained by the server system, so they
	// are persistant across level loads, while all other game visible
	// data is completely reset

	G_SET_USERINFO,     // ( int num, const char *buffer );

	G_GET_SERVERINFO,   // ( char *buffer, int bufferSize );
	// the serverinfo info string has all the cvars visible to server browsers

	G_SET_BRUSH_MODEL,  // ( gentity_t *ent, const char *name );
	// sets mins and maxs based on the brushmodel name

	G_TRACE,    // ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection against all linked entities

	G_POINT_CONTENTS,   // ( const vec3_t point, int passEntityNum );
	// point contents against all linked entities

	G_IN_PVS,           // ( const vec3_t p1, const vec3_t p2 );

	G_IN_PVS_IGNORE_PORTALS,    // ( const vec3_t p1, const vec3_t p2 );

	G_ADJUST_AREA_PORTAL_STATE, // ( gentity_t *ent, bool open );

	G_AREAS_CONNECTED,  // ( int area1, int area2 );

	G_LINKENTITY,       // ( gentity_t *ent );
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.

	G_UNLINKENTITY,     // ( gentity_t *ent );
	// call before removing an interactive entity

	G_ENTITIES_IN_BOX,  // ( const vec3_t mins, const vec3_t maxs, gentity_t **list, int maxcount );
	// EntitiesInBox will return brush models based on their bounding box,
	// so exact determination must still be done with EntityContact

	G_ENTITY_CONTACT,   // ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape

	// access for bots to get and free a server client (FIXME?)
	G_BOT_ALLOCATE_CLIENT,  // ( void );

	G_BOT_FREE_CLIENT,  // ( int clientNum );

	G_GET_USERCMD,  // ( int clientNum, usercmd_t *cmd )

	G_GET_ENTITY_TOKEN, // bool ( char *buffer, int bufferSize )
	// Retrieves the next string token from the entity spawn text, returning
	// false when all tokens have been parsed.
	// This should only be done at GAME_INIT time.

	G_FS_GETFILELIST,
	G_DEBUG_POLYGON_CREATE,
	G_DEBUG_POLYGON_DELETE,
	G_REAL_TIME,
	G_SNAPVECTOR,
// MrE:

	G_TRACECAPSULE, // ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection using capsule against all linked entities

	G_ENTITY_CONTACTCAPSULE,    // ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape
// done.

	G_GETTAG,

	BOTLIB_SETUP = 200,             // ( void );
	BOTLIB_SHUTDOWN,                // ( void );
	BOTLIB_LIBVAR_SET,
	BOTLIB_LIBVAR_GET,
	BOTLIB_PC_ADD_GLOBAL_DEFINE,
	BOTLIB_START_FRAME,
	BOTLIB_LOAD_MAP,
	BOTLIB_UPDATENTITY,
	BOTLIB_TEST,

	BOTLIB_GET_SNAPSHOT_ENTITY,     // ( int client, int ent );
	BOTLIB_GET_CONSOLE_MESSAGE,     // ( int client, char *message, int size );
	BOTLIB_USER_COMMAND,            // ( int client, usercmd_t *ucmd );

	BOTLIB_AAS_ENTITY_VISIBLE = 300,    //FIXME: remove
	BOTLIB_AAS_IN_FIELD_OF_VISION,      //FIXME: remove
	BOTLIB_AAS_VISIBLE_CLIENTS,         //FIXME: remove
	BOTLIB_AAS_ENTITY_INFO,

	BOTLIB_AAS_INITIALIZED,
	BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX,
	BOTLIB_AAS_TIME,

	// Ridah
	BOTLIB_AAS_SETCURRENTWORLD,
	// done.

	BOTLIB_AAS_POINT_AREA_NUM,
	BOTLIB_AAS_TRACE_AREAS,

	BOTLIB_AAS_POINT_CONTENTS,
	BOTLIB_AAS_NEXT_BSP_ENTITY,
	BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY,
	BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY,

	BOTLIB_AAS_AREA_REACHABILITY,

	BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA,

	BOTLIB_AAS_SWIMMING,
	BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT,

	// Ridah, route-tables
	BOTLIB_AAS_RT_SHOWROUTE,
	BOTLIB_AAS_RT_GETHIDEPOS,
	BOTLIB_AAS_FINDATTACKSPOTWITHINRANGE,
	BOTLIB_AAS_SETAASBLOCKINGENTITY,
	// done.

	BOTLIB_EA_SAY = 400,
	BOTLIB_EA_SAY_TEAM,
	BOTLIB_EA_USE_ITEM,
	BOTLIB_EA_DROP_ITEM,
	BOTLIB_EA_USE_INV,
	BOTLIB_EA_DROP_INV,
	BOTLIB_EA_GESTURE,
	BOTLIB_EA_COMMAND,

	BOTLIB_EA_SELECT_WEAPON,
	BOTLIB_EA_TALK,
	BOTLIB_EA_ATTACK,
	BOTLIB_EA_RELOAD,
	BOTLIB_EA_USE,
	BOTLIB_EA_RESPAWN,
	BOTLIB_EA_JUMP,
	BOTLIB_EA_DELAYED_JUMP,
	BOTLIB_EA_CROUCH,
	BOTLIB_EA_MOVE_UP,
	BOTLIB_EA_MOVE_DOWN,
	BOTLIB_EA_MOVE_FORWARD,
	BOTLIB_EA_MOVE_BACK,
	BOTLIB_EA_MOVE_LEFT,
	BOTLIB_EA_MOVE_RIGHT,
	BOTLIB_EA_MOVE,
	BOTLIB_EA_VIEW,

	BOTLIB_EA_END_REGULAR,
	BOTLIB_EA_GET_INPUT,
	BOTLIB_EA_RESET_INPUT,


	BOTLIB_AI_LOAD_CHARACTER = 500,
	BOTLIB_AI_FREE_CHARACTER,
	BOTLIB_AI_CHARACTERISTIC_FLOAT,
	BOTLIB_AI_CHARACTERISTIC_BFLOAT,
	BOTLIB_AI_CHARACTERISTIC_INTEGER,
	BOTLIB_AI_CHARACTERISTIC_BINTEGER,
	BOTLIB_AI_CHARACTERISTIC_STRING,

	BOTLIB_AI_ALLOC_CHAT_STATE,
	BOTLIB_AI_FREE_CHAT_STATE,
	BOTLIB_AI_QUEUE_CONSOLE_MESSAGE,
	BOTLIB_AI_REMOVE_CONSOLE_MESSAGE,
	BOTLIB_AI_NEXT_CONSOLE_MESSAGE,
	BOTLIB_AI_NUM_CONSOLE_MESSAGE,
	BOTLIB_AI_INITIAL_CHAT,
	BOTLIB_AI_REPLY_CHAT,
	BOTLIB_AI_CHAT_LENGTH,
	BOTLIB_AI_ENTER_CHAT,
	BOTLIB_AI_STRING_CONTAINS,
	BOTLIB_AI_FIND_MATCH,
	BOTLIB_AI_MATCH_VARIABLE,
	BOTLIB_AI_UNIFY_WHITE_SPACES,
	BOTLIB_AI_REPLACE_SYNONYMS,
	BOTLIB_AI_LOAD_CHAT_FILE,
	BOTLIB_AI_SET_CHAT_GENDER,
	BOTLIB_AI_SET_CHAT_NAME,

	BOTLIB_AI_RESET_GOAL_STATE,
	BOTLIB_AI_RESET_AVOID_GOALS,
	BOTLIB_AI_PUSH_GOAL,
	BOTLIB_AI_POP_GOAL,
	BOTLIB_AI_EMPTY_GOAL_STACK,
	BOTLIB_AI_DUMP_AVOID_GOALS,
	BOTLIB_AI_DUMP_GOAL_STACK,
	BOTLIB_AI_GOAL_NAME,
	BOTLIB_AI_GET_TOP_GOAL,
	BOTLIB_AI_GET_SECOND_GOAL,
	BOTLIB_AI_CHOOSE_LTG_ITEM,
	BOTLIB_AI_CHOOSE_NBG_ITEM,
	BOTLIB_AI_TOUCHING_GOAL,
	BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE,
	BOTLIB_AI_GET_LEVEL_ITEM_GOAL,
	BOTLIB_AI_AVOID_GOAL_TIME,
	BOTLIB_AI_INIT_LEVEL_ITEMS,
	BOTLIB_AI_UPDATE_ENTITY_ITEMS,
	BOTLIB_AI_LOAD_ITEM_WEIGHTS,
	BOTLIB_AI_FREE_ITEM_WEIGHTS,
	BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC,
	BOTLIB_AI_ALLOC_GOAL_STATE,
	BOTLIB_AI_FREE_GOAL_STATE,

	BOTLIB_AI_RESET_MOVE_STATE,
	BOTLIB_AI_MOVE_TO_GOAL,
	BOTLIB_AI_MOVE_IN_DIRECTION,
	BOTLIB_AI_RESET_AVOID_REACH,
	BOTLIB_AI_RESET_LAST_AVOID_REACH,
	BOTLIB_AI_REACHABILITY_AREA,
	BOTLIB_AI_MOVEMENT_VIEW_TARGET,
	BOTLIB_AI_ALLOC_MOVE_STATE,
	BOTLIB_AI_FREE_MOVE_STATE,
	BOTLIB_AI_INIT_MOVE_STATE,
	// Ridah
	BOTLIB_AI_INIT_AVOID_REACH,
	// done.

	BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON,
	BOTLIB_AI_GET_WEAPON_INFO,
	BOTLIB_AI_LOAD_WEAPON_WEIGHTS,
	BOTLIB_AI_ALLOC_WEAPON_STATE,
	BOTLIB_AI_FREE_WEAPON_STATE,
	BOTLIB_AI_RESET_WEAPON_STATE,

	BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION,
	BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC,
	BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC,
	BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL,
	BOTLIB_AI_GET_MAP_LOCATION_GOAL,
	BOTLIB_AI_NUM_INITIAL_CHATS,
	BOTLIB_AI_GET_CHAT_MESSAGE,
	BOTLIB_AI_REMOVE_FROM_AVOID_GOALS,
	BOTLIB_AI_PREDICT_VISIBLE_POSITION,

	BOTLIB_AI_SET_AVOID_GOAL_TIME,
	BOTLIB_AI_ADD_AVOID_SPOT,
	BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL,
	BOTLIB_AAS_PREDICT_ROUTE,
	BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX,

	BOTLIB_PC_LOAD_SOURCE,
	BOTLIB_PC_FREE_SOURCE,
	BOTLIB_PC_READ_TOKEN,
	BOTLIB_PC_SOURCE_FILE_AND_LINE

} gameImport_t;


//
// functions exported by the game subsystem
//
//typedef enum {
//	GAME_INIT,  // ( int levelTime, int randomSeed, int restart );
//	// init and shutdown will be called every single level
//	// The game should call G_GET_ENTITY_TOKEN to parse through all the
//	// entity configuration text and spawn gentities.
//
//	GAME_SHUTDOWN,  // (void);
//
//	GAME_CLIENT_CONNECT,    // ( int clientNum, bool firstTime, bool isBot );
//	// return NULL if the client is allowed to connect, otherwise return
//	// a text string with the reason for denial
//
//	GAME_CLIENT_BEGIN,              // ( int clientNum );
//
//	GAME_CLIENT_USERINFO_CHANGED,   // ( int clientNum );
//
//	GAME_CLIENT_DISCONNECT,         // ( int clientNum );
//
//	GAME_CLIENT_COMMAND,            // ( int clientNum );
//
//	GAME_CLIENT_THINK,              // ( int clientNum );
//
//	GAME_RUN_FRAME,                 // ( int levelTime );
//
//	GAME_CONSOLE_COMMAND,           // ( void );
//	// ConsoleCommand will be called when a command has been issued
//	// that is not recognized as a builtin function.
//	// The game can issue trap_argc() / trap_argv() commands to get the command
//	// and parameters.  Return false if the game doesn't recognize it as a command.
//
//	BOTAI_START_FRAME               // ( int time );
//
//	// Ridah, Cast AI
//	,AICAST_VISIBLEFROMPOS
//	,AICAST_CHECKATTACKATPOS
//	// done.
//
//	,GAME_RETRIEVE_MOVESPEEDS_FROM_CLIENT
//
//} gameExport_t;
int VM_Call_GAME_INIT(int levelTime, int randomSeed, int restart);
int  VM_Call_GAME_SHUTDOWN(int restart);
char* VM_Call_GAME_CLIENT_CONNECT(int clientNum, bool firstTime, bool isBot);
int VM_Call_GAME_CLIENT_THINK(int clientNum);
int VM_Call_GAME_CLIENT_USERINFO_CHANGED(int clientNum);
int VM_Call_GAME_CLIENT_DISCONNECT(int clientNum);
int VM_Call_GAME_CLIENT_BEGIN(int clientNum);
int VM_Call_GAME_CLIENT_COMMAND(int clientNum);
int VM_Call_GAME_RUN_FRAME(int levelTime);
bool VM_Call_GAME_CONSOLE_COMMAND();
int VM_Call_BOTAI_START_FRAME(int time);
bool VM_Call_AICAST_VISIBLEFROMPOS(vec3_t srcpos, int srcnum,
	vec3_t destpos, int destnum, bool updateVisPos);
bool VM_Call_AICAST_CHECKATTACKATPOS(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld);
int VM_Call_GAME_RETRIEVE_MOVESPEEDS_FROM_CLIENT(int entnum, char* text);

int syscall_G_PRINT(const char* fmt);
int syscall_G_ERROR(const char* fmt);
int syscall_G_MILLISECONDS();
int syscall_G_CVAR_REGISTER(vmCvar_t* vmCvar, const char* varName, const char* defaultValue, int flags);
int syscall_G_CVAR_UPDATE(vmCvar_t* vmCvar);
int syscall_G_CVAR_SET(const char* var_name, const char* value);
int syscall_G_CVAR_VARIABLE_INTEGER_VALUE(const char* var_name);
int syscall_G_CVAR_VARIABLE_STRING_BUFFER(const char* var_name, char* buffer, int bufsize);
int syscall_G_ARGC();
int syscall_G_ARGV(int arg, char* buffer, int bufferLength);
int syscall_G_SEND_CONSOLE_COMMAND(int exec_when, const char* text);
int syscall_G_FS_FOPEN_FILE(const char* qpath, fileHandle_t* f, fsMode_t mode);
int syscall_G_FS_READ(void* buffer, int len, fileHandle_t f);
int syscall_G_FS_WRITE(const void* buffer, int len, fileHandle_t h);
int syscall_G_FS_RENAME(const char* from, const char* to);
int syscall_G_FS_FCLOSE_FILE(fileHandle_t f);
int syscall_G_FS_GETFILELIST(const char* path, const char* extension, char* listbuf, int bufsize);
int syscall_G_LOCATE_GAME_DATA(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t,
	playerState_t* clients, int sizeofGameClient);
int syscall_G_DROP_CLIENT(int clientNum, const char* reason);
int syscall_G_SEND_SERVER_COMMAND(int clientNum, const char* text);
int syscall_G_LINKENTITY(sharedEntity_t* gEnt);
int syscall_G_UNLINKENTITY(sharedEntity_t* gEnt);
int syscall_G_ENTITIES_IN_BOX(const vec3_t mins, const vec3_t maxs, int* entityList, int maxcount);
int syscall_G_ENTITY_CONTACT(const vec3_t mins, const vec3_t maxs, const sharedEntity_t* gEnt);
int syscall_G_ENTITY_CONTACTCAPSULE(const vec3_t mins, const vec3_t maxs, const sharedEntity_t* gEnt);
int syscall_G_TRACE(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
int syscall_G_TRACECAPSULE(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
int syscall_G_POINT_CONTENTS(const vec3_t p, int passEntityNum);
int syscall_G_SET_BRUSH_MODEL(sharedEntity_t* ent, const char* nam);
int syscall_G_IN_PVS(const vec3_t p1, const vec3_t p2);
int syscall_G_IN_PVS_IGNORE_PORTALS(const vec3_t p1, const vec3_t p2);
int syscall_G_SET_CONFIGSTRING(int index, const char* val);
int syscall_G_GET_CONFIGSTRING(int index, char* buffer, int bufferSize);
int syscall_G_SET_USERINFO(int index, const char* val);
int syscall_G_GET_USERINFO(int index, char* buffer, int bufferSize);
int syscall_G_GET_SERVERINFO(char* buffer, int bufferSize);
int syscall_G_ADJUST_AREA_PORTAL_STATE(sharedEntity_t* ent, bool open_);
int syscall_G_AREAS_CONNECTED(int area1, int area2);
int syscall_G_BOT_ALLOCATE_CLIENT();
int syscall_G_BOT_FREE_CLIENT(int clientNum);
int syscall_G_GET_USERCMD(int clientNum, usercmd_t* cmd);
int syscall_G_GET_ENTITY_TOKEN(char* dest, int destsize);
int syscall_G_DEBUG_POLYGON_CREATE(int color, int numPoints, vec3_t* points);
int syscall_G_DEBUG_POLYGON_DELETE(int id);
int syscall_G_REAL_TIME(qtime_t* qtime);
int syscall_G_SNAPVECTOR(float* v);
int syscall_G_GETTAG(int clientNum, char* tagname, orientation_t* orx);
int syscall_BOTLIB_SETUP();
int syscall_BOTLIB_SHUTDOWN();
int syscall_BOTLIB_LIBVAR_SET(char* var_name, char* value);
int syscall_BOTLIB_LIBVAR_GET(char* var_name, char* value, int size);
int syscall_BOTLIB_PC_ADD_GLOBAL_DEFINE(char* string);
int syscall_BOTLIB_PC_LOAD_SOURCE(const char* filename);
int syscall_BOTLIB_PC_FREE_SOURCE(int handle);
int syscall_BOTLIB_PC_READ_TOKEN(int handle, pc_token_t* pc_token);
int syscall_BOTLIB_PC_SOURCE_FILE_AND_LINE(int handle, char* filename, int* line);
int syscall_BOTLIB_START_FRAME(float time);
int syscall_BOTLIB_LOAD_MAP(const char* mapname);
int syscall_BOTLIB_UPDATENTITY(int ent, bot_entitystate_t* state);
int syscall_BOTLIB_TEST(int parm0, char* parm1, vec3_t parm2, vec3_t parm3);
int syscall_BOTLIB_GET_SNAPSHOT_ENTITY(int client, int sequenc);
int syscall_BOTLIB_GET_CONSOLE_MESSAGE(int client, char* buf, int size);
int syscall_BOTLIB_USER_COMMAND(int clientNum, usercmd_t* ucmd);
int syscall_BOTLIB_AAS_ENTITY_INFO(int entnum, aas_entityinfo_t* info);
int syscall_BOTLIB_AAS_INITIALIZED();
int syscall_BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX(int presencetype, vec3_t mins, vec3_t maxs);
int syscall_BOTLIB_AAS_TIME();
int syscall_BOTLIB_AAS_SETCURRENTWORLD(int index);
int syscall_BOTLIB_AAS_POINT_AREA_NUM(vec3_t point);
int syscall_BOTLIB_AAS_TRACE_AREAS(vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas);
int syscall_BOTLIB_AAS_POINT_CONTENTS(vec3_t point);
int syscall_BOTLIB_AAS_NEXT_BSP_ENTITY(int ent);
int syscall_BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY(int ent, char* key, char* value, int size);
int syscall_BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY(int ent, char* key, vec3_t v);
int syscall_BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY(int ent, char* key, float* value);
int syscall_BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY(int ent, char* key, int* value);
int syscall_BOTLIB_AAS_AREA_REACHABILITY(int areanum);
int syscall_BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA(int areanum, vec3_t origin, int goalareanum, int travelflag);
int syscall_BOTLIB_AAS_SWIMMING(vec3_t origin);
int syscall_BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT(struct aas_clientmove_s* move,
	int entnum, vec3_t origin,
	int presencetype, int onground,
	vec3_t velocity, vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, int visualize);
int syscall_BOTLIB_AAS_RT_SHOWROUTE(vec3_t srcpos, int srcnum, int destnum);
int syscall_BOTLIB_AAS_RT_GETHIDEPOS(vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos);
int syscall_BOTLIB_AAS_FINDATTACKSPOTWITHINRANGE(int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float* outpos);
int syscall_BOTLIB_AAS_SETAASBLOCKINGENTITY(vec3_t absmin, vec3_t absmax, bool blocking);
int syscall_BOTLIB_EA_SAY(int client, char* str);
int syscall_BOTLIB_EA_SAY_TEAM(int client, char* str);
int syscall_BOTLIB_EA_USE_ITEM(int client, char* it);
int syscall_BOTLIB_EA_DROP_ITEM(int client, char* it);
int syscall_BOTLIB_EA_USE_INV(int client, char* inv);
int syscall_BOTLIB_EA_DROP_INV(int client, char* inv);
int syscall_BOTLIB_EA_GESTURE(int client);
int syscall_BOTLIB_EA_COMMAND(int client, char* command);
int syscall_BOTLIB_EA_SELECT_WEAPON(int client, int weapon);
int syscall_BOTLIB_EA_TALK(int client);
int syscall_BOTLIB_EA_ATTACK(int client);
int syscall_BOTLIB_EA_RELOAD(int client);
int syscall_BOTLIB_EA_USE(int client);
int syscall_BOTLIB_EA_RESPAWN(int client);
int syscall_BOTLIB_EA_JUMP(int client);
int syscall_BOTLIB_EA_DELAYED_JUMP(int client);
int syscall_BOTLIB_EA_CROUCH(int client);
int syscall_BOTLIB_EA_MOVE_UP(int client);
int syscall_BOTLIB_EA_MOVE_DOWN(int client);
int syscall_BOTLIB_EA_MOVE_FORWARD(int client);
int syscall_BOTLIB_EA_MOVE_BACK(int client);
int syscall_BOTLIB_EA_MOVE_LEFT(int client);
int syscall_BOTLIB_EA_MOVE_RIGHT(int client);
int syscall_BOTLIB_EA_MOVE(int client, vec3_t dir, float speed);
int syscall_BOTLIB_EA_VIEW(int client, vec3_t viewangles);
int syscall_BOTLIB_EA_END_REGULAR(int client, float thinktime);
int syscall_BOTLIB_EA_GET_INPUT(int client, float thinktime, bot_input_t* input);
int syscall_BOTLIB_EA_RESET_INPUT(int client, bot_input_t* init);
int syscall_BOTLIB_AI_LOAD_CHARACTER(char* charfile, int skill);
int syscall_BOTLIB_AI_FREE_CHARACTER(int handle);
int syscall_BOTLIB_AI_CHARACTERISTIC_FLOAT(int character, int index);
int syscall_BOTLIB_AI_CHARACTERISTIC_BFLOAT(int character, int index, float minv, float maxv);
int syscall_BOTLIB_AI_CHARACTERISTIC_INTEGER(int character, int index);
int syscall_BOTLIB_AI_CHARACTERISTIC_BINTEGER(int character, int index, int minv, int maxv);
int syscall_BOTLIB_AI_CHARACTERISTIC_STRING(int character, int index, char* buf, int size);
int syscall_BOTLIB_AI_ALLOC_CHAT_STATE();
int syscall_BOTLIB_AI_FREE_CHAT_STATE(int handle);
int syscall_BOTLIB_AI_QUEUE_CONSOLE_MESSAGE(int chatstate, int type, char* message);
int syscall_BOTLIB_AI_REMOVE_CONSOLE_MESSAGE(int chatstate, int handle);
int syscall_BOTLIB_AI_NEXT_CONSOLE_MESSAGE(int chatstate, bot_consolemessage_t* cm);
int syscall_BOTLIB_AI_NUM_CONSOLE_MESSAGE(int chatstate);
int syscall_BOTLIB_AI_INITIAL_CHAT(int chatstate, char* type, int mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7);
int syscall_BOTLIB_AI_NUM_INITIAL_CHATS(int chatstate, char* type);
int syscall_BOTLIB_AI_REPLY_CHAT(int chatstate, char* message, int mcontext, int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7);
int syscall_BOTLIB_AI_CHAT_LENGTH(int chatstate);
int syscall_BOTLIB_AI_ENTER_CHAT(int chatstate, int client, int sendto);
int syscall_BOTLIB_AI_GET_CHAT_MESSAGE(int chatstate, char* buf, int size);
int syscall_BOTLIB_AI_STRING_CONTAINS(char* str1, char* str2, int casesensitive);
int syscall_BOTLIB_AI_FIND_MATCH(char* str, bot_match_t* match, unsigned long int context);
int syscall_BOTLIB_AI_MATCH_VARIABLE(bot_match_t* match, int variable, char* buf, int size);
int syscall_BOTLIB_AI_UNIFY_WHITE_SPACES(char* string);
int syscall_BOTLIB_AI_REPLACE_SYNONYMS(char* string, unsigned long int context);
int syscall_BOTLIB_AI_LOAD_CHAT_FILE(int chatstate, char* chatfile, char* chatname);
int syscall_BOTLIB_AI_SET_CHAT_GENDER(int chatstate, int gender);
int syscall_BOTLIB_AI_SET_CHAT_NAME(int chatstate, char* name);
int syscall_BOTLIB_AI_RESET_GOAL_STATE(int goalstate);
int syscall_BOTLIB_AI_RESET_AVOID_GOALS(int goalstate);
int syscall_BOTLIB_AI_REMOVE_FROM_AVOID_GOALS(int goalstate, int number);
int syscall_BOTLIB_AI_PUSH_GOAL(int goalstate, bot_goal_t* goal);
int syscall_BOTLIB_AI_POP_GOAL(int goalstate);
int syscall_BOTLIB_AI_EMPTY_GOAL_STACK(int goalstate);
int syscall_BOTLIB_AI_DUMP_AVOID_GOALS(int goalstate);
int syscall_BOTLIB_AI_DUMP_GOAL_STACK(int goalstate);
int syscall_BOTLIB_AI_GOAL_NAME(int number, char* name, int size);
int syscall_BOTLIB_AI_GET_TOP_GOAL(int goalstate, bot_goal_t* goal);
int syscall_BOTLIB_AI_GET_SECOND_GOAL(int goalstate, bot_goal_t* goal);
int syscall_BOTLIB_AI_CHOOSE_LTG_ITEM(int goalstate, vec3_t origin, int* inventory, int travelflags);
int syscall_BOTLIB_AI_CHOOSE_NBG_ITEM(int goalstate, vec3_t origin, int* inventory, int travelflags,
	bot_goal_t* ltg, float maxtime);
int syscall_BOTLIB_AI_TOUCHING_GOAL(vec3_t origin, bot_goal_t* goal);
int syscall_BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE(int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t* goal);
int syscall_BOTLIB_AI_GET_LEVEL_ITEM_GOAL(int index, char* name, bot_goal_t* goal);
int syscall_BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL(int num, bot_goal_t* goal);
int syscall_BOTLIB_AI_GET_MAP_LOCATION_GOAL(char* name, bot_goal_t* goal);
int syscall_BOTLIB_AI_AVOID_GOAL_TIME(int goalstate, int number);
int syscall_BOTLIB_AI_INIT_LEVEL_ITEMS();
int syscall_BOTLIB_AI_UPDATE_ENTITY_ITEMS();
int syscall_BOTLIB_AI_LOAD_ITEM_WEIGHTS(int goalstate, char* filename);
int syscall_BOTLIB_AI_FREE_ITEM_WEIGHTS(int goalstate);
int syscall_BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC(int parent1, int parent2, int child);
int syscall_BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC(int goalstate, char* filename);
int syscall_BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC(int goalstate, float range);
int syscall_BOTLIB_AI_ALLOC_GOAL_STATE(int client);
int syscall_BOTLIB_AI_FREE_GOAL_STATE(int handle);
int syscall_BOTLIB_AI_RESET_MOVE_STATE(int movestate);
int syscall_BOTLIB_AI_MOVE_TO_GOAL(bot_moveresult_t* result, int movestate, bot_goal_t* goal, int travelflags);
int syscall_BOTLIB_AI_MOVE_IN_DIRECTION(int movestate, vec3_t dir, float speed, int type);
int syscall_BOTLIB_AI_RESET_AVOID_REACH(int movestate);
int syscall_BOTLIB_AI_RESET_LAST_AVOID_REACH(int movestate);
int syscall_BOTLIB_AI_REACHABILITY_AREA(vec3_t origin, int client);
int syscall_BOTLIB_AI_MOVEMENT_VIEW_TARGET(int movestate, bot_goal_t* goal, int travelflags, float lookahead, vec3_t target);
int syscall_BOTLIB_AI_PREDICT_VISIBLE_POSITION(vec3_t origin, int areanum, bot_goal_t* goal, int travelflags, vec3_t target);
int syscall_BOTLIB_AI_ALLOC_MOVE_STATE();
int syscall_BOTLIB_AI_FREE_MOVE_STATE(int handle);
int syscall_BOTLIB_AI_INIT_MOVE_STATE(int handle, bot_initmove_t* initmove);
	int syscall_BOTLIB_AI_INIT_AVOID_REACH(int handle);
	int syscall_BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON(int weaponstate, int* inventory);
	int syscall_BOTLIB_AI_GET_WEAPON_INFO(int weaponstate, int weapon, weaponinfo_t* weaponinfo);
	int syscall_BOTLIB_AI_LOAD_WEAPON_WEIGHTS(int weaponstate, char* filename);
	int syscall_BOTLIB_AI_ALLOC_WEAPON_STATE();
	int syscall_BOTLIB_AI_FREE_WEAPON_STATE(int handle);
	int syscall_BOTLIB_AI_RESET_WEAPON_STATE(int weaponstate);
	int syscall_BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION(int numranks, float* ranks, int* parent1, int* parent2, int* child);