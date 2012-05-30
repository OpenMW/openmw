/*
This source file is a *modified* version of bg_pmove.c from the Quake 3 Arena source code,
which was released under the GNU GPL (v2) in 2005.
Quake 3 Arena is copyright (C) 1999-2005 Id Software, Inc.
*/


#include "pmove.h"



//#include "bprintf.h"

//#include "..\..\ESMParser\ESMParser\CELL.h"

//#include "GameTime.h"

//#include "Object.h"

//#include "Sound.h"

//#include "..\..\ESMParser\ESMParser\SNDG.h"
//#include "..\..\ESMParser\ESMParser\SOUN.h"

#include <map>

//SceneInstance* global_lastscene = NULL;

// Forward declaration:
void PM_AirMove();

static playerMove* pm = NULL;

//extern std::map<CellCoords, CELL* const> ExtCellLookup;

static struct playermoveLocal
{
	playermoveLocal() : frametime(1.0f / 20.0f), groundPlane(true), walking(true), msec(50)
	{
		forward = Ogre::Vector3(0.0f, 0.0f, 0.0f);
		right = Ogre::Vector3(0.0f, 0.0f, 0.0f);
		up = Ogre::Vector3(0.0f, 0.0f, 0.0f);

		previous_origin = Ogre::Vector3(0.0f, 0.0f, 0.0f);
		previous_velocity = Ogre::Vector3(0.0f, 0.0f, 0.0f);
	}

	traceResults groundTrace;

	//SceneInstance* scene;

	float frametime; // in seconds (usually something like 0.01f)
	float impactSpeed;

	Ogre::Vector3 forward;
	Ogre::Vector3 right;
	Ogre::Vector3 up;

	int msec;

	Ogre::Vector3 previous_origin, previous_velocity;

	int previous_waterlevel; // the waterlevel before this pmove

	bool groundPlane; // if we're standing on a groundplane this frame

	bool walking;
	int waterHeight;
	bool hasWater;
	bool isInterior;
	//Object* traceObj;

} pml;

static inline void PM_ClipVelocity(const Ogre::Vector3& in, const Ogre::Vector3& normal, Ogre::Vector3& out, const float overbounce)
{
	float	backoff;
	//float	change;
	//int		i;
	
	// backoff = in dot normal
	//backoff = DotProduct (in, normal);
	backoff = in.dotProduct(normal);
	
	if ( backoff < 0 )
		backoff *= overbounce;
	else
		backoff /= overbounce;

	// change = normal * backoff
	// out = in - change
	/*for ( i=0 ; i<3 ; i++ ) 
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;

	}*/
	float changex = normal.x * backoff;
	out.x = in.x - changex;
	float changey = normal.y * backoff;
	out.y = in.y - changey;
	float changez = normal.z * backoff;
	out.z = in.z - changez;
}

float VectorNormalize2( const Ogre::Vector3& v, Ogre::Vector3& out) 
{
	float	length, ilength;

	length = v.x * v.x+ v.y * v.y + v.z * v.z;
	length = sqrt(length);

	if (length)
	{
#ifndef Q3_VM // bk0101022 - FPE related
//	  assert( ((Q_fabs(v[0])!=0.0f) || (Q_fabs(v[1])!=0.0f) || (Q_fabs(v[2])!=0.0f)) );
#endif
		ilength = 1 / length;
		out.x= v.x * ilength;
		out.y = v.y * ilength;
		out.z = v.z * ilength;
	} else 
	{
#ifndef Q3_VM // bk0101022 - FPE related
//	  assert( ((Q_fabs(v[0])==0.0f) && (Q_fabs(v[1])==0.0f) && (Q_fabs(v[2])==0.0f)) );
#endif
		//VectorClear( out );
		out.x = 0; out.y = 0; out.z = 0;
	}
		
	return length;

}


float VectorNormalize(Ogre::Vector3& out) 
{
	float	length, ilength;

	length = out.x * out.x + out.y * out.y + out.z * out.z;
	length = sqrt(length);

	if (length)
	{
#ifndef Q3_VM // bk0101022 - FPE related
//	  assert( ((Q_fabs(v[0])!=0.0f) || (Q_fabs(v[1])!=0.0f) || (Q_fabs(v[2])!=0.0f)) );
#endif
		ilength = 1 / length;
		out.x = out.x * ilength;
		out.y = out.y * ilength;
		out.z = out.z * ilength;
	} 
		
	return length;

}

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/

