#include "tr_local.h"

#define __MODEL_OPTIMIZE_ACMR_METHOD__
//#define __MODEL_OPTIMIZE_MESHOPTIMIZE_METHOD__

//
// ACMR Indexes optimization...
//
#ifdef __MODEL_OPTIMIZE_ACMR_METHOD__
#include "meshOptimization/triListOpt.h"


//#define __DEBUG_INDEXES_OPTIMIZATION__


extern int getMilliCount();
extern int getMilliSpan(int nTimeStart);

unsigned const VBUF_SZ = 1024;// 32;

typedef struct
{
	unsigned ix;
	unsigned pos;
} vbuf_entry_t;

vbuf_entry_t vbuf[VBUF_SZ];

float calc_acmr(uint32_t numIndexes, uint32_t *indexes) 
{
	vbuf_entry_t vbuf[VBUF_SZ];
	unsigned num_cm(0); // cache misses

	for (unsigned i = 0; i < numIndexes; ++i) {
		bool found(0);
		unsigned best_entry(0), oldest_pos((unsigned)-1);

		for (unsigned n = 0; n < VBUF_SZ; ++n) {
			if (vbuf[n].ix == indexes[i]) { found = 1; break; }
			if (vbuf[n].pos < oldest_pos) { best_entry = n; oldest_pos = vbuf[n].pos; }
		}
		if (found) continue;
		vbuf[best_entry].ix = indexes[i];
		vbuf[best_entry].pos = i;
		++num_cm;
	}
	return float(num_cm) / float(numIndexes);
}

void R_VertexCacheOptimizeMeshIndexes(uint32_t *numVerts, uint32_t *numIndexes, uint32_t *indexes, vec3_t *verts)
{
	if (*numIndexes < 1.5* *numVerts || *numVerts < 2 * VBUF_SZ /*|| num_verts < 100000*/) return;
	float const orig_acmr(calc_acmr(*numIndexes, indexes)), perfect_acmr(float(*numVerts) / float(*numIndexes));
	if (orig_acmr < 1.05*perfect_acmr) return;

#ifdef __DEBUG_INDEXES_OPTIMIZATION__
	int start_time = getMilliCount();
#endif //__DEBUG_INDEXES_OPTIMIZATION__

	vector<unsigned> out_indices(*numIndexes);
	TriListOpt::OptimizeTriangleOrdering(*numVerts, *numIndexes, indexes, &out_indices.front());

	for (int i = 0; i < out_indices.size(); i++)
	{
		indexes[i] = out_indices[i];
	}

#ifdef __DEBUG_INDEXES_OPTIMIZATION__
	float optimized_acmr = calc_acmr(numIndexes, indexes);
	float optPercent = (1.0 - (optimized_acmr / orig_acmr)) * 100.0;
	int totalTime = getMilliSpan(start_time);
	ri->Printf(PRINT_WARNING, "R_OptimizeMesh: Original acmr: %f. Optimized acmr: %f. Perfect acmr: %f. Optimization Ratio: %f. Time: %i ms.\n", orig_acmr, optimized_acmr, perfect_acmr, optPercent, totalTime);
#endif //__DEBUG_INDEXES_OPTIMIZATION__
}
#endif //__MODEL_OPTIMIZE_ACMR_METHOD__

//
// Full Mesh Optimization...
//
#ifdef __MODEL_OPTIMIZE_MESHOPTIMIZE_METHOD__
#include "meshOptimization/meshoptimizer.h"

void R_OptimizeMeshFull(uint32_t *numVerts, uint32_t *numIndexes, uint32_t *indexes, vec3_t *verts)
{
	// Init out_indices
	std::vector<unsigned int> out_indices(*numIndexes); // allocate temporary memory for the remap table
	//meshopt_generateVertexRemap(&remap[0], NULL, index_count, &unindexed_vertices[0], index_count, sizeof(Vertex));
	size_t out_vertex_count = meshopt_generateVertexRemap(&out_indices[0], indexes, *numIndexes, verts, *numVerts, sizeof(vec3_t));

	// Init out_verts
	std::vector<vec3_t> out_verts(out_vertex_count); // allocate temporary memory for the remap table
	meshopt_remapIndexBuffer(indexes, indexes, *numIndexes, &out_indices[0]);
	meshopt_remapVertexBuffer(&out_verts[0], verts, *numVerts, sizeof(vec3_t), &out_indices[0]);

	// Vertex cache optimization
	meshopt_optimizeVertexCache(&out_indices[0], &out_indices[0], *numIndexes, out_vertex_count);

	// Overdraw optimization
	meshopt_optimizeOverdraw(&out_indices[0], &out_indices[0], *numIndexes, (const float *)&out_verts[0], out_vertex_count, sizeof(out_verts[0]), 1.05f);

	// Vertex fetch optimization
	meshopt_optimizeVertexFetch(&out_verts[0], &out_indices[0], *numIndexes, &out_verts[0], out_vertex_count, sizeof(out_verts[0]));

	for (uint32_t i = 0; i < out_indices.size(); i++)
	{
		indexes[i] = out_indices[i];
	}

	for (uint32_t i = 0; i < out_verts.size(); i++)
	{
		verts[i][0] = out_verts[i][0];
		verts[i][1] = out_verts[i][1];
		verts[i][2] = out_verts[i][2];
	}

	*numIndexes = out_indices.size();
	*numVerts = out_verts.size();
}
#endif //__MODEL_OPTIMIZE_MESHOPTIMIZE_METHOD__


//
// Mesh Optimization Runner...
//
void R_OptimizeMesh(uint32_t *numVerts, uint32_t *numIndexes, uint32_t *indexes, vec3_t *verts)
{
#ifdef __MODEL_OPTIMIZE_MESHOPTIMIZE_METHOD__
	if (verts != NULL)
	{// Full Mesh Optimization
		R_OptimizeMeshFull(numVerts, numIndexes, indexes, verts);
	}
	else
#endif //__MODEL_OPTIMIZE_MESHOPTIMIZE_METHOD__

#ifdef __MODEL_OPTIMIZE_ACMR_METHOD__
	{// ACMR Indexes optimization...
		R_VertexCacheOptimizeMeshIndexes(numVerts, numIndexes, indexes, verts);
	}
#else //__MODEL_OPTIMIZE_ACMR_METHOD__
	{// None...

	}
#endif //__MODEL_OPTIMIZE_ACMR_METHOD
}
