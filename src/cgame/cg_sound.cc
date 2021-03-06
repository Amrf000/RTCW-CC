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

// Ridah, cg_sound.c - parsing and use of sound script files

#include "cg_local.h"

typedef struct soundScriptSound_s
{
	char filename[MAX_QPATH];
	sfxHandle_t sfxHandle;
	int lastPlayed;

	struct soundScriptSound_s   *next;
} soundScriptSound_t;

typedef struct soundScript_s
{
	int index;
	char name[MAX_QPATH];
	int channel;
	int attenuation;
	bool streaming;
	bool looping;
	bool random;    // TODO
	int numSounds;
	soundScriptSound_t  *soundList;         // pointer into the global list of soundScriptSounds (defined below)

	struct soundScript_s    *nextHash;      // next soundScript in our hashTable list position
} soundScript_t;

// we have to define these static lists, since we can't alloc memory within the cgame

#define FILE_HASH_SIZE          1024
static soundScript_t*      hashTable[FILE_HASH_SIZE];

#define MAX_SOUND_SCRIPTS       4096
static soundScript_t soundScripts[MAX_SOUND_SCRIPTS];
int numSoundScripts = 0;

#define MAX_SOUND_SCRIPT_SOUNDS 8192
static soundScriptSound_t soundScriptSounds[MAX_SOUND_SCRIPT_SOUNDS];
int numSoundScriptSounds = 0;

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while ( fname[i] != '\0' ) {
		letter = tolower( fname[i] );
		if ( letter == '.' ) {
			break;                          // don't include extension
		}
		if ( letter == '\\' ) {
			letter = '/';                   // damn path names
		}
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( FILE_HASH_SIZE - 1 );
	return hash;
}

/*
==============
CG_SoundScriptPrecache

  returns the index+1 of the script in the global list, for fast calling
==============
*/
int CG_SoundScriptPrecache( const char *name ) {
	soundScriptSound_t *scriptSound;
	long hash;
	char *s;
	soundScript_t   *sound;

	if ( !name || !name[0] ) {
		return 0;
	}

	hash = generateHashValue( name );

	s = (char *)name;
	sound = hashTable[hash];
	while ( sound ) {
		if ( !Q_strcasecmp( s, sound->name ) ) {
			// found a match, precache these sounds
			scriptSound = sound->soundList;
			if ( !sound->streaming ) {
				while ( scriptSound ) {
					scriptSound->sfxHandle = trap_S_RegisterSound( scriptSound->filename );
					scriptSound = scriptSound->next;
				}
			} else if ( cg_buildScript.integer ) {
				while ( scriptSound ) {
					// just open the file so it gets copied to the build dir
					fileHandle_t f;
					trap_FS_FOpenFile( scriptSound->filename, &f, FS_READ );
					trap_FS_FCloseFile( f );
					scriptSound = scriptSound->next;
				}
			}
			return sound->index + 1;
		}
		sound = sound->nextHash;
	}

	return 0;
}

/*
==============
CG_SoundPickOldestRandomSound
==============
*/
void CG_SoundPickOldestRandomSound( soundScript_t *sound, vec3_t org, int entnum ) {
	int oldestTime = 0; // TTimo: init
	soundScriptSound_t *oldestSound;
	soundScriptSound_t *scriptSound;

	oldestSound = NULL;
	scriptSound = sound->soundList;
	while ( scriptSound ) {
		if ( !oldestSound || ( scriptSound->lastPlayed < oldestTime ) ) {
			oldestTime = scriptSound->lastPlayed;
			oldestSound = scriptSound;
		}
		scriptSound = scriptSound->next;
	}

	if ( oldestSound ) {
		// play this sound
		if ( !sound->streaming ) {
			if ( !oldestSound->sfxHandle ) {
				oldestSound->sfxHandle = trap_S_RegisterSound( oldestSound->filename );
			}
			trap_S_StartSound( org, entnum, sound->channel, oldestSound->sfxHandle );
		} else {
			trap_S_StartStreamingSound( oldestSound->filename, sound->looping ? oldestSound->filename : NULL, entnum, sound->channel, sound->attenuation );
		}
		oldestSound->lastPlayed = cg.time;
	} else {
		CG_Error( "Unable to locate a valid sound for soundScript: %s\n", sound->name );
	}
}