bool	PM_SlideMove( bool gravity ) 
{
	int			bumpcount, numbumps;
	Ogre::Vector3		dir;
	float		d;
	int			numplanes;
	Ogre::Vector3		planes[MAX_CLIP_PLANES];
	Ogre::Vector3		primal_velocity;
	Ogre::Vector3		clipVelocity;
	int			i, j, k;
	struct traceResults	trace;
	Ogre::Vector3		end;
	float		time_left;
	float		into;
	Ogre::Vector3		endVelocity;
	Ogre::Vector3		endClipVelocity;
	
	numbumps = 4;

	// primal_velocity = pm->ps->velocity
	//VectorCopy (pm->ps->velocity, primal_velocity);
	primal_velocity = pm->ps.velocity;

	if ( gravity ) 
	{
		// endVelocity = pm->ps->velocity - vec3(0, 0, pm->ps->gravity * pml.frametime)
		//VectorCopy( pm->ps->velocity, endVelocity );
		endVelocity = pm->ps.velocity;
		//endVelocity[2] -= pm->ps->gravity * pml.frametime;
		endVelocity.z -= pm->ps.gravity * pml.frametime;

		// pm->ps->velocity = avg(pm->ps->velocity.z, endVelocity.z)
		//pm->ps->velocity[2] = ( pm->ps->velocity[2] + endVelocity[2] ) * 0.5;
		pm->ps.velocity.z= (pm->ps.velocity.z + endVelocity.z) * 0.5f;

		//primal_velocity[2] = endVelocity[2];
		primal_velocity.z = endVelocity.z;

		if ( pml.groundPlane ) 
			// slide along the ground plane
			//PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP );
			PM_ClipVelocity(pm->ps.velocity, pml.groundTrace.planenormal, pm->ps.velocity, OVERCLIP);
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane ) 
	{
		numplanes = 1;

		// planes[0] = pml.groundTrace.plane.normal
		//VectorCopy( pml.groundTrace.plane.normal, planes[0] );
		planes[0] = pml.groundTrace.planenormal;
	} else 
		numplanes = 0;

	// never turn against original velocity
	VectorNormalize2( pm->ps.velocity, planes[numplanes] );
	numplanes++;

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ ) 
	{

		// calculate position we are trying to move to
		//VectorMA( pm->ps->origin, time_left, pm->ps->velocity, end );
		end = pm->ps.origin + pm->ps.velocity * time_left;

		// see if we can make it there
		//pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemaskg);
		//tracefunc(&trace, *(const D3DXVECTOR3* const)&(pm->ps.origin), *(const D3DXVECTOR3* const)&(end), *(const D3DXVECTOR3* const)&(pm->ps.velocity), 0, pml.traceObj);
		newtrace(&trace, pm->ps.origin, end, halfExtents, Ogre::Math::DegreesToRadians (pm->ps.viewangles.y), pm->isInterior, pm->mEngine);

		if (trace.allsolid) 
		{
			// entity is completely trapped in another solid
			//pm->ps->velocity[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			pm->ps.velocity.z = 0;
			return true;
		}

		if (trace.fraction > 0) 
			// actually covered some distance
			//VectorCopy (trace.endpos, pm->ps->origin);
			pm->ps.origin = trace.endpos;

		if (trace.fraction == 1)
			 break;		// moved the entire distance

		// save entity for contact8
		//PM_AddTouchEnt( trace.entityNum );

		time_left -= time_left * trace.fraction;

		if (numplanes >= MAX_CLIP_PLANES) 
		{
			// this shouldn't really happen
			//VectorClear( pm->ps->velocity );
			pm->ps.velocity = Ogre::Vector3(0,0,0);
			return true;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0 ; i < numplanes ; i++ ) 
		{
			if (trace.planenormal.dotProduct(planes[i]) > 0.99)     //OGRE::VECTOR3  ?
			//if ( DotProduct( trace.plane.normal, planes[i] ) > 0.99 ) 
			{
				// pm->ps->velocity += (trace.plane.normal + pm->ps->velocity)
				//VectorAdd( trace.plane.normal, pm->ps->velocity, pm->ps->velocity );
				pm->ps.velocity =  trace.planenormal + pm->ps.velocity;
				break;
			}
		}

		if ( i < numplanes )
			continue;

		//VectorCopy (trace.plane.normal, planes[numplanes]);
		planes[numplanes] = trace.planenormal;
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0 ; i < numplanes ; i++ ) 
		{
			//into = DotProduct( pm->ps->velocity, planes[i] );
			into = pm->ps.velocity.dotProduct(planes[i]);
			if ( into >= 0.1 )
				continue;		// move doesn't interact with the plane

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed )
				pml.impactSpeed = -into;

			// slide along the plane
			//PM_ClipVelocity (pm->ps->velocity, planes[i], clipVelocity, OVERCLIP );
			PM_ClipVelocity(pm->ps.velocity, planes[i], clipVelocity, OVERCLIP);

			// slide along the plane
			PM_ClipVelocity (endVelocity, planes[i], endClipVelocity, OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0 ; j < numplanes ; j++ ) 
			{
				if ( j == i )
					continue;

				if (clipVelocity.dotProduct(planes[j]) >= 0.1)
				//if ( DotProduct( clipVelocity, planes[j] ) >= 0.1 )
					continue;		// move doesn't interact with the plane

				// try clipping the move to the plane
				PM_ClipVelocity( clipVelocity, planes[j], clipVelocity, OVERCLIP );
				PM_ClipVelocity( endClipVelocity, planes[j], endClipVelocity, OVERCLIP );

				// see if it goes back into the first clip plane
				if (clipVelocity.dotProduct(planes[i]) >= 0)
				//if ( DotProduct( clipVelocity, planes[i] ) >= 0 )
					continue;
                

				// slide the original velocity along the crease
				//dProduct (planes[i], planes[j], dir);
				dir = planes[i].crossProduct(planes[j]) ;

				//VectorNormalize( dir );
				//D3DXVec3Normalize( (D3DXVECTOR3* const)&dir, (const D3DXVECTOR3* const)&dir);
				VectorNormalize(dir);

				//d = DotProduct( dir, pm->ps->velocity );
				d = dir.dotProduct(pm->ps.velocity);

				//VectorScale( dir, d, clipVelocity );
				clipVelocity = dir * d;

				//CrossProduct (planes[i], planes[j], dir);
				dir = planes[i].crossProduct(planes[j]) ;
		

				//VectorNormalize( dir );
				//D3DXVec3Normalize( (D3DXVECTOR3* const)&dir, (const D3DXVECTOR3* const)&dir);
				VectorNormalize(dir);

				//d = DotProduct( dir, endVelocity );
				d = dir.dotProduct(endVelocity);

				//VectorScale( dir, d, endClipVelocity );
				endClipVelocity = dir * d;

				// see if there is a third plane the the new move enters
				for ( k = 0 ; k < numplanes ; k++ ) 
				{
                    
					if ( k == i || k == j )
						continue;

					if (clipVelocity.dotProduct(planes[k]) >= 0.1)
					//if ( DotProduct( clipVelocity, planes[k] ) >= 0.1 )
						continue;		// move doesn't interact with the plane

					// stop dead at a tripple plane interaction
					//VectorClear( pm->ps->velocity );
					//printf("Stop dead at a triple plane interaction\n");
					pm->ps.velocity = Ogre::Vector3(0,0,0);
					return true;
				}
			}

			// if we have fixed all interactions, try another move
			//VectorCopy( clipVelocity, pm->ps->velocity );
			pm->ps.velocity = clipVelocity;

			//VectorCopy( endClipVelocity, endVelocity );
			endVelocity = endClipVelocity;
			break;
		}
	}

	if ( gravity )
		//VectorCopy( endVelocity, pm->ps->velocity );
		pm->ps.velocity = endVelocity;

	// don't change velocity if in a timer (FIXME: is this correct?)
	if ( pm->ps.pm_time )
		//VectorCopy( primal_velocity, pm->ps->velocity );
		pm->ps.velocity = primal_velocity;

	//return ( (qboolean)(bumpcount != 0) );
	return bumpcount != 0;
}

