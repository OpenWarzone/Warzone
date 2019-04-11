/*
===========================================================================
Copyright (C) 2016-2017 Unique One

This file is part of the Warzone navlib source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "navlib_local.h"
#include "nav.h"

int numNavData = 0;
NavData_t BotNavData[ MAX_NAV_DATA ];

vec3_t navmeshSize;
vec3_t navmeshMins;
vec3_t navmeshMaxs;

float navmeshScale = 1.0f;
float navmeshScaleInv = 1.0f;

LinearAllocator alloc( 1024 * 1024 * 16 );
FastLZCompressor comp;

void BotSaveOffMeshConnections( NavData_t *nav )
{
	char mapname[ MAX_QPATH ];
	char filePath[ MAX_QPATH ];
	fileHandle_t f = 0;

	trap->Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
	
	Com_sprintf( filePath, sizeof( filePath ), "maps/%s-%s.navcon", mapname, nav->name );
	f = trap->FS_Open( filePath, &f, FS_WRITE );
	
	if ( !f )
	{
		return;
	}

	int conCount = nav->process.con.offMeshConCount;
	OffMeshConnectionHeader header;
	header.version = LittleLong( NAVMESHCON_VERSION );
	header.numConnections = LittleLong( conCount );
	trap->FS_Write( &header, sizeof( header ), f );

	size_t size = sizeof( float ) * 6 * conCount;
	float *verts = ( float * ) dtAlloc( size, DT_ALLOC_TEMP );
	memcpy( verts, nav->process.con.verts, size );
	SwapArray( verts, conCount * 6 );
	trap->FS_Write( verts, size, f );
	dtFree( verts );

	size = sizeof( float ) * conCount;
	float *rad = ( float * ) dtAlloc( size, DT_ALLOC_TEMP );
	memcpy( rad, nav->process.con.rad, size );
	SwapArray( rad, conCount );
	trap->FS_Write( rad, size, f );
	dtFree( rad );

	size = sizeof( unsigned short ) * conCount;
	unsigned short *flags = ( unsigned short * ) dtAlloc( size, DT_ALLOC_TEMP );
	memcpy( flags, nav->process.con.flags, size );
	SwapArray( flags, conCount );
	trap->FS_Write( flags, size, f );
	dtFree( flags );

	trap->FS_Write( nav->process.con.areas, sizeof( unsigned char ) * conCount, f );
	trap->FS_Write( nav->process.con.dirs, sizeof( unsigned char ) * conCount, f );

	size = sizeof( unsigned int ) * conCount;
	unsigned int *userids = ( unsigned int * ) dtAlloc( size, DT_ALLOC_TEMP );
	memcpy( userids, nav->process.con.userids, size );
	SwapArray( userids, conCount );
	trap->FS_Write( userids, size, f );
	dtFree( userids );

	trap->FS_Close( f );
}

void BotLoadOffMeshConnections( const char *filename, NavData_t *nav )
{
	char mapname[ MAX_QPATH ];
	char filePath[ MAX_QPATH ];
	fileHandle_t f = 0;

	trap->Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
	Com_sprintf( filePath, sizeof( filePath ), "maps/%s-%s.navcon", mapname, filename );
	trap->FS_Open( filePath, &f, FS_READ );

	if ( !f )
	{
		return;
	}

	OffMeshConnectionHeader header;
	trap->FS_Read( &header, sizeof( header ), f );

	header.version = LittleLong( header.version );
	header.numConnections = LittleLong( header.numConnections );

	if ( header.version != NAVMESHCON_VERSION )
	{
		trap->FS_Close( f );
		return;
	}

	int conCount = header.numConnections;

	if ( conCount > nav->process.con.MAX_CON )
	{
		trap->FS_Close( f );
		return;
	}

	nav->process.con.offMeshConCount = conCount;

	trap->FS_Read( nav->process.con.verts, sizeof( float ) * 6 * conCount, f );
	SwapArray( nav->process.con.verts, conCount * 6 );

	trap->FS_Read( nav->process.con.rad, sizeof( float ) * conCount, f );
	SwapArray( nav->process.con.rad, conCount );

	trap->FS_Read( nav->process.con.flags, sizeof( unsigned short ) * conCount, f );
	SwapArray( nav->process.con.flags, conCount );

	trap->FS_Read( nav->process.con.areas, sizeof( unsigned char ) * conCount, f );
	trap->FS_Read( nav->process.con.dirs, sizeof( unsigned char ) * conCount, f );

	trap->FS_Read( nav->process.con.userids, sizeof( unsigned int ) * conCount, f );
	SwapArray( nav->process.con.userids, conCount );

	trap->FS_Close( f );
}

bool BotLoadNavMesh( const char *filename, NavData_t &nav )
{
	char mapname[ MAX_QPATH ];
	char filePath[ MAX_QPATH ];
	char gameName[ MAX_STRING_CHARS ];
	fileHandle_t f = 0;

	BotLoadOffMeshConnections( filename, &nav );

	trap->Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
	trap->Cvar_VariableStringBuffer( "fs_game", gameName, sizeof( gameName ) );
	Com_sprintf( filePath, sizeof( filePath ), "maps/%s-%s.navMesh", mapname, filename );
	trap->Print( " loading navigation mesh file '%s'...\n", filePath );

	int len = trap->FS_Open( filePath, &f, FS_READ );

	if ( !f )
	{
		trap->Print("Cannot open Navigaton Mesh file\n" );
		return false;
	}

	if ( len < 0 )
	{
		trap->Print("Negative Length for Navigation Mesh file\n");
		return false;
	}

	NavMeshSetHeader header;
	
	trap->FS_Read( &header, sizeof( header ), f );

	SwapNavMeshSetHeader( header );

	if ( header.magic != NAVMESHSET_MAGIC )
	{
		trap->Print("File is wrong magic\n" );
		trap->FS_Close( f );
		return false;
	}

	//if ( header.version != NAVMESHSET_VERSION )
	if (header.version < 3)
	{
		trap->Print("File is wrong version found: %d want: %d+\n", header.version, NAVMESHSET_VERSION );
		trap->FS_Close( f );
		return false;
	}

	trap->FS_Read(&navmeshMins, sizeof(vec3_t), f);
	trap->FS_Read(&navmeshMaxs, sizeof(vec3_t), f);

	navmeshSize[0] = navmeshMaxs[0] - navmeshMins[0];
	navmeshSize[1] = navmeshMaxs[1] - navmeshMins[1];
	navmeshSize[2] = navmeshMaxs[2] - navmeshMins[2];

	trap->Print("navmesh mins: %f %f %f.\n", navmeshMins[0], navmeshMins[1], navmeshMins[2]);
	trap->Print("navmesh maxs: %f %f %f.\n", navmeshMaxs[0], navmeshMaxs[1], navmeshMaxs[2]);
	trap->Print("navmesh size: %f %f %f.\n", navmeshSize[0], navmeshSize[1], navmeshSize[2]);

	if (header.version > 3)
	{// Added scale with version 3.0...
		trap->FS_Read(&navmeshScale, sizeof(float), f);
		navmeshScaleInv = 1.0f / navmeshScale;
	}
	else
	{
		navmeshScale = 1.0f;
	}

	nav.mesh = dtAllocNavMesh();

	if ( !nav.mesh )
	{
		trap->Print("Unable to allocate nav mesh\n" );
		trap->FS_Close( f );
		return false;
	}

	dtStatus status = nav.mesh->init( &header.params );

	if ( dtStatusFailed( status ) )
	{
		trap->Print("Could not init navmesh\n" );
		dtFreeNavMesh( nav.mesh );
		nav.mesh = nullptr;
		trap->FS_Close( f );
		return false;
	}

	nav.cache = dtAllocTileCache();

	if ( !nav.cache )
	{
		trap->Print("Could not allocate tile cache\n" );
		dtFreeNavMesh( nav.mesh );
		nav.mesh = nullptr;
		trap->FS_Close( f );
		return false;
	}

	status = nav.cache->init( &header.cacheParams, &alloc, &comp, &nav.process );

	if ( dtStatusFailed( status ) )
	{
		trap->Print("Could not init tile cache\n" );
		dtFreeNavMesh( nav.mesh );
		dtFreeTileCache( nav.cache );
		nav.mesh = nullptr;
		nav.cache = nullptr;
		trap->FS_Close( f );
		return false;
	}

	for ( int i = 0; i < header.numTiles; i++ )
	{
		NavMeshTileHeader tileHeader;

		trap->FS_Read( &tileHeader, sizeof( tileHeader ), f );

		SwapNavMeshTileHeader( tileHeader );

		if ( !tileHeader.tileRef || !tileHeader.dataSize )
		{
			trap->Print("NUll Tile in navmesh\n" );
			dtFreeNavMesh( nav.mesh );
			dtFreeTileCache( nav.cache );
			nav.cache = nullptr;
			nav.mesh = nullptr;
			trap->FS_Close( f );
			return false;
		}

		unsigned char *data = ( unsigned char * ) dtAlloc( tileHeader.dataSize, DT_ALLOC_PERM );

		if ( !data )
		{
			trap->Print("Failed to allocate memory for tile data\n" );
			dtFreeNavMesh( nav.mesh );
			dtFreeTileCache( nav.cache );
			nav.cache = nullptr;
			nav.mesh = nullptr;
			trap->FS_Close( f );
			return false;
		}

		memset( data, 0, tileHeader.dataSize );

		trap->FS_Read( data, tileHeader.dataSize, f );

		if ( LittleLong( 1 ) != 1 )
		{
			dtTileCacheHeaderSwapEndian( data, tileHeader.dataSize );
		}

		dtCompressedTileRef tile = 0;
		dtStatus status = nav.cache->addTile( data, tileHeader.dataSize, DT_TILE_FREE_DATA, &tile );

		if ( dtStatusFailed( status ) )
		{
			trap->Print("Failed to add tile to navmesh\n" );
			dtFree( data );
			dtFreeTileCache( nav.cache );
			dtFreeNavMesh( nav.mesh );
			nav.cache = nullptr;
			nav.mesh = nullptr;
			trap->FS_Close( f );
			return false;
		}

		if ( tile )
		{
			nav.cache->buildNavMeshTile( tile, nav.mesh );
		}
	}

	trap->FS_Close( f );
	return true;
}

inline void *dtAllocCustom( size_t size, dtAllocHint )
{
	return malloc( size );
}

inline void dtFreeCustom( void *ptr )
{
	free( ptr );
}

void Navlib::NavlibShutdown( void )
{
	for ( int i = 0; i < numNavData; i++ )
	{
		NavData_t *nav = &BotNavData[ i ];

		if ( nav->cache )
		{
			dtFreeTileCache( nav->cache );
			nav->cache = 0;
		}

		if ( nav->mesh )
		{
			dtFreeNavMesh( nav->mesh );
			nav->mesh = 0;
		}

		if ( nav->query )
		{
			dtFreeNavMeshQuery( nav->query );
			nav->query = 0;
		}

		nav->process.con.reset();
		memset( nav->name, 0, sizeof( nav->name ) );
	}

#ifndef BUILD_SERVER
	NavEditShutdown();
#endif
	numNavData = 0;
}

bool Navlib::NavlibSetup( const navlibClass_t *botClass, qhandle_t *navHandle )
{
	//vmCvar_t *maxNavNodes = NULL;
	//trap->Cvar_Register(maxNavNodes, "bot_maxNavNodes", "4096", CVAR_LATCH);

	int maxNodes = 16384;// 4096;
	
	if ( !numNavData )
	{
		vec3_t clearVec = { 0, 0, 0 };

		dtAllocSetCustom( dtAllocCustom, dtFreeCustom );

		for ( int i = 0; i < MAX_GENTITIES; i++ )
		{
			// should only init the corridor once
			if ( !agents[ i ].corridor.getPath() )
			{
				if ( !agents[ i ].corridor.init( MAX_BOT_PATH ) )
				{
					return false;
				}
			}

			agents[ i ].corridor.reset( 0, clearVec );
			agents[ i ].clientNum = i;
			agents[ i ].needReplan = true;
			agents[ i ].nav = nullptr;
			agents[ i ].offMesh = false;
			memset( agents[ i ].routeResults, 0, sizeof( agents[ i ].routeResults ) );
		}
#ifndef BUILD_SERVER
		NavEditInit();
#endif
	}

	if ( numNavData == MAX_NAV_DATA )
	{
		trap->Print( "maximum number of navigation meshes exceeded\n" );
		return false;
	}

	NavData_t *nav = &BotNavData[ numNavData ];
	const char *filename = botClass->name;

	if ( !BotLoadNavMesh( filename, *nav ) )
	{
		NavlibShutdown();
		return false;
	}

	Q_strncpyz( nav->name, botClass->name, sizeof( nav->name ) );
	nav->query = dtAllocNavMeshQuery();

	if ( !nav->query )
	{
		trap->Print( "Could not allocate Detour Navigation Mesh Query for navmesh %s\n", filename );
		NavlibShutdown();
		return false;
	}

	if ( dtStatusFailed( nav->query->init( nav->mesh, maxNodes) ) )
	{
		trap->Print( "Could not init Detour Navigation Mesh Query for navmesh %s\n", filename );
		NavlibShutdown();
		return false;
	}

	nav->filter.setIncludeFlags( botClass->polyFlagsInclude );
	nav->filter.setExcludeFlags( botClass->polyFlagsExclude );
	*navHandle = numNavData;
	numNavData++;
	return true;
}
