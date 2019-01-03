//[CoOp]
//ported from SP
#include "b_local.h"

//custom anims:
	//both_attack1 - running attack
	//both_attack2 - crouched attack
	//both_attack3 - standing attack
	//both_stand1idle1 - idle
	//both_crouch2stand1 - uncrouch
	//both_death4 - running death

#define	ASSASSIN_SHIELD_SIZE	75
#define TURN_ON					0x00000000
#define TURN_OFF				0x00000100



////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
qboolean BubbleShield_IsOn(gentity_t *self)
{
	return (qboolean)(self->flags&FL_SHIELDED);
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_TurnOn(gentity_t *self)
{
	if (!BubbleShield_IsOn(self))
	{
		self->flags |= FL_SHIELDED;
		//aiEnt->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE;
		NPC_SetSurfaceOnOff( self, "force_shield", TURN_ON );
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_TurnOff(gentity_t *self)
{
	if ( BubbleShield_IsOn(self))
	{
		self->flags &= ~FL_SHIELDED;
		//aiEnt->client->ps.powerups[PW_GALAK_SHIELD] = 0;
		NPC_SetSurfaceOnOff( self, "force_shield", TURN_OFF );
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Push A Particular Ent
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_PushEnt(gentity_t *aiEnt, gentity_t* pushed, vec3_t smackDir)
{
	//RAFIXME - add MOD_ELECTROCUTE?
	G_Damage(pushed, aiEnt, aiEnt, smackDir, aiEnt->r.currentOrigin, Q_irand( 20, 50), DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN/*MOD_ELECTROCUTE*/); 
	G_Throw(pushed, smackDir, 10);

	// Make Em Electric
	//------------------
	if(pushed->client)
	{
		pushed->client->ps.electrifyTime = level.time + 1000;
	}
	/* using MP equivilent
 	pushed->s.powerups |= (1 << PW_SHOCKED);
	if (pushed->client)
	{
		pushed->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
	}
	*/
}

////////////////////////////////////////////////////////////////////////////////////////
// Go Through All The Ents Within The Radius Of The Shield And Push Them
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_PushRadiusEnts(gentity_t *aiEnt)
{
	int			numEnts, i;
	int entityList[MAX_GENTITIES];
	gentity_t*	radiusEnt;
	const float	radius = ASSASSIN_SHIELD_SIZE;
	vec3_t		mins, maxs;
	vec3_t		smackDir;
	float		smackDist;

	for ( i = 0; i < 3; i++ )
	{
		mins[i] = aiEnt->r.currentOrigin[i] - radius;
		maxs[i] = aiEnt->r.currentOrigin[i] + radius;
	}

	numEnts = trap->EntitiesInBox(mins, maxs, entityList, 128);
	for (i=0; i<numEnts; i++)
	{
		radiusEnt = &g_entities[entityList[i]];
		// Only Clients
		//--------------
		if (!radiusEnt || !radiusEnt->client)
		{
			continue;
		}

		// Don't Push Away Other Assassin Droids
		//---------------------------------------
		if (radiusEnt->client->NPC_class==aiEnt->client->NPC_class)
		{
			continue;
		}

		// Should Have Already Pushed The Enemy If He Touched Us
		//-------------------------------------------------------
		if (aiEnt->enemy &&  aiEnt->NPC->touchedByPlayer==aiEnt->enemy && radiusEnt==aiEnt->enemy)
		{
			continue;
		}

		// Do The Vector Distance Test
		//-----------------------------
		VectorSubtract(radiusEnt->r.currentOrigin, aiEnt->r.currentOrigin, smackDir);
		smackDist = VectorNormalize(smackDir);
		if (smackDist<radius)
		{
			BubbleShield_PushEnt(aiEnt, radiusEnt, smackDir);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_Update(gentity_t *aiEnt)
{
	// Shields Go When You Die
	//-------------------------
	if (aiEnt->health<=0)
	{
		BubbleShield_TurnOff(aiEnt);
		return;
	}


	// Recharge Shields
	//------------------
 	aiEnt->client->ps.stats[STAT_ARMOR] += 1;
	if (aiEnt->client->ps.stats[STAT_ARMOR]>250)
	{
		aiEnt->client->ps.stats[STAT_ARMOR] = 250;
	}




	// If We Have Enough Armor And Are Not Shooting Right Now, Kick The Shield On
	//----------------------------------------------------------------------------
 	if (aiEnt->client->ps.stats[STAT_ARMOR]>100 && TIMER_Done(aiEnt, "ShieldsDown"))
	{
		// Check On Timers To Raise And Lower Shields
		//--------------------------------------------
		if ((level.time - aiEnt->NPC->enemyLastSeenTime)<1000 && TIMER_Done(aiEnt, "ShieldsUp"))
		{
			TIMER_Set(aiEnt, "ShieldsDown", 2000);		// Drop Shields
			TIMER_Set(aiEnt, "ShieldsUp", Q_irand(4000, 5000));	// Then Bring Them Back Up For At Least 3 sec
		} 

		BubbleShield_TurnOn(aiEnt);
		if (BubbleShield_IsOn(aiEnt))
		{
			// Update Our Shader Value
			//-------------------------
	 	 	aiEnt->s.customRGBA[0] = 
			aiEnt->s.customRGBA[1] = 
			aiEnt->s.customRGBA[2] =
  			aiEnt->s.customRGBA[3] = (aiEnt->client->ps.stats[STAT_ARMOR] - 100);


			// If Touched By An Enemy, ALWAYS Shove Them
			//-------------------------------------------
			if (aiEnt->enemy &&  aiEnt->NPC->touchedByPlayer==aiEnt->enemy)
			{
				vec3_t dir;
				VectorSubtract(aiEnt->enemy->r.currentOrigin, aiEnt->r.currentOrigin, dir);
				VectorNormalize(dir);
				BubbleShield_PushEnt(aiEnt, aiEnt->enemy, dir);
			}

			// Push Anybody Else Near
			//------------------------
			BubbleShield_PushRadiusEnts(aiEnt);
		}
	}


	// Shields Gone
	//--------------
	else
	{
		BubbleShield_TurnOff(aiEnt);
	}
}
//[/CoOp]