/*
==================
PM_StepSlideMove

==================
*/
int PM_StepSlideMove( bool gravity ) 
{
	Ogre::Vector3		start_o, start_v;
	Ogre::Vector3		down_o, down_v;
	traceResults		trace;
//	float		down_dist, up_dist;
//	vec3_t		delta, delta2;
	Ogre::Vector3		up, down;
	float		stepSize;
	
    //std::cout << "StepSlideMove\n";
	// start_o = pm->ps->origin
	//VectorCopy (pm->ps->origin, start_o);
	start_o = pm->ps.origin;

	// start_v = pm->ps->velocity
	//VectorCopy (pm->ps->velocity, start_v);
	start_v = pm->ps.velocity;

	if ( PM_SlideMove( gravity ) == false )
		return 1;		// we got exactly where we wanted to go first try	

	
	// down = start_o - vec3(0, 0, STEPSIZE)
	//VectorCopy(start_o, down);
	down = start_o;
	down.z -= STEPSIZE;

	//pm->trace (&trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	//tracefunc(&trace, start_o, down, , 0, pml.scene);
	//tracefunc(&trace, *(const D3DXVECTOR3* const)&start_o, *(const D3DXVECTOR3* const)&down, D3DXVECTOR3(0.0f, -STEPSIZE, 0.0f), 0, pml.traceObj);
	newtrace(&trace, start_o, down, halfExtents, Ogre::Math::DegreesToRadians(pm->ps.viewangles.y), pm->isInterior, pm->mEngine);
	
	// up = vec3(0, 0, 1)
	//VectorSet(up, 0, 0, 1);
	up = Ogre::Vector3(0.0f, 0.0f, 1.0f);

	// never step up when you still have up velocity
	//if ( pm->ps->velocity[2] > 0 && (trace.fraction == 1.0 || DotProduct(trace.plane.normal, up) < 0.7)) 
	if (pm->ps.velocity.z > 0 && (
		trace.fraction == 1.0 || trace.planenormal.dotProduct(up) < 0.7
		) )
		return 2;

	// down_o = pm->ps->origin
	//VectorCopy (pm->ps->origin, down_o);
	down_o = pm->ps.origin;

	// down_v = pm->ps->velocity
	//VectorCopy (pm->ps->velocity, down_v);
	down_v = pm->ps.velocity;

	// up = start_o + vec3(0, 0, STEPSIZE)
	//VectorCopy (start_o, up);
	up = start_o;
	//up[2] += STEPSIZE;
	up.z += STEPSIZE;

	// test the player position if they were a stepheight higher
	//pm->trace (&trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask);
	//tracefunc(&trace, *(const D3DXVECTOR3* const)&start_o, *(const D3DXVECTOR3* const)&up, D3DXVECTOR3(0.0f, STEPSIZE, 0.0f), 0, pml.traceObj);
	newtrace(&trace, start_o, up, halfExtents, Ogre::Math::DegreesToRadians(pm->ps.viewangles.y), pm->isInterior, pm->mEngine);
	if ( trace.allsolid ) 
	{
		//if ( pm->debugLevel ) 
			//Com_Printf("%i:bend can't step\n", c_pmove);
		//bprintf("bend can't step\n");
		return 3;		// can't step up
	}

	//stepSize = trace.endpos[2] - start_o[2];
	stepSize = trace.endpos.z - start_o.z;

	// try slidemove from this position
	//VectorCopy (trace.endpos, pm->ps->origin); // pm->ps->origin = trace.endpos
	pm->ps.origin = trace.endpos;
	//VectorCopy (start_v, pm->ps->velocity); // pm->ps->velocity = start_v
	pm->ps.velocity = start_v;

	PM_SlideMove( gravity );

	// push down the final amount

	// down = pm->ps->origin - vec3(0, 0, stepSize)
	//VectorCopy (pm->ps->origin, down);
	down = pm->ps.origin;
	//down[2] -= stepSize;
	down.z -= stepSize;


	//pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask);
	//tracefunc(&trace, *(const D3DXVECTOR3* const)&(pm->ps.origin), *(const D3DXVECTOR3* const)&down, D3DXVECTOR3(0.0f, -STEPSIZE, 0.0f), 0, pml.traceObj);
	newtrace(&trace, pm->ps.origin, down, halfExtents, Ogre::Math::DegreesToRadians(pm->ps.viewangles.y), pm->isInterior, pm->mEngine);
	if ( !trace.allsolid )
		//VectorCopy (trace.endpos, pm->ps->origin);
		pm->ps.origin = trace.endpos;

	if ( trace.fraction < 1.0 )
		//PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP );
		PM_ClipVelocity(pm->ps.velocity, trace.planenormal, pm->ps.velocity, OVERCLIP);

	{
		// use the step move
		float	delta;

		//delta = pm->ps->origin[2] - start_o[2];
		delta = pm->ps.origin.z - start_o.z;
		if ( delta > 2 ) 
		{
            pm->ps.counter = 10;

            /*
			if (gravity)
				printf("g on: %f ", delta);
			else
				printf("g off: %f ",  delta);

			if ( delta < 7 ) 
				printf("stepped 3 < x < 7\n");
				//PM_AddEvent( EV_STEP_4 );
			else if ( delta < 11 ) 
				printf("stepped 7 < x < 11\n");
				//PM_AddEvent( EV_STEP_8 );
			else if ( delta < 15 ) 
				printf("stepped 11 < x < 15\n");
				//PM_AddEvent( EV_STEP_12 );
			else 
				printf("stepped 15+\n");
				//PM_AddEvent( EV_STEP_16 );
            */
		}
		/*if ( pm->debugLevel )
			Com_Printf("%i:stepped\n", c_pmove);*/
	}

	return 4;
}

