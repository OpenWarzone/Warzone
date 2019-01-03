//Rocket Trooper AI code
#include "g_local.h"

void RT_FireDecide( void )
{
}

//=====================================================================================
//FLYING behavior 
//=====================================================================================
qboolean RT_Flying( gentity_t *self )
{
	return ((qboolean)(self->client->ps.eFlags2 & EF2_FLYING));
}

