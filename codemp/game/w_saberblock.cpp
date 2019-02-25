//This file contains functions relate to the saber impact behavior of OJP Enhanced's saber system.
//Stoiss NOTE //[NewSaberSys] new functions call has been modified or delete in this section here and does not behave like OJP anymore
#include "g_local.h"
#include "bg_local.h"

extern int G_PowerLevelForSaberAnim(gentity_t *ent, int saberNum, qboolean mySaberHit);
extern qboolean BG_SuperBreakWinAnim(int anim);
extern stringID_table_t StanceTable[];
extern qboolean BG_SabersOff(playerState_t *ps);
extern qboolean BG_StabDownAnim(int anim);
extern qboolean G_ClientIdleInWorld(gentity_t *ent);
extern qboolean BG_SaberInNonIdleDamageMove(playerState_t *ps, int AnimIndex);
extern qboolean WP_SabersCheckLock(gentity_t *ent1, gentity_t *ent2);
extern stringID_table_t animTable[MAX_ANIMATIONS + 1];
//[SaberSys]
#ifdef __MISSILES_AUTO_PARRY__
extern void WP_SaberBlockNonRandom(gentity_t *self, vec3_t hitloc, qboolean missileBlock);
#else
extern void WP_SaberBlock(gentity_t *playerent, vec3_t hitloc, qboolean missileBlock);
#endif //__MISSILES_AUTO_PARRY__
//[/SaberSys]
extern void G_SaberBounce(gentity_t* self, gentity_t* other, qboolean hitBody);
extern int PM_SaberBounceForAttack(int move);
qboolean SaberAttacking(gentity_t *self);
extern qboolean saberKnockOutOfHand(gentity_t *saberent, gentity_t *saberOwner, vec3_t velocity);
extern qboolean BG_InKnockDown(int anim);
extern qboolean BG_SaberInTransitionAny(int move);
qboolean PM_SaberInBrokenParry(int move);
extern qboolean CheckManualBlocking(gentity_t *attacker, gentity_t *defender);

void SabBeh_AttackVsAttack(gentity_t *self,	gentity_t *otherOwner)
{//set the saber behavior for two attacking blades hitting each other

	if (WP_SabersCheckLock(self, otherOwner))
	{
		self->client->ps.saberBlocked = BLOCKED_NONE;
		otherOwner->client->ps.saberBlocked = BLOCKED_NONE;

	}
	
	if (WP_SabersCheckLock(otherOwner, self))
	{
		self->client->ps.saberBlocked = BLOCKED_NONE;
		otherOwner->client->ps.saberBlocked = BLOCKED_NONE;

	}
	
}

extern void G_ForcePowerDrain(gentity_t *victim, gentity_t *attacker, int amount);
extern int G_SaberFPDrain(gentity_t *defender, gentity_t *attacker, vec3_t hitLoc);
extern void BG_AddForcePowerToPlayer(playerState_t * ps, int Forcepower);
void SabBeh_AttackVsBlock( gentity_t *attacker, gentity_t *blocker, vec3_t hitLoc, qboolean hitSaberBlade)
{//set the saber behavior for an attacking vs blocking/parrying blade impact
	qboolean startSaberLock = qfalse;

	if (SaberAttacking(attacker))
	{
		G_SaberBounce(blocker, attacker, qfalse);
	}

	if(!OnSameTeam(attacker, blocker) || g_friendlySaber.integer)
	{//don't do parries or charge/regen DP unless we're in a situation where we can actually hurt the target.
		if(!startSaberLock)
		{//normal saber blocks
			//update the blocker's block move
			blocker->client->ps.saberLockFrame = 0; //break out of saberlocks.
			//[SaberSys]
#ifdef __MISSILES_AUTO_PARRY__
			WP_SaberBlockNonRandom(blocker, hitLoc, qfalse);
#else
			WP_SaberBlock(blocker, hitLoc, qfalse);
#endif //__MISSILES_AUTO_PARRY__
			//[/SaberSys]
		}
	}

#ifdef __BLOCK_FP_DRAIN__
	G_ForcePowerDrain(blocker, attacker);
#endif //__BLOCK_FP_DRAIN__

	//costs FP as well.
	BG_AddForcePowerToPlayer(&blocker->client->ps, 1);
}

