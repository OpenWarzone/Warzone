#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <vector>

#include "qcommon/qfiles.h"
#include "qcommon/q_shared.h"
#include "botlib/botlib.h"
#ifdef __USE_NAVMESH__
#if defined(_GAME)
#include "g_local.h"
#include "ai_dominance_main.h"
#else
#include "../cgame/cg_local.h"
#endif
#include "ai_dominance_navmesh.h"

#include "../Recast/Recast/Recast.h"
#include "../Recast/Detour/DetourNavMeshBuilder.h"

#include <unordered_set>
#include "../Recast/Recast/Recast.h"
#include "../Recast/Detour/DetourCommon.h"
#include "../Recast/Detour/DetourNavMesh.h"
#include "../Recast/Detour/DetourNavMeshBuilder.h"
#include "../Recast/Detour/DetourNavMeshQuery.h"
#if defined(_CGAME)
#include "../Recast/Detour/DebugDraw.h"
#include "../Recast/Detour/RecastDebugDraw.h"
#include "../Recast/Detour/RecastDump.h"
#include "../Recast/Detour/DetourDebugDraw.h"
#endif

using namespace shooter;

//#define __NAVMESH_OLD_METHOD__

#if defined(_CGAME)
extern qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y);

void CG_DrawDebugLine(const vec3_t from, const vec3_t to, vec4_t color)
{
	refEntity_t		re;

	memset(&re, 0, sizeof(re));

	re.reType = RT_LINE;
	re.radius = 1;

	re.shaderRGBA[0] = color[0] / 255.0;
	re.shaderRGBA[1] = color[1] / 255.0;
	re.shaderRGBA[2] = color[2] / 255.0;
	re.shaderRGBA[3] = color[3] / 255.0;

	re.customShader = cgs.media.whiteShader;

	VectorCopy(from, re.origin);
	//re.origin[2] += 16;
	VectorCopy(to, re.oldorigin);
	//re.oldorigin[2] += 16;

	AddRefEntityToScene(&re);
}

void CG_DrawDebugBox(const vec3_t bmin, const vec3_t bmax, vec4_t color)
{
	float x, y, x2, y2;
	vec4_t col;
	VectorSet4(col, color[0] / 255.0, color[1] / 255.0, color[2] / 255.0, color[3] / 255.0);
	CG_WorldCoordToScreenCoordFloat((float *)bmin, &x, &y);
	CG_WorldCoordToScreenCoordFloat((float *)bmax, &x2, &y2);
	CG_DrawRect(x, y, x2-x, y2-y, 1.0f, col);
}

void CG_DrawDebugTriangle(const vec3_t point1, const vec3_t point2, const vec3_t point3, vec4_t color)
{
	CG_DrawDebugLine(point1, point2, color);
	CG_DrawDebugLine(point2, point3, color);
	CG_DrawDebugLine(point3, point1, color);
}

void CG_DrawDebugPoint(const vec3_t point, vec4_t color)
{
	CG_DrawDebugLine(point, point, color);
}
#endif

int NavMesh_CountIndices(const dsurface_t *surfaces, int numSurfaces)
{
	int count = 0;
	for (int i = 0; i < numSurfaces; i++, surfaces++)
	{
		if (surfaces->surfaceType != MST_PLANAR && surfaces->surfaceType != MST_TRIANGLE_SOUP)
		{
			continue;
		}

		count += surfaces->numIndexes;
	}

	return count;
}

void NavMesh_LoadTriangles(const int *indexes, int* tris, const dsurface_t *surfaces, int numSurfaces)
{
	int t = 0;
	int v = 0;
	for (int i = 0; i < numSurfaces; i++, surfaces++)
	{
		if (surfaces->surfaceType != MST_PLANAR && surfaces->surfaceType != MST_TRIANGLE_SOUP)
		{
			continue;
		}

		for (int j = surfaces->firstIndex, k = 0; k < surfaces->numIndexes; j++, k++)
		{
			tris[t++] = v + indexes[j];
		}

		v += surfaces->numVerts;
	}
}

void NavMesh_LoadVertices(const drawVert_t *vertices, float* verts, const dsurface_t *surfaces, const int numSurfaces)
{
	int v = 0;
	for (int i = 0; i < numSurfaces; i++, surfaces++)
	{
		// will handle patches later...
		if (surfaces->surfaceType != MST_PLANAR && surfaces->surfaceType != MST_TRIANGLE_SOUP)
		{
			continue;
		}

		for (int j = surfaces->firstVert, k = 0; k < surfaces->numVerts; j++, k++)
		{
			verts[v++] = -vertices[j].xyz[0];
			verts[v++] = vertices[j].xyz[2];
			verts[v++] = -vertices[j].xyz[1];
		}
	}
}

namespace {

	inline bool inRange(const float* v1, const float* v2, const float r, const float h)
	{
		const float dx = v2[0] - v1[0];
		const float dy = v2[1] - v1[1];
		const float dz = v2[2] - v1[2];
		return (dx*dx + dz*dz) < r*r && fabsf(dy) < h;
	}

	inline void VelocityVerlet(float dt, float* pos, float* vel)
	{
		static const float acc[3] = { 0.f, -10.f, 0.f };

		// pos += dt * (vel + .5f * dt * acc);
		// vel += dt * acc;
		rcVmad(pos, pos, vel, dt);
		rcVmad(pos, pos, acc, .5f * dt * dt);
		rcVmad(vel, vel, acc, dt);
	}

	// Copied from RecastLayers.cpp to avoid dependencies
	inline bool overlapRange(const float amin, const float amax, const float bmin, const float bmax)
	{
		return (amin > bmax || amax < bmin) ? false : true;
	}

	// Copied from RecastDebugDraw.cpp to avoid dependencies
	void getContourCenter(const rcContour* cont, const float* orig, float cs, float ch, float* center)
	{
		center[0] = 0;
		center[1] = 0;
		center[2] = 0;
		if (!cont->nverts)
			return;
		for (int i = 0; i < cont->nverts; ++i)
		{
			const int* v = &cont->verts[i * 4];
			center[0] += (float)v[0];
			center[1] += (float)v[1];
			center[2] += (float)v[2];
		}
		const float s = 1.0f / cont->nverts;
		center[0] *= s * cs;
		center[1] *= s * ch;
		center[2] *= s * cs;
		center[0] += orig[0];
		center[1] += orig[1] + 4 * ch;
		center[2] += orig[2];
	}

	// Copied from RecastDebugDraw.cpp to avoid dependencies
	const rcContour* findContourFromSet(const rcContourSet& cset, unsigned short reg)
	{
		for (int i = 0; i < cset.nconts; ++i)
		{
			if (cset.conts[i].reg == reg)
				return &cset.conts[i];
		}
		return 0;
	}

#if defined(_CGAME)
	/// OpenGL debug draw implementation.
	class DebugDrawGL : public duDebugDraw
	{
	public:
		virtual void depthMask(bool state) {  }

		virtual void texture(bool state)
		{
			
		}

		virtual void begin(duDebugDrawPrimitives prim, float size = 1.0f)
		{
			switch (prim)
			{
			case DU_DRAW_POINTS:
				//glPointSize(size);
				//glBegin(GL_POINTS);
				break;
			case DU_DRAW_LINES:
				//glLineWidth(size);
				//glBegin(GL_LINES);
				break;
			case DU_DRAW_LINE_STRIP:
				//glLineWidth(size);
				//glBegin(GL_LINE_STRIP);
				break;
			case DU_DRAW_TRIS:
				//glBegin(GL_TRIANGLES);
				break;
			case DU_DRAW_QUADS:
				//glBegin(GL_QUADS);
				break;
			};
		}

		virtual void vertex(const float* pos, unsigned int color)
		{
			//glColor4ubv((GLubyte*)&color);
			//glVertex3fv(pos);
		}

		virtual void vertex(const float x, const float y, const float z, unsigned int color)
		{
			//glColor4ubv((GLubyte*)&color);
			//glVertex3f(x, y, z);
		}

		virtual void vertex(const float* pos, unsigned int color, const float* uv)
		{
			//glColor4ubv((GLubyte*)&color);
			//glTexCoord2fv(uv);
			//glVertex3fv(pos);
		}

		virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
		{
			//glColor4ubv((GLubyte*)&color);
			//glTexCoord2f(u, v);
			//glVertex3f(x, y, z);
		}

		virtual void end()
		{
			//glEnd();
			//glLineWidth(1.0f);
			//glPointSize(1.0f);
		}
	};
#endif
}