/*
==============
CG_SoundPlaySoundScript

  returns true is a script is found
==============
*/
bool CG_SoundPlaySoundScript( const char *name, vec3_t org, int entnum ) {
	long hash;
	char *s;
	soundScript_t   *sound;

	if ( !name || !name[0] ) {
		return false;
	}

	hash = generateHashValue( name );

	s = (char *)name;
	sound = hashTable[hash];
	while ( sound ) {
		if ( !Q_strcasecmp( s, sound->name ) ) {
			// found a match, pick the oldest sound
			CG_SoundPickOldestRandomSound( sound, org, entnum );
			return true;
		}
		sound = sound->nextHash;
	}

	//CG_Printf( S_COLOR_RED "CG_SoundPlaySoundScript: cannot find sound script '%s'\n", name );
	return false;
}

/*
==============
CG_SoundPlayIndexedScript

  returns true is a script is found
==============
*/
void CG_SoundPlayIndexedScript( int index, vec3_t org, int entnum ) {
	soundScript_t   *sound;

	if ( !index ) {
		return;
	}

	if ( index > numSoundScripts ) {
		return;
	}

	sound = &soundScripts[index - 1];
	// pick the oldest sound
	CG_SoundPickOldestRandomSound( sound, org, entnum );
}

/*
===============
CG_SoundParseSounds
===============
*/
static void CG_SoundParseSounds( char *filename, char *buffer ) {
	char *token, **text;
	int s;
	long hash;
	soundScript_t sound;                // the current sound being read
	soundScriptSound_t  *scriptSound;
	bool inSound, wantSoundName;

	s = 0;
	inSound = false;
	wantSoundName = true;
	text = &buffer;

	while ( 1 ) {
		token = COM_ParseExt( text, true );
		if ( !token[0] ) {
			if ( inSound ) {
				CG_Error( "no concluding '}' in sound %s, file %s\n", sound.name, filename );
			}
			return;
		}
		if ( !Q_strcasecmp( token, "{" ) ) {
			if ( inSound ) {
				CG_Error( "no concluding '}' in sound %s, file %s\n", sound.name, filename );
			}
			if ( wantSoundName ) {
				CG_Error( "'{' found but not expected, after %s, file %s\n", sound.name, filename );
			}
			inSound = true;
			continue;
		}
		if ( !Q_strcasecmp( token, "}" ) ) {
			if ( !inSound ) {
				CG_Error( "'}' unexpected after sound %s, file %s\n", sound.name, filename );
			}

			// end of a sound, copy it to the global list and stick it in the hashTable
			hash = generateHashValue( sound.name );
			sound.nextHash = hashTable[hash];
			soundScripts[numSoundScripts] = sound;
			hashTable[hash] = &soundScripts[numSoundScripts++];

			if ( numSoundScripts == MAX_SOUND_SCRIPTS ) {
				CG_Error( "MAX_SOUND_SCRIPTS exceeded.\nReduce number of sound scripts.\n" );
			}

			inSound = false;
			wantSoundName = true;
			continue;
		}
		if ( !inSound ) {
			// this is the identifier for a new sound
			if ( !wantSoundName ) {
				CG_Error( "'%s' unexpected after sound %s, file %s\n", token, sound.name, filename );
			}
			memset( &sound, 0, sizeof( sound ) );
			Q_strncpyz( sound.name, token, sizeof( sound.name ) );
			wantSoundName = false;
			sound.index = numSoundScripts;
			// setup the new sound defaults
			sound.channel = CHAN_AUTO;
			sound.attenuation = 1;  // default to fade away with distance (for streaming sounds)
			//
			continue;
		}

		// we are inside a sound script

		if ( !Q_strcasecmp( token, "channel" ) ) {
			// ignore this now, just look for the channel identifiers explicitly
			continue;
		}
		if ( !Q_strcasecmp( token, "local" ) ) {
			sound.channel = CHAN_LOCAL;
			continue;
		} else if ( !Q_strcasecmp( token, "announcer" ) ) {
			sound.channel = CHAN_ANNOUNCER;
			continue;
		} else if ( !Q_strcasecmp( token, "body" ) ) {
			sound.channel = CHAN_BODY;
			continue;
		} else if ( !Q_strcasecmp( token, "voice" ) ) {
			sound.channel = CHAN_VOICE;
			continue;
		} else if ( !Q_strcasecmp( token, "weapon" ) ) {
			sound.channel = CHAN_WEAPON;
			continue;
		} else if ( !Q_strcasecmp( token, "item" ) ) {
			sound.channel = CHAN_ITEM;
			continue;
		} else if ( !Q_strcasecmp( token, "auto" ) ) {
			sound.channel = CHAN_AUTO;
			continue;
		}
		if ( !Q_strcasecmp( token, "global" ) ) {
			sound.attenuation = 0;
			continue;
		}
		if ( !Q_strcasecmp( token, "streaming" ) ) {
			sound.streaming = true;
			continue;
		}
		if ( !Q_strcasecmp( token, "looping" ) ) {
			sound.looping = true;
			continue;
		}
		if ( !Q_strcasecmp( token, "sound" ) ) {
			// grab a free scriptSound
			scriptSound = &soundScriptSounds[numSoundScriptSounds++];

			if ( numSoundScripts == MAX_SOUND_SCRIPT_SOUNDS ) {
				CG_Error( "MAX_SOUND_SCRIPT_SOUNDS exceeded.\nReduce number of sound scripts.\n" );
			}

			token = COM_ParseExt( text, true );
			Q_strncpyz( scriptSound->filename, token, sizeof( scriptSound->filename ) );
			scriptSound->lastPlayed = 0;
			scriptSound->sfxHandle = 0;
			scriptSound->next = sound.soundList;
			sound.soundList = scriptSound;
			continue;
		}
	}
}