extern qboolean NPC_IsAlive(gentity_t *self, gentity_t *NPC); // Also valid for non-npcs.
void Update_Saberblocking(gentity_t *self, gentity_t *otherOwner, vec3_t hitLoc, qboolean *didHit, qboolean otherHitSaberBlade)
{	
	if (!self || !self->client || !NPC_IsAlive(self, self))
		return;

	if (!otherOwner || !otherOwner->client || !NPC_IsAlive(self, otherOwner))
		return;

	{//whatever other states self can be in.  (returns, bounces, or something)
		G_SaberBounce(otherOwner, self, qfalse);
		G_SaberBounce(self, otherOwner, qfalse);
	}
}

//[NewSaberSys]
void G_SaberBounce(gentity_t* self, gentity_t* other, qboolean hitBody)
{
	if (self && self->client && NPC_IsAlive(self, self) && other->client && NPC_IsAlive(self, other))
	{
		if (!BG_SaberInSpecialAttack(self->client->ps.torsoAnim))
		{
			if (SaberAttacking(self))
			{// Saber is in attack, use bounce for this attack.
				self->client->ps.saberMove = PM_SaberBounceForAttack(self->client->ps.saberMove);
				self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;
			}
			else
			{// Saber is in defense, use defensive bounce.
				self->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			}
		}
	}
}
//[/NewSaberSys]

void G_Stagger(gentity_t *hitEnt, gentity_t *atk, qboolean allowAnyMove)
{
	if (PM_StaggerAnim(hitEnt->client->ps.torsoAnim) || PM_StaggerAnim(atk->client->ps.torsoAnim))
	{
		return;
	}
	if (PM_InGetUpAnimation(hitEnt->client->ps.legsAnim))
	{
		return;
	}
	if (hitEnt->client->ps.forceHandExtend == HANDEXTEND_KNOCKDOWN &&
		hitEnt->client->ps.forceHandExtendTime > level.time)
	{
		return;
	}
	
	int animChoice = irand(0, 6); // this could possibly be based on animation done when the clash happend, but this should do for now.
	int useAnim = -1;

	switch (animChoice) {
	default:
	case 0:
		useAnim = BOTH_BASHED1;
		break;
	case 1:
		useAnim = BOTH_H1_S1_T_;
		break;
	case 2:
		useAnim = BOTH_H1_S1_TR;
		break;
	case 3:
		useAnim = BOTH_H1_S1_TL;
		break;
	case 4:
		useAnim = BOTH_H1_S1_BL;
		break;
	case 5:
		useAnim = BOTH_H1_S1_B_;
		break;
	case 6:
		useAnim = BOTH_H1_S1_BR;
		break;
	}

	G_SetAnim(hitEnt, &(hitEnt->client->pers.cmd), SETANIM_TORSO, useAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

	if (PM_StaggerAnim(hitEnt->client->ps.torsoAnim))
	{
		hitEnt->client->ps.saberMove = LS_NONE;
		hitEnt->client->ps.saberBlocked = BLOCKED_NONE;
		hitEnt->client->ps.weaponTime = hitEnt->client->ps.torsoTimer;
		hitEnt->client->StaggerAnimTime = hitEnt->client->ps.torsoTimer + level.time;
	}
}

//[NewSaberSys]
qboolean WP_PlayerSaberAttack(gentity_t *self)
{
	//We have a lot of different checks to see if we are actually attacking
	if (BG_SaberInAttack(self->client->ps.saberMove))
	{
		return qtrue;
	}

	if (PM_InSaberAnim(self->client->ps.torsoAnim) && !self->client->ps.saberBlocked &&
		self->client->ps.saberMove != LS_READY && self->client->ps.saberMove != LS_NONE)
	{
		if (self->client->ps.saberMove < LS_PARRY_UP || self->client->ps.saberMove > LS_REFLECT_LL)
		{
			return qtrue;
		}
	}

	if (PM_SaberInBrokenParry(self->client->ps.saberMove))
	{
		return qtrue;
	}

	if (self->client->pers.cmd.buttons & BUTTON_ATTACK)
	{ //don't block when the player is trying to slash
		return qtrue;
	}


	if (SaberAttacking(self))
	{ //attacking, can't block now
		return qtrue;
	}

	if (self->client->ps.saberMove != LS_READY &&
		!self->client->ps.saberBlocking)
	{
		return qtrue;
	}

	return qfalse;
}
//[/NewSaberSys]