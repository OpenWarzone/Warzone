/*
 * Copyright (c) 2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Intel Corporation nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */



//#define __SOFTWARE_OCCLUSION__
//#define __THREADED_OCCLUSION__


#pragma once

/*!
 *  \file MaskedOcclusionCulling.h
 *  \brief Masked Occlusion Culling
 * 
 *  General information
 *   - Input to all API functions are (x,y,w) clip-space coordinates (x positive left, y positive up, w positive away from camera).
 *     We entirely skip the z component and instead compute it as 1 / w, see next bullet. For TestRect the input is NDC (x/w, y/w).
 *   - We use a simple z = 1 / w transform, which is a bit faster than OGL/DX depth transforms. Thus, depth is REVERSED and z = 0 at
 *     the far plane and z = inf at w = 0. We also have to use a GREATER depth function, which explains why all the conservative
 *     tests will be reversed compared to what you might be used to (for example zMaxTri >= zMinBuffer is a visibility test)
 *   - We support different layouts for vertex data (basic AoS and SoA), but note that it's beneficial to store the position data
 *     as tightly in memory as possible to reduce cache misses. Big strides are bad, so it's beneficial to keep position as a separate
 *     stream (rather than bundled with attributes) or to keep a copy of the position data for the occlusion culling system.
 *   - The resolution width must be a multiple of 8 and height a multiple of 4.
 *   - The hierarchical Z buffer is stored OpenGL-style with the y axis pointing up. This includes the scissor box.
 *   - This code is only tested with Visual Studio 2015, but should hopefully be easy to port to other compilers.
 */


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Defines used to configure the implementation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef QUICK_MASK
/*!
 * Configure the algorithm used for updating and merging hierarchical z buffer entries. If QUICK_MASK
 * is defined to 1, use the algorithm from the paper "Masked Software Occlusion Culling", which has good
 * balance between performance and low leakage. If QUICK_MASK is defined to 0, use the algorithm from
 * "Masked Depth Culling for Graphics Hardware" which has less leakage, but also lower performance.
 */
#define QUICK_MASK			1

#endif

#ifndef USE_D3D
/*!
 * Configures the library for use with Direct3D (default) or OpenGL rendering. This changes whether the 
 * screen space Y axis points downwards (D3D) or upwards (OGL), and is primarily important in combination 
 * with the PRECISE_COVERAGE define, where this is important to ensure correct rounding and tie-breaker
 * behaviour. It also affects the ScissorRect screen space coordinates and the memory layout of the buffer 
 * returned by ComputePixelDepthBuffer().
 */
#define USE_D3D				1

#endif

#ifndef PRECISE_COVERAGE
/*!
 * Define PRECISE_COVERAGE to 1 to more closely match GPU rasterization rules. The increased precision comes
 * at a cost of slightly lower performance.
 */
#define PRECISE_COVERAGE	0

#endif

#ifndef ENABLE_STATS
/*!
 * Define ENABLE_STATS to 1 to gather various statistics during occlusion culling. Can be used for profiling 
 * and debugging. Note that enabling this function will reduce performance significantly.
 */