void PM_Friction(void)
{
	
	Ogre::Vector3	vec;
	float* vel;
	float	speed, newspeed, control;
	float	drop;
	
	vel = &(pm->ps.velocity.x);
	
	// vec = vel
	//VectorCopy( vel, vec );
	vec = pm->ps.velocity;

	if ( pml.walking )
		//vec[2] = 0;	// ignore slope movement
		vec.z = 0;

	//speed = VectorLength(vec);
	speed = vec.length();
	if (speed < 1) 
	{
		vel[0] = 0;
		vel[1] = 0;		// allow sinking underwater
		// FIXME: still have z friction underwater?
		//bprintf("Static friction (vec = [%f, %f, %f]) (vec.length = %f)\n", vec.x, vec.y, vec.z, speed);
		return;
	}

	drop = 0;

	// apply ground friction
	if ( pm->ps.waterlevel <= 1 ) 
	{
		if ( pml.walking )//&& !(pml.groundTrace.surfaceFlags & SURF_SLICK) ) 
		{
			// if getting knocked back, no friction
			//if ( ! (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) 
			{
				control = (speed < pm_stopspeed) ? pm_stopspeed : speed;
				drop += control * pm_friction * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->ps.waterlevel ) 
		drop += speed * pm_waterfriction * pm->ps.waterlevel * pml.frametime;

	// apply flying friction
	/*if ( pm->ps->powerups[PW_FLIGHT])
		drop += speed * pm_flightfriction * pml.frametime;

	if ( pm->ps->pm_type == PM_SPECTATOR)
		drop += speed * pm_spectatorfriction * pml.frametime;*/
	if (pm->ps.move_type == PM_SPECTATOR)
		drop += speed * pm_flightfriction * pml.frametime;

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	newspeed /= speed;

	// vel *= newspeed
	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

float PM_CmdScale(playerMove::playercmd* const cmd)
{
	int		max;
	float	total;
	float	scale;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max )
		max = abs( cmd->rightmove );

	if ( abs( cmd->upmove ) > max )
		max = abs( cmd->upmove );

	if ( !max )
		return 0;

	total = sqrtf( (const float)(cmd->forwardmove * cmd->forwardmove
		+ cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove) );
	scale = (float)pm->ps.speed * max / ( 127.0f * total );
    if(pm->ps.move_type == PM_NOCLIP)
        scale *= 10;

	return scale;
}

static void PM_Accelerate( Ogre::Vector3& wishdir, float wishspeed, float accel )
{
//	int			i;
	float		addspeed, accelspeed, currentspeed;

	// currentspeed = pm->ps->velocity dot wishdir
	//currentspeed = DotProduct (pm->ps->velocity, wishdir);
	currentspeed = pm->ps.velocity.dotProduct(wishdir);

	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) 
		return;

	accelspeed = accel * pml.frametime * wishspeed;

	// Clamp accelspeed at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// pm->ps->velocity += accelspeed * wishdir
	//for (i=0 ; i<3 ; i++)
		//pm->ps->velocity[i] += accelspeed * wishdir[i];	
	pm->ps.velocity += (wishdir * accelspeed);
}

static bool PM_CheckJump(void)
{
	//if ( pm->ps->pm_flags & PMF_RESPAWNED )
		//return qfalse;		// don't allow jump until all buttons are up

	if ( pm->cmd.upmove < 10 )
		// not holding jump
		return false;

	pm->cmd.upmove = 0;

	// must wait for jump to be released
	/*if ( pm->ps->pm_flags & PMF_JUMP_HELD ) 
	{
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return false;
	}*/

	pml.groundPlane = false;		// jumping away
	pml.walking = false;
	//pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps.groundEntityNum = ENTITYNUM_NONE;
	pm->ps.velocity.z = JUMP_VELOCITY;
    pm->ps.bSnap = false;
	//PM_AddEvent( EV_JUMP );

	/*if ( pm->cmd.forwardmove >= 0 ) 
	{
		PM_ForceLegsAnim( LEGS_JUMP );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} 
	else 
	{
		PM_ForceLegsAnim( LEGS_JUMPB );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}*/

	return true;
}

static void PM_WaterMove( playerMove* const pm ) 
{
	//int		i;
	//vec3_t	wishvel;
	Ogre::Vector3 wishvel;
	float	wishspeed;
	//vec3_t	wishdir;
	Ogre::Vector3 wishdir;
	float	scale;
	float	vel;

    pm->ps.bSnap = false;

	/*if ( PM_CheckWaterJump() ) 
	{
		PM_WaterJumpMove();
		return;
	}*/
#if 0
	// jump = head for surface
	if ( pm->cmd.upmove >= 10 ) {
		if (pm->ps->velocity[2] > -300) {
			if ( pm->watertype == CONTENTS_WATER ) {
				pm->ps->velocity[2] = 100;
			} else if (pm->watertype == CONTENTS_SLIME) {
				pm->ps->velocity[2] = 80;
			} else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction ();

	if (pm->cmd.forwardmove || pm->cmd.rightmove)
	{
		//NEEDS TO BE REWRITTEN FOR OGRE TIME---------------------------------------------------
		/*
		static const TimeTicks footstep_duration = GetTimeFreq(); // make each splash last 1.0s
		static TimeTicks lastStepTime = 0;
		const TimeTicks thisStepTime = GetTimeQPC();
		static bool lastWasLeft = false;
		if (thisStepTime > lastStepTime)
		{
			if (pm->cmd.ducking)
				lastStepTime = thisStepTime + footstep_duration * 2; // splashes while ducking are twice as slow
			else
				lastStepTime = thisStepTime + footstep_duration;

			lastWasLeft = !lastWasLeft;
		*/
		//-----------------jhooks1

			/*
			namestruct defaultCreature;
			const SNDG* const sndg = SNDG::GetFromMap(defaultCreature, lastWasLeft ? SNDG::r_swim : SNDG::l_swim);
			if (sndg)
			{
				const namestruct& SOUNID = sndg->soundID;
				const SOUN* const soun = SOUN::GetSound(SOUNID);
				if (soun)
				{
					PlaySound2D(soun->soundFilename, soun->soundData->GetVolumeFloat() );
				}
			}*/
			//Sound, ignore for now -- jhooks1
		//}
	}

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) 
	{
		/*wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;		// sink towards bottom
		*/
		wishvel.x = 0;
		wishvel.z = -60;
		wishvel.y = 0;
	} 
	else 
	{
		/*for (i=0 ; i<3 ; i++)
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;*/
		wishvel = pml.forward * scale * pm->cmd.forwardmove + pml.right * scale * pm->cmd.rightmove;

		//wishvel[2] += scale * pm->cmd.upmove;
		wishvel.z += pm->cmd.upmove * scale;
	}

	//VectorCopy (wishvel, wishdir);
	wishdir = wishvel;
	wishspeed = VectorNormalize(wishdir);

	if ( wishspeed > pm->ps.speed * pm_swimScale )
		wishspeed = pm->ps.speed * pm_swimScale;

	PM_Accelerate (wishdir, wishspeed, pm_wateraccelerate);

	// make sure we can go up slopes easily under water
	//if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 ) 
	if (pml.groundPlane && pm->ps.velocity.dotProduct(pml.groundTrace.planenormal) < 0.0f)
	{
		//vel = VectorLength(pm->ps->velocity);
		vel = pm->ps.velocity.length();

		// slide along the ground plane
		//PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP );
		PM_ClipVelocity(pm->ps.velocity, pml.groundTrace.planenormal, pm->ps.velocity, OVERCLIP);

		VectorNormalize(pm->ps.velocity);
		//VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
		pm->ps.velocity = pm->ps.velocity * vel;
	}

	PM_SlideMove( false );
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( playerMove* const pmove ) 
{
//	int			i;
	Ogre::Vector3		wishvel;
	float		fmove, smove;
	Ogre::Vector3		wishdir;
	float		wishspeed;
	float		scale;
	playerMove::playercmd cmd;
	float		accelerate;
	float		vel;
	//pm->ps.gravity = 4000;

	if ( pm->ps.waterlevel > 2 && //DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 ) 
		pml.forward.dotProduct(pml.groundTrace.planenormal) > 0.0f)
	{
		// begin swimming
		PM_WaterMove(pmove);
		return;
	}


	if ( PM_CheckJump () ) 
	{
		
		// jumped away
		if ( pm->ps.waterlevel > 1 ) 
			PM_WaterMove(pmove);
		else
			PM_AirMove();
		//printf("Jumped away\n");
		return;
	}

	// Footsteps time
	if (pmove->cmd.forwardmove || pmove->cmd.rightmove)
	{
		bool step_underwater = false;
		//if (pmove->traceObj)
		//{


			//jhooks1 - Water handling, deal with later



			if (pmove->hasWater)
			{
				if (pmove->hasWater )
				{
					const float waterHeight = pmove->waterHeight;
					const float waterSoundStepHeight = waterHeight + halfExtents.y;
					if (pmove->ps.origin.y < waterSoundStepHeight)
						step_underwater = true;
				}
			}
		//}

		/*
		static const TimeTicks footstep_duration = GetTimeFreq() / 2; // make each footstep last 500ms
		static TimeTicks lastStepTime = 0;
		const TimeTicks thisStepTime = GetTimeQPC();
		static bool lastWasLeft = false;
		if (thisStepTime > lastStepTime)
		{
			if (pmove->cmd.ducking)
				lastStepTime = thisStepTime + footstep_duration * 2; // footsteps while ducking are twice as slow
			else
				lastStepTime = thisStepTime + footstep_duration;

			lastWasLeft = !lastWasLeft;
			*/

			if (step_underwater)
			{
				/*
				const namestruct ns(lastWasLeft ? "FootWaterRight" : "FootWaterLeft");
				const SOUN* const soun = SOUN::GetSound(ns);
				if (soun)
				{
					PlaySound2D(soun->soundFilename, soun->soundData->GetVolumeFloat() );
				}*/
			}
			else
			{
				/*
				namestruct defaultCreature;
				const SNDG* const sndg = SNDG::GetFromMap(defaultCreature, lastWasLeft ? SNDG::r_foot : SNDG::l_foot);
				if (sndg)
				{
					const namestruct& SOUNID = sndg->soundID;
					const SOUN* const soun = SOUN::GetSound(SOUNID);
					if (soun)
					{
						PlaySound2D(soun->soundFilename, soun->soundData->GetVolumeFloat() );
					}
				}*/
			}
		}


	PM_Friction ();
	

	//bprintf("vel: (%f, %f, %f)\n", pm->ps.velocity.x, pm->ps.velocity.y, pm->ps.velocity.z);

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	//PM_SetMovementDir();

	// project moves down to flat plane
	//pml.forward[2] = 0;
	pml.forward.z = 0;

	//pml.right[2] = 0;
	pml.right.z = 0;
	//std::cout << "Further down" << pm->ps.velocity << "\n";

	
	// project the forward and right directions onto the ground plane
	PM_ClipVelocity (pml.forward, pml.groundTrace.planenormal, pml.forward, OVERCLIP );
	PM_ClipVelocity (pml.right, pml.groundTrace.planenormal, pml.right, OVERCLIP );
	//std::cout << "Clip velocity" << pm->ps.velocity << "\n";
	//
	
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);
	//pml.forward = pml.forward.normalise();
	//pml.right = pml.right.normalise();
	//std::cout << "forward2" << pml.forward << "\n";
	//std::cout << "right2" << pml.right << "\n";
	

	// wishvel = (pml.forward * fmove) + (pml.right * smove);
	//for ( i = 0 ; i < 3 ; i++ ) 
		//wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel = pml.forward * fmove + pml.right * smove;
	
	
	//bprintf("f: (%f, %f, %f), s: (%f, %f, %f)\n", fmove, smove);


	// when going up or down slopes the wish velocity should Not be zero
//	wishvel[2] = 0;

	// wishdir = wishvel
	//VectorCopy (wishvel, wishdir);
	//wishvel = wishdir;
	wishdir = wishvel;

	wishspeed = VectorNormalize(wishdir);
	//std::cout << "Wishspeed: " << wishspeed << "\n";
	wishspeed *= scale;
	//std::cout << "Wishspeed scaled:" << wishspeed << "\n";

	// clamp the speed lower if ducking
	if ( pm->cmd.ducking ) 
		if ( wishspeed > pm->ps.speed * pm_duckScale )
			wishspeed = pm->ps.speed * pm_duckScale;

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->ps.waterlevel ) 
	{
		float	waterScale;

		waterScale = pm->ps.waterlevel / 3.0f;
		waterScale = 1.0f - ( 1.0f - pm_swimScale ) * waterScale;
		if ( wishspeed > pm->ps.speed * waterScale )
			wishspeed = pm->ps.speed * waterScale;
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	//if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
		//accelerate = pm_airaccelerate;
	//else
		accelerate = pm_accelerate;


	PM_Accelerate (wishdir, wishspeed, accelerate);
	//std::cout << "Velocityafter: " << pm->ps.velocity << "\n";

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	//if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
		//pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	//else 
	//{
		// don't reset the z velocity for slopes
//		pm->ps->velocity[2] = 0;
	//}

	//vel = VectorLength(pm->ps->velocity);
	vel = pm->ps.velocity.length();
	//std::cout << "The length" << vel << "\n";

	// slide along the ground plane
	PM_ClipVelocity (pm->ps.velocity, pml.groundTrace.planenormal, 
		pm->ps.velocity, OVERCLIP );
	//std::cout << "Velocity clipped" << pm->ps.velocity << "\n";

	// don't decrease velocity when going up or down a slope
	VectorNormalize(pm->ps.velocity);
	//pm->ps.velocity = pm->ps.velocity.normalise();
	
	//std::cout << "Final:" << pm->ps.velocity << "\n";
	//VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	pm->ps.velocity = pm->ps.velocity * vel;

	// don't do anything if standing still
	//if (!pm->ps->velocity[0] && !pm->ps->velocity[1])
	if (!pm->ps.velocity.x && !pm->ps.velocity.z)
		return;

	PM_StepSlideMove( false );

	//Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));
	

}

void PM_UpdateViewAngles( playerMove::playerStruct* const ps, playerMove::playercmd* const cmd ) 
{
	short		temp;
	int		i;
	
	//while(1);

	//if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION) 
		//return;		// no view changes at all

	//if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 )
		//return;		// no view changes at all

	// circularly clamp the angles with deltas
	//bprintf("View angles: %i, %i, %i\n", cmd->angles[0], cmd->angles[1], cmd->angles[2]);
	for (i = 0 ; i < 3 ; i++) 
	{
		temp = cmd->angles[i];// + ps->delta_angles[i];
		//if ( i == PITCH ) 
		{
			// don't let the player look up or down more than 90 degrees
			/*if ( temp > 16000 ) 
			{
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} 
			else if ( temp < -16000 ) 
			{
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}*/
		}
		(&(ps->viewangles.x) )[i] = SHORT2ANGLE(temp);
		//cmd->angles[i] += ps->delta_angles[i];
	}
	//ps->delta_angles[0] = ps->delta_angles[1] = ps->delta_angles[2] = 0;

}

void AngleVectors( const Ogre::Vector3& angles, Ogre::Vector3* const forward, Ogre::Vector3* const right, Ogre::Vector3* const up) 
{
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	//angle = angles[YAW] * (M_PI*2 / 360);
	angle = angles.x * (M_PI * 2.0f / 360.0f);
	sp = sinf(angle);
	cp = cosf(angle);

	//angle = angles[PITCH] * (M_PI*2 / 360);
	angle = angles.y * (-M_PI * 2.0f / 360.0f);
	sy = sinf(angle);
	cy = cosf(angle);

	//angle = angles[ROLL] * (M_PI*2 / 360);
	angle = angles.z * (M_PI * 2.0f / 360.0f);
	sr = sinf(angle);
	cr = cosf(angle);

	if (forward)
	{
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}
	if (right)
	{
		right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
		right->y = (-1 * sr * sp * sy + -1 * cr * cy);
		right->z = 0;
	}
	if (up)
	{
		up->x =(cr * sp * cy + -sr * -sy);
		up->y=(cr * sp * sy + -sr * cy);
		up->z = cr * cp;
	}
	
}

void PM_GroundTraceMissed()
{
	traceResults		trace;
	Ogre::Vector3		point;
    //std::cout << "Ground trace missed\n";
		// we just transitioned into freefall
		//if ( pm->debugLevel )
			//Com_Printf("%i:lift\n", c_pmove);

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		//VectorCopy( pm->ps->origin, point );
		point = pm->ps.origin;
		//point[2] -= 64;
		point.z -= 32;

		//pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		//tracefunc(&trace, *(const D3DXVECTOR3* const)&(pm->ps.origin), *(const D3DXVECTOR3* const)&point, D3DXVECTOR3(0.0f, -64.0f, 0.0f), 0, pml.traceObj);
		newtrace(&trace, pm->ps.origin, point, halfExtents, Ogre::Math::DegreesToRadians(pm->ps.viewangles.y), pm->isInterior, pm->mEngine);
		//It hit the ground below
        if ( trace.fraction < 1.0 && pm->ps.origin.z > trace.endpos.z) 
		{
			   pm->ps.origin = trace.endpos;
               pml.walking = true;
               pml.groundPlane = true;
                pm->ps.groundEntityNum = trace.entityNum;
           
		}
        else{
        pm->ps.groundEntityNum = ENTITYNUM_NONE;
	    pml.groundPlane = false;
	    pml.walking = false;
        pm->ps.bSnap = false;
	    }

	
}

static bool PM_CorrectAllSolid(traceResults* const trace)
{
	int			i, j, k;
	Ogre::Vector3	point;

	//if ( pm->debugLevel )
		//Com_Printf("%i:allsolid\n", c_pmove);
	//bprintf("allsolid\n");

	// jitter around
	for (i = -1; i <= 1; i++) 
	{
		for (j = -1; j <= 1; j++) 
		{
			for (k = -1; k <= 1; k++) 
			{
				//VectorCopy(pm->ps->origin, point);
				point = pm->ps.origin;

				/*point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;*/
				point += Ogre::Vector3( (const float)i, (const float)j, (const float)k);

				//pm->trace (trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				//tracefunc(trace, *(const D3DXVECTOR3* const)&point, *(const D3DXVECTOR3* const)&point, D3DXVECTOR3(0.0f, 0.0f, 0.0f), 0, pml.traceObj);
				newtrace(trace, point, point, halfExtents, Ogre::Math::DegreesToRadians(pm->ps.viewangles.y), pm->isInterior, pm->mEngine);

				if ( !trace->allsolid ) 
				{
					/*point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;*/
					point = pm->ps.origin;
					point.z -= 0.25f;

					//pm->trace (trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					//tracefunc(trace, *(const D3DXVECTOR3* const)&(pm->ps.origin), *(const D3DXVECTOR3* const)&point, D3DXVECTOR3(0.0f, -0.25f, 0.0f), 0, pml.traceObj);
					newtrace(trace, pm->ps.origin, point, halfExtents, Ogre::Math::DegreesToRadians(pm->ps.viewangles.y), pm->isInterior, pm->mEngine);
					pml.groundTrace = *trace;
					return true;
				}
			}
		}
	}

	//pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pm->ps.groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = false;
	pml.walking = false;

	return false;
}

static void PM_CrashLand( void ) 
{
	float		delta;
	float		dist ;
	float		vel, acc;
	float		t;
	float		a, b, c, den;

	// decide which landing animation to use
	/*if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) 
		PM_ForceLegsAnim( LEGS_LANDB );
	else
		PM_ForceLegsAnim( LEGS_LAND );
		
	pm->ps->legsTimer = TIMER_LAND;*/

	// calculate the exact velocity on landing
	//dist = pm->ps->origin[2] - pml.previous_origin[2];

	dist = pm->ps.origin.z - pml.previous_origin.z;

	//vel = pml.previous_velocity[2];
	vel = pml.previous_velocity.z;

	//acc = -pm->ps->gravity;
	acc = -pm->ps.gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if ( den < 0 ) 
		return;

	t = (-b - sqrtf( den ) ) / ( 2 * a );

	delta = vel + t * acc; 
	delta = delta * delta * 0.0001f;

	// ducking while falling doubles damage
	/*if ( pm->ps->pm_flags & PMF_DUCKED )
		delta *= 2;*/
	if (pm->cmd.upmove < -20)
		delta *= 2;

	// never take falling damage if completely underwater
	if ( pm->ps.waterlevel == 3 ) 
		return;

	// reduce falling damage if there is standing water
	if ( pm->ps.waterlevel == 2 )
		delta *= 0.25;
	if ( pm->ps.waterlevel == 1 )
		delta *= 0.5;

	if ( delta < 1 ) 
		return;
/*
	if (delta > 60)
		printf("Far crashland: %f\n", delta);
	else if (delta > 40)
		printf("Medium crashland: %f\n", delta);
	else if (delta > 4)
		printf("Short crashland: %f\n", delta);
*/
	if (delta > 60)
	{
		/*
		static const namestruct healthDamage("Health Damage");
		const SOUN* const soun = SOUN::GetSound(healthDamage);
		if (soun)
		{
			PlaySound2D(soun->soundFilename, soun->soundData->GetVolumeFloat() );
		}*/
	}

	if (delta > 3) // We need at least a short crashland to proc the sound effects:
	{
		bool splashSound = false;
		
			if (pm->hasWater)
			{
				
					const float waterHeight = pm->waterHeight;
					const float waterHeightSplash = waterHeight + halfExtents.y;
					if (pm->ps.origin.z < waterHeightSplash)
					{
						splashSound = true;
					}
				
			}
		

		if (splashSound)
		{
			//Change this later-----------------------------------
			/*
			const namestruct ns("DefaultLandWater");
			const SOUN* const soun = SOUN::GetSound(ns);
			if (soun)
			{
				PlaySound2D(soun->soundFilename, soun->soundDatga->GetVolumeFloat() );
			}*/
		}
		else
		{
			//Change this later---------------------------------
			/*
			namestruct defaultCreature;
			const SNDG* const sndg = SNDG::GetFromMap(defaultCreature, SNDG::land);
			if (sndg)
			{
				const namestruct& SOUNID = sndg->soundID;
				const SOUN* const soun = SOUN::GetSound(SOUNID);
				if (soun)
				{
					PlaySound2D(soun->soundFilename, soun->soundData->GetVolumeFloat() );
				}
			}*/
		}
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	//if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) )  
	{
		/*if ( delta > 60 ) 
			PM_AddEvent( EV_FALL_FAR );
		else if ( delta > 40 ) 
		{
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 )
				PM_AddEvent( EV_FALL_MEDIUM );
		} 
		else if ( delta > 7 ) 
			PM_AddEvent( EV_FALL_SHORT );
		else 
			PM_AddEvent( PM_FootstepForSurface() );*/
	}

	// start footstep cycle over
	//pm->ps->bobCycle = 0;
}

static void PM_GroundTrace( void ) 
{
	Ogre::Vector3		point;
	traceResults		trace;

	/*point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;*/
	point = pm->ps.origin;
	point.z -= 0.25f;

	//pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	//tracefunc(&trace, *(const D3DXVECTOR3* const)&(pm->ps.origin), *(const D3DXVECTOR3* const)&point, D3DXVECTOR3(0.0f, -0.25f, 0.0f), 0, pml.traceObj);
	newtrace(&trace, pm->ps.origin, point, halfExtents, Ogre::Math::DegreesToRadians(pm->ps.viewangles.y), pm->isInterior, pm->mEngine);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		//std::cout << "ALL SOLID\n";
		if ( !PM_CorrectAllSolid(&trace) ){
			//std::cout << "Returning after correct all solid\n";
			return;
		}
	}
    // if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0) 
	{
        if(pm->ps.snappingImplemented){
            if(pm->ps.bSnap && pm->ps.counter <= 0)
		        PM_GroundTraceMissed();
        }
            

		return;
	}
    else
    {
        //It hit something, so we are on the ground
        pm->ps.bSnap = true;

    }
    // check if getting thrown off the ground
	//if ( pm->ps->velocity[2] > 0 && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10 ) 
	if (pm->ps.velocity.z > 0 && pm->ps.velocity.dotProduct(trace.planenormal) > 10.0f )
	{
		//if ( pm->debugLevel ) 
			//Com_Printf("%i:kickoff\n", c_pmove);

		// go into jump animation
		/*if ( pm->cmd.forwardmove >= 0 ) 
		{
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} 
		else 
		{
			PM_ForceLegsAnim( LEGS_JUMPB );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}*/
        if(!pm->ps.bSnap){
		pm->ps.groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = false;
		pml.walking = false;
        }
        else
        {
            pml.groundPlane = true;
		    pml.walking = true;
        }
		return;
	}
	

	
	
	// slopes that are too steep will not be considered onground
	//if ( trace.plane.normal[2] < MIN_WALK_NORMAL ) 
    //std::cout << "MinWalkNormal" << trace.planenormal.z;
	if (trace.planenormal.z < MIN_WALK_NORMAL)
	{
		//if ( pm->debugLevel )
			//Com_Printf("%i:steep\n", c_pmove);

		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps.groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = true;
		pml.walking = false;
		return;
	}

	pml.groundPlane = true;
	pml.walking = true;

	// hitting solid ground will end a waterjump
	/*if (pm->ps.pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}*/

	if ( pm->ps.groundEntityNum == ENTITYNUM_NONE ) 
	{
		// just hit the ground
		/*if ( pm->debugLevel )
			Com_Printf("%i:Land\n", c_pmove);*/
		//bprintf("Land\n");
		
		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		//if ( pml.previous_velocity[2] < -200 ) 
		if (pml.previous_velocity.z < -200)
		{
			// don't allow another jump for a little while
			//pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps.pm_time = 250;
		}
	}

	pm->ps.groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;
	
	//PM_AddTouchEnt( trace.entityNum );
}

void PM_AirMove()
{
	//int			i;
	Ogre::Vector3		wishvel;
	float		fmove, smove;
	Ogre::Vector3		wishdir;
	float		wishspeed;
	float		scale;
	playerMove::playercmd	cmd;
    //pm->ps.gravity = 800;
	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );
	// set the movementDir so clients can rotate the legs for strafing
	//PM_SetMovementDir();

	// project moves down to flat plane
	//pml.forward[2] = 0;
	pml.forward.z = 0;             //Z or Y?
	//pml.right[2] = 0;
	pml.right.z = 0;
	//VectorNormalize (pml.forward);
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);
	//VectorNormalize (pml.right);

	//for ( i = 0 ; i < 2 ; i++ )
		//wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel = pml.forward * fmove + pml.right * smove;

	//wishvel[2] = 0;
	wishvel.z = 0;

	//VectorCopy (wishvel, wishdir);
	wishdir = wishvel;
	//wishspeed = VectorNormalize(wishdir);
	wishspeed = VectorNormalize(wishdir);

	wishspeed *= scale;

	// not on ground, so little effect on velocity
	PM_Accelerate (wishdir, wishspeed, pm_airaccelerate);

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane )
		PM_ClipVelocity (pm->ps.velocity, pml.groundTrace.planenormal, pm->ps.velocity, OVERCLIP );

