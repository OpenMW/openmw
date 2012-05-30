#ifndef OENGINE_BULLET_PMOVE_H
#define OENGINE_BULLET_PMOVE_H
/*
This source file is a *modified* version of various header files from the Quake 3 Arena source code,
which was released under the GNU GPL (v2) in 2005.
Quake 3 Arena is copyright (C) 1999-2005 Id Software, Inc.
*/

#include <Ogre.h>
#include <OgreMath.h>
#include <float.h>
#include "trace.h"
#include "physic.hpp"


//#include "GameMath.h"
//#include "GameTime.h"

// Forwards-declare it!

/*#ifndef COMPILING_PMOVE
#include "Scene.h"
extern SceneInstance* global_lastscene;
#endif*/

static const Ogre::Vector3 halfExtents(14.64f * 2, 14.24f * 2, 33.25f * 2);

#define	MAX_CLIP_PLANES	5
#define	OVERCLIP 1.001f
//#define	STEPSIZE 18 // 18 is way too much
#define STEPSIZE (18 / 2)
#ifndef M_PI
	#define M_PI 3.14159265358979323846f
#endif
#define YAW 0
#define PITCH /*1*/2
#define ROLL /*2*/1
#define	SHORT2ANGLE(x) ( (x) * (360.0f / 65536.0f) )
#define	ANGLE2SHORT(x) ( (const short)( (x) / (360.0f / 65536.0f) ) )
#define	GENTITYNUM_BITS 10 // don't need to send any more
#define	MAX_GENTITIES (1 << GENTITYNUM_BITS)
#define	ENTITYNUM_NONE (MAX_GENTITIES - 1)
#define ENTITYNUM_WORLD (MAX_GENTITIES - 2)
#define	MIN_WALK_NORMAL .7f // can't walk on very steep slopes
#define	JUMP_VELOCITY (270)
#define PS_PMOVEFRAMECOUNTBITS 6
#define	MINS_Z -24
#define	DEFAULT_VIEWHEIGHT 26
#define CROUCH_VIEWHEIGHT 12
#define	DEAD_VIEWHEIGHT (-16)
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_FOG			64
static const float	pm_accelerate = 10.0f;
static const float	pm_stopspeed = 100.0f;
static const float	pm_friction = 12.0f;
static const float  pm_flightfriction = 3.0f;
static const float	pm_waterfriction = 1.0f;
static const float	pm_airaccelerate = 1.0f;
static const float	pm_swimScale = 0.50f;
static const float	pm_duckScale = 0.25f;
static const float  pm_flyaccelerate = 8.0f;
static const float	pm_wateraccelerate = 4.0f;

enum pmtype_t
{
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_SPINTERMISSION	// no movement or status bar
};

enum waterlevel_t
{
	WL_DRYLAND = 0,
	WL_ANKLE,
	WL_WAIST,
	WL_UNDERWATER
};


//#include "bprintf.h"

struct playerMove
{
	struct playerStruct
	{
		playerStruct() : gravity(800.0f), speed(500.0f), pmove_framecount(20), groundEntityNum(ENTITYNUM_NONE), commandTime(40), move_type(PM_NOCLIP), pm_time(0), snappingImplemented(true), bSnap(false), counter(-1)
		{
			origin = Ogre::Vector3(733.164f,900.0f, 839.432f);
			velocity = Ogre::Vector3(0.0f, 0.0f, 0.0f);

			viewangles = Ogre::Vector3(0.0f, 0.0f, 0.0f);

			delta_angles[0] = delta_angles[1] = delta_angles[2] = 0;

			lastframe_origin.x = lastframe_origin.y = lastframe_origin.z = 0;
			lerp_multiplier.x = lerp_multiplier.y = lerp_multiplier.z = 0;
		}

		inline void SpeedUp(void)
		{
			//printf("speed up to: %f\n", speed);
			speed *= 1.25f;
		}

		inline void SpeedDown(void)
		{
			//printf("speed down to %f\n", speed);
			speed /= 1.25f;
		}

		Ogre::Vector3 velocity;
		Ogre::Vector3 origin;
        bool bSnap;
        bool snappingImplemented;
        int counter;
		float gravity; // default = 800
		float speed; // default = 320

		int commandTime; // the time at which this command was issued (in milliseconds)

		int pm_time;

		Ogre::Vector3 viewangles;

		int groundEntityNum;

		int pmove_framecount;

		int watertype;
		waterlevel_t waterlevel;

		signed short delta_angles[3];

		pmtype_t move_type;

		float last_compute_time;
		Ogre::Vector3 lastframe_origin;
		Ogre::Vector3 lerp_multiplier;
	} ps;

	struct playercmd
	{
		enum CMDstateChange
		{
			NO_CHANGE,
			KEYDOWN,
			KEYUP
		};

		playercmd() : forwardmove(0), rightmove(0), upmove(0), serverTime(50), ducking(false), 
			activating(false), lastActivatingState(false), procActivating(NO_CHANGE),
			dropping(false), lastDroppingState(false), procDropping(NO_CHANGE)
		{
			angles[0] = angles[1] = angles[2] = 0;
		}

		int serverTime;

		short angles[3];

		signed char forwardmove;
		signed char rightmove;
		signed char upmove;

		bool ducking;
		bool activating; // if the user is holding down the activate button
		bool dropping; // if the user is dropping an item

		bool lastActivatingState;
		bool lastDroppingState;

		CMDstateChange procActivating;
		CMDstateChange procDropping;
	} cmd;

	playerMove() : msec(50), pmove_fixed(false), pmove_msec(50), waterHeight(0), isInterior(true), hasWater(false)
	{
	}

	int msec;
	int pmove_msec;
	bool pmove_fixed;
	int waterHeight;
	bool hasWater;
	bool isInterior;
	//Object* traceObj;
	OEngine::Physic::PhysicEngine* mEngine;
};

void Pmove (playerMove* const pmove);
void Ext_UpdateViewAngles(playerMove* const pm);
void AngleVectors( const Ogre::Vector3& angles, Ogre::Vector3* const forward, Ogre::Vector3* const right, Ogre::Vector3* const up) ;
#endif