// based on Sample_SoloMesh::handleBuild()
NavMesh::NavMesh(
#ifdef __NAVMESH_OLD_METHOD__
	/*float agentHeight,
	float agentRadius,
	float agentMaxClimb,
	float agentWalkableSlopeAngle,
	float cellSize,
	float cellHeight,
	float maxEdgeLen,
	float maxEdgeError,
	float regionMinSize,
	float regionMergeSize,
	float detailSampleDist,
	float detailSampleMaxError,
	const float* verts,
	const float* normals,
	const int* tris,
	int ntris,
	const float* minBound,
	const float* maxBound,
	float maxJumpGroundRange,
	float maxJumpDistance,
	float initialJumpForwardSpeed,
	float initialJumpUpSpeed,
	float idealJumpPointsDist,
	float maxIntersectionPosHeight*/
#else
	const float* verts,
	const float* normals,
	const int* tris,
	int ntris,
	rcConfig in_cfg,
	float maxJumpGroundRange,
	float maxJumpDistance,
	float initialJumpForwardSpeed,
	float initialJumpUpSpeed,
	float idealJumpPointsDist,
	float maxIntersectionPosHeight
#endif
)
	: m_keepInterResults(true)
	, m_totalBuildTimeMs(0)
	, m_triareas(nullptr)
	, m_hf(nullptr)
	, m_chf(nullptr)
	, m_cset(nullptr)
	, m_pmesh(nullptr)
	, m_cfg(nullptr)
	, m_dmesh(nullptr)
	, m_navMesh(nullptr)
	, m_navQuery(nullptr)
	, m_filter(nullptr)
{
	//
	// Step 1. Initialize build config.
	//

	int nverts = ntris * 3;

#ifdef __NAVMESH_OLD_METHOD__
	m_cfg = new rcConfig();
	m_cfg->cs = cellSize;
	m_cfg->ch = cellHeight;
	m_cfg->walkableSlopeAngle = agentWalkableSlopeAngle;
	m_cfg->walkableHeight = (int)ceil(agentHeight / cellHeight);
	m_cfg->walkableClimb = (int)floor(agentMaxClimb / cellHeight);
	m_cfg->walkableRadius = (int)ceil(agentRadius / cellSize);
	m_cfg->maxEdgeLen = (int)(maxEdgeLen / cellSize);
	m_cfg->maxSimplificationError = maxEdgeError;
	m_cfg->minRegionArea = (int)rcSqr(regionMinSize);    // Note: area = size*size
	m_cfg->mergeRegionArea = (int)rcSqr(regionMergeSize);  // Note: area = size*size
	m_cfg->maxVertsPerPoly = DT_VERTS_PER_POLYGON;
	m_cfg->detailSampleDist = detailSampleDist < 0.9f ? 0 : cellSize * detailSampleDist;
	m_cfg->detailSampleMaxError = cellHeight * detailSampleMaxError;
#else
	//m_cfg = &in_cfg;
	m_cfg = new rcConfig();
	memcpy(m_cfg, (const void *)&in_cfg, sizeof(in_cfg));
#endif


	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
#ifdef __NAVMESH_OLD_METHOD__
	rcVcopy(m_cfg->bmin, minBound);
	rcVcopy(m_cfg->bmax, maxBound);
#endif
	rcCalcGridSize(m_cfg->bmin, m_cfg->bmax, m_cfg->cs, &m_cfg->width, &m_cfg->height);

	rcContext *m_ctx = new rcContext;

	// Reset build times gathering.
	m_ctx->resetTimers();

	// Start the build process.  
	m_ctx->startTimer(RC_TIMER_TOTAL);

	trap->Print("Building navigation:\n");
	trap->Print(" - %d x %d cells\n", m_cfg->width, m_cfg->height);
	trap->Print(" - %.1fK verts, %.1fK tris\n", nverts / 1000.0f, ntris / 1000.0f);

	//
	// Step 2. Rasterize input polygon soup.
	//

	// Allocate voxel heightfield where we rasterize our input data to.
	m_hf = rcAllocHeightfield();
	if (!m_hf)
	{
		trap->Print("buildNavigation: Out of memory 'solid'.\n");
		return;
	}
	if (!rcCreateHeightfield(m_ctx, *m_hf, m_cfg->width, m_cfg->height, m_cfg->bmin, m_cfg->bmax, m_cfg->cs, m_cfg->ch))
	{
		trap->Print("buildNavigation: Could not create solid heightfield.\n");
		return;
	}

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	m_triareas = new unsigned char[ntris];
	if (!m_triareas)
	{
		trap->Print("buildNavigation: Out of memory 'm_triareas' (%d).\n", ntris);
		return;
	}

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(m_triareas, 0, ntris * sizeof(unsigned char));
	rcMarkWalkableTriangles(m_ctx, m_cfg->walkableSlopeAngle, verts, nverts, tris, ntris, m_triareas);
	rcRasterizeTriangles(m_ctx, verts, nverts, tris, m_triareas, ntris, *m_hf, m_cfg->walkableClimb);

	delete[] verts;
	delete[] tris;
	//delete[] m_normals;
	nverts = 0;
	ntris = 0;

	if (!m_keepInterResults)
	{
		delete[] m_triareas;
		m_triareas = 0;
	}

	//
	// Step 3. Filter walkables surfaces.
	//

	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg->walkableClimb, *m_hf);
	rcFilterLedgeSpans(m_ctx, m_cfg->walkableHeight, m_cfg->walkableClimb, *m_hf);
	rcFilterWalkableLowHeightSpans(m_ctx, m_cfg->walkableHeight, *m_hf);


	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	m_chf = rcAllocCompactHeightfield();
	if (!m_chf)
	{
		trap->Print("buildNavigation: Out of memory 'chf'.\n");
		return;
	}
	if (!rcBuildCompactHeightfield(m_ctx, m_cfg->walkableHeight, m_cfg->walkableClimb, *m_hf, *m_chf))
	{
		trap->Print("buildNavigation: Could not build compact data.\n");
		return;
	}

	if (!m_keepInterResults)
	{
		rcFreeHeightField(m_hf);
		m_hf = 0;
	}

	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(m_ctx, m_cfg->walkableRadius, *m_chf))
	{
		trap->Print("buildNavigation: Could not erode.\n");
		return;
	}

	/*
	// (Optional) Mark areas.
	const ConvexVolume* vols = m_geom->getConvexVolumes();
	for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
	rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);
	*/

	// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
	// There are 3 martitioning methods, each with some pros and cons:
	// 1) Watershed partitioning
	//   - the classic Recast partitioning
	//   - creates the nicest tessellation
	//   - usually slowest
	//   - partitions the heightfield into nice regions without holes or overlaps
	//   - the are some corner cases where this method creates produces holes and overlaps
	//      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
	//      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
	//   * generally the best choice if you precompute the nacmesh, use this if you have large open areas
	// 2) Monotone partioning
	//   - fastest
	//   - partitions the heightfield into regions without holes and overlaps (guaranteed)
	//   - creates long thin polygons, which sometimes causes paths with detours
	//   * use this if you want fast navmesh generation
	// 3) Layer partitoining
	//   - quite fast
	//   - partitions the heighfield into non-overlapping regions
	//   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
	//   - produces better triangles than monotone partitioning
	//   - does not have the corner cases of watershed partitioning
	//   - can be slow and create a bit ugly tessellation (still better than monotone)
	//     if you have large open areas with small obstacles (not a problem if you use tiles)
	//   * good choice to use for tiled navmesh with medium and small sized tiles

	// Watershed partitioning

	// Prepare for region partitioning, by calculating distance field along the walkable surface.
	if (!rcBuildDistanceField(m_ctx, *m_chf))
	{
		trap->Print("buildNavigation: Could not build distance field.\n");
		return;
	}

	// Partition the walkable surface into simple regions without holes.
	if (!rcBuildRegions(m_ctx, *m_chf, 0, m_cfg->minRegionArea, m_cfg->mergeRegionArea))
	{
		trap->Print("buildNavigation: Could not build watershed regions.\n");
		return;
	}


	//
	// Step 5. Trace and simplify region contours.
	//

	// Create contours.
	m_cset = rcAllocContourSet();
	if (!m_cset)
	{
		trap->Print("buildNavigation: Out of memory 'cset'.\n");
		return;
	}
	if (!rcBuildContours(m_ctx, *m_chf, m_cfg->maxSimplificationError, m_cfg->maxEdgeLen, *m_cset))
	{
		trap->Print("buildNavigation: Could not create contours.\n");
		return;
	}

	//
	// Step 6. Build polygons mesh from contours.
	//

	// Build polygon navmesh from the contours.
	m_pmesh = rcAllocPolyMesh();
	if (!m_pmesh)
	{
		trap->Print("buildNavigation: Out of memory 'pmesh'.\n");
		return;
	}
	if (!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg->maxVertsPerPoly, *m_pmesh))
	{
		trap->Print("buildNavigation: Could not triangulate contours.\n");
		return;
	}

	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//

	m_dmesh = rcAllocPolyMeshDetail();
	if (!m_dmesh)
	{
		trap->Print("buildNavigation: Out of memory 'pmdtl'.\n");
		return;
	}

	if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf, m_cfg->detailSampleDist, m_cfg->detailSampleMaxError, *m_dmesh))
	{
		trap->Print("buildNavigation: Could not build detail mesh.\n");
		return;
	}

	// Build the Jump Down OffMesh connections
	BuildJumpConnections(
#ifdef __NAVMESH_OLD_METHOD__
		agentHeight,
		agentRadius,
#else
		m_cfg->walkableHeight,
		m_cfg->walkableRadius,
#endif
		maxJumpGroundRange,
		maxJumpDistance,
		initialJumpForwardSpeed,
		initialJumpUpSpeed,
		idealJumpPointsDist);

	if (!m_keepInterResults)
	{
		rcFreeCompactHeightfield(m_chf);
		m_chf = 0;
		rcFreeContourSet(m_cset);
		m_cset = 0;
	}

	// At this point the navigation mesh data is ready, you can access it from m_pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//

	// Update poly flags from areas.
	for (int i = 0; i < m_pmesh->npolys; ++i)
	{
		if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
			m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;

		if (m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND)
		{
			m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
		}
		else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER)
		{
			m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
		}
	}

	dtNavMeshCreateParams params;
	memset(&params, 0, sizeof(params));
	params.verts = m_pmesh->verts;
	params.vertCount = m_pmesh->nverts;
	params.polys = m_pmesh->polys;
	params.polyAreas = m_pmesh->areas;
	params.polyFlags = m_pmesh->flags;
	params.polyCount = m_pmesh->npolys;
	params.nvp = m_pmesh->nvp;
	params.detailMeshes = m_dmesh->meshes;
	params.detailVerts = m_dmesh->verts;
	params.detailVertsCount = m_dmesh->nverts;
	params.detailTris = m_dmesh->tris;
	params.detailTriCount = m_dmesh->ntris;

	params.offMeshConVerts = &m_OffMeshConVerts[0];
	params.offMeshConRad = &m_OffMeshConRad[0];
	params.offMeshConDir = &m_OffMeshConDir[0];
	params.offMeshConAreas = &m_OffMeshConAreas[0];
	params.offMeshConFlags = &m_OffMeshConFlags[0];
	params.offMeshConUserID = &m_OffMeshConUserID[0];
	params.offMeshConCount = m_OffMeshConVerts.size() / 6;