/*#if 0
	//ZOID:  If we are on the grapple, try stair-stepping
	//this allows a player to use the grapple to pull himself
	//over a ledge
	if (pm->ps->pm_flags & PMF_GRAPPLE_PULL)
		PM_StepSlideMove ( qtrue );
	else
		PM_SlideMove ( qtrue );
#endif*/
	
	/*bprintf("%i ", */PM_StepSlideMove ( true )/* )*/;
}

static void PM_NoclipMove( void ) 
{
	float	speed, drop, friction, control, newspeed;
//	int			i;
	Ogre::Vector3		wishvel;
	float		fmove, smove;
	Ogre::Vector3		wishdir;
	float		wishspeed;
	float		scale;

	//pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	//speed = VectorLength (pm->ps->velocity);
	speed = pm->ps.velocity.length();
	if (speed < 1)
		//VectorCopy (vec3_origin, pm->ps->velocity);
		pm->ps.velocity = Ogre::Vector3(0.0f, 0.0f, 0.0f);
	else
	{
		drop = 0;

		friction = pm_friction * 1.5f;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		//VectorScale (pm->ps->velocity, newspeed, pm->ps->velocity);
		pm->ps.velocity = pm->ps.velocity * newspeed;
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	
	//for (i=0 ; i<3 ; i++)
		//wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	
	wishvel = pml.forward * fmove + pml.right * smove;
	//wishvel[2] += pm->cmd.upmove;
	wishvel.z += pm->cmd.upmove;

	//VectorCopy (wishvel, wishdir);
	wishdir = wishvel;
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;


	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	//VectorMA (pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
	pm->ps.origin = pm->ps.origin + pm->ps.velocity * pml.frametime;
}

static void PM_DropTimers( void ) 
{
	// drop misc timing counter
	if ( pm->ps.pm_time ) 
	{
		if ( pml.msec >= pm->ps.pm_time ) 
		{
			//pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps.pm_time = 0;
		} 
		else
			pm->ps.pm_time -= pml.msec;
	}

	//bprintf("Time: %i\n", pm->ps.pm_time);

	// drop animation counter
	/*if ( pm->ps->legsTimer > 0 ) 
	{
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) 
			pm->ps->legsTimer = 0;
	}

	if ( pm->ps->torsoTimer > 0 ) 
	{
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 )
			pm->ps->torsoTimer = 0;
	}*/
}

static void PM_FlyMove( void ) 
{
	//int		i;
	Ogre::Vector3	wishvel;
	float	wishspeed;
	Ogre::Vector3	wishdir;
	float	scale;

	// normal slowdown
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) 
	{
		/*wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;*/
		wishvel = Ogre::Vector3(0,0,0);
	} 
	else 
	{
		//for (i=0 ; i<3 ; i++) 
			//wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;
		wishvel = pml.forward * scale * pm->cmd.forwardmove + pml.right * scale * pm->cmd.rightmove;

		//wishvel[2] += scale * pm->cmd.upmove;
		wishvel.z += /*6.35f * */pm->cmd.upmove * scale;
	}

	//VectorCopy (wishvel, wishdir);
	wishdir = wishvel;

	//wishspeed = VectorNormalize(wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate (wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove( false );
}