#define ENABLE_STATS		0

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Masked occlusion culling class
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MaskedOcclusionCulling 
{
public:

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Memory management callback functions
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	typedef void *(*pfnAlignedAlloc)(size_t alignment, size_t size);
	typedef void  (*pfnAlignedFree) (void *ptr);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Enums
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	enum Implementation {
		SSE2 = 0,
		SSE41 = 1,
		AVX2 = 2
	};

	enum CullingResult
	{
		VISIBLE = 0x0,
		OCCLUDED = 0x1,
		VIEW_CULLED = 0x3
	};

	enum ClipPlanes
	{
		CLIP_PLANE_NONE = 0x00,
		CLIP_PLANE_NEAR = 0x01,
		CLIP_PLANE_LEFT = 0x02,
		CLIP_PLANE_RIGHT = 0x04,
		CLIP_PLANE_BOTTOM = 0x08,
		CLIP_PLANE_TOP = 0x10,
		CLIP_PLANE_SIDES = (CLIP_PLANE_LEFT | CLIP_PLANE_RIGHT | CLIP_PLANE_BOTTOM | CLIP_PLANE_TOP),
		CLIP_PLANE_ALL = (CLIP_PLANE_LEFT | CLIP_PLANE_RIGHT | CLIP_PLANE_BOTTOM | CLIP_PLANE_TOP | CLIP_PLANE_NEAR)
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Structs
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*!
	 * Used to specify custom vertex layout. Memory offsets to y and z coordinates are set through 
	 * mOffsetY and mOffsetW, and vertex stride is given by mStride. It's possible to configure both 
	 * AoS and SoA layouts. Note that large strides may cause more cache misses and decrease 
	 * performance. It is advicable to store position data as compactly in memory as possible.
	 */
	struct VertexLayout
	{
		VertexLayout() {}
		VertexLayout(int stride, int offsetY, int offsetZW) :
			mStride(stride), mOffsetY(offsetY), mOffsetW(offsetZW) {}

		int mStride;  //< byte stride between vertices
		int mOffsetY; //< byte offset from X to Y coordinate
		union {
			int mOffsetZ; //!< byte offset from X to Z coordinate
			int mOffsetW; //!< byte offset from X to W coordinate
		};
	};

	/*!
	 * Used to control scissoring during rasterization. Note that we only provide coarse scissor support. 
	 * The scissor box x coordinates must be a multiple of 32, and the y coordinates a multiple of 8. 
	 * Scissoring is mainly meant as a means of enabling binning (sort middle) rasterizers in case
	 * application developers want to use that approach for multithreading.
	 */
	struct ScissorRect
	{
		ScissorRect() {}
		ScissorRect(int minX, int minY, int maxX, int maxY) :
			mMinX(minX), mMinY(minY), mMaxX(maxX), mMaxY(maxY) {}

		int mMinX;	//!< Screen space X coordinate for left side of scissor rect, inclusive and must be a multiple of 32
		int mMinY;	//!< Screen space Y coordinate for bottom side of scissor rect, inclusive and must be a multiple of 8
		int mMaxX;	//!< Screen space X coordinate for right side of scissor rect, <B>non</B> inclusive and must be a multiple of 32
		int mMaxY;	//!< Screen space Y coordinate for top side of scissor rect, <B>non</B> inclusive and must be a multiple of 8
	};

	/*!
	 * Used to specify storage area for a binlist, containing triangles. This struct is used for binning 
	 * and multithreading. The host application is responsible for allocating memory for the binlists.
	 */
	struct TriList
	{
		unsigned int mNumTriangles; //!< Maximum number of triangles that may be stored in mPtr
		unsigned int mTriIdx;       //!< Index of next triangle to be written, clear before calling BinTriangles to start from the beginning of the list
		float		 *mPtr;         //!< Scratchpad buffer allocated by the host application
	};
	struct OcclusionCullingStatistics
	{
		struct
		{
			long long mNumProcessedTriangles;
			long long mNumRasterizedTriangles;
			long long mNumTilesTraversed;
			long long mNumTilesUpdated;
		} mOccluders;

		struct
		{
			long long mNumProcessedRectangles;
			long long mNumProcessedTriangles;
			long long mNumRasterizedTriangles;
			long long mNumTilesTraversed;
		} mOccludees;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Functions
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*!
	 * \brief Creates a new object with default state, no z buffer attached/allocated.
	 */
	static MaskedOcclusionCulling *Create();
	
	/*!
	 * \brief Creates a new object with default state, no z buffer attached/allocated.
	 * \param memAlloc Pointer to a callback function used when allocating memory 
	 * \param memFree Pointer to a callback function used when freeing memory 
	 */
	static MaskedOcclusionCulling *Create(pfnAlignedAlloc memAlloc, pfnAlignedFree memFree);

	/*!
	 * \brief Destroys an object and frees the z buffer memory. Note that you cannot 
	 * use the delete operator, and should rather use this function to free up memory.
	 */
	static void Destroy(MaskedOcclusionCulling *moc);

	/*!
	 * \brief Sets the resolution of the hierarchical depth buffer. This function will
	 *        re-allocate the current depth buffer (if present). The contents of the
	 *        buffer is undefined until ClearBuffer() is called.
	 *
	 * \param witdh The width of the buffer in pixels, must be a multiple of 8
	 * \param height The height of the buffer in pixels, must be a multiple of 4
	 */
	virtual void SetResolution(unsigned int width, unsigned int height) = 0;

	/*!
	* \brief Gets the resolution of the hierarchical depth buffer. 
	*
	* \param witdh Output: The width of the buffer in pixels
	* \param height Output: The height of the buffer in pixels
	*/
	virtual void GetResolution(unsigned int &width, unsigned int &height) = 0;

	/*!
	 * \brief Sets the distance for the near clipping plane. Default is nearDist = 0.
	 *
	 * \param nearDist The distance to the near clipping plane, given as clip space w
	 */
	virtual void SetNearClipPlane(float nearDist) = 0;

	/*!
	* \brief Gets the distance for the near clipping plane. 
	*/
	virtual float GetNearClipPlane() = 0;

	/*!
	 * \brief Clears the hierarchical depth buffer.
	 */
	virtual void ClearBuffer() = 0;

	/*! 
	 * \brief Renders a mesh of occluder triangles and updates the hierarchical z buffer
	 *        with conservative depth values.
	 *
	 * This function is optimized for vertex layouts with stride 16 and y and w
	 * offsets of 4 and 12 bytes, respectively.
	 *
	 * \param inVtx Pointer to an array of input vertices, should point to the x component
	 *        of the first vertex. The input vertices are given as (x,y,w) cooordinates
	 *        in clip space. The memory layout can be changed using vtxLayout.
	 * \param inTris Pointer to an arrray of vertex indices. Each triangle is created 
	 *        from three indices consecutively fetched from the array.
	 * \param nTris The number of triangles to render (inTris must contain atleast 3*nTris
	 *        entries)
	 * \param modelToClipMatrix all vertices will be transformed by this matrix before
	 *        performing projection. If nullptr is passed the transform step will be skipped
	 * \param clipPlaneMask A mask indicating which clip planes should be considered by the
	 *        triangle clipper. Can be used as an optimization if your application can 
	 *        determine (for example during culling) that a group of triangles does not 
	 *        intersect a certein frustum plane. However, setting an incorrect mask may 
	 *        cause out of bounds memory accesses.
	 * \param scissor A scissor rectangle used to limit the active screen area. Note that
	 *        scissoring is only meant as a means of threading an implementation. The 
	 *        scissor rectangle coordinates must be a multiple of the tile size (32x8).
	 *        This argument is optional. Setting scissor to nullptr disables scissoring.
	 * \param vtxLayout A struct specifying the vertex layout (see struct for detailed 
	 *        description). For best performance, it is advicable to store position data
	 *        as compactly in memory as possible.
	 * \return Will return VIEW_CULLED if all triangles are either outside the frustum or
	 *         backface culled, returns VISIBLE otherwise.
	 */
	virtual CullingResult RenderTriangles(const float *inVtx, const unsigned int *inTris, int nTris, const float *modelToClipMatrix = nullptr, ClipPlanes clipPlaneMask = CLIP_PLANE_ALL, const ScissorRect *scissor = nullptr, const VertexLayout &vtxLayout = VertexLayout(16, 4, 12)) = 0;

	/*!
	 * \brief Occlusion query for a rectangle with a given depth. The rectangle is given 
	 *        in normalized device coordinates where (x,y) coordinates between [-1,1] map 
	 *        to the visible screen area. The query uses a GREATER_EQUAL (reversed) depth 
	 *        test meaning that depth values equal to the contents of the depth buffer are
	 *        counted as visible.
	 *
	 * \param xmin NDC coordinate of the left side of the rectangle.
	 * \param ymin NDC coordinate of the bottom side of the rectangle.
	 * \param xmax NDC coordinate of the right side of the rectangle.
	 * \param ymax NDC coordinate of the top side of the rectangle.
	 * \param ymax NDC coordinate of the top side of the rectangle.
	 * \param wmin Clip space W coordinate for the rectangle.
	 * \return The query will return VISIBLE if the rectangle may be visible, OCCLUDED
	 *         if the rectangle is occluded by a previously rendered  object, or VIEW_CULLED
	 *         if the rectangle is outside the view frustum.
	 */
	virtual CullingResult TestRect(float xmin, float ymin, float xmax, float ymax, float wmin) const = 0;

	/*!
	 * \brief This function is similar to RenderTriangles(), but performs an occlusion
	 *        query instead and does not update the hierarchical z buffer. The query uses 
	 *        a GREATER_EQUAL (reversed) depth test meaning that depth values equal to the 
	 *        contents of the depth buffer are counted as visible.
	 *
	 * This function is optimized for vertex layouts with stride 16 and y and w
	 * offsets of 4 and 12 bytes, respectively.
	 *
	 * \param inVtx Pointer to an array of input vertices, should point to the x component
	 *        of the first vertex. The input vertices are given as (x,y,w) cooordinates
	 *        in clip space. The memory layout can be changed using vtxLayout.
	 * \param inTris Pointer to an arrray of triangle indices. Each triangle is created 
	 *        from three indices consecutively fetched from the array.
	 * \param nTris The number of triangles to render (inTris must contain atleast 3*nTris
	 *        entries)
	 * \param modelToClipMatrix all vertices will be transformed by this matrix before
	 *        performing projection. If nullptr is passed the transform step will be skipped
	 * \param clipPlaneMask A mask indicating which clip planes should be considered by the
	 *        triangle clipper. Can be used as an optimization if your application can
	 *        determine (for example during culling) that a group of triangles does not
	 *        intersect a certein frustum plane. However, setting an incorrect mask may
	 *        cause out of bounds memory accesses.
	 * \param scissor A scissor rectangle used to limit the active screen area. Note that
	 *        scissoring is only meant as a means of threading an implementation. The
	 *        scissor rectangle coordinates must be a multiple of the tile size (32x8).
	 *        This argument is optional. Setting scissor to nullptr disables scissoring.
	 * \param vtxLayout A struct specifying the vertex layout (see struct for detailed 
	 *        description). For best performance, it is advicable to store position data
	 *        as compactly in memory as possible.
	 * \return The query will return VISIBLE if the triangle mesh may be visible, OCCLUDED
	 *         if the mesh is occluded by a previously rendered object, or VIEW_CULLED if all
	 *         triangles are entirely outside the view frustum or backface culled.
	 */
	virtual CullingResult TestTriangles(const float *inVtx, const unsigned int *inTris, int nTris, const float *modelToClipMatrix = nullptr, ClipPlanes clipPlaneMask = CLIP_PLANE_ALL, const ScissorRect *scissor = nullptr, const VertexLayout &vtxLayout = VertexLayout(16, 4, 12)) = 0;

	/*!
	 * \brief Perform input assembly, clipping , projection, triangle setup, and write
	 *        triangles to the screen space bins they overlap. This function can be used to
	 *        distribute work for threading (See the CullingThreadpool class for an example)
	 *
	 * \param inVtx Pointer to an array of input vertices, should point to the x component
	 *        of the first vertex. The input vertices are given as (x,y,w) cooordinates
	 *        in clip space. The memory layout can be changed using vtxLayout.
	 * \param inTris Pointer to an arrray of vertex indices. Each triangle is created
	 *        from three indices consecutively fetched from the array.
	 * \param nTris The number of triangles to render (inTris must contain atleast 3*nTris
	 *        entries)
	 * \param triLists Pointer to an array of TriList objects with one TriList object per
	 *        bin. If a triangle overlaps a bin, it will be written to the corresponding
	 *        trilist. Note that this method appends the triangles to the current list, to
	 *        start writing from the beginning of the list, set triList.mTriIdx = 0
	 * \param nBinsW Number of vertical bins, the screen is divided into nBinsW x nBinsH
	 *        rectangular bins.
	 * \param nBinsH Number of horizontal bins, the screen is divided into nBinsW x nBinsH
	 *        rectangular bins.
	 * \param modelToClipMatrix all vertices will be transformed by this matrix before
	 *        performing projection. If nullptr is passed the transform step will be skipped
	 * \param clipPlaneMask A mask indicating which clip planes should be considered by the
	 *        triangle clipper. Can be used as an optimization if your application can
	 *        determine (for example during culling) that a group of triangles does not
	 *        intersect a certein frustum plane. However, setting an incorrect mask may
	 *        cause out of bounds memory accesses.
	 * \param vtxLayout A struct specifying the vertex layout (see struct for detailed
	 *        description). For best performance, it is advicable to store position data
	 *        as compactly in memory as possible.
	 */
	virtual void BinTriangles(const float *inVtx, const unsigned int *inTris, int nTris, TriList *triLists, unsigned int nBinsW, unsigned int nBinsH, const float *modelToClipMatrix = nullptr, ClipPlanes clipPlaneMask = CLIP_PLANE_ALL, const VertexLayout &vtxLayout = VertexLayout(16, 4, 12)) = 0;

	/*!
	 * \brief Renders all occluder triangles in a trilist. This function can be used in
	 *        combination with BinTriangles() to create a threded (binning) rasterizer. The
	 *        bins can be processed independently by different threads without risking writing
	 *        to overlapping memory regions.
	 *
	 * \param triLists A triangle list, filled using the BinTriangles() function that is to
	 *        be rendered.
	 * \param scissor A scissor box limiting the rendering region to the bin. The size of each
	 *        bin must be a multiple of 32x8 pixels due to implementation constrants. For a
	 *        render target with (width, height) resolution and (nBinsW, nBinsH) bins, the
	 *        size of a bin is:
	 *          binWidth = (width / nBinsW) - (width / nBinsW) % 32;
	 *          binHeight = (height / nBinsH) - (height / nBinsH) % 8;
	 *        The last row and column of tiles have a different size:
	 *          lastColBinWidth = width - (nBinsW-1)*binWidth;
	 *          lastRowBinHeight = height - (nBinsH-1)*binHeight;
	 */
	virtual void RenderTrilist(const TriList &triList, const ScissorRect *scissor) = 0;

	/*!
	 * \brief Creates a per-pixel depth buffer from the hierarchical z buffer representation.
	 *        Intended for visualizing the hierarchical depth buffer for debugging. The 
	 *        buffer is written in scanline order, from the top to bottom (D3D) or bottom to 
	 *        top (OGL) of the surface. See the USE_D3D define.
	 *
	 * \param depthData Pointer to memory where the per-pixel depth data is written. Must
	 *        hold storage for atleast width*height elements as set by setResolution.
	 */
	virtual void ComputePixelDepthBuffer(float *depthData) = 0;
	
	/*!
	 * \brief Fetch occlusion culling statistics, returns zeroes if ENABLE_STATS define is
	 *        not defined. The statistics can be used for profiling or debugging.
	 */
	virtual OcclusionCullingStatistics GetStatistics() = 0;

	/*!
	 * \brief Returns the implementation (CPU instruction set) version of this object.
	 */
	virtual Implementation GetImplementation() = 0;

	/*!
	 * \brief Utility function for transforming vertices and outputting them to an (x,y,z,w)
	 *        format suitable for the occluder rasterization and occludee testing functions.
	 *
	 * \param mtx Pointer to matrix data. The matrix should column major for post 
	 *        multiplication (OGL) and row major for pre-multiplication (DX). This is 
	 *        consistent with OpenGL / DirectX behavior.
	 * \param inVtx Pointer to an array of input vertices. The input vertices are given as
	 *        (x,y,z) cooordinates. The memory layout can be changed using vtxLayout.
	 * \param xfVtx Pointer to an array to store transformed vertices. The transformed
	 *        vertices are always stored as array of structs (AoS) (x,y,z,w) packed in memory.
	 * \param nVtx Number of vertices to transform.
	 * \param vtxLayout A struct specifying the vertex layout (see struct for detailed 
	 *        description). For best performance, it is advicable to store position data
	 *        as compactly in memory as possible. Note that for this function, the
	 *        w-component is assumed to be 1.0.
	 */
	static void TransformVertices(const float *mtx, const float *inVtx, float *xfVtx, unsigned int nVtx, const VertexLayout &vtxLayout = VertexLayout(12, 4, 8));

protected:
	pfnAlignedAlloc mAlignedAllocCallback;
	pfnAlignedFree  mAlignedFreeCallback;

	mutable OcclusionCullingStatistics mStats;

	virtual ~MaskedOcclusionCulling() {}
};



#ifdef __THREADED_OCCLUSION__

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

/*!
  * \file CullingThreadpool.h
  * \brief Worker threadpool example for threaded masked occlusion culling.
  *
  * This class implements a threadpool for occluder rendering. Calls to CullingThreadpool::RenderTriangle()
  * will immediately return, after adding work items to a queue, and occluder rendering is performed
  * by worker threads as quickly as possible. Occlusion queries are performed directly on the calling 
  * threadand can be performed either synchronosly, by calling Flush() before executing the query, or 
  * asynchronosly, by performing the query without waiting for the worker threads to finish. 
  *
  * Note that this implementation should be considered an example rather than the best threading
  * solution. You may want to integrate threading in your own task system, and it may also be beneficial 
  * to thread the traversal code. Refer to MaskedOcclusionCulling::BinTriangles() and 
  * MaskedOcclusionCulling::RenderTrilist() for functions that can be used to make your own 
  * threaded culling system. 
  */

class CullingThreadpool
{
protected:
	static const int TRIS_PER_JOB = 1024; // Maximum number of triangles per job (bigger drawcalls are split), affects memory requirements

	typedef MaskedOcclusionCulling::CullingResult	CullingResult;
	typedef MaskedOcclusionCulling::ClipPlanes		ClipPlanes;
	typedef MaskedOcclusionCulling::ScissorRect		ScissorRect;
	typedef MaskedOcclusionCulling::VertexLayout	VertexLayout;
	typedef MaskedOcclusionCulling::TriList			TriList;

	// Small utility class for 4x4 matrices
	struct Matrix4x4
	{
		float mValues[16];
		Matrix4x4() {}
		Matrix4x4(const float *matrix)
		{
			for (int i = 0; i < 16; ++i)
				mValues[i] = matrix[i];
		}
	};

	// Internal utility class for a (mostly) lockless queue for binning & rendering jobs
	struct RenderJobQueue
	{
		struct BinningJob
		{
			const float*		mVerts;
			const unsigned int*	mTris;
			unsigned int		nTris;

			const float*		mMatrix;
			ClipPlanes			mClipPlanes;
			const VertexLayout* mVtxLayout;
		};

		struct Job
		{
			volatile unsigned int	mBinningJobStartedIdx;
			volatile unsigned int	mBinningJobCompletedIdx;
			BinningJob				mBinningJob;
			TriList					*mRenderJobs;
		};

		unsigned int			mNumBins;
		unsigned int			mMaxJobs;

		volatile unsigned int	mWritePtr;
		std::atomic_uint		mBinningPtr;
		std::atomic_uint		mBinningCompletedPtr;
		std::atomic_uint		*mRenderPtrs;
		std::atomic_uint		*mBinMutexes;

		float					*mTrilistData;
		Job						*mJobs;

		RenderJobQueue(unsigned int nBins, unsigned int maxJobs);
		~RenderJobQueue();

		unsigned int GetMinRenderPtr() const;
		unsigned int GetBestGlobalQueue() const;
		bool IsPipelineEmpty() const;

		bool CanWrite() const;
		bool CanBin() const;
		bool CanRender(int binIdx) const;

		Job *GetWriteJob();
		void AdvanceWriteJob();

		Job *GetBinningJob();
		void FinishedBinningJob(Job *job);

		Job *GetRenderJob(int binIdx);
		void AdvanceRenderJob(int binIdx);

		void Reset();
	};

	// Internal utility class for state (matrix / vertex layout)
	template<class T> struct StateData
	{
		unsigned int	mMaxJobs;
		unsigned int	mCurrentIdx;
		T				*mData;

		StateData(unsigned int maxJobs);
		~StateData();
		void AddData(const T &data);
		const T *GetData() const;
	};

	// Number of worker threads and bins
	unsigned int					mNumThreads;
	unsigned int					mNumBins;
	unsigned int					mMaxJobs;
	unsigned int					mBinsW;
	unsigned int					mBinsH;

	// Threads and control variables
	std::mutex						mSuspendedMutex;
	std::condition_variable			mSuspendedCV;
	volatile bool					mKillThreads;
	volatile bool					mSuspendThreads;
	volatile unsigned int			mNumSuspendedThreads;
	std::thread						*mThreads;

	// State variables and command queue
	const float						*mCurrentMatrix;
	StateData<Matrix4x4>			mModelToClipMatrices;
	StateData<VertexLayout>			mVertexLayouts;
	RenderJobQueue					*mRenderQueue;

	// Occlusion culling object and related scissor rectangles
	ScissorRect						*mRects;
	MaskedOcclusionCulling			*mMOC;

	void SetupScissors();

	static void ThreadRun(CullingThreadpool *threadPool, unsigned int threadId);
	void ThreadMain(unsigned int threadIdx);

public:
	/*!
	 * \brief Creates a new threadpool for masked occlusion culling. This object has a 
	 *        similar API to the MaskedOcclusionCulling class, but performs occluder
	 *        rendering asynchronously on worker threads (similar to how DX/GL works). 
	 *
	 * \param numThreads Number of worker threads to perform occluder rendering. Best 
	 *        balance may be scene/machine dependent, but it's good practice to leave at 
	 *        least one full core (2 threads with hyperthreading) for the main thread.
	 * \param binsW The screen is divided into binsW x binsH rectangular bins for load
	 *        balancing. The number of bins should be atleast equal to the number of
	 *        worker threads.
	 * \param binsH See description for the binsW parameter.
	 * \param maxJobs Maximum number of jobs that may be in flight at any given time. If
	 *        the caller thread generates jobs faster than the worker threads can finish
	 *        them, then the job queue will fill up and the caller thread will stall once
	 *        "maxJobs" items have been queued up. For culling systems interleaving occlusion 
	 *        queries and rendering, this value should be kept quite low to minimize false
	 *        positives (see TestRect()). We've observed that 32 [default] items typically
	 *        works well for our interleaved queries, while also allowing good load-balancing,
	 *        and this is the recommended setting. 
	 */
	CullingThreadpool(unsigned int numThreads, unsigned int binsW, unsigned int binsH, unsigned int maxJobs = 32);
	
	/*!
	 * \brief Destroys the threadpool and terminates all worker threads.
	 */
	~CullingThreadpool();

	/*!
	 * \brief Wakes up culling worker threads from suspended sleep, and puts them in a
	 *        ready state (using an idle spinlock with significantly higher CPU overhead). 
	 *
	 * It may take on the order of 100us to wake up the threads, so this function should
	 * preferably be called slightly ahead of starting occlusion culling work.
	 */
	void WakeThreads();

	/*!
	 * \brief Suspend all culling worker threads to a low CPU overhead sleep state. 
	 *
	 * For performance and latency reasons, the culling work is performed in an active 
	 * processing loop (with no thread sleeping) with high CPU overhead. In a system 
	 * with more worker threads it's important to put the culling worker threads in a 
	 * low overhead sleep state after occlusion culling work has completed.
	 */
	void SuspendThreads();

	/*!
	 * \brief Waits for all outstanding occluder rendering work to complete. Can be used
	 *        to ensure that rendering has completed before performing a TestRect() or 
	 *        TestTriangles() call.
	 */
	void Flush();

	/*
	 * \brief Sets the MaskedOcclusionCulling object (buffer) to be used for rendering and
	 *        testing calls. This method causes a Flush() to ensure that all unfinished 
	 *        rendering is completed.
	 */
	void SetBuffer(MaskedOcclusionCulling *moc);

	/*
	 * \brief Changes the resolution of the occlusion buffer, see MaskedOcclusionCulling::SetResolution(). 
	 *        This method causes a Flush() to ensure that all unfinished rendering is completed.
	 */
	void SetResolution(unsigned int width, unsigned int height);

	/*
	 * \brief Sets the near clipping plane, see MaskedOcclusionCulling::SetNearClipPlane(). This 
	 *        method causes a Flush() to ensure that all unfinished rendering is completed.
	 */
	void SetNearClipPlane(float nearDist);

	/*
	 * \brief Sets the model to clipspace transform matrix used for the RenderTriangles() and TestTriangles() 
	 *        function calls. The contents of the matrix is copied, and it's safe to modify it without calling
	 *        Flush(). The copy may be costly, which is the reason for passing this parameter as "state".
	 *
	 * \param modelToClipMatrix All vertices will be transformed by the specified model to clipspace matrix. 
	 *        Passing nullptr [default] disables the transform (equivalent to using an identity matrix).
	 */
	void SetMatrix(const float *modelToClipMatrix = nullptr);

	/*
	 * \brief Sets the vertex layout used for the RenderTriangles() and TestTriangles() function calls.
	 *        The vertex layout is copied, and it's safe to modify it without calling Flush(). The copy 
	 *        may be costly, which is the reason for passing this parameter as "state".
	 *
	 * \param vtxLayout A struct specifying the vertex layout (see struct for detailed
	 *        description). For best performance, it is advicable to store position data
	 *        as compactly in memory as possible.
	 */
	void SetVertexLayout(const VertexLayout &vtxLayout = VertexLayout(16, 4, 12));

	/*
	 * \brief Clears the occlusion buffer, see MaskedOcclusionCulling::ClearBuffer(). This method
	 *        causes a Flush() to ensure that all unfinished rendering is completed.
	 */
	void ClearBuffer();

	/*
	 * \brief Asynchronously render occluder triangles, see MaskedOcclusionCulling::RenderTriangles().
	 *
	 * This method puts the drawcall into a command queue, and immediately returns. The rendering is 
	 * performed by the worker threads at the earliest opportunity.
	 *
	 * <B>Important:</B> As rendering is performed asynchronously, the application is not allowed to 
	 * change the contents of the *inVtx or *inTris buffers until after rendering is completed. If 
	 * you wish to use dynamic buffers, the application must perform a Flush() to ensure that rendering 
	 * is finished, or make sure to rotate between more buffers than the maximum number of outstanding
	 * render jobs (see the CullingThreadpool() constructor).
	 */
	void RenderTriangles(const float *inVtx, const unsigned int *inTris, int nTris, ClipPlanes clipPlaneMask = MaskedOcclusionCulling::CLIP_PLANE_ALL);

	/*
	 * \brief Occlusion query for a rectangle with a given depth, see MaskedOcclusionCulling::TestRect().
	 *
	 * <B>Important:</B> This method is performed on the main thread and does not wait for outstanding 
	 * occluder rendering to be finished. To ensure that all occluder rendering is completed you must 
	 * perform a Flush() prior to calling this function. 
	 *
	 * It is conservatively correct to perform occlusion queries without calling Flush() (it may only 
	 * lead to objects being incorrectly classified as visible), and it can lead to much better performance 
	 * if occlusion queries are used for traversing a BVH or similar data structure. It's possible to 
	 * use "asynchronous" queries during traversal, and removing false positives later, when rendering 
	 * has completed.
	 */
	CullingResult TestRect(float xmin, float ymin, float xmax, float ymax, float wmin);

	/*
	 * \brief Occlusion query for a mesh, see MaskedOcclusionCulling::TestTriangles().
	 *
	 * <B>Important:</B> See the TestRect() method for a brief discussion about asynchronous occlusion 
	 * queries.
	 */
	CullingResult TestTriangles(const float *inVtx, const unsigned int *inTris, int nTris, ClipPlanes clipPlaneMask = MaskedOcclusionCulling::CLIP_PLANE_ALL);

	/*!
	 * \brief Creates a per-pixel depth buffer from the hierarchical z buffer representation, see
	 *        MaskedOcclusionCulling::ComputePixelDepthBuffer(). This method causes a Flush() to 
	 *        ensure that all unfinished rendering is completed.
	 */
	void ComputePixelDepthBuffer(float *depthData);
};

#endif //__THREADED_OCCLUSION__