#ifdef __NAVMESH_OLD_METHOD__
	params.walkableHeight = agentHeight;
	params.walkableRadius = agentRadius;
	params.walkableClimb = agentMaxClimb;
#else
	params.walkableHeight = m_cfg->walkableHeight;
	params.walkableRadius = m_cfg->walkableRadius;
	params.walkableClimb = m_cfg->walkableClimb;
#endif
	rcVcopy(params.bmin, m_pmesh->bmin);
	rcVcopy(params.bmax, m_pmesh->bmax);
	params.cs = m_cfg->cs;
	params.ch = m_cfg->ch;
	params.buildBvTree = true;

	unsigned char* navData = 0;
	int navDataSize = 0;
	if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
	{
		trap->Print("Could not build Detour navmesh.\n");
		return;
	}

	m_navMesh = dtAllocNavMesh();
	if (!m_navMesh)
	{
		dtFree(navData);
		trap->Print("Could not create Detour navmesh.\n");
		return;
	}

	dtStatus status;

	status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
	if (dtStatusFailed(status))
	{
		dtFree(navData);
		trap->Print("Could not init Detour navmesh.\n");
		return;
	}

	m_navQuery = dtAllocNavMeshQuery();
	if (!m_navQuery)
	{
		dtFree(m_navQuery);
		trap->Print("Could not create Detour navmesh query.\n");
		return;
	}

	status = m_navQuery->init(m_navMesh, 2048);
	if (dtStatusFailed(status))
	{
		trap->Print("Could not init Detour navmesh query.\n");
		return;
	}

	m_filter = new dtQueryFilter;
	if (m_filter)
	{
		m_filter->setAreaCost(SAMPLE_POLYAREA_GROUND, 1.0f);
		m_filter->setAreaCost(SAMPLE_POLYAREA_WATER, 10.0f);
		m_filter->setAreaCost(SAMPLE_POLYAREA_JUMP, 1.5f);
		m_filter->setIncludeFlags(SAMPLE_POLYFLAGS_ALL ^ SAMPLE_POLYFLAGS_DISABLED);
		m_filter->setExcludeFlags(0);
	}

	CalcIntersectionPositions(maxIntersectionPosHeight);

	m_ctx->stopTimer(RC_TIMER_TOTAL);

	// Show performance stats.
	//duLogBuildTimes(*m_ctx, m_ctx->getAccumulatedTime(RC_TIMER_TOTAL));
	trap->Print(">> Polymesh: %d vertices  %d polygons.\n", m_pmesh->nverts, m_pmesh->npolys);

	m_totalBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL) / 1000.0f;

	trap->Print("Navmesh built in %f ms.\n", m_totalBuildTimeMs);
}

shooter::NavMesh::~NavMesh()
{
	delete m_filter;
	dtFreeNavMeshQuery(m_navQuery);
	dtFreeNavMesh(m_navMesh);
	rcFreePolyMeshDetail(m_dmesh);
	rcFreePolyMesh(m_pmesh);
	rcFreeContourSet(m_cset);
	rcFreeCompactHeightfield(m_chf);
	rcFreeHeightField(m_hf);
	delete[] m_triareas;
	delete m_cfg;
}

#if defined(_CGAME)
void duDebugDrawPolyMeshDetail2(duDebugDraw* dd, const struct rcPolyMeshDetail& dmesh)
{
	if (!dd) return;

	//dd->begin(DU_DRAW_TRIS);

	for (int i = 0; i < dmesh.nmeshes; ++i)
	{
		const unsigned int* m = &dmesh.meshes[i * 4];
		const unsigned int bverts = m[0];
		const unsigned int btris = m[2];
		const int ntris = (int)m[3];
		const float* verts = &dmesh.verts[bverts * 3];
		const unsigned char* tris = &dmesh.tris[btris * 4];

		unsigned int color = duIntToCol(i, 192);

		for (int j = 0; j < ntris; ++j)
		{
			//dd->vertex(&verts[tris[j * 4 + 0] * 3], color);
			//dd->vertex(&verts[tris[j * 4 + 1] * 3], color);
			//dd->vertex(&verts[tris[j * 4 + 2] * 3], color);
			CG_DrawDebugTriangle(&verts[tris[j * 4 + 0] * 3], &verts[tris[j * 4 + 1] * 3], &verts[tris[j * 4 + 2] * 3], vec4_t{ (float)i / (float)dmesh.nmeshes, (float)i / (float)dmesh.nmeshes, (float)i / (float)dmesh.nmeshes, 192 });
		}
	}
	//dd->end();

	// Internal edges.
	//dd->begin(DU_DRAW_LINES, 1.0f);
	const unsigned int coli = duRGBA(0, 0, 0, 64);
	for (int i = 0; i < dmesh.nmeshes; ++i)
	{
		const unsigned int* m = &dmesh.meshes[i * 4];
		const unsigned int bverts = m[0];
		const unsigned int btris = m[2];
		const int ntris = (int)m[3];
		const float* verts = &dmesh.verts[bverts * 3];
		const unsigned char* tris = &dmesh.tris[btris * 4];

		for (int j = 0; j < ntris; ++j)
		{
			const unsigned char* t = &tris[j * 4];
			for (int k = 0, kp = 2; k < 3; kp = k++)
			{
				unsigned char ef = (t[3] >> (kp * 2)) & 0x3;
				if (ef == 0)
				{
					// Internal edge
					if (t[kp] < t[k])
					{
						//dd->vertex(&verts[t[kp] * 3], coli);
						//dd->vertex(&verts[t[k] * 3], coli);

						//CG_DrawDebugTriangle(&verts[t[kp] * 3], &verts[t[kp] * 3], &verts[t[kp] * 3], vec4_t{ 0, 0, 0, 64 });
						CG_DrawDebugLine(&verts[t[kp] * 3], &verts[t[k] * 3], vec4_t{ 0, 0, 0, 64 });
					}
				}
			}
		}
	}
	//dd->end();

	// External edges.
	//dd->begin(DU_DRAW_LINES, 2.0f);
	const unsigned int cole = duRGBA(0, 0, 0, 64);
	for (int i = 0; i < dmesh.nmeshes; ++i)
	{
		const unsigned int* m = &dmesh.meshes[i * 4];
		const unsigned int bverts = m[0];
		const unsigned int btris = m[2];
		const int ntris = (int)m[3];
		const float* verts = &dmesh.verts[bverts * 3];
		const unsigned char* tris = &dmesh.tris[btris * 4];

		for (int j = 0; j < ntris; ++j)
		{
			const unsigned char* t = &tris[j * 4];
			for (int k = 0, kp = 2; k < 3; kp = k++)
			{
				unsigned char ef = (t[3] >> (kp * 2)) & 0x3;
				if (ef != 0)
				{
					// Ext edge
					//dd->vertex(&verts[t[kp] * 3], cole);
					//dd->vertex(&verts[t[k] * 3], cole);
					CG_DrawDebugLine(&verts[t[kp] * 3], &verts[t[k] * 3], vec4_t{ 0, 0, 0, 64 });
				}
			}
		}
	}
	//dd->end();

	//dd->begin(DU_DRAW_POINTS, 3.0f);
	const unsigned int colv = duRGBA(0, 0, 0, 64);
	for (int i = 0; i < dmesh.nmeshes; ++i)
	{
		const unsigned int* m = &dmesh.meshes[i * 4];
		const unsigned int bverts = m[0];
		const int nverts = (int)m[1];
		const float* verts = &dmesh.verts[bverts * 3];
		for (int j = 0; j < nverts; ++j)
			//dd->vertex(&verts[j * 3], colv);
			CG_DrawDebugLine(&verts[j * 3], &verts[j * 3], vec4_t{ 0, 0, 0, 64 });
	}
	//dd->end();
}