void PM_SetWaterLevel( playerMove* const pm ) 
{
	Ogre::Vector3 point;
	//int			cont;
	int			sample1;
	int			sample2;

	//
	// get waterlevel, accounting for ducking
	//

	pm->ps.waterlevel = WL_DRYLAND;
	pm->ps.watertype = 0;

	/*point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;	*/
	point.x = pm->ps.origin.x;
	point.y = pm->ps.origin.y;
	point.z = pm->ps.origin.z + MINS_Z + 1;

	//cont = pm->pointcontents( point, pm->ps->clientNum );
	bool checkWater = (pml.hasWater && pml.waterHeight > point.z);
	//if ( cont & MASK_WATER ) 
	if ( checkWater)
	{
		sample2 = /*pm->ps.viewheight*/DEFAULT_VIEWHEIGHT - MINS_Z;
		sample1 = sample2 / 2;

		pm->ps.watertype = CONTENTS_WATER;//cont;
		pm->ps.waterlevel = WL_ANKLE;
		//point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		point.z = pm->ps.origin.z + MINS_Z + sample1;
        checkWater = (pml.hasWater && pml.waterHeight > point.z);
		//cont = pm->pointcontents (point, pm->ps->clientNum );
		//if ( cont & MASK_WATER ) 
		if (checkWater)
		{
			pm->ps.waterlevel = WL_WAIST;
			//point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			point.z = pm->ps.origin.z + MINS_Z + sample2;
			//cont = pm->pointcontents (point, pm->ps->clientNum );
			//if ( cont & MASK_WATER )
            checkWater = (pml.hasWater && pml.waterHeight > point.z);
			if (checkWater )
				pm->ps.waterlevel = WL_UNDERWATER;
		}
	}
}

