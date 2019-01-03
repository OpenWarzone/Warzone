#pragma once

#include "qcommon/q_shared.h"

#ifdef __USE_NAVMESH__

#ifndef NAV_MESH_HPP
#define NAV_MESH_HPP

#include <vector>

#include "../Recast/Recast/Recast.h" // for dtPolyRef
#include "../Recast/Detour/DetourNavMesh.h" // for dtPolyRef

struct rcHeightfield;
struct rcCompactHeightfield;
struct rcContourSet;
struct rcPolyMesh;
struct rcConfig;
struct rcPolyMeshDetail;
class dtNavMesh;
class dtNavMeshQuery;
class dtQueryFilter;

namespace shooter
{

	/// These are just sample areas to use consistent values across the samples.
	/// The use should specify these base on his needs.
	enum SamplePolyAreas
	{
		SAMPLE_POLYAREA_GROUND,
		SAMPLE_POLYAREA_WATER,
		SAMPLE_POLYAREA_JUMP,
	};

	enum SamplePolyFlags
	{
		SAMPLE_POLYFLAGS_WALK = 0x01,  // Ability to walk (ground, grass, road)
		SAMPLE_POLYFLAGS_SWIM = 0x02,  // Ability to swim (water).
		SAMPLE_POLYFLAGS_JUMP = 0x08,  // Ability to jump.
		SAMPLE_POLYFLAGS_DISABLED = 0x10, // Disabled polygon
		SAMPLE_POLYFLAGS_ALL = 0xffff // All abilities.
	};

	/// Wrapper class around Recast&Detour functionality. Contains the navigation mesh data 
	/// and all the functionality needed to calculate walkable/jumpable paths and move an agent along these paths. 
	/// Also calculates map intersection positions which can be used place revived dead agents. 
	/// It doesn't have any dependencies with glm to keep consistent with Recast & Detour interface
	/// and be easily used in other projects.
	class NavMesh
	{
	public:

		/// Initializes all recast & detour member structures, 
		/// creates OffMesh links for all positions where the agent can Jump Down
		/// and calculates the node intersections positions (where the agent can be revived).
		/// Most parameters are recast & detour specific config variables in world units (see rcConfig). 
		NavMesh(
#if 0
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
			float maxJumpGroundRange, ///< Maximum ground range of the jump down OffMesh links
			float maxJumpDistance, ///< Maximum forward distance of the jump down OffMesh links
			float initialJumpForwardSpeed, ///< Initial jump down forward speed
			float initialJumpUpSpeed, ///< Initial jump down upward speed
			float idealJumpPointsDist, ///< Ideal distance between jump down points on a NavMesh edge
			float maxIntersectionPosHeight); ///< Maximum height of the intersection positions
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
			);
#endif

											 /// Destructor
		~NavMesh();

		/// Accessors
		const dtNavMeshQuery* GetNavMeshQuery() const { return m_navQuery; }
		const dtQueryFilter* GetQueryFilter() const { return m_filter; }
		const std::vector<float>& GetIntersectionPositions() const { return m_IntersectionPositions; }

		/// Debug Rendering function. 
		/// Render the NavMesh, OffMesh connections and the intersection positions. 
#if defined(_CGAME)
		void DebugRender() const;
#endif

		/// Find a walkable/jumpable path between 2 positions on the NavMesh
		bool FindPath(
			const float* startPos, ///< Path's start position
			const float* endPos, ///< Path's end position
			float* outPathStartPos, ///< Path's start position on the navigation mesh
			float* outPathEndPos, ///< Path's end position on the navigation mesh
			dtPolyRef* outPathPolys, ///< Path's polygons refs
			int& outNrPathPolys ///< Number of polygon refs in the path
		) const;

		/// Find the next steering position and removes the polygon refs in inoutVisitedPolys from inoutPathPolys.
		void GetSteerPosOnPath(
			const float* startPos, ///< Current position
			const float* endPos, ///< Path's end position
			const dtPolyRef* visited, ///< recently visited polygons
			int visitedSize, ///< Number of recently visited polygons
			dtPolyRef* inoutPath, ///< Path's polygons
			int& inoutPathSize, ///< Number of path's polygons
			float minTargetDist, ///< A target is considered reached if outSteerPos is within minTargetDist from it
			float* outSteerPos, ///< 3D Steering position
			bool& outOffMeshConn, ///< true if outSteerPos is within minTargetDist from an Offmesh connection
			bool& outEndOfPath ///< true if outSteerPos is within minTargetDist from endPathPos
		) const;

