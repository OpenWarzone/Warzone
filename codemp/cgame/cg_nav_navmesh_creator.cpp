/*=========================================================
// Navigation Mesh Editor
//---------------------------------------------------------
// Description:
// Code for the creation, and editing of the navigation mesh
// used by NPCs.
//
// Version:
// $Id: jkg_navmesh_creator.cpp 389 2011-07-21 01:03:36Z Didz $
//=========================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <vector>

#include "../Recast/Recast/Recast.h"
#include "../Recast/Detour/DetourNavMeshBuilder.h"

#include "cg_local.h"
//#include "qcommon/q_shared.h"
#include "qcommon/qfiles.h"

extern float aw_percent_complete;
extern char task_string1[255];
extern char task_string2[255];
extern char task_string3[255];
extern char last_node_added_string[255];
extern clock_t	aw_stage_start_time;

void UpdatePercentBar(float percent, char *text, char *text2, char *text3)
{
	aw_percent_complete = percent;

	if (text[0] != '\0') trap->Print("^1*** ^3%s^5: %s\n", "NAVMESH-GENERATOR", text);
	strcpy(task_string1, va("^5%s", text));
	strcpy(task_string2, va("^5%s", text2));
	strcpy(task_string3, va("^5%s", text3));
	trap->UpdateScreen();
}

int CountIndices ( const dsurface_t *surfaces, int numSurfaces )
{
    int count = 0;
    for ( int i = 0; i < numSurfaces; i++, surfaces++ )
    {
        if ( surfaces->surfaceType != MST_PLANAR && surfaces->surfaceType != MST_TRIANGLE_SOUP )
        {
            continue;
        }
        
        count += surfaces->numIndexes;
    }
    
    return count;
}

void LoadTriangles ( const int *indexes, int* tris, const dsurface_t *surfaces, int numSurfaces )
{
    int t = 0;
    int v = 0;
    for ( int i = 0; i < numSurfaces; i++, surfaces++ )
    {
        if ( surfaces->surfaceType != MST_PLANAR && surfaces->surfaceType != MST_TRIANGLE_SOUP )
        {
            continue;
        }
        
        for ( int j = surfaces->firstIndex, k = 0; k < surfaces->numIndexes; j++, k++ )
        {
            tris[t++] = v + indexes[j];
        }
        
        v += surfaces->numVerts;
    }
}

void LoadVertices ( const drawVert_t *vertices, float* verts, const dsurface_t *surfaces, const int numSurfaces )
{
    int v = 0;
    for ( int i = 0; i < numSurfaces; i++, surfaces++ )
    {
        // will handle patches later...
        if ( surfaces->surfaceType != MST_PLANAR && surfaces->surfaceType != MST_TRIANGLE_SOUP )
        {
            continue;
        }
        
        for ( int j = surfaces->firstVert, k = 0; k < surfaces->numVerts; j++, k++ )
        {
            verts[v++] = -vertices[j].xyz[0];
            verts[v++] = vertices[j].xyz[2];
            verts[v++] = -vertices[j].xyz[1];
        }
    }
}

qboolean LoadMapGeometry ( const char *buffer, vec3_t mapmins, vec3_t mapmaxs, float* &verts, int &numverts, int* &tris, int &numtris )
{
    const dheader_t *header = (const dheader_t *)buffer;
    if ( header->ident != BSP_IDENT )
    {
        trap->Print (va("Expected ident '%d', found %d.\n", BSP_IDENT, header->ident));
        return qfalse;
    }
    
    if ( header->version != BSP_VERSION )
    {
        trap->Print (va("Expected version '%d', found %d.\n", BSP_VERSION, header->version));
        return qfalse;
    }
    
    // Load indices
    const int *indexes = (const int *)(buffer + header->lumps[LUMP_DRAWINDEXES].fileofs);
    const dsurface_t *surfaces = (const dsurface_t *)(buffer + header->lumps[LUMP_SURFACES].fileofs);
    int numSurfaces = header->lumps[LUMP_SURFACES].filelen / sizeof (dsurface_t);
    
    numtris = CountIndices (surfaces, numSurfaces);
    tris = new int[numtris];
    numtris /= 3;
    
    LoadTriangles (indexes, tris, surfaces, numSurfaces);
    
    // Load vertices
    const drawVert_t *vertices = (const drawVert_t *)(buffer + header->lumps[LUMP_DRAWVERTS].fileofs);
    numverts = header->lumps[LUMP_DRAWVERTS].filelen / sizeof (drawVert_t);
    
    verts = new float[3 * numverts];
    LoadVertices (vertices, verts, surfaces, numSurfaces);
    
    // Get map bounds. First model is always the entire map
    const dmodel_t *models = (const dmodel_t *)(buffer + header->lumps[LUMP_MODELS].fileofs);
    mapmins[0] = -models[0].maxs[0];
    mapmins[1] = models[0].mins[2];
    mapmins[2] = -models[0].maxs[1];
    
    mapmaxs[0] = -models[0].mins[0];
    mapmaxs[1] = models[0].maxs[2];
    mapmaxs[2] = -models[0].mins[1];
    
    return qtrue;
}

template <typename T>
void SwapYZ ( int numVerts, T *verts )
{
    for ( int i = 0, j = 0; j < numVerts; j++, i += 3 )
    {
        T temp = verts[i + 1];
        
        verts[i + 0] = -verts[i + 0];
        verts[i + 1] = verts[i + 2];
        verts[i + 2] = -temp;
    }
}

struct navMeshDataHeader_t
{
    int version;
    int filesize;
    
    // Poly Mesh
    int vertsOffset;
    int numVerts;
    int polysOffset;
    int areasOffset;
    int flagsOffset;
    int numPolys;
    int numVertsPerPoly;
    vec3_t mins, maxs;
    
    // Detailed Mesh
    int dMeshesOffset;
    int dNumMeshes;
    int dVertsOffset;
    int dNumVerts;
    int dTrisOffset;
    int dNumTris;
    
    // Cell size
    float cellSize;
    float cellHeight;
};
void CacheNavMeshData ( const rcPolyMesh *polyMesh, const rcPolyMeshDetail *detailedPolyMesh, const rcConfig *cfg )
{
	char mapname[128] = { 0 };
	sprintf(mapname, "maps/%s.jnd", cgs.currentmapname);

    navMeshDataHeader_t header;
    int vertsSize = polyMesh->nverts * sizeof (unsigned short) * 3;
    int polysSize = polyMesh->npolys * sizeof (unsigned short) * polyMesh->nvp * 2;
    int areasSize = polyMesh->npolys * sizeof (unsigned char);
    int flagsSize = polyMesh->npolys * sizeof (unsigned short);
    int dMeshesSize = detailedPolyMesh->nmeshes * sizeof (unsigned int) * 4;
    int dVertsSize = detailedPolyMesh->nverts * sizeof (float) * 3;
    int dTrisSize = detailedPolyMesh->ntris * sizeof (unsigned char) * 3;
    char *buffer = NULL;

	/*
	trap->Print("vertsSize %i\n", vertsSize);
	trap->Print("polysSize %i\n", polysSize);
	trap->Print("areasSize %i\n", areasSize);
	trap->Print("flagsSize %i\n", flagsSize);
	trap->Print("dMeshesSize %i\n", dMeshesSize);
	trap->Print("dVertsSize %i\n", dVertsSize);
	trap->Print("dTrisSize %i\n", dTrisSize);
    */

    int ptr = sizeof (header);
    
    header.version = 1;
    header.vertsOffset = ptr; ptr += vertsSize;
    header.numVerts = polyMesh->nverts;
    header.polysOffset = ptr; ptr += polysSize;
    header.areasOffset = ptr; ptr += areasSize;
    header.flagsOffset = ptr; ptr += flagsSize;
    header.numPolys = polyMesh->npolys;
    header.numVertsPerPoly = polyMesh->nvp;
    VectorCopy (polyMesh->bmin, header.mins);
    VectorCopy (polyMesh->bmax, header.maxs);
    
    header.dMeshesOffset = ptr; ptr += dMeshesSize;
    header.dNumMeshes = detailedPolyMesh->nmeshes;
    header.dVertsOffset = ptr; ptr += dVertsSize;
    header.dNumVerts = detailedPolyMesh->nverts;
    header.dTrisOffset = ptr; ptr += dTrisSize;
    header.dNumTris = detailedPolyMesh->ntris;
    
    header.cellSize = cfg->cs;
    header.cellHeight = cfg->ch;
    
    header.filesize = ptr;
    
    buffer = new char[header.filesize];
    
    // Write header
    ptr = 0;
    navMeshDataHeader_t *start = (navMeshDataHeader_t *)&buffer[ptr];
    *start = header; ptr += sizeof (header);
    
    // Write verts
    unsigned short *s = (unsigned short *)&buffer[ptr];
    for ( int i = 0, j = 0; i < polyMesh->nverts; i++, j += 3 )
    {
        *s++ = polyMesh->verts[j + 0];
        *s++ = polyMesh->verts[j + 1];
        *s++ = polyMesh->verts[j + 2];
    }
    ptr += vertsSize;
    
    // Write polys
    s = (unsigned short *)&buffer[ptr];
    for ( int i = 0, j = 0; i < polyMesh->npolys; i++, j += polyMesh->nvp * 2 )
    {
        for ( int k = 0; k < polyMesh->nvp * 2; k++ )
        {
            *s++ = polyMesh->polys[j + k];
        }
    }
    ptr += polysSize;
    
    // Write areas
    unsigned char *c = (unsigned char *)&buffer[ptr];
    for ( int i = 0; i < polyMesh->npolys; i++ )
    {
        *c++ = polyMesh->areas[i];
    }
    ptr += areasSize;
    
    // Write flags
    s = (unsigned short *)&buffer[ptr];
    for ( int i = 0; i < polyMesh->npolys; i++ )
    {
        *s++ = polyMesh->flags[i];
    }
    ptr += flagsSize;
    
    // Write detailed meshes...
    unsigned int *u = (unsigned int *)&buffer[ptr];
    for ( int i = 0, j = 0; i < detailedPolyMesh->nmeshes; i++, j += 4 )
    {
        *u++ = detailedPolyMesh->meshes[j + 0];
        *u++ = detailedPolyMesh->meshes[j + 1];
        *u++ = detailedPolyMesh->meshes[j + 2];
        *u++ = detailedPolyMesh->meshes[j + 3];
    }
    ptr += dMeshesSize;
    
    // Write detailed verts..
    float *f = (float *)&buffer[ptr];
    for ( int i = 0, j = 0; i < detailedPolyMesh->nverts; i++, j += 3 )
    {
        *f++ = detailedPolyMesh->verts[j + 0];
        *f++ = detailedPolyMesh->verts[j + 1];
        *f++ = detailedPolyMesh->verts[j + 2];
    }
    ptr += dVertsSize;
    
    // Write detailed tris...
    c = (unsigned char *)&buffer[ptr];
    for ( int i = 0, j = 0; i < detailedPolyMesh->ntris; i++, j += 3 )
    {
        *c++ = detailedPolyMesh->tris[j + 0];
        *c++ = detailedPolyMesh->tris[j + 1];
        *c++ = detailedPolyMesh->tris[j + 2];
    }
    ptr += dTrisSize;
    
    if ( ptr != header.filesize )
    {
        trap->Print ("WE DUN GOOFED UP SIRZ\n");
    }
    else
    {
        fileHandle_t f;
        trap->FS_Open (mapname, &f, FS_WRITE);
        if ( !f )
        {
            trap->Print ("Failed to create cache file for navmesh.\n");
        }
        else
        {
            trap->FS_Write (buffer, header.filesize, f);
            trap->FS_Close (f);
            
            trap->Print (va("Navmesh data cached (%d bytes written)...\n", header.filesize));
        }
    }
    
    delete[] buffer;
}