void NavMesh::DebugRender() const
{
	DebugDrawGL dd;

	// Draw bounds
	const float* bmin = m_cfg->bmin;
	const float* bmax = m_cfg->bmax;

	//duDebugDrawBoxWire(&dd, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], duRGBA(255, 255, 255, 128), 1.0f);
	CG_DrawDebugBox(bmin, bmax, vec4_t{ 255, 255, 255, 128 });

	//dd.begin(DU_DRAW_POINTS, 5.0f);
	//dd.vertex(bmin[0], bmin[1], bmin[2], duRGBA(255, 255, 255, 128));
	//dd.end();
	CG_DrawDebugPoint(bmin, vec4_t{ 255, 255, 255, 128 });

	// Draw mesh
	//duDebugDrawHeightfieldSolid(&dd, *m_hf);
	//duDebugDrawHeightfieldWalkable(&dd, *m_hf);
	//duDebugDrawCompactHeightfieldSolid(&dd, *m_chf);
	//duDebugDrawCompactHeightfieldDistance(&dd, *m_chf);
	//duDebugDrawRawContours(&dd, *m_cset, 0.5f);
	//duDebugDrawContours(&dd, *m_cset);
	//duDebugDrawRegionConnections(&dd, *m_cset);
	//duDebugDrawPolyMesh(&dd, *m_pmesh);
	duDebugDrawPolyMeshDetail2(&dd, *m_dmesh);
	//duDebugDrawNavMesh(&dd, *m_navMesh, 0);
	//duDebugDrawNavMeshNodes(&dd, *m_navQuery);

	for (const auto& verts : m_DebugOffMeshConVerts)
	{
		//dd.begin(DU_DRAW_LINE_STRIP, 1.f);
		for (unsigned i = 0; i < verts.size(); i += 3)
		{
			//dd.vertex(verts[i], verts[i + 1], verts[i + 2], duRGBA(255, 0, 0, 255));
			CG_DrawDebugTriangle(&verts[i], &verts[i + 1], &verts[i + 2], vec4_t{ 255, 0, 0, 255 });
		}
		//dd.end();

		//dd.begin(DU_DRAW_POINTS, 2.f);
		for (unsigned i = 0; i < verts.size(); i += 3)
		{
			//dd.vertex(verts[i], verts[i + 1], verts[i + 2], duRGBA(0, 255, 0, 255));
			CG_DrawDebugTriangle(&verts[i], &verts[i + 1], &verts[i + 2], vec4_t{ 0, 255, 0, 255 });
		}
		//dd.end();
	}

	//dd.begin(DU_DRAW_POINTS, 5.f);
	for (unsigned i = 0; i < m_IntersectionPositions.size(); i += 3)
	{
		//dd.vertex(m_IntersectionPositions[i], m_IntersectionPositions[i + 1], m_IntersectionPositions[i + 2], duRGBA(0, 255, 0, 255));
		CG_DrawDebugTriangle(&m_IntersectionPositions[i], &m_IntersectionPositions[i + 1], &m_IntersectionPositions[i + 2], vec4_t{ 0, 255, 0, 255 });
	}
	//dd.end();
}
#endif

bool NavMesh::FindPath(
	const float* startPos,
	const float* endPos,
	float* outPathStartPos,
	float* outPathEndPos,
	dtPolyRef* outPathPolys,
	int& outNrPathPolys) const
{
	if (!m_navMesh)
	{
		//trap->Print("ERROR: No navmesh.\n");
		return false;
	}

	dtPolyRef startRef = cInvalidPolyRef;
	dtPolyRef endRef = cInvalidPolyRef;
	float polyPickExt[3] = { 8192.f, 16550.f, 8192.f }; //{ 2500.f, 5000.f, 2500.f }; //{ 2.f, 4.f, 2.f };

	//trap->Print("Find path between %f %f %f and %f %f %f.\n", startPos[0], startPos[1], startPos[2], endPos[0], endPos[1], endPos[2]);

	m_navQuery->findNearestPoly(startPos, polyPickExt, m_filter, &startRef, outPathStartPos);
	m_navQuery->findNearestPoly(endPos, polyPickExt, m_filter, &endRef, outPathEndPos);

	if (startRef && endRef)
	{
		return m_navQuery->findPath(startRef, endRef, outPathStartPos, outPathEndPos, m_filter, outPathPolys, &outNrPathPolys, MAX_POLYS) == DT_SUCCESS;
	}

	return false;
}