/*
===============
CG_SoundLoadSoundFiles
===============
*/
#define MAX_SOUND_FILES     128
#define MAX_BUFFER          20000
static void CG_SoundLoadSoundFiles( void ) {
	char soundFiles[MAX_SOUND_FILES][MAX_QPATH];
	char buffer[MAX_BUFFER];
	char *text;
	char filename[MAX_QPATH];
	fileHandle_t f;
	int numSounds;
	int i, len;
	char *token;

	// scan for sound files
	Com_sprintf( filename, MAX_QPATH, "sound/scripts/filelist.txt" );
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		CG_Printf( S_COLOR_RED "WARNING: no sound files found (filelist.txt not found in sound/scripts)\n" );
		return;
	}
	if ( len > MAX_BUFFER ) {
		CG_Error( "%s is too big, make it smaller (max = %i bytes)\n", filename, MAX_BUFFER );
	}
	// load the file into memory
	trap_FS_Read( buffer, len, f );
	buffer[len] = 0;
	trap_FS_FCloseFile( f );
	// parse the list
	text = buffer;
	numSounds = 0;
	while ( 1 ) {
		token = COM_ParseExt( &text, true );
		if ( !token[0] ) {
			break;
		}
		Com_sprintf( soundFiles[numSounds++], MAX_QPATH, token );
	}

	if ( !numSounds ) {
		CG_Printf( S_COLOR_RED "WARNING: no sound files found\n" );
		return;
	}

	// load and parse sound files
	for ( i = 0; i < numSounds; i++ )
	{
		Com_sprintf( filename, sizeof( filename ), "sound/scripts/%s", soundFiles[i] );
		CG_Printf( "...loading '%s'\n", filename );
		len = trap_FS_FOpenFile( filename, &f, FS_READ );
		if ( len <= 0 ) {
			CG_Error( "Couldn't load %s", filename );
		}
		if ( len > MAX_BUFFER ) {
			CG_Error( "%s is too big, make it smaller (max = %i bytes)\n", filename, MAX_BUFFER );
		}
		memset( buffer, 0, sizeof( buffer ) );
		trap_FS_Read( buffer, len, f );
		trap_FS_FCloseFile( f );
		CG_SoundParseSounds( filename, buffer );
	}
}

/*
==============
CG_SoundInit
==============
*/
void CG_SoundInit( void ) {

	if ( numSoundScripts ) {
		// keep all the information, just reset the vars
		int i;

		for ( i = 0; i < numSoundScriptSounds; i++ ) {
			soundScriptSounds[i].lastPlayed = 0;
			soundScriptSounds[i].sfxHandle = 0;
		}
	} else {
		CG_Printf(  "\n.........................\n"
					"Initializing Sound Scripts\n" );
		CG_SoundLoadSoundFiles();
		CG_Printf(  "done.\n" );
	}

}