		/// Return the floor information for a world position.
		bool GetFloorInfo(
			const float* pos, ///< Position in world coordinates
			const float hrange,  ///< Height Range
			float& outY, ///< Floor's Y world coordinate
			float& outDistY, ///< Vertical distance to the floor
			bool* outWalkable = nullptr, ///< True if pos's Floor projection is walkable
			float* outBorderDistance = nullptr ///< Distance between the pos's Floor projection and NavMesh border
		) const;

	private:
		static const int MAX_POLYS = 256;
		static const dtPolyRef cInvalidPolyRef = 0;

		/// Check if the volume inside a rectangular prism of height and range intersects the heightfield
		/// when moving it from pos1 to pos2.
		bool CheckCollision(
			const float* pos1,
			const float* pos2,
			const float height,
			const float range
		) const;

		/// Based on Detour example
		static int FixupCorridor(
			dtPolyRef* path,
			const int npath,
			const int maxPath,
			const dtPolyRef* visited,
			const int nvisited);

		/// Based on Detour example
		static int FixupShortcuts(
			dtPolyRef* path,
			int npath,
			const dtNavMeshQuery* navQuery);

		/// Build the jump down OffMesh connections.
		void BuildJumpConnections(
			float agentHeight,
			float agentRadius,
			float maxJumpGroundRange,
			float maxJumpDistance,
			float initialJumpForwardSpeed,
			float initialJumpUpSpeed,
			float idealJumpPointsDist);


		/// Check if one OffMesh connection collides with the map.
		bool CheckOffMeshLink(
			float agentHeight,
			float agentRadius,
			const float* origPos,
			const float* origVel,
			float maxHeight,
			float* outLinkPt);

		/// Calculate the position of the intersection nodes in the map. A node is considered an intersection if 
		/// it has 1 or more then 2 neighbour nodes.
		void CalcIntersectionPositions(
			float maxIntersectionPosHeight);

		unsigned char* m_triareas;
		rcHeightfield* m_hf;
		rcCompactHeightfield* m_chf;
		rcContourSet* m_cset;
		rcPolyMesh* m_pmesh;
		rcConfig* m_cfg;
		rcPolyMeshDetail* m_dmesh;
		dtNavMesh* m_navMesh;
		dtNavMeshQuery* m_navQuery;
		dtQueryFilter* m_filter;

		/// OffMesh connections info.

		std::vector<float> m_OffMeshConVerts;
		std::vector<float> m_OffMeshConRad;
		std::vector<unsigned short> m_OffMeshConFlags;
		std::vector<unsigned char> m_OffMeshConAreas;
		std::vector<unsigned char> m_OffMeshConDir;
		std::vector<unsigned int> m_OffMeshConUserID;

		std::vector<std::vector<float> > m_DebugOffMeshConVerts; ///< Polylines describing the OffMesh connections used for debug rendering

		std::vector<float> m_IntersectionPositions; ///< Map intersection positions

		bool m_keepInterResults;
		float m_totalBuildTimeMs;
	};

}

struct CompNavMeshPos
{
	static const uint32_t cMaxPolys = 16;

	CompNavMeshPos()
		: poly(0)
		, nrPolys(0)
	{
		std::fill_n(visitedPolys, cMaxPolys, 0);
	}

	dtPolyRef poly;
	dtPolyRef visitedPolys[cMaxPolys];
	int nrPolys;
};

struct CompNavMeshPath
{
	static const int MAX_POLYS = 256;

	CompNavMeshPath()
		: nrPathPolys(0)
	{}

	vec3_t pathStartPos; ///< Start position on the NavMesh
	vec3_t pathEndPos; ///< End position on the NavMesh
	dtPolyRef pathPolys[MAX_POLYS]; ///< Path's polygon Ids
	int nrPathPolys; ///< Number of polygons in the path
};

#endif //NAV_MESH_HPP

#endif //__USE_NAVMESH__
