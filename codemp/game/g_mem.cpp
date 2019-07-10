// Copyright (C) 1999-2000 Id Software, Inc.
//
// Simple linear memory allocator
// g_mem.c
//

#include "g_local.h"

/*
  The purpose of G_Alloc is to efficiently allocate memory for objects
  typically when the game is starting up. It shouldn't be used as a general
  purpose allocator that's used when the game is running, and especially not
  from a client command! It is *by design* that memory blocks can't be
  deallocated while the game is running!

  If your mod experiences the allocation failed error often, use g_debugAlloc
  to trace down where G_Alloc is being called and to make sure it isn't used
  often while the game is running. Feel free to increase POOLSIZE for your mod!

  More information about Linear Allocators:
  http://www.altdevblogaday.com/2011/02/12/alternatives-to-malloc-and-new/
*/

#ifdef __AAS_AI_TESTING__
#define POOLSIZE	(256 * 1024 * 1024) // UQ1: 256mb - Now that we removed B_Alloc system and npc's use a botinfo...
#else //!__AAS_AI_TESTING__
#define POOLSIZE	(96/*128*/ * 1024 * 1024) // UQ1: 128mb - Now that we removed B_Alloc system...
#endif //__AAS_AI_TESTING__

static char		memoryPool[POOLSIZE];
static int		allocPoint;

void *G_Alloc( int size, char *requestedBy ) {
	char	*p;

	if ( size <= 0 ) {
		trap->Error( ERR_DROP, "G_Alloc: zero-size allocation requested from %s\n", size, requestedBy );
		return NULL;
	}

	if ( g_debugAlloc.integer ) {
		trap->Print( "G_Alloc: %s requested %i bytes (%i left)\n", requestedBy, size, POOLSIZE - allocPoint - ( ( size + 31 ) & ~31 ) );
	}

	if ( allocPoint + size > POOLSIZE ) {
	  trap->Error( ERR_DROP, "G_Alloc: %s failed on allocation of %i bytes\n", requestedBy, size ); // bk010103 - was %u, but is signed
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 31 ) & ~31;

	return p;
}

void G_InitMemory( void ) {
	allocPoint = 0;
}

void Svcmd_GameMem_f( void ) {
	trap->Print( "Game memory status: %i out of %i bytes allocated\n", allocPoint, POOLSIZE );
}