void NavMesh::GetSteerPosOnPath(
	const float* startPos,
	const float* endPos,
	const dtPolyRef* visited,
	int visitedSize,
	dtPolyRef* inoutPath,
	int& inoutPathSize,
	float minTargetDist,
	float* outSteerPos,
	bool& outOffMeshConn,
	bool& outEndOfPath) const
{
	if (!m_navMesh)
	{
		return;
	}

	if (!inoutPathSize)
	{
		//trap->Print("!inoutPathSize\n");
		return;
	}

	if (visited)
	{
		inoutPathSize = FixupCorridor(inoutPath, inoutPathSize, MAX_POLYS, visited, visitedSize);
		inoutPathSize = FixupShortcuts(inoutPath, inoutPathSize, m_navQuery);
	}

	// Find steer target.
	static const int MAX_STEER_POINTS = 3;
	float steerPath[MAX_STEER_POINTS * 3];
	unsigned char steerPathFlags[MAX_STEER_POINTS];
	dtPolyRef steerPathPolys[MAX_STEER_POINTS];
	int nsteerPath = 0;

	m_navQuery->findStraightPath(startPos, endPos, inoutPath, inoutPathSize,
		steerPath, steerPathFlags, steerPathPolys, &nsteerPath, MAX_STEER_POINTS);

	if (!nsteerPath) 
	{ 
		//trap->Print("!nsteerPath");
		return; 
	}

	// Find vertex far enough to steer to.
	int ns = 0;
	while (ns < nsteerPath)
	{
		// Stop at Off-Mesh link or when end of path or when point is further than slop away.
		if ((steerPathFlags[ns] & (DT_STRAIGHTPATH_OFFMESH_CONNECTION | DT_STRAIGHTPATH_END)) ||
			!inRange(&steerPath[ns * 3], startPos, minTargetDist, 1.0f))
			break;
		ns++;
	}
	// Failed to find good point to steer to.
	if (ns >= nsteerPath) 
	{ 
		//trap->Print("ns >= nsteerPath");
		return; 
	}

	dtVcopy(outSteerPos, &steerPath[ns * 3]);
	outSteerPos[1] = startPos[1];

	unsigned char steerFlags = steerPathFlags[ns];
	dtPolyRef steerPoly = steerPathPolys[ns];

	if (Distance(startPos, outSteerPos) < minTargetDist)//inRange(startPos, outSteerPos, minTargetDist, 1.0f))
	{
		//trap->Print("In range\n");
		if (steerFlags & DT_STRAIGHTPATH_END)
		{
			// Reached end of path.
			inoutPathSize--;
			dtVcopy(outSteerPos, endPos);
			outEndOfPath = true;
			trap->Print("In range - DT_STRAIGHTPATH_END\n");
		}
		else if (steerFlags & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
		{
			assert(steerPoly == inoutPath[1]);

			float offMeshStartPos[3] = { 0 }, offMeshEndPos[3] = { 0 };
			dtStatus status = m_navMesh->getOffMeshConnectionPolyEndPoints(inoutPath[0], inoutPath[1], offMeshStartPos, offMeshEndPos);
			assert(status == DT_SUCCESS);

			// Advance the path over the off-mesh connection.
			inoutPathSize -= 2;
			for (int i = 0; i < inoutPathSize; ++i)
				inoutPath[i] = inoutPath[i + 2];

			dtVcopy(outSteerPos, offMeshEndPos);
			outOffMeshConn = true;
			trap->Print("In range - DT_STRAIGHTPATH_OFFMESH_CONNECTION\n");
		}
		else if (steerFlags & DT_STRAIGHTPATH_START)
		{
			//inoutPathSize--;
			//dtVcopy(outSteerPos, endPos);
			//outEndOfPath = true;
			inoutPathSize -= 1;
			for (int i = 0; i < inoutPathSize; ++i)
				inoutPath[i] = inoutPath[i + 1];
			//dtVcopy(outSteerPos, endPos);
			trap->Print("In range - DT_STRAIGHTPATH_START\n");
		}
		else
		{
			//trap->Print("In range - DT_UNKNOWN\n");
		}
	}
	//else
	//	trap->Print("NOT in range\n");
}

bool NavMesh::GetFloorInfo(
	const float* pt,
	const float hrange,
	float& outY,
	float& outDistY,
	bool* outWalkable,
	float* outBorderDistance) const
{
	const int ix = (int)floorf((pt[0] - m_chf->bmin[0]) / m_chf->cs);
	const int iz = (int)floorf((pt[2] - m_chf->bmin[2]) / m_chf->cs);

	if (ix < 0 || iz < 0 || ix >= m_chf->width || iz >= m_chf->height)
		return false;

	bool found = false;
	int foundIx = -1;
	float foundY = FLT_MAX;
	float foundDistY = FLT_MAX;

	const rcCompactCell& c = m_chf->cells[ix + iz * m_chf->width];
	for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
	{
		const rcCompactSpan& s = m_chf->spans[i];
		const float y = m_chf->bmin[1] + s.y * m_chf->ch;
		const float dist = abs(pt[1] - y);
		if (dist < hrange && dist < foundDistY)
		{
			found = true;
			foundIx = i;
			foundY = y;
			foundDistY = dist;
		}
	}

	if (found)
	{
		outY = foundY;
		outDistY = foundDistY;

		bool walkable = (m_chf->areas[foundIx] != RC_NULL_AREA);

		if (outWalkable)
		{
			*outWalkable = walkable;
		}

		if (walkable && outBorderDistance)
		{
			*outBorderDistance = 1.f * m_chf->dist[foundIx] / m_chf->maxDistance;
		}
	}

	return found;
}

bool NavMesh::CheckCollision(
	const float* pos1,
	const float* pos2,
	const float height,
	const float range) const
{
	const int w = m_hf->width;
	const int h = m_hf->height;
	const float cs = m_hf->cs;
	const float ch = m_hf->ch;
	const float* orig = m_hf->bmin;

	float ptMin[3] = { pos1[0], pos1[1], pos1[2] };
	float ptMax[3] = { pos2[0], pos2[1], pos2[2] };

	for (unsigned i = 0; i < 3; i++)
		if (ptMin[i] > ptMax[i])
			rcSwap(ptMin[i], ptMax[i]);

	const float ymin = ptMin[1];
	const float ymax = ptMax[1] + height;
	const int ix0 = rcClamp((int)floorf((ptMin[0] - range - orig[0]) / cs), 0, w - 1);
	const int ix1 = rcClamp((int)floorf((ptMax[0] + range - orig[0]) / cs), 0, w - 1);
	const int iz0 = rcClamp((int)floorf((ptMin[2] - range - orig[2]) / cs), 0, h - 1);
	const int iz1 = rcClamp((int)floorf((ptMax[2] + range - orig[2]) / cs), 0, h - 1);

	for (int z = iz0; z <= iz1; ++z)
	{
		for (int x = ix0; x <= ix1; ++x)
		{
			const rcSpan* s = m_hf->spans[x + z*w];
			if (!s) continue;

			while (s)
			{
				const float symin = orig[1] + s->smin*ch;
				const float symax = orig[1] + s->smax*ch;
				if (overlapRange(ymin, ymax, symin, symax))
					return true;
				s = s->next;
			}
		}
	}

	return false;
}

int NavMesh::FixupCorridor(dtPolyRef* path, const int npath, const int maxPath,
	const dtPolyRef* visited, const int nvisited)
{
	int furthestPath = -1;
	int furthestVisited = -1;

	// Find furthest common polygon.
	for (int i = npath - 1; i >= 0; --i)
	{
		bool found = false;
		for (int j = nvisited - 1; j >= 0; --j)
		{
			if (path[i] == visited[j])
			{
				furthestPath = i;
				furthestVisited = j;
				found = true;
			}
		}
		if (found)
			break;
	}

	// If no intersection found just return current path. 
	if (furthestPath == -1 || furthestVisited == -1)
		return npath;

	// Concatenate paths.  

	// Adjust beginning of the buffer to include the visited.
	const int req = nvisited - furthestVisited;
	const int orig = rcMin(furthestPath + 1, npath);
	int size = rcMax(0, npath - orig);
	if (req + size > maxPath)
		size = maxPath - req;
	if (size)
		memmove(path + req, path + orig, size * sizeof(dtPolyRef));

	// Store visited
	for (int i = 0; i < req; ++i)
		path[i] = visited[(nvisited - 1) - i];

	return req + size;
}

// This function checks if the path has a small U-turn, that is,
// a polygon further in the path is adjacent to the first polygon
// in the path. If that happens, a shortcut is taken.
// This can happen if the target (T) location is at tile boundary,
// and we're (S) approaching it parallel to the tile edge.
// The choice at the vertex can be arbitrary, 
//  +---+---+
//  |:::|:::|
//  +-S-+-T-+
//  |:::|   | <-- the step can end up in here, resulting U-turn path.
//  +---+---+
int NavMesh::FixupShortcuts(dtPolyRef* path, int npath, const dtNavMeshQuery* navQuery)
{
	if (npath < 3)
		return npath;

	// Get connected polygons
	static const int maxNeis = 16;
	dtPolyRef neis[maxNeis];
	int nneis = 0;

	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;
	if (dtStatusFailed(navQuery->getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
		return npath;

	for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
	{
		const dtLink* link = &tile->links[k];
		if (link->ref != 0)
		{
			if (nneis < maxNeis)
				neis[nneis++] = link->ref;
		}
	}

	// If any of the neighbour polygons is within the next few polygons
	// in the path, short cut to that polygon directly.
	static const int maxLookAhead = 6;
	int cut = 0;
	for (int i = dtMin(maxLookAhead, npath) - 1; i > 1 && cut == 0; i--) {
		for (int j = 0; j < nneis; j++)
		{
			if (path[i] == neis[j]) {
				cut = i;
				break;
			}
		}
	}
	if (cut > 1)
	{
		int offset = cut - 1;
		npath -= offset;
		for (int i = 1; i < npath; i++)
			path[i] = path[i + offset];
	}

	return npath;
}

void NavMesh::BuildJumpConnections(
	float agentHeight,
	float agentRadius,
	float maxGroundRange,
	float maxJumpDownDistance,
	float initialForwardSpeed,
	float initialUpSpeed,
	float idealJumpPointsDist)
{
	static const float up[3] = { 0.f, 1.f, 0.f };

	const rcPolyMesh& mesh = *m_pmesh;
	const int nvp = mesh.nvp;
	const float cs = mesh.cs;
	const float ch = mesh.ch;
	const float* orig = mesh.bmin;

	// for all navmesh border segments
	for (int i = 0; i < mesh.npolys; ++i)
	{
		const unsigned short* p = &mesh.polys[i*nvp * 2];
		for (int j = 0; j < nvp; ++j)
		{
			if (p[j] == RC_MESH_NULL_IDX) break;
			if ((p[nvp + j] & 0x8000) == 0) continue;
			const int nj = (j + 1 >= nvp || p[j + 1] == RC_MESH_NULL_IDX) ? 0 : j + 1;
			const int vi[2] = { p[j], p[nj] };

			const unsigned short* v1 = &mesh.verts[vi[0] * 3];
			float pt1[3] = {
				orig[0] + v1[0] * cs,
				orig[1] + v1[1] * ch + ch,
				orig[2] + v1[2] * cs
			};

			const unsigned short* v2 = &mesh.verts[vi[1] * 3];
			float pt2[3] = {
				orig[0] + v2[0] * cs,
				orig[1] + v2[1] * ch + ch,
				orig[2] + v2[2] * cs
			};

			float segDir[3] = { 0 };
			rcVsub(segDir, pt2, pt1);

			float segLen = dtVlen(segDir);

			// rotate segDir 90 degrees and normalize
			float normal[3] = { -segDir[2], 0.f, segDir[0] };
			rcVnormalize(normal);

			float vel[3] = { 0 }; // velocity vector
			rcVmad(vel, vel, normal, initialForwardSpeed);
			rcVmad(vel, vel, up, initialUpSpeed);

			int nrJumpPoints = (int)round(segLen / idealJumpPointsDist);
			if (!nrJumpPoints) continue;

			// try to make off-mesh connections at equally distant points long the segment
			float jumpPointDist = segLen / nrJumpPoints;
			for (float t = jumpPointDist; t < segLen; t += jumpPointDist)
			{
				float ptJump[3] = { 0 };
				rcVmad(ptJump, pt1, segDir, t / segLen);

				float ptLand[3] = { 0 };
				if (CheckOffMeshLink(agentHeight, agentRadius, ptJump, vel, maxJumpDownDistance, ptLand))
				{
					m_OffMeshConUserID.push_back(1000 + m_OffMeshConVerts.size() / 6);

					m_OffMeshConVerts.push_back(ptJump[0]);
					m_OffMeshConVerts.push_back(ptJump[1]);
					m_OffMeshConVerts.push_back(ptJump[2]);

					m_OffMeshConVerts.push_back(ptLand[0]);
					m_OffMeshConVerts.push_back(ptLand[1]);
					m_OffMeshConVerts.push_back(ptLand[2]);
				}
			}
		}
	}

	unsigned nbOffMeshConVerts = m_OffMeshConVerts.size() / 6;
	m_OffMeshConRad.assign(nbOffMeshConVerts, 0.8f);
	m_OffMeshConFlags.assign(nbOffMeshConVerts, SAMPLE_POLYFLAGS_JUMP);
	m_OffMeshConAreas.assign(nbOffMeshConVerts, SAMPLE_POLYAREA_JUMP);
	m_OffMeshConDir.assign(nbOffMeshConVerts, 0);
}

bool NavMesh::CheckOffMeshLink(
	float agentHeight,
	float agentRadius,
	const float* origPos,
	const float* origVel,
	float maxHeight,
	float* outLinkPt)
{
	static const float cSimulationStep = 0.016f;
	float cellDiagSq = 2 * m_hf->cs * m_hf->cs + m_hf->ch * m_hf->ch;
	float startY = origPos[1];
	float pos[3], vel[3], lastPos[3];
	rcVcopy(pos, origPos);
	rcVcopy(vel, origVel);
	int cnt = 0;

	m_DebugOffMeshConVerts.push_back(std::vector<float>());

	bool walkable = false;
	float floorDist = FLT_MAX, floorY = FLT_MAX;

	do
	{
		rcVcopy(lastPos, pos);

		m_DebugOffMeshConVerts.back().push_back(pos[0]);
		m_DebugOffMeshConVerts.back().push_back(pos[1]);
		m_DebugOffMeshConVerts.back().push_back(pos[2]);

		// Optimization: It's quite expensive to call checkCollision for all the sampled points 
		// along the jump down curve, so we do the physics simulation with a fixed 16 ms step
		// and only check the collision if we traveled a distance more then the heightfield cell's diagonal
		do
		{
			VelocityVerlet(cSimulationStep, pos, vel);
		} while (rcVdistSqr(lastPos, pos) < cellDiagSq);

		if (CheckCollision(lastPos, pos, agentHeight, agentRadius))
		{
			bool hasFloorPos = GetFloorInfo(pos, 1.0f, floorY, floorDist, &walkable);
			if (!hasFloorPos || (hasFloorPos && pos[1] < floorY))
			{
				break;
			}
		}

		cnt++;
	} while (cnt < 100);

	float prevFloorDist = FLT_MAX, prevFloorY = FLT_MAX;
	if (GetFloorInfo(lastPos, 1.0f, prevFloorY, prevFloorDist, &walkable)
		&& walkable
		&& (startY - prevFloorY > 1.f)
		&& (startY - prevFloorY < maxHeight))
	{
		float t = prevFloorDist / (prevFloorDist + floorDist);
		dtVlerp(pos, lastPos, pos, t);

		rcVcopy(outLinkPt, pos);

		m_DebugOffMeshConVerts.back().push_back(pos[0]);
		m_DebugOffMeshConVerts.back().push_back(pos[1]);
		m_DebugOffMeshConVerts.back().push_back(pos[2]);

		return true;
	}

	m_DebugOffMeshConVerts.pop_back();

	return false;
}

void NavMesh::CalcIntersectionPositions(float maxIntersectionPosHeight)
{
	const float* orig = m_cset->bmin;
	const float cs = m_cset->cs;
	const float ch = m_cset->ch;

	for (int i = 0; i < m_cset->nconts; ++i)
	{
		const rcContour* cont = &m_cset->conts[i];

		std::unordered_set<unsigned short> regs;
		for (int j = 0; j < cont->nverts; ++j)
		{
			const int* v = &cont->verts[j * 4];
			unsigned short reg = (unsigned short)v[3];
			if ((reg == 0) || (reg == cont->reg)) continue;

			regs.insert(reg);
		}

		if (regs.size() == 1 || regs.size() > 2)
		{
			float pos[3];
			getContourCenter(cont, orig, cs, ch, pos);

			// Make sure we don't end up on the roof tops
			if (pos[1] < maxIntersectionPosHeight)
			{
				m_IntersectionPositions.push_back(pos[0]);
				m_IntersectionPositions.push_back(pos[1]);
				m_IntersectionPositions.push_back(pos[2]);
			}
		}
	}

	trap->Print("Created %i intersection positions. nconts %i.\n", m_IntersectionPositions.size(), m_cset->nconts);
}

#include <memory>
#include <iostream>
#include <fstream>
#include <algorithm>

std::unique_ptr<NavMesh> mNavMesh; ///< Navigation Mesh

qboolean NavMesh_LoadMapGeometry(const char *buffer, vec3_t mapmins, vec3_t mapmaxs, float* &verts, int &numverts, int* &tris, int &numtris)
{
	const dheader_t *header = (const dheader_t *)buffer;
	if (header->ident != BSP_IDENT)
	{
		trap->Print(va("Expected ident '%d', found %d.\n", BSP_IDENT, header->ident));
		return qfalse;
	}

	if (header->version != BSP_VERSION)
	{
		trap->Print(va("Expected version '%d', found %d.\n", BSP_VERSION, header->version));
		return qfalse;
	}

	// Load indices
	const int *indexes = (const int *)(buffer + header->lumps[LUMP_DRAWINDEXES].fileofs);
	const dsurface_t *surfaces = (const dsurface_t *)(buffer + header->lumps[LUMP_SURFACES].fileofs);
	int numSurfaces = header->lumps[LUMP_SURFACES].filelen / sizeof(dsurface_t);

	numtris = NavMesh_CountIndices(surfaces, numSurfaces);
	tris = new int[numtris];
	numtris /= 3;

	NavMesh_LoadTriangles(indexes, tris, surfaces, numSurfaces);

	// Load vertices
	const drawVert_t *vertices = (const drawVert_t *)(buffer + header->lumps[LUMP_DRAWVERTS].fileofs);
	numverts = header->lumps[LUMP_DRAWVERTS].filelen / sizeof(drawVert_t);

	verts = new float[3 * numverts];
	NavMesh_LoadVertices(vertices, verts, surfaces, numSurfaces);

	// Get map bounds. First model is always the entire map
	const dmodel_t *models = (const dmodel_t *)(buffer + header->lumps[LUMP_MODELS].fileofs);
	mapmins[0] = -models[0].maxs[0];
	mapmins[1] = models[0].mins[2];
	mapmins[2] = -models[0].maxs[1];

	mapmaxs[0] = -models[0].mins[0];
	mapmaxs[1] = models[0].maxs[2];
	mapmaxs[2] = -models[0].mins[1];

	trap->Print("Map mins %f %f %f. maxs %f %f %f.\n", mapmins[0], mapmins[2], mapmins[1], mapmaxs[0], mapmaxs[2], mapmaxs[1]);

	return qtrue;
}

void NavMesh_CreateNavMesh(const char *mapname)
{
	fileHandle_t f = 0;
	int fileLength = trap->FS_Open(mapname, &f, FS_READ);
	if (fileLength == -1 || !f)
	{
		trap->Print(va("Unable to open '%s' to create the navigation mesh.\n", mapname));
		return;
	}

	char *buffer = new char[fileLength + 1];
	trap->FS_Read(buffer, fileLength, f);
	buffer[fileLength] = '\0';
	trap->FS_Close(f);

	vec3_t mapmins;
	vec3_t mapmaxs;
	float *verts = NULL;
	int numverts;
	int *tris = NULL;
	int numtris;

	if (!NavMesh_LoadMapGeometry(buffer, mapmins, mapmaxs, verts, numverts, tris, numtris))
	{
		trap->Print(va("Unable to load map geometry from '%s'.\n", mapname));
		return;
	}

	delete[] buffer;

	/*
	float* m_verts = new float[numverts];
	for (int i = 0; i < numverts; i++)
	{
		m_verts[i] = verts[i];
	}
	*/

#if 0
	// Calculate normals.
	float* m_normals = new float[numtris/* * 3*/];
	for (int i = 0; i < numtris/* * 3*/; i += 3)
	{
		const float* v0 = &verts[tris[i] * 3];
		const float* v1 = &verts[tris[i + 1] * 3];
		const float* v2 = &verts[tris[i + 2] * 3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = v1[j] - v0[j];
			e1[j] = v2[j] - v0[j];
		}
		float* n = &m_normals[i];
		n[0] = e0[1] * e1[2] - e0[2] * e1[1];
		n[1] = e0[2] * e1[0] - e0[0] * e1[2];
		n[2] = e0[0] * e1[1] - e0[1] * e1[0];
		float d = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (d > 0)
		{
			d = 1.0f / d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}
	}
#endif

	/*
	float agentHeight,
	float agentRadius,
	float agentMaxClimb,
	float agentWalkableSlopeAngle,
	float cellSize,
	float cellHeight,
	float maxEdgeLen,
	float maxEdgeError,
	float regionMinSize,
	float regionMergeSize,
	float detailSampleDist,
	float detailSampleMaxError,
	const float* verts,
	const float* normals,
	const int* tris,
	int ntris,
	const float* minBound,
	const float* maxBound,
	float maxJumpGroundRange,
	float maxJumpDistance,
	float initialJumpForwardSpeed,
	float initialJumpUpSpeed,
	float idealJumpPointsDist,
	float maxIntersectionPosHeight
	*/

	/*
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
	*/
#ifdef __NAVMESH_OLD_METHOD__
	vec3_t size;
	VectorSet(size, mapmaxs[0] - mapmins[0], mapmaxs[1] - mapmins[1], mapmaxs[2] - mapmins[2]);
	float largest = max(size[0], size[1]);
	largest = max(size[2], largest);
	float useSize = largest / 512.0;// 2500.0;

	mNavMesh.reset(
		new NavMesh(
			/*agentHeight*/					64.0f / (useSize * 8.0),//1.8f,
			/*agentRadius*/					22.0f / useSize,//.8f,
			/*agentMaxClimb*/				STEPSIZE / (useSize * 8.0),//.5f,
			/*agentWalkableSlopeAngle*/		45.0f,//60.f,
			/*cellSize*/					useSize,//32.0f,//.25f,
			/*cellHeight*/					useSize * 8.0,// 256.0f,//.05f,
			/*maxEdgeLen*/					useSize * 10.0,// / 384.0f,//10.f, 
			/*maxEdgeError*/				.8f,
			/*regionMinSize*/				22.0f,//64.0f,//256.0f,//8.f, 
			/*regionMergeSize*/				48.0f,//400.0f,//400.0f,//20.f,
			/*detailSampleDist*/			8.f * useSize,//64.0f,//256.0f,//8.f, 
			/*detailSampleMaxError*/		0.9f * (useSize * 8.0),
			/*verts*/						verts,
			/*normals*/						NULL,//m_normals,
			/*tris*/						tris,
			/*ntris*/						numtris,// / 3,
			/*minBound*/					(const float *)&mapmins,
			/*maxBound*/					(const float *)&mapmaxs,
			/*maxJumpGroundRange*/			6.f,//512.0f,//6.f, 
			/*maxJumpDistance*/				10.f,//512.0f,//10.f, 
			/*initialJumpForwardSpeed*/		3.f,//512.0f,//3.f, 
			/*initialJumpUpSpeed*/			4.f,//512.0f,//4.f, 
			/*idealJumpPointsDist*/			9.f,//512.0f,//.9f, 
			/*maxIntersectionPosHeight*/	mapmaxs[1] - 128.0//18.f
		));
#else
	vec3_t bmin, bmax;

	VectorCopy(mapmins, bmin);
	VectorCopy(mapmaxs, bmax);

	const float MaxSimplificationError = 1.3f;

	const float MinRegionSize = 8;
	const float MergeRegionSize = 20;

	const int vertsPerPoly = 6;

	const float DetailSampleDist = 8.0;
	const float DetailSampleMaxError = 0.9;

	const float TileSize = (1600.0 / 3.0) * 60;// 533.0f + (1.0f / 3.0f);
	const int VoxelCount = 1778;
	const float CellSize = TileSize / (float)VoxelCount;
	const float CellHeight = (TileSize / (float)VoxelCount) / 5.0;

	const float WorldUnitWalkableHeight = 64.0 / CellHeight;
	const float WorldUnitWalkableRadius = 16.0 / CellSize;
	const float WalkableSlopeAngle = 45.0;
	const float WorldUnitWalkableClimb = STEPSIZE / CellHeight;

	rcConfig m_cfg;
	// Init build configuration from GUI
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = CellSize;
	m_cfg.ch = CellHeight;
	m_cfg.walkableSlopeAngle = WalkableSlopeAngle;
	m_cfg.walkableHeight = (int)ceilf(WorldUnitWalkableHeight / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(WorldUnitWalkableClimb / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(WorldUnitWalkableRadius / m_cfg.cs);
	m_cfg.maxEdgeLen = 8; //(int)MaxEdgeLen;
	m_cfg.maxSimplificationError = MaxSimplificationError;
	m_cfg.minRegionArea = (int)rcSqr(MinRegionSize);		// Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(MergeRegionSize);	// Note: area = size*size
	m_cfg.maxVertsPerPoly = (int)vertsPerPoly;
	m_cfg.tileSize = (int)TileSize;
	m_cfg.borderSize = m_cfg.walkableRadius + 3; // Reserve enough padding.
	m_cfg.width = m_cfg.tileSize;// + m_cfg.borderSize*2;
	m_cfg.height = m_cfg.tileSize;// + m_cfg.borderSize*2;
	m_cfg.detailSampleDist = DetailSampleDist;
	m_cfg.detailSampleMaxError = DetailSampleMaxError;

	/* Xyc's settings */
	m_cfg.ch = 64.0f;// 64.0f;// 9.0f;// 3.0f;
	m_cfg.cs = 384.0;// 512.0f;// 128.0;// 45.0f;// 15.0f;
	m_cfg.walkableSlopeAngle = 45.0f; // worked out from MIN_WALK_NORMAL - i think it's correct? :x
	m_cfg.walkableHeight = 64 / m_cfg.ch;
	m_cfg.walkableClimb = STEPSIZE / m_cfg.ch;
	m_cfg.walkableRadius = 15 / m_cfg.cs;
	m_cfg.maxEdgeLen = 12 / m_cfg.cs;
	m_cfg.maxSimplificationError = 1.3f;
	m_cfg.minRegionArea = 64;
	m_cfg.mergeRegionArea = 400;
	m_cfg.maxVertsPerPoly = 6;
	m_cfg.detailSampleDist = 6.0f * m_cfg.cs;
	m_cfg.detailSampleMaxError = 1.0f * m_cfg.ch;

	trap->Print("Cell size: %0.2f\n", m_cfg.cs);
	trap->Print("Cell height: %0.2f\n", m_cfg.ch);
	trap->Print("Walkable slope angle : %0.2f\n", m_cfg.walkableSlopeAngle);
	trap->Print("Walkable height: %d\n", m_cfg.walkableHeight);
	trap->Print("Walkable climb: %d\n", m_cfg.walkableClimb);
	trap->Print("Walkable radius: %d\n", m_cfg.walkableRadius);
	trap->Print("Max edge length : %d\n", m_cfg.maxEdgeLen);
	trap->Print("Tile size: %d\n", m_cfg.tileSize);
	trap->Print("Border size: %d\n", m_cfg.borderSize);
	trap->Print("Height: %d\n", m_cfg.height);
	trap->Print("Width: %d\n", m_cfg.width);

	trap->Print("Max edge length float: %0.2f\n", (WorldUnitWalkableRadius / CellSize) * 8);
	trap->Print("%f %f\n", WorldUnitWalkableRadius, CellSize);

	rcVcopy(m_cfg.bmin, bmin);
	rcVcopy(m_cfg.bmax, bmax);

	// pad bounds with a border
	m_cfg.bmin[0] -= m_cfg.borderSize*m_cfg.cs;
	m_cfg.bmin[2] -= m_cfg.borderSize*m_cfg.cs;
	m_cfg.bmax[0] += m_cfg.borderSize*m_cfg.cs;
	m_cfg.bmax[2] += m_cfg.borderSize*m_cfg.cs;

	//trap->Print("Generating mesh(%d, %d) ", tx, ty);

	trap->Print(" 3x3 bounds...\n");
	trap->Print("  min: {%0.2f, %0.2f, %0.2f}\n", bmin[0], bmin[1], bmin[2]);
	trap->Print("  max: {%0.2f, %0.2f, %0.2f}\n", bmax[0], bmax[1], bmax[2]);

	mNavMesh.reset(
		new NavMesh(verts,
			NULL,//normals,
			tris,
			numtris, 
			m_cfg,
			/*maxJumpGroundRange*/			6.f,//512.0f,//6.f, 
			/*maxJumpDistance*/				10.f,//512.0f,//10.f, 
			/*initialJumpForwardSpeed*/		3.f,//512.0f,//3.f, 
			/*initialJumpUpSpeed*/			4.f,//512.0f,//4.f, 
			/*idealJumpPointsDist*/			9.f,//512.0f,//.9f, 
			/*maxIntersectionPosHeight*/	mapmaxs[1] - 128.0//18.f),
		));
#endif

	//delete[] verts;
	//delete[] tris;
	////delete[] m_normals;
}

void Warzone_Nav_CreateNavMesh(void)
{
#if defined(_GAME)
	char			mapname[128] = { 0 };
	vmCvar_t		cvmapname;
	trap->Cvar_Register(&cvmapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);
	sprintf(mapname, "maps/%s.bsp", cvmapname.string);
#else //!defined(_GAME)
	char mapname[128] = { 0 };
	sprintf(mapname, "maps/%s.bsp", cgs.currentmapname);
#endif //!defined(_GAME)

	trap->Print("Creating navigation mesh...\n");
	NavMesh_CreateNavMesh(mapname);
}

#if defined(_CGAME)
void Warzone_Nav_VisualizeNavMesh(void)
{
	if (nav_render.integer && mNavMesh && mNavMesh.get())
		mNavMesh.get()->DebugRender();
}
#endif

#if defined(_GAME)
extern qboolean NPC_IsAlive (gentity_t *self, gentity_t *NPC );
extern qboolean UQ1_UcmdMoveForDir(gentity_t *self, usercmd_t *cmd, vec3_t dir, qboolean walk, vec3_t dest);
extern qboolean NPC_FacePosition(gentity_t *aiEnt, vec3_t position, qboolean doPitch);

void Warzone_Nav_UpdateEntity(gentity_t *ent)
{
	int state = ent->NPC->behaviorState;
	CompNavMeshPath *patrol = &ent->patrol;
	CompNavMeshPos *navMeshPos = &ent->navMeshPos;
	vec3_t pos;
	vec3_t front;

	if (!mNavMesh) return;

	VectorCopy(ent->r.currentOrigin, pos);
	VectorCopy(ent->r.currentAngles, front);

	VectorSet(pos, pos[0], pos[2], pos[1]);
	VectorSet(front, front[0], front[2], front[1]);

	AngleVectors(front, front, NULL, NULL);
	front[2] = 0;
	VectorNormalize(front);

	/*
	BS_DEFAULT = 0,//# default behavior for that NPC
	BS_ADVANCE_FIGHT,//# Advance to captureGoal and shoot enemies if you can
	BS_SLEEP,//# Play awake script when startled by sound
	BS_FOLLOW_LEADER,//# Follow your leader and shoot any enemies you come across
	BS_JUMP,//# Face navgoal and jump to it.
	BS_SEARCH,//# Using current waypoint as a base, search the immediate branches of waypoints for enemies
	BS_WANDER,//# Wander down random waypoint paths
	BS_NOCLIP,//# Moves through walls, etc.
	BS_REMOVE,//# Waits for player to leave PVS then removes itself
	BS_CINEMATIC,//# Does nothing but face it's angles and move to a goal if it has one
	//# #eol
	//internal bStates only
	BS_WAIT,//# Does nothing but face it's angles
	BS_STAND_GUARD,
	BS_PATROL,
	BS_INVESTIGATE,//# head towards temp goal and look for enemies and listen for sounds
	BS_STAND_AND_SHOOT,
	BS_HUNT_AND_KILL,
	BS_FLEE,//# Run away!
	NUM_BSTATES
	*/

	if (!patrol->nrPathPolys)
	{
		// Calculate a new path to a random position on the NavMesh
		vec3_t pathEnd;
		
		if (/*(state & BS_HUNT_AND_KILL) 
			&&*/ ent->enemy 
			&& NPC_IsAlive(ent, ent->enemy)
			&& ent->enemy == ent->NPC->goalEntity)
		{
			VectorCopy(ent->enemy->r.currentOrigin, pathEnd);
			VectorSet(pathEnd, pathEnd[0], pathEnd[2], pathEnd[1]);
		}
		else
		{
			const std::vector<float>& patrolPos = mNavMesh->GetIntersectionPositions();
			int posIx = rand() % (patrolPos.size() / 3);
			VectorCopy(&patrolPos[posIx * 3], pathEnd);
			//trap->Print("patrolPos.size() %i. pathEnd %f %f %f.\n", (int)patrolPos.size(), pathEnd[0], pathEnd[1], pathEnd[2]);
		}

		
		if (!mNavMesh->FindPath(pos, pathEnd, patrol->pathStartPos, patrol->pathEndPos, patrol->pathPolys, patrol->nrPathPolys))
		{
			//trap->Print("Failed to find a path.\n");
			ent->bot_strafe_jump_timer = 0;
			return;
		}
		

		//VectorSet(movable.velocity, 0.f, 0.f, 128.0/*irand(cMinPatrolVelZ, cMaxPatrolVelZ)*/);
	}

	// Update the steering position
	vec3_t steerPos;
	bool offMeshConn = false, endOfPath = false;
	mNavMesh->GetSteerPosOnPath(pos, patrol->pathEndPos, navMeshPos->visitedPolys, navMeshPos->nrPolys, patrol->pathPolys, patrol->nrPathPolys, 48.0f/*0.1f*/, steerPos, offMeshConn, endOfPath);

	VectorSet(steerPos, steerPos[0], steerPos[2], steerPos[1]);
	VectorSet(pos, pos[0], pos[2], pos[1]);

	vec3_t steerDir;
	VectorSubtract(steerPos, pos, steerDir);
	//steerDir[2] = 0.f; // should always be on the XZ plane
	//steerDir = VectorNormalize(steerDir, front);
	//VectorCopy(VectorNormalize(steerDir), front);

	bool doJump = false;

	VectorNormalize(steerDir);
	if (steerDir[2] > 0.5) doJump = true;
	steerDir[2] = 0;
	VectorCopy(steerDir, ent->movedir);
	VectorCopy(ent->movedir, ent->client->ps.moveDir);

	/*if (!patrol->nrPathPolys)
	{
		ent->bot_strafe_jump_timer = 0;
		return;
	}*/

	if (VectorLength(patrol->pathEndPos) == 0)
	{
		ent->bot_strafe_jump_timer = 0;
		patrol->nrPathPolys = 0;
		return;
	}

	/*if (steerPos[2] > pos[2] + 256.0)
	{
		patrol->nrPathPolys = 0;
		ent->bot_strafe_jump_timer = 0;
		return;
	}*/

	//if (steerPos[2] > pos[2] + 18) doJump = true;

	// Update orientation
	if (offMeshConn)
	{
		VectorCopy(steerDir, front);
	}
	else if (Distance(front, steerDir) > FLT_EPSILON)
	{
		//RotateYFixedStep(front, steerDir);
		VectorCopy(steerDir, front);
		//steerDir[0] *= -1.0;
		//steerDir[1] *= -1.0;
		//steerDir[2] *= -1.0;
	}

	// Update velocity
	if (doJump)
	{
		trap->Print("Jump!\n");
		ent->bot_strafe_jump_timer = level.time + 100.0;
	}
	else if (offMeshConn)
	{
		//movable.velocity = cJumpVel;
		trap->Print("Jump2!\n");
		ent->bot_strafe_jump_timer = level.time + 100.0;
	}
	else if (endOfPath)
	{
		//movable.velocity = vec3();
		ent->bot_strafe_jump_timer = 0;
		patrol->nrPathPolys = 0;
		return;
	}

	//m_OffMeshConFlags.assign(nbOffMeshConVerts, SAMPLE_POLYFLAGS_JUMP);
	//m_OffMeshConAreas.assign(nbOffMeshConVerts, SAMPLE_POLYAREA_JUMP);
	

	/*
	trap->Print("steerDir %f %f %f. steerPos %f %f %f. aiPos %f %f %f. endPos %f %f %f\n"
		, steerDir[0], steerDir[1], steerDir[2]
		, steerPos[0], steerPos[1], steerPos[2]
		, pos[0], pos[1], pos[2]
		, patrol->pathEndPos[0], patrol->pathEndPos[1], patrol->pathEndPos[2]);
	trap->Print("Goal distance %f.\n", Distance(pos, patrol->pathEndPos));
	*/

	NPC_FacePosition(ent, steerPos, qfalse);
	UQ1_UcmdMoveForDir(ent, &ent->client->pers.cmd, steerDir, qfalse, steerPos);
}
#endif //defined(_GAME)
#endif //__USE_NAVMESH__