void CreateNavMesh ( const char *mapname )
{
    // gcc doesn't like data being initalized between cross-scope gotos, sorry
    rcHeightfield *heightField;
    unsigned char *navData;
    int navDataSize;
    
    fileHandle_t f = 0;
    int fileLength = trap->FS_Open (mapname, &f, FS_READ);
    if ( fileLength == -1 || !f )
    {
        trap->Print (va("Unable to open '%s' to create the navigation mesh.\n", mapname));
        return;
    }
    
    char *buffer = new char[fileLength + 1];
    trap->FS_Read (buffer, fileLength, f);
    buffer[fileLength] = '\0';
    trap->FS_Close (f);
    
    vec3_t mapmins;
    vec3_t mapmaxs;
    float *verts = NULL;
    int numverts;
    int *tris = NULL;
    int numtris;
    unsigned char *triareas = NULL;
    rcContourSet *contours = NULL;
    rcCompactHeightfield *compHeightField = NULL;
    rcPolyMesh *polyMesh = NULL;
    rcPolyMeshDetail *detailedPolyMesh = NULL;
    
    rcContext context (false);

	aw_stage_start_time = clock();

	UpdatePercentBar(1, "Loading Map Geometry...", "", "");
    
    if ( !LoadMapGeometry (buffer, mapmins, mapmaxs, verts, numverts, tris, numtris) )
    {
        trap->Print (va("Unable to load map geometry from '%s'.\n", mapname));
		UpdatePercentBar(0, "", "", "");
        goto cleanup;
    }
    
    // 1. Create build config...
    rcConfig cfg;
    memset (&cfg, 0, sizeof (cfg));
    VectorCopy (mapmaxs, cfg.bmax);
    VectorCopy (mapmins, cfg.bmin);
	cfg.ch = 64.0f;// 64.0f;// 9.0f;// 3.0f;
	cfg.cs = 384.0;// 512.0f;// 128.0;// 45.0f;// 15.0f;
    cfg.walkableSlopeAngle = 45.0f; // worked out from MIN_WALK_NORMAL - i think it's correct? :x
    cfg.walkableHeight = 64 / cfg.ch;
    cfg.walkableClimb = STEPSIZE / cfg.ch;
    cfg.walkableRadius = 15 / cfg.cs;
    cfg.maxEdgeLen = 12 / cfg.cs;
    cfg.maxSimplificationError = 1.3f;
    cfg.minRegionArea = 64;
    cfg.mergeRegionArea = 400;
    cfg.maxVertsPerPoly = 6;
    cfg.detailSampleDist = 6.0f * cfg.cs;
    cfg.detailSampleMaxError = 1.0f * cfg.ch;


	UpdatePercentBar(2, "Calculating Grid Size...", "", "");
    
    rcCalcGridSize (cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	UpdatePercentBar(5, "Creating Height Field...", "", "");

    // 2. Rasterize input polygon soup!
    heightField = rcAllocHeightfield();
    if ( !rcCreateHeightfield (&context, *heightField, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch) )
    {
        trap->Print ("Failed to create heightfield for navigation mesh.\n");
		UpdatePercentBar(0, "", "", "");
        goto cleanup;
    }

	UpdatePercentBar(10, "Marking Walkable Triangles...", "", "");
    
    triareas = new unsigned char[numtris];
    memset (triareas, 0, numtris);
    rcMarkWalkableTriangles (&context, cfg.walkableSlopeAngle, verts, numverts, tris, numtris, triareas);
    rcRasterizeTriangles (&context, verts, numverts, tris, triareas, numtris, *heightField, cfg.walkableClimb);
    delete[] triareas;
    triareas = NULL;

	UpdatePercentBar(20, "Filterring Walkable Surfaces...", "", "");
    
    // 3. Filter walkable surfaces
    rcFilterLowHangingWalkableObstacles (&context, cfg.walkableClimb, *heightField);
    rcFilterLedgeSpans (&context, cfg.walkableHeight, cfg.walkableClimb, *heightField);
    rcFilterWalkableLowHeightSpans (&context, cfg.walkableHeight, *heightField);

	UpdatePercentBar(25, "Building Compact Height Field...", "", "");
    
    // 4. Partition walkable surface to simple regions
    compHeightField = rcAllocCompactHeightfield();
    if ( !rcBuildCompactHeightfield (&context, cfg.walkableHeight, cfg.walkableClimb, *heightField, *compHeightField) )
    {
        trap->Print ("Failed to create compact heightfield for navigation mesh.\n");
		UpdatePercentBar(0, "", "", "");
        goto cleanup;
    }

	UpdatePercentBar(30, "Eroding Walkable Area...", "", "");
    
    if ( !rcErodeWalkableArea (&context, cfg.walkableRadius, *compHeightField) )
    {
        trap->Print ("Unable to erode walkable surfaces.\n");
		UpdatePercentBar(0, "", "", "");
        goto cleanup;
    }

	UpdatePercentBar(40, "Building Distance Field...", "", "");
    
    if ( !rcBuildDistanceField (&context, *compHeightField) )
    {
        trap->Print ("Failed to build distance field for navigation mesh.\n");
		UpdatePercentBar(0, "", "", "");
        goto cleanup;
    }

	UpdatePercentBar(50, "Building Regions...", "", "");
    
    if ( !rcBuildRegions (&context, *compHeightField, 0, cfg.minRegionArea, cfg.mergeRegionArea) )
    {
        trap->Print ("Failed to build regions for navigation mesh.\n");
		UpdatePercentBar(0, "", "", "");
        goto cleanup;
    }

	UpdatePercentBar(60, "Creating Contours...", "", "");
    
    // 5. Create contours
    contours = rcAllocContourSet();
    if ( !rcBuildContours (&context, *compHeightField, cfg.maxSimplificationError, cfg.maxEdgeLen, *contours) )
    {
        trap->Print ("Failed to create contour set for navigation mesh.\n");
		UpdatePercentBar(0, "", "", "");
        goto cleanup;
    }

	UpdatePercentBar(70, "Building polygons mesh from contours...", "", "");
    
    // 6. Build polygons mesh from contours
    polyMesh = rcAllocPolyMesh();
    if ( !rcBuildPolyMesh (&context, *contours, cfg.maxVertsPerPoly, *polyMesh) )
    {
        trap->Print ("Failed to triangulate contours.\n");
		UpdatePercentBar(0, "", "", "");
        goto cleanup;
    }

	UpdatePercentBar(80, "Creating detail mesh...", "", "");
    
    // 7. Create detail mesh
    detailedPolyMesh = rcAllocPolyMeshDetail();
    if ( !rcBuildPolyMeshDetail (&context, *polyMesh, *compHeightField, cfg.detailSampleDist, cfg.detailSampleMaxError, *detailedPolyMesh) )
    {
        trap->Print ("Failed to create detail mesh for navigation mesh.\n");
		UpdatePercentBar(0, "", "", "");
        goto cleanup;
    }
    
	UpdatePercentBar(90, "Caching nav mesh query object...", "", "");

    // Cache the stuffffffff
    CacheNavMeshData (polyMesh, detailedPolyMesh, &cfg);
    
    // 8. Create navigation mesh query object
    dtNavMeshCreateParams nvParams;
    memset (&nvParams, 0, sizeof (nvParams));
    nvParams.verts = polyMesh->verts;
    nvParams.vertCount = polyMesh->nverts;
    nvParams.polys = polyMesh->polys;
    nvParams.polyAreas = polyMesh->areas;
    nvParams.polyFlags = polyMesh->flags;
    nvParams.polyCount = polyMesh->npolys;
    nvParams.nvp = polyMesh->nvp;
    nvParams.detailMeshes = detailedPolyMesh->meshes;
    nvParams.detailVerts = detailedPolyMesh->verts;
    nvParams.detailVertsCount = detailedPolyMesh->nverts;
    nvParams.detailTris = detailedPolyMesh->tris;
    nvParams.detailTriCount = detailedPolyMesh->ntris;
    nvParams.walkableHeight = 64.0f;
    nvParams.walkableRadius = 22.0f;
    nvParams.walkableClimb = STEPSIZE;
    VectorCopy (polyMesh->bmin, nvParams.bmin);
    VectorCopy (polyMesh->bmax, nvParams.bmax);
    nvParams.cs = cfg.cs;
    nvParams.ch = cfg.ch;
    nvParams.buildBvTree = true;

	UpdatePercentBar(100, "Creating Nav Mesh Data...", "", "");
    
    navData = NULL;
    navDataSize = 0;
    if ( !dtCreateNavMeshData (&nvParams, &navData, &navDataSize) )
    {
        trap->Print ("Failed to create navigation mesh.\n");
		UpdatePercentBar(0, "", "", "");
        goto cleanup;
    }
    
	UpdatePercentBar(0, "", "", "");
    
cleanup:
    rcFreeHeightField (heightField);
    rcFreeCompactHeightfield (compHeightField);
    rcFreeContourSet (contours);
    rcFreePolyMesh (polyMesh);
    rcFreePolyMeshDetail (detailedPolyMesh);
    
    delete[] verts;
    delete[] tris;
    delete[] buffer;
}

void Warzone_Nav_CreateNavMesh_Xycaleth(void)
{
	char mapname[128] = { 0 };

	sprintf(mapname, "maps/%s.bsp", cgs.currentmapname);

	trap->Print("Creating navigation mesh...this may take a while.\n");
	CreateNavMesh(mapname);
	trap->Print("Finished!\n");
}

#if 0
#define GRID_SIZE (15.0f)
typedef enum direction_e
{
    NORTH = 0,
    EAST,
    SOUTH,
    WEST,
    
    NUM_DIRECTIONS
} direction_t;
typedef enum generationStage_e
{
    GS_NONE,
    GS_CREATING_NODES,
    GS_CREATING_AREAS,
} generationStage_t;
typedef struct meshNode_s
{
    vec3_t position;
    vec3_t normal;
    
    qboolean visited;
    int directionsVisited;
} meshNode_t;
// Mesh node builder
static jkgArray_t nodeList;
static meshNode_t *startBuildNode = NULL;
static generationStage_t buildStage = GS_NONE;
// Mesh node visualization
static qboolean forceRevisualization = qfalse;
static int showLabelsToClient;
extern vmCvar_t jkg_nav_edit;
static void JKG_Nav_Visualize ( void );
static void JKG_Nav_Generate ( void );
//=========================================================
// SnapToGrid
//---------------------------------------------------------
// Description:
// Snaps an XY coordinate to a given grid size.
//=========================================================
static void SnapToGrid ( const vec2_t pos, vec2_t newPos )
{
    float cx = pos[0] / GRID_SIZE;
    float cy = pos[1] / GRID_SIZE;
    
    cx = floor (cx + 0.5f);
    cy = floor (cy + 0.5f);
    
    newPos[0] = GRID_SIZE * cx;
    newPos[1] = GRID_SIZE * cy;
}
//=========================================================
// JKG_Nav_Init
//---------------------------------------------------------
// Description:
// Initializes the navigation mesh system.
//=========================================================
void JKG_Nav_Init ( void )
{
    JKG_Array_Init (&nodeList, sizeof (meshNode_t), 16);
    showLabelsToClient = 0;
}
//=========================================================
// JKG_Nav_Shutdown
//---------------------------------------------------------
// Description:
// Shuts down the navigation mesh system.
//=========================================================
void JKG_Nav_Shutdown ( void )
{
    JKG_Array_Free (&nodeList);
}
//=========================================================
// JKG_Nav_Cmd_Generate_f
//---------------------------------------------------------
// Description:
// Begins the navigation mesh generation.
//=========================================================
void JKG_Nav_Cmd_Generate_f ( gentity_t *ent )
{
    if ( !jkg_nav_edit.integer )
    {
        return;
    }
    
    if ( nodeList.size == 0 )
    {
        return;
    }
    
    startBuildNode = (meshNode_t *)nodeList.data;
    buildStage = GS_CREATING_NODES;
}
//=========================================================
// JKG_Nav_Cmd_MarkWalkableSurface_f
//---------------------------------------------------------
// Description:
// Marks the surface the player is looking at as a walkable
// surface.
//=========================================================
void JKG_Nav_Cmd_MarkWalkableSurface_f ( gentity_t *ent )
{
    vec3_t forward;
    trace_t trace;
    vec3_t start, end;
    qboolean reachedGroundOrEnd = qfalse;
    
    if ( !jkg_nav_edit.integer )
    {
        return;
    }
    
    if ( !ent->client )
    {
        return;
    }
    
    VectorCopy (ent->client->ps.origin, start);
    start[2] += ent->client->ps.viewheight;
    
    AngleVectors (ent->client->ps.viewangles, forward, NULL, NULL);
    
    do
    {
        VectorMA (start, 16384.0f, forward, end);
    
        trap->Trace (&trace, start, NULL, NULL, end, ent->s.number, CONTENTS_SOLID);
        
        if ( trace.fraction == 1.0f )
        {
            // Managed to reach the end without hitting anything???
            reachedGroundOrEnd = qtrue;
        }
        else if ( trace.startsolid != qtrue )
        {
            if ( trace.entityNum == ENTITYNUM_WORLD )
            {
                meshNode_t node;
                VectorCopy (trace.endpos, node.position);
                SnapToGrid (node.position, node.position);
                
                VectorCopy (trace.plane.normal, node.normal);
                node.directionsVisited = 0;
                node.visited = qfalse;
                
                #ifdef _DEBUG
                trap->SendServerCommand (ent->s.number, va ("print \"Placed mesh node at %s.\n\"", vtos (node.position)));
                #endif
                
                JKG_Array_Add (&nodeList, (void *)&node);
                
                reachedGroundOrEnd = qtrue;
                forceRevisualization = qtrue;
            }
        }
    }
    while ( !reachedGroundOrEnd );
}
//=========================================================
// JKG_Nav_Editor_Run
//---------------------------------------------------------
// Description:
// Performs per-frame actions for the navigation mesh such
// as generating the mesh and sending visual data back to
// the players.
//=========================================================
void JKG_Nav_Editor_Run ( void )
{
    JKG_Nav_Visualize();
}
//=========================================================
// JKG_Nav_Cmd_VisLabelsClient_f
//---------------------------------------------------------
// Description:
// Command to set the client who can see the navigation
// mesh visuals.
//=========================================================
void JKG_Nav_Cmd_VisLabelsClient_f ( gentity_t *ent )
{
    char buffer[MAX_TOKEN_CHARS] = { 0 };
    int clientId = -1;
    gentity_t *e = NULL;
    
    if ( !jkg_nav_edit.integer )
    {
        return;
    }
    
    if ( trap->Argc() < 2 )
    {
        trap->SendServerCommand (ent->s.number, "print \"Usage: nav_vislabels_client <client id>\n\"");
        return;
    }
    
    trap->Argv (1, buffer, sizeof (buffer));
    if ( buffer[0] < '0' || buffer[0] > '9' )
    {
        trap->SendServerCommand (ent->s.number, "print \"Invalid client ID.\n\"");
        return;
    }
    
    clientId = atoi (buffer);
    if ( clientId >= MAX_CLIENTS )
    {
        trap->SendServerCommand (ent->s.number, "print \"Invalid client ID.\n\"");
        return;
    }
    
    e = &g_entities[clientId];
    if ( !e->client || e->client->pers.connected != CON_CONNECTED )
    {
        trap->SendServerCommand (ent->s.number, "print \"Client is not connected.\n\"");
        return;
    }
    
    showLabelsToClient = clientId;
}
static void JKG_Nav_Node_Recurse ( meshNode_t *node )
{
	int i;
	trace_t tr;
	const float dropDistance = 65.0f;
	const int crouchHeight = CROUCH_MAXS_2 - DEFAULT_MINS_2;
	const int playerHeight = DEFAULT_MAXS_2 - DEFAULT_MINS_2;
	const vec2_t steps[NUM_DIRECTIONS] =
	{
	    { 0.0f,  GRID_SIZE }, // North
	    { GRID_SIZE,  0.0f }, // East
	    { 0.0f, -GRID_SIZE }, // South
	    { -GRID_SIZE, 0.0f }, // West
	};
	if ( node->visited )
	{
		return;
	}
    node->visited = qtrue;
	for ( i = 0; i < NUM_DIRECTIONS; i++ )
	{
	    int dir;
	    for ( dir = (int)NORTH; dir < (int)NUM_DIRECTIONS; dir++ )
	    {
		    if ( !(node->directionsVisited & dir) )
		    {
			    vec3_t start, end, drop;
			    int dropHeight;
			    
			    node->directionsVisited |= (1 << dir);
			    VectorCopy (node->position, start);
			    VectorAdd (node->position, steps[dir], end);
			    
			    // Check deadly drop and get new Z height for end position.
			    VectorCopy (end, drop);
			    drop[2] -= dropDistance; // Just drop a bit further than player height
                trap->Trace (&tr, end, NULL, NULL, drop, -1, CONTENTS_SOLID);
                
                dropHeight = (int)(tr.fraction * dropDistance);
                if ( dropHeight > playerHeight )
                {
                    continue;
                }
                
			    // Make sure the player can at least crouch
			    start[2] += crouchHeight;
			    end[2] += crouchHeight;
			    // Do trace, make sure we can reachend position.
    			
			    // All good, we add a new node to the list.
		    }
		}
	}
}
static void JKG_Nav_Create_Nodes ( void )
{
    //int i;
    if ( !jkg_nav_edit.integer || buildStage != GS_CREATING_NODES )
    {
        return;
    }
    
    
}
//=========================================================
// JKG_Nav_Generate
//---------------------------------------------------------
// Description:
// Performs more generation of the navigation mesh. Does
// nothing if a navigation mesh is not flagged to be
// generated.
//=========================================================
static void JKG_Nav_Generate ( void )
{
    if ( !jkg_nav_edit.integer )
    {
        return;
    }
    
    if ( buildStage == GS_NONE )
    {
        return;
    }
    
	while ( buildStage != GS_NONE )
	{
		switch ( buildStage )
		{
		case GS_CREATING_NODES:
			JKG_Nav_Create_Nodes ();
			buildStage = GS_CREATING_AREAS;
		break;
	    
		case GS_CREATING_AREAS:
			buildStage = GS_NONE;
		break;
	    
		default:
			trap->Print ("Unknown build stage of navigation mesh generation reached. Stopping generation...\n");
			buildStage = GS_NONE;
		break;
		}
	}
}
//=========================================================
// JKG_Nav_Visualize
//---------------------------------------------------------
// Description:
// Displays visually the navigation mesh to a single
// player.
//=========================================================
extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
static void JKG_Nav_Visualize ( void )
{
    static int nextDrawTime = 0;
    gentity_t *show = NULL;
    
    if ( !jkg_nav_edit.integer )
    {
        return;
    }
    
    show = &g_entities[showLabelsToClient];
    if ( !show->inuse || !show->client )
    {
        return;
    }
    
    if ( show->client->pers.connected != CON_CONNECTED )
    {
        return;
    }
    
    if ( forceRevisualization || nextDrawTime <= level.time )
    {
        int i;
        vec3_t lineEnd;
        vec3_t displacement;
        meshNode_t *nodeListData = (meshNode_t *)nodeList.data;
        meshNode_t *closestNode = NULL;
        float shortestDistance = 999999999.9f;
        float distance = 0.0f;
    
        forceRevisualization = qfalse;
        for ( i = 0; i < nodeList.size; i++ )
        {
            VectorMA (nodeListData[i].position, 20.0f, nodeListData[i].normal, lineEnd);
            G_TestLine (nodeListData[i].position, lineEnd, 0x000000FF, 5000);
            
            VectorSubtract (nodeListData[i].position, show->s.origin, displacement);
            
            distance = VectorLength (displacement);
            if ( distance < shortestDistance )
            {
                shortestDistance = distance;
                closestNode = &nodeListData[i];
            }
        }
        
        // TODO: do something with closest node. somehow display information about it to the player?
        
        nextDrawTime = level.time + 4000;
    }
}
#endif
