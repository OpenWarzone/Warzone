#ifndef JKG_DAMAGETYPES_H
#define JKG_DAMAGETYPES_H

#include "g_local.h"
#include "bg_public.h"

/*
The damage radius function describes how the damage radius changes over time.

RF_CONSTANT - Damage radius stays constant.
RF_LINEAR - Damage radius changes in a linear fashion.
RF_NONLINEAR - Damage radius stays at start value until some proportion of lifetime
has passed. Following this, the radius changes linearly until the end radius is reached.
RF_CLAMP - Damage radius changes in a linear fashion to its end radius, until some
proportion of lifetime has passed.
RF_WAVE - Damage radius oscillates over time, between the start and end radius at a
given frequency.
*/
typedef enum radiusFunc_e
{
    RF_CONSTANT,
    RF_LINEAR,
    RF_NONLINEAR,
    RF_CLAMP,
    RF_WAVE
} radiusFunc_t;

/*
Damage function describes how the damage changes over distance from the centre of the
damage origin, e.g. centre of explosion.
*/
typedef enum damageFunc_e
{
    DF_CONSTANT,
    DF_LINEAR,
    DF_GAUSSIAN
} damageFunc_t;

typedef struct radiusParams_s
{
    int startRadius;
    int endRadius;
    int generic1;
    damageFunc_t damageFunc;
    radiusFunc_t radiusFunc;
} radiusParams_t;

typedef enum penetrationType_e
{
    PT_NONE,
    PT_SHIELD,
    PT_SHIELD_ARMOR,
    PT_SHIELD_ARMOR_BUILDING
} penetrationType_t;

typedef struct damageSettings_s
{
    damageType_t        type;
    qboolean            radial;
    radiusParams_t      radiusParams;
    int                 lifetime;
    int                 delay;
    int                 damage;
    int                 damageDelay;
    int					damageType;
    penetrationType_t   penetrationType;
    qboolean            planar;

	// Damage override stuff
	int					weapon;
	int					weaponVariation;
} damageSettings_t;

qhandle_t JKG_RegisterDamageSettings ( const damageSettings_t *settings );
void JKG_DoDamage ( qhandle_t handle, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t origin, int dflags, int mod );
void JKG_DoSplashDamage ( qhandle_t handle, const vec3_t origin, gentity_t *inflictor, gentity_t *attacker, gentity_t *ignoreEnt, int mod );
void JKG_DoDirectDamage ( qhandle_t handle, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t origin, int dflags, int mod );
void JKG_DamagePlayers ( void );
void JKG_DoPlayerDamageEffects ( gentity_t *ent );

#endif