void PmoveSingle (playerMove* const pmove) 
{
    pmove->ps.counter--;
	//pm = pmove;

	// Aedra doesn't support Q3-style VM traps D:	//while(1);

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	//c_pmove++;

	// clear results
	//pm->numtouch = 0;
	pm->ps.watertype = 0;
	pm->ps.waterlevel = WL_DRYLAND;

	//if ( pm->ps->stats[STAT_HEALTH] <= 0 )
		//pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
		

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	//if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 )
		//pm->cmd.buttons &= ~BUTTON_WALKING;


	// set the talk balloon flag
	//if ( pm->cmd.buttons & BUTTON_TALK )
		//pm->ps->eFlags |= EF_TALK;
	//else
		//pm->ps->eFlags &= ~EF_TALK;

	// set the firing flag for continuous beam weapons
	/*if ( !(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION
		&& ( pm->cmd.buttons & BUTTON_ATTACK ) && pm->ps->ammo[ pm->ps->weapon ] )
		pm->ps->eFlags |= EF_FIRING;
	else
		pm->ps->eFlags &= ~EF_FIRING;*/

	// clear the respawned flag if attack and use are cleared
	/*if ( pm->ps->stats[STAT_HEALTH] > 0 && 
		!( pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) ) )
		pm->ps->pm_flags &= ~PMF_RESPAWNED;*/

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	/*if ( pmove->cmd.buttons & BUTTON_TALK ) 
	{
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}*/

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml) );

	// Aedra-specific code:
	//pml.scene = global_lastscene;
	

	// End Aedra-specific code
	pml.hasWater = pmove->hasWater;
	pml.isInterior = pmove->isInterior;
	pml.waterHeight = pmove->waterHeight;
#ifdef _DEBUG
	if (!pml.traceObj)
		__debugbreak();

	if (!pml.traceObj->incellptr)
		__debugbreak();
#endif

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps.commandTime;
	if ( pml.msec < 1 )
		pml.msec = 1;
	else if ( pml.msec > 200 )
		pml.msec = 200;

	//pm->ps->commandTime = pmove->cmd.serverTime;

	// Commented out as a hack
	pm->ps.commandTime = pmove->cmd.serverTime;

	// Handle state change procs:
	if (pm->cmd.activating != pm->cmd.lastActivatingState)
	{
		if (!pm->cmd.lastActivatingState && pm->cmd.activating)
			pm->cmd.procActivating = playerMove::playercmd::KEYDOWN;
		else
			pm->cmd.procActivating = playerMove::playercmd::KEYUP;
	}
	else
	{
		pm->cmd.procActivating = playerMove::playercmd::NO_CHANGE;
	}
	pm->cmd.lastActivatingState = pm->cmd.activating;

	if (pm->cmd.dropping != pm->cmd.lastDroppingState)
	{
		if (!pm->cmd.lastDroppingState && pm->cmd.dropping)
			pm->cmd.procDropping = playerMove::playercmd::KEYDOWN;
		else
			pm->cmd.procDropping = playerMove::playercmd::KEYUP;
	}
	else
	{
		pm->cmd.procDropping = playerMove::playercmd::NO_CHANGE;
	}
	pm->cmd.lastDroppingState = pm->cmd.dropping;

	// save old org in case we get stuck
	//VectorCopy (pm->ps->origin, pml.previous_origin);
	pml.previous_origin = pm->ps.origin;

	// Copy over the lastframe origin
	pmove->ps.lastframe_origin = pmove->ps.origin;

	//pmove->ps.lastframe_origin = pmove->ps.origin;

	// save old velocity for crashlanding
	//VectorCopy (pm->ps->velocity, pml.previous_velocity);
	pml.previous_velocity = pm->ps.velocity;

	pml.frametime = pml.msec * 0.001f;

	// update the viewangles
	//PM_UpdateViewAngles( &(pm->ps), &(pm->cmd) );

	AngleVectors (pm->ps.viewangles, &(pml.forward), &(pml.right), &(pml.up) );

	//if ( pm->cmd.upmove < 10 ) 
		// not holding jump
		//pm->ps->pm_flags &= ~PMF_JUMP_HELD;

	// decide if backpedaling animations should be used
	/*if ( pm->cmd.forwardmove < 0 ) 
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) )
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;*/

	/*if ( pm->ps->pm_type >= PM_DEAD ) 
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}*/

	if ( pm->ps.move_type == PM_SPECTATOR ) 
	{
		
		//PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		return;
	}

	if ( pm->ps.move_type == PM_NOCLIP ) 
	{
		
		PM_NoclipMove ();
		PM_DropTimers ();
		return;
	}

	if (pm->ps.move_type == PM_FREEZE){
		
		return;		// no movement at all

	}

	if ( pm->ps.move_type == PM_INTERMISSION || pm->ps.move_type == PM_SPINTERMISSION){
		return;		// no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel(pmove);
	pml.previous_waterlevel = pmove->ps.waterlevel;

	// set mins, maxs, and viewheight
	//PM_CheckDuck ();

	// set groundentity
	PM_GroundTrace();

	/*if ( pm->ps->pm_type == PM_DEAD )
		PM_DeadMove ();

	PM_DropTimers();*/

	PM_DropTimers();

/*#ifdef MISSIONPACK
	if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
		PM_InvulnerabilityMove();
	} else
#endif*/
	/*if ( pm->ps->powerups[PW_FLIGHT] ) 
		// flight powerup doesn't allow jump and has different friction
		PM_FlyMove();
	else if (pm->ps->pm_flags & PMF_GRAPPLE_PULL) 
	{
		PM_GrappleMove();
		// We can wiggle a bit
		PM_AirMove();
	} 
	else if (pm->ps->pm_flags & PMF_TIME_WATERJUMP) 
		PM_WaterJumpMove();*/
	if ( pmove->ps.waterlevel > 1 ) 
		// swimming
		PM_WaterMove(pmove);
	else if ( pml.walking ) 
	{
		
		// walking on ground
		PM_WalkMove(pmove);
		//bprintf("WalkMove\n");
	}
	else 
	{
		// airborne
		//std::cout << "AIRMOVE\n";
		PM_AirMove();
		//bprintf("AirMove\n");
	}

	//PM_Animate();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel(pmove);

	// weapons
	/*PM_Weapon();

	// torso animation
	PM_TorsoAnimation();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	// snap some parts of playerstate to save network bandwidth
	trap_SnapVector( pm->ps->velocity );*/
}

void Ext_UpdateViewAngles(playerMove* const pm)
{
	playerMove::playerStruct* const ps = &(pm->ps);
	playerMove::playercmd* const cmd = &(pm->cmd);
	PM_UpdateViewAngles(ps, cmd);
}

void Pmove (playerMove* const pmove) 
{
    // warning: unused variable ‘fmove’
	//int fmove = pmove->cmd.forwardmove;

	pm = pmove;

	int			finalTime;

	finalTime = pmove->cmd.serverTime;

	pmove->ps.commandTime = 40;

	if ( finalTime < pmove->ps.commandTime )
		return;	// should not happen

	if ( finalTime > pmove->ps.commandTime + 1000 )
		pmove->ps.commandTime = finalTime - 1000;

	pmove->ps.pmove_framecount = (pmove->ps.pmove_framecount + 1) & ( (1 << PS_PMOVEFRAMECOUNTBITS) - 1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps.commandTime != finalTime ) 
	{
		int		msec;

		msec = finalTime - pmove->ps.commandTime;

		if ( pmove->pmove_fixed )
        {
			if ( msec > pmove->pmove_msec ) 
				msec = pmove->pmove_msec;
        }
		else
        {
			if ( msec > 66 ) 
				msec = 66;
        }

		pmove->cmd.serverTime = pmove->ps.commandTime + msec;

		if (pmove->isInterior)
		{
			PmoveSingle( pmove );
		}
		else
		{
			PmoveSingle( pmove );
			/*
			std::map<CellCoords, CELL* const>::const_iterator it = ExtCellLookup.find(PositionToCell(pmove->ps.origin) );
			if (it != ExtCellLookup.end() )
			{
				pmove->traceObj->incellptr = it->second;
			}*/
		}

		//if ( pmove->ps->pm_flags & PMF_JUMP_HELD ) 
			//pmove->cmd.upmove = 20;
	}

	//pmove->ps.last_compute_time = GetTimeQPC();
	//pmove->ps.lerp_multiplier = (pmove->ps.origin - pmove->ps.lastframe_origin);// * (1.000 / 31.0);

	//PM_CheckStuck();

}


