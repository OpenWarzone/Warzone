////////////////////////////////////////////////////////////////////////////////////////
// RAVEN SOFTWARE - STAR WARS: JK II
//  (c) 2002 Activision
//
// World Effects
//
//
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#include "tr_local.h"
#include "tr_WorldEffects.h"

#include "Ravl/CVec.h"
#include "Ratl/vector_vs.h"
#include "Ratl/bits_vs.h"

#ifdef _WIN32
#include "glext.h"
#endif

#ifdef __JKA_WEATHER__

extern qboolean	JKA_WEATHER_ENABLED;
extern qboolean	WZ_WEATHER_ENABLED;
extern qboolean	WZ_WEATHER_SOUND_ONLY;

extern qboolean CONTENTS_INSIDE_OUTSIDE_FOUND;

extern void Volumetric_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, const int passEntityNum, const int contentmask);

////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
#define GLS_ALPHA				(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)
#define	MAX_WIND_ZONES			10
#define MAX_WEATHER_ZONES		10
#define	MAX_PUFF_SYSTEMS		2
#define	MAX_PARTICLE_CLOUDS		64//5

#define POINTCACHE_CELL_SIZE	96.0f


////////////////////////////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////////////////////////////
float		mMillisecondsElapsed = 0;
float		mSecondsElapsed = 0;
bool		mFrozen = false;

CVec3		mGlobalWindVelocity;
CVec3		mGlobalWindDirection;
float		mGlobalWindSpeed;
int			mParticlesRendered;



////////////////////////////////////////////////////////////////////////////////////////
// Handy Functions
////////////////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#pragma warning( disable : 4512 )
#endif

// Returns a float min <= x < max (exclusive; will get max - 0.00001; but never max)
inline float WE_flrand(float min, float max) {
	return ((rand() * (max - min)) / (RAND_MAX)) + min;
}

////////////////////////////////////////////////////////////////////////////////////////
// Externs & Fwd Decl.
////////////////////////////////////////////////////////////////////////////////////////
extern void			SetViewportAndScissor( void );

inline void VectorFloor(vec3_t in)
{
	in[0] = floorf(in[0]);
	in[1] = floorf(in[1]);
	in[2] = floorf(in[2]);
}

inline void VectorCeil(vec3_t in)
{
	in[0] = ceilf(in[0]);
	in[1] = ceilf(in[1]);
	in[2] = ceilf(in[2]);
}

inline float	FloatRand(void)
{
	return ((float)rand() / (float)RAND_MAX);
}

inline	void	SnapFloatToGrid(float& f, int GridSize)
{
	f = (int)(f);

	bool	fNeg		= (f<0);
	if (fNeg)
	{
		f *= -1;		// Temporarly make it positive
	}

	int		Offset		= ((int)(f) % (int)(GridSize));
	int		OffsetAbs	= abs(Offset);
	if (OffsetAbs>(GridSize/2))
	{
		Offset = (GridSize - OffsetAbs) * -1;
	}

	f -= Offset;

	if (fNeg)
	{
		f *= -1;		// Put It Back To Negative
	}

	f = (int)(f);

	assert(((int)(f)%(int)(GridSize)) == 0);
}

inline	void	SnapVectorToGrid(CVec3& Vec, int GridSize)
{
	SnapFloatToGrid(Vec[0], GridSize);
	SnapFloatToGrid(Vec[1], GridSize);
	SnapFloatToGrid(Vec[2], GridSize);
}





////////////////////////////////////////////////////////////////////////////////////////
// Range Structures
////////////////////////////////////////////////////////////////////////////////////////
struct	SVecRange
{
	CVec3	mMins;
	CVec3	mMaxs;

	inline void	Clear()
	{
		mMins.Clear();
		mMaxs.Clear();
	}

	inline void Pick(CVec3& V)
	{
		V[0] = WE_flrand(mMins[0], mMaxs[0]);
		V[1] = WE_flrand(mMins[1], mMaxs[1]);
		V[2] = WE_flrand(mMins[2], mMaxs[2]);
	}
	inline void Wrap(CVec3& V, SVecRange &spawnRange)
	{
		if (V[0]<mMins[0])
		{
			const float d = mMins[0]-V[0];
			V[0] = mMaxs[0]-fmod(d, mMaxs[0]-mMins[0]);
		}
		if (V[0]>mMaxs[0])
		{
			const float d = V[0]-mMaxs[0];
			V[0] = mMins[0]+fmod(d, mMaxs[0]-mMins[0]);
		}

		if (V[1]<mMins[1])
		{
			const float d = mMins[1]-V[1];
			V[1] = mMaxs[1]-fmod(d, mMaxs[1]-mMins[1]);
		}
		if (V[1]>mMaxs[1])
		{
			const float d = V[1]-mMaxs[1];
			V[1] = mMins[1]+fmod(d, mMaxs[1]-mMins[1]);
		}

		if (V[2]<mMins[2])
		{
			const float d = mMins[2]-V[2];
			V[2] = mMaxs[2]-fmod(d, mMaxs[2]-mMins[2]);
		}
		if (V[2]>mMaxs[2])
		{
			const float d = V[2]-mMaxs[2];
			V[2] = mMins[2]+fmod(d, mMaxs[2]-mMins[2]);
		}
	}

	inline bool In(const CVec3& V)
	{
		return (V>mMins && V<mMaxs);
	}
};

struct	SFloatRange
{
	float	mMin;
	float	mMax;

	inline void	Clear()
	{
		mMin = 0;
		mMin = 0;
	}
	inline void Pick(float& V)
	{
		V = WE_flrand(mMin, mMax);
	}
	inline bool In(const float& V)
	{
		return (V>mMin && V<mMax);
	}
};

struct	SIntRange
{
	int	mMin;
	int	mMax;

	inline void	Clear()
	{
		mMin = 0;
		mMin = 0;
	}
	inline void Pick(int& V)
	{
		V = Q_irand(mMin, mMax);
	}
	inline bool In(const int& V)
	{
		return (V>mMin && V<mMax);
	}
};





////////////////////////////////////////////////////////////////////////////////////////
// The Particle Class
////////////////////////////////////////////////////////////////////////////////////////
class	CWeatherParticle
{
public:
	enum
	{
		FLAG_RENDER = 0,

		FLAG_FADEIN,
		FLAG_FADEOUT,
		FLAG_RESPAWN,

		FLAG_MAX
	};
	typedef		ratl::bits_vs<FLAG_MAX>		TFlags;

	float	mAlpha;
	TFlags	mFlags;
	CVec3	mPosition;
	CVec3	mVelocity;
	float	mMass;			// A higher number will more greatly resist force and result in greater gravity
};





////////////////////////////////////////////////////////////////////////////////////////
// The Wind
////////////////////////////////////////////////////////////////////////////////////////
class	CWindZone
{
public:
	bool		mGlobal;
	SVecRange	mRBounds;
	SVecRange	mRVelocity;
	SIntRange	mRDuration;
	SIntRange	mRDeadTime;
	float		mMaxDeltaVelocityPerUpdate;
	float		mChanceOfDeadTime;

	CVec3		mCurrentVelocity;
	CVec3		mTargetVelocity;
	int			mTargetVelocityTimeRemaining;


public:
	////////////////////////////////////////////////////////////////////////////////////
	// Initialize - Will setup default values for all data
	////////////////////////////////////////////////////////////////////////////////////
	void		Initialize()
	{
		mRBounds.Clear();
		mGlobal						= true;

		mRVelocity.mMins			= -1500.0f;
		mRVelocity.mMins[2]			= -10.0f;
		mRVelocity.mMaxs			= 1500.0f;
		mRVelocity.mMaxs[2]			= 10.0f;

		mMaxDeltaVelocityPerUpdate	= 10.0f;

		mRDuration.mMin				= 1000;
		mRDuration.mMax				= 2000;

		mChanceOfDeadTime			= 0.3f;
		mRDeadTime.mMin				= 1000;
		mRDeadTime.mMax				= 3000;

		mCurrentVelocity.Clear();
		mTargetVelocity.Clear();
		mTargetVelocityTimeRemaining = 0;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Update - Changes wind when current target velocity expires
	////////////////////////////////////////////////////////////////////////////////////
	void		Update()
	{
		if (mTargetVelocityTimeRemaining==0)
		{
			if (FloatRand()<mChanceOfDeadTime)
			{
				mRDeadTime.Pick(mTargetVelocityTimeRemaining);
				mTargetVelocity.Clear();
			}
			else
			{
				mRDuration.Pick(mTargetVelocityTimeRemaining);
				mRVelocity.Pick(mTargetVelocity);
			}
		}
		else if (mTargetVelocityTimeRemaining!=-1)
		{
			mTargetVelocityTimeRemaining--;

			CVec3	DeltaVelocity(mTargetVelocity - mCurrentVelocity);
			float	DeltaVelocityLen = VectorNormalize(DeltaVelocity.v);
			if (DeltaVelocityLen > mMaxDeltaVelocityPerUpdate)
			{
				DeltaVelocityLen = mMaxDeltaVelocityPerUpdate;
			}
			DeltaVelocity *= (DeltaVelocityLen);
			mCurrentVelocity += DeltaVelocity;
		}
	}
};
ratl::vector_vs<CWindZone, MAX_WIND_ZONES>		mWindZones;

bool R_GetWindVector(vec3_t windVector)
{
	VectorCopy(mGlobalWindDirection.v, windVector);
	return true;
}

bool R_GetWindSpeed(float &windSpeed)
{
	windSpeed = mGlobalWindSpeed;
	return true;
}

bool R_GetWindGusting()
{
	return (mGlobalWindSpeed>1000.0f);
}



////////////////////////////////////////////////////////////////////////////////////////
// Outside Point Cache
////////////////////////////////////////////////////////////////////////////////////////
class COutside
{
public:
	////////////////////////////////////////////////////////////////////////////////////
	//Global Public Outside Variables
	////////////////////////////////////////////////////////////////////////////////////
	bool			mOutsideShake;
	float			mOutsidePain;

private:
	////////////////////////////////////////////////////////////////////////////////////
	// The Outside Cache
	////////////////////////////////////////////////////////////////////////////////////
	bool			mCacheInit;			// Has It Been Cached?

	struct SWeatherZone
	{
		static bool	mMarkedOutside;
		uint32_t*	mPointCache;
		SVecRange	mExtents;
		SVecRange	mSize;
		int			mWidth;
		int			mHeight;
		int			mDepth;

		////////////////////////////////////////////////////////////////////////////////////
		// Convert To Cell
		////////////////////////////////////////////////////////////////////////////////////
		inline	void	ConvertToCell(const CVec3& pos, int& x, int& y, int& z, int& bit)
		{
			x = (int)((pos[0] / POINTCACHE_CELL_SIZE) - mSize.mMins[0]);
			y = (int)((pos[1] / POINTCACHE_CELL_SIZE) - mSize.mMins[1]);
			z = (int)((pos[2] / POINTCACHE_CELL_SIZE) - mSize.mMins[2]);

			bit = (z & 31);
			z >>= 5;
		}

		////////////////////////////////////////////////////////////////////////////////////
		// CellOutside - Test to see if a given cell is outside
		////////////////////////////////////////////////////////////////////////////////////
		inline	bool	CellOutside(int x, int y, int z, int bit)
		{
			//if (r_weather->integer >= 2 || (JKA_WEATHER_ENABLED && !CONTENTS_INSIDE_OUTSIDE_FOUND)) return true;

			if ((x < 0 || x >= mWidth) || (y < 0 || y >= mHeight) || (z < 0 || z >= mDepth) || (bit < 0 || bit >= 32))
			{
				return !(mMarkedOutside);
			}
			return (mMarkedOutside==(!!(mPointCache[((z * mWidth * mHeight) + (y * mWidth) + x)]&(1 << bit))));
		}
	};
	ratl::vector_vs<SWeatherZone, MAX_WEATHER_ZONES>	mWeatherZones;


private:
	////////////////////////////////////////////////////////////////////////////////////
	// Iteration Variables
	////////////////////////////////////////////////////////////////////////////////////
	int				mWCells;
	int				mHCells;

	int				mXCell;
	int				mYCell;
	int				mZBit;

	int				mXMax;
	int				mYMax;
	int				mZMax;


private:


	////////////////////////////////////////////////////////////////////////////////////
	// Contents Outside
	////////////////////////////////////////////////////////////////////////////////////
	/*inline	bool	ContentsOutside(int contents)
	{
		if (r_weather->integer >= 2 || (JKA_WEATHER_ENABLED && !CONTENTS_INSIDE_OUTSIDE_FOUND)) return true;

		if (contents&CONTENTS_WATER || contents&CONTENTS_SOLID)
		{
			return false;
		}
		if (mCacheInit)
		{
			if (SWeatherZone::mMarkedOutside)
			{
				return (!!(contents&CONTENTS_OUTSIDE));
			}
			return (!(contents&CONTENTS_INSIDE));
		}
		return !!(contents&CONTENTS_OUTSIDE);
	}*/




public:
	////////////////////////////////////////////////////////////////////////////////////
	// Constructor - Will setup default values for all data
	////////////////////////////////////////////////////////////////////////////////////
	void Reset()
	{
		mOutsideShake = false;
		mOutsidePain = 0.0;
		mCacheInit = false;
		SWeatherZone::mMarkedOutside = false;
		for (int wz=0; wz<mWeatherZones.size(); wz++)
		{
			Z_Free(mWeatherZones[wz].mPointCache);
			mWeatherZones[wz].mPointCache = 0;
		}
		mWeatherZones.clear();
	}

	COutside()
	{
		Reset();
	}
	~COutside()
	{
		Reset();
	}

	bool			Initialized()
	{
		return mCacheInit;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// AddWeatherZone - Will add a zone of mins and maxes
	////////////////////////////////////////////////////////////////////////////////////
	void			AddWeatherZone(vec3_t mins, vec3_t maxs)
	{
		if (!mWeatherZones.full())
		{
			SWeatherZone&	Wz = mWeatherZones.push_back();
			Wz.mExtents.mMins = mins;
			Wz.mExtents.mMaxs = maxs;

			SnapVectorToGrid(Wz.mExtents.mMins, POINTCACHE_CELL_SIZE);
			SnapVectorToGrid(Wz.mExtents.mMaxs, POINTCACHE_CELL_SIZE);

			Wz.mSize.mMins = Wz.mExtents.mMins;
			Wz.mSize.mMaxs = Wz.mExtents.mMaxs;

			Wz.mSize.mMins /= POINTCACHE_CELL_SIZE;
			Wz.mSize.mMaxs /= POINTCACHE_CELL_SIZE;
			Wz.mWidth		=  (int)(Wz.mSize.mMaxs[0] - Wz.mSize.mMins[0]);
			Wz.mHeight		=  (int)(Wz.mSize.mMaxs[1] - Wz.mSize.mMins[1]);
			Wz.mDepth		= ((int)(Wz.mSize.mMaxs[2] - Wz.mSize.mMins[2]) + 31) >> 5;

			int arraySize	= (Wz.mWidth * Wz.mHeight * Wz.mDepth);
			Wz.mPointCache  = (uint32_t *)Z_Malloc(arraySize*sizeof(uint32_t), TAG_POINTCACHE, qtrue);
		}
		else
		{
			ri->Printf(PRINT_ERROR, "Weather zones full trying to add zone %i.\n", mWeatherZones.size());
		}
	}



	////////////////////////////////////////////////////////////////////////////////////
	// Cache - Will Scan the World, Creating The Cache
	////////////////////////////////////////////////////////////////////////////////////
#if 0
	////////////////////////////////////////////////////////////////////////////////////
	// Cache - Will Scan the World, Creating The Cache
	////////////////////////////////////////////////////////////////////////////////////
	void			Cache()
	{
		if (!tr.world || mCacheInit)
		{
			return;
		}

		CVec3		CurPos;
		CVec3		Size;
		CVec3		Mins;
		int			x, y, z, q, zbase;
		//bool		curPosOutside;
		uint32_t	contents;
		uint32_t	bit;

		// Record The Extents Of The World Incase No Other Weather Zones Exist
		//---------------------------------------------------------------------
		if (!mWeatherZones.size())
		{
			ri->Printf(PRINT_ALL, "No Weather Zones Encountered... Generating...\n");
			//AddWeatherZone(tr.world->bmodels[0].bounds[0], tr.world->bmodels[0].bounds[1]);

			// Let's make multiple zones by default, so we can do proper indoor/outdoor testing...
#define ZONE_DEFAULT_SIZE 384.0

			for (float zx = tr.world->bmodels[0].bounds[0][0]; zx < tr.world->bmodels[0].bounds[1][0]; zx += ZONE_DEFAULT_SIZE)
			{
				for (float zy = tr.world->bmodels[0].bounds[0][1]; zy < tr.world->bmodels[0].bounds[1][1]; zy += ZONE_DEFAULT_SIZE)
				{
					for (float zz = tr.world->bmodels[0].bounds[0][2]; zz < tr.world->bmodels[0].bounds[1][2]; zz += ZONE_DEFAULT_SIZE)
					{
						vec3_t thisMins, thisMaxs;
						VectorSet(thisMins, zx, zy, zz);
						VectorSet(thisMaxs, zx + ZONE_DEFAULT_SIZE, zy + ZONE_DEFAULT_SIZE, zz + ZONE_DEFAULT_SIZE);
						AddWeatherZone(thisMins, thisMaxs);
					}
				}
			}

			ri->Printf(PRINT_ALL, "Generated %i weather zones for this map...\n", mWeatherZones.size());
		}

		mCacheInit = true;
		SWeatherZone::mMarkedInside = false;		// Assume All Is Outside, Except Solid

													//
													// Load the weather zone info from a cache file, if one exists
													//------------------------------------------------------------
		extern char currentMapName[128];

		fileHandle_t f;
		long fSize = ri->FS_FOpenFileRead(va("weatherZones/%s.wzones", currentMapName), &f, qtrue);

		if (f && fSize)
		{
			int numInside = 0;
			int numOutside = 0;

			ri->Printf(PRINT_ALL, "Loading weather zones cache from file \"%s\".\n", va("weatherZones/%s.wzones", currentMapName));

			ri->FS_Read(&mCacheInit, sizeof(mCacheInit), f);
			ri->FS_Read(&SWeatherZone::mMarkedInside, sizeof(SWeatherZone::mMarkedInside), f);

			// Iterate Over All Weather Zones
			//--------------------------------
			for (int zone = 0; zone < mWeatherZones.size(); zone++)
			{
				SWeatherZone	wz = mWeatherZones[zone];

				ri->FS_Read(&wz.mDepth, sizeof(wz.mDepth), f);
				ri->FS_Read(&wz.mExtents, sizeof(wz.mExtents), f);
				ri->FS_Read(&wz.mHeight, sizeof(wz.mHeight), f);
				ri->FS_Read(&wz.mMarkedInside, sizeof(wz.mMarkedInside), f);
				ri->FS_Read(&wz.mSize, sizeof(wz.mSize), f);
				ri->FS_Read(&wz.mWidth, sizeof(wz.mWidth), f);

				uint32_t pointCahceSize = sizeof(uint32_t) * (wz.mWidth * wz.mHeight * wz.mDepth);
				//ri->FS_Read(&pointCahceSize, sizeof(uint32_t), f);
				ri->FS_Read(wz.mPointCache, pointCahceSize, f);

				if (wz.mMarkedInside)
					numOutside++;
				else
					numInside++;
			}

			ri->FS_FCloseFile(f);

			ri->Printf(PRINT_ALL, "Loaded %i weather zones from cache. %i are inside and %i are outside.\n", mWeatherZones.size(), numInside, numOutside);
			return;
		}

		f = NULL;

		ri->Printf(PRINT_ALL, "Weather zones cache file does not exist, generating...\n");

		// Iterate Over All Weather Zones
		//--------------------------------
		for (int zone = 0; zone<mWeatherZones.size(); zone++)
		{
			SWeatherZone	wz = mWeatherZones[zone];
			uint32_t		thisPointCahceSize = sizeof(uint32_t) * (wz.mWidth * wz.mHeight * wz.mDepth);

			//bool curPosOutside = false;

			// Make Sure Point Contents Checks Occur At The CENTER Of The Cell
			//-----------------------------------------------------------------
			Mins = wz.mExtents.mMins;
			for (x = 0; x<3; x++)
			{
				Mins[x] += (POINTCACHE_CELL_SIZE / 2);
			}

			int numInside = 0;
			int numOutside = 0;

			// Start Scanning
			//----------------
			for (z = 0; z < wz.mDepth; z++)
			{
				for (q = 0; q < 32; q++)
				{
					bit = (1 << q);
					zbase = (z << 5);

					for (x = 0; x < wz.mWidth; x++)
					{
						for (y = 0; y < wz.mHeight; y++)
						{
							CurPos[0] = x			* POINTCACHE_CELL_SIZE;
							CurPos[1] = y			* POINTCACHE_CELL_SIZE;
							CurPos[2] = (zbase + q)	* POINTCACHE_CELL_SIZE;
							CurPos += Mins;

							// How about we scan for sky???
							qboolean hit = qfalse;

							vec3_t from, to;
							VectorCopy(CurPos.v, from);
							from[2] += 16.0;
							VectorCopy(CurPos.v, to);
							to[2] += 524288.0;

							while (!hit)
							{
								trace_t trace;

								Volumetric_Trace(&trace, from, NULL, NULL, to, -1, (CONTENTS_SOLID | CONTENTS_TERRAIN));

								if (trace.surfaceFlags & SURF_SKY)
								{// Found the sky... We know this is outside...
									numOutside++;
									hit = qtrue;
									ri->Printf(PRINT_ALL, "Zone %i Hit sky!\n", zone);
									break;
								}

								if (trace.endpos[2] > tr.world->bmodels[0].bounds[1][2])
								{// Outside of the map...
								 //ri->Printf(PRINT_ALL, "Zone %i Hit outside of map, assuming sky!\n", zone);
								 //numOutside++;
									hit = qtrue;
									break;
								}

								if (trace.endpos[2] >= wz.mExtents.mMaxs[2] && !(trace.surfaceFlags & SURF_NODRAW))
								{// Hit something, and it's above our cell, but not sky... Inside...
									numInside++;
									hit = qtrue;
									ri->Printf(PRINT_ALL, "Zone %i Hit something, not sky!\n", zone);
									break;
								}

								if ((trace.surfaceFlags & SURF_NODRAW))
								{// Hit something, and it's above our cell, but not sky... Inside...
								 // The trace did not leave our own cell, probably hit the ground, scan again from end pos...
									from[2] = trace.endpos[2] + 32.0;

									ri->Printf(PRINT_ALL, "Zone %i Trying from higher up...\n", zone);
									continue;
								}

								// Attempt fallback to JKA method...
								contents = ri->CM_PointContents(CurPos.v, 0);

								if (contents&CONTENTS_INSIDE)
								{
									ri->Printf(PRINT_ALL, "Zone %i Point contents INSIDE!\n", zone);
									numInside++;
									hit = qtrue;
									break;
								}
								else if (contents&CONTENTS_OUTSIDE)
								{
									ri->Printf(PRINT_ALL, "Zone %i Point contents OUTSIDE!\n", zone);
									numOutside++;
									hit = qtrue;
									break;
								}

								// The trace did not leave our own cell, probably hit the ground, scan again from end pos...
								//from[2] = trace.endpos[2] + POINTCACHE_CELL_SIZE;

								//ri->Printf(PRINT_ALL, "Zone %i Trying from higher up...\n", zone);

								numInside++;
								hit = qtrue;
								break;
							}

							// Mark The Point
							//----------------
							wz.mPointCache[((z * wz.mWidth * wz.mHeight) + (y * wz.mWidth) + x)] |= bit;
						}// for (y)
					}// for (x)
				}// for (q)
			}// for (z)

			ri->Printf(PRINT_WARNING, "zone %i: %i outside and %i inside.\n", zone, numOutside, numInside);

			if (numOutside >= numInside)
			{
				wz.mMarkedInside = false;
			}
			else
			{
				wz.mMarkedInside = true;
			}
		}

		// If no indoor or outdoor brushes were found
		//--------------------------------------------
		/*if (!mCacheInit)
		{
		mCacheInit = true;
		SWeatherZone::mMarkedInside = false;		// Assume All Is Outside, Except Solid
		}*/


		//
		// Save the weather zone info to a cache file for later use
		//---------------------------------------------------------
		extern char currentMapName[128];

		f = ri->FS_FOpenFileWrite(va("weatherZones/%s.wzones", currentMapName), qfalse);

		if (f)
		{
			ri->Printf(PRINT_ALL, "Saving weather zones cache to file \"%s\".\n", va("weatherZones/%s.wzones", currentMapName));

			int numInside = 0;
			int numOutside = 0;

			ri->FS_Write(&mCacheInit, sizeof(mCacheInit), f);
			ri->FS_Write(&SWeatherZone::mMarkedInside, sizeof(SWeatherZone::mMarkedInside), f);

			// Iterate Over All Weather Zones
			//--------------------------------
			for (int zone = 0; zone < mWeatherZones.size(); zone++)
			{
				SWeatherZone	wz = mWeatherZones[zone];

				ri->FS_Write(&wz.mDepth, sizeof(wz.mDepth), f);
				ri->FS_Write(&wz.mExtents, sizeof(wz.mExtents), f);
				ri->FS_Write(&wz.mHeight, sizeof(wz.mHeight), f);
				ri->FS_Write(&wz.mMarkedInside, sizeof(wz.mMarkedInside), f);
				ri->FS_Write(&wz.mSize, sizeof(wz.mSize), f);
				ri->FS_Write(&wz.mWidth, sizeof(wz.mWidth), f);

				uint32_t pointCahceSize = sizeof(uint32_t) * (wz.mWidth * wz.mHeight * wz.mDepth);
				//ri->FS_Write(&pointCahceSize, sizeof(uint32_t), f);
				ri->FS_Write(wz.mPointCache, pointCahceSize, f);

				if (wz.mMarkedInside)
					numOutside++;
				else
					numInside++;
			}

			ri->FS_FCloseFile(f);

			ri->Printf(PRINT_ALL, "Saved %i weather zones to cache. %i are inside and %i are outside.\n", mWeatherZones.size(), numInside, numOutside);
		}
	}
#else
	void			Cache()
	{
		if (!tr.world || mCacheInit)
		{
			return;
		}

		CVec3		CurPos;
		CVec3		Size;
		CVec3		Mins;
		int			x, y, z, q, zbase;
		bool		curPosOutside;
		uint32_t	contents;
		uint32_t	bit;


		// Record The Extents Of The World Incase No Other Weather Zones Exist
		//---------------------------------------------------------------------
		if (!mWeatherZones.size())
		{
			ri->Printf(PRINT_ALL, "No Weather Zones Encountered... Generating...\n");
			//AddWeatherZone(tr.world->bmodels[0].bounds[0], tr.world->bmodels[0].bounds[1]);

			// Let's make multiple zones by default, so we can do proper indoor/outdoor testing...
#define ZONE_DEFAULT_SIZE 192.0 // 384.0

			for (float zx = tr.world->bmodels[0].bounds[0][0]; zx < tr.world->bmodels[0].bounds[1][0]; zx += ZONE_DEFAULT_SIZE)
			{
				for (float zy = tr.world->bmodels[0].bounds[0][1]; zy < tr.world->bmodels[0].bounds[1][1]; zy += ZONE_DEFAULT_SIZE)
				{
					for (float zz = tr.world->bmodels[0].bounds[0][2]; zz < tr.world->bmodels[0].bounds[1][2]; zz += ZONE_DEFAULT_SIZE)
					{
						vec3_t thisMins, thisMaxs;
						VectorSet(thisMins, zx, zy, zz);
						VectorSet(thisMaxs, zx + ZONE_DEFAULT_SIZE, zy + ZONE_DEFAULT_SIZE, zz + ZONE_DEFAULT_SIZE);
						AddWeatherZone(thisMins, thisMaxs);
					}
				}
			}

			ri->Printf(PRINT_ALL, "Generated %i weather zones for this map...\n", mWeatherZones.size());
		}

		//
		// Load the weather zone info from a cache file, if one exists
		//------------------------------------------------------------
		extern char currentMapName[128];

		fileHandle_t f;
		long fSize = ri->FS_FOpenFileRead(va("weatherZones/%s.wzones", currentMapName), &f, qtrue);

		if (f && fSize)
		{
			int numInside = 0;
			int numOutside = 0;

			ri->Printf(PRINT_ALL, "Loading weather zones cache from file \"%s\".\n", va("weatherZones/%s.wzones", currentMapName));

			ri->FS_Read(&mCacheInit, sizeof(mCacheInit), f);
			ri->FS_Read(&SWeatherZone::mMarkedOutside, sizeof(SWeatherZone::mMarkedOutside), f);

			// Iterate Over All Weather Zones
			//--------------------------------
			for (int zone = 0; zone < mWeatherZones.size(); zone++)
			{
				SWeatherZone	wz = mWeatherZones[zone];

				ri->FS_Read(&wz.mDepth, sizeof(wz.mDepth), f);
				ri->FS_Read(&wz.mExtents, sizeof(wz.mExtents), f);
				ri->FS_Read(&wz.mHeight, sizeof(wz.mHeight), f);
				ri->FS_Read(&wz.mMarkedOutside, sizeof(wz.mMarkedOutside), f);
				ri->FS_Read(&wz.mSize, sizeof(wz.mSize), f);
				ri->FS_Read(&wz.mWidth, sizeof(wz.mWidth), f);

				uint32_t pointCahceSize = sizeof(uint32_t) * (wz.mWidth * wz.mHeight * wz.mDepth);
				//ri->FS_Read(&pointCahceSize, sizeof(uint32_t), f);
				ri->FS_Read(wz.mPointCache, pointCahceSize, f);

				if (wz.mMarkedOutside)
					numOutside++;
				else
					numInside++;
			}

			ri->FS_FCloseFile(f);

			ri->Printf(PRINT_ALL, "Loaded %i weather zones from cache. %i are inside and %i are outside.\n", mWeatherZones.size(), numInside, numOutside);
			return;
		}

		f = NULL;

		ri->Printf(PRINT_ALL, "Weather zones cache file does not exist, generating...\n");

		// Iterate Over All Weather Zones
		//--------------------------------
		for (int zone=0; zone<mWeatherZones.size(); zone++)
		{
			SWeatherZone	wz = mWeatherZones[zone];
			uint32_t		thisPointCahceSize = sizeof(uint32_t) * (wz.mWidth * wz.mHeight * wz.mDepth);

			// Make Sure Point Contents Checks Occur At The CENTER Of The Cell
			//-----------------------------------------------------------------
			Mins = wz.mExtents.mMins;
			for (x=0; x<3; x++)
			{
				Mins[x] += (POINTCACHE_CELL_SIZE/2);
			}

			// Start Scanning
			//----------------
			for(z = 0; z < wz.mDepth; z++)
			{
				for(q = 0; q < 32; q++)
				{
					bit = (1 << q);
					zbase = (z << 5);

					for(x = 0; x < wz.mWidth; x++)
					{
						for(y = 0; y < wz.mHeight; y++)
						{
							CurPos[0] = x			* POINTCACHE_CELL_SIZE;
							CurPos[1] = y			* POINTCACHE_CELL_SIZE;
							CurPos[2] = (zbase + q)	* POINTCACHE_CELL_SIZE;
							CurPos	  += Mins;

							contents = ri->CM_PointContents(CurPos.v, 0);

							if (/*contents&CONTENTS_INSIDE ||*/ contents&CONTENTS_OUTSIDE)
							{
								curPosOutside = ((contents&CONTENTS_OUTSIDE) != 0);
								if (!mCacheInit)
								{
									mCacheInit = true;
									SWeatherZone::mMarkedOutside = curPosOutside;
								}
								/*else if (SWeatherZone::mMarkedOutside!=curPosOutside)
								{
									assert(0);
									Com_Error (ERR_DROP, "Weather Effect: Both Indoor and Outdoor brushs encountered in map.\n" );
									return;
								}*/ // Why?!?!?!?!?!?!? FFS...

								// Mark The Point
								//----------------
								wz.mPointCache[((z * wz.mWidth * wz.mHeight) + (y * wz.mWidth) + x)] |= bit;
							}
						}// for (y)
					}// for (x)
				}// for (q)
			}// for (z)
		}

		// If no indoor or outdoor brushes were found
		//--------------------------------------------
		if (!mCacheInit)
		{
			mCacheInit = true;
			SWeatherZone::mMarkedOutside = false;		// Assume All Is Outside, Except Solid
		}

		//
		// Save the weather zone info to a cache file for later use
		//---------------------------------------------------------
		extern char currentMapName[128];

		f = ri->FS_FOpenFileWrite(va("weatherZones/%s.wzones", currentMapName), qfalse);

		if (f)
		{
			ri->Printf(PRINT_ALL, "Saving weather zones cache to file \"%s\".\n", va("weatherZones/%s.wzones", currentMapName));

			int numInside = 0;
			int numOutside = 0;

			ri->FS_Write(&mCacheInit, sizeof(mCacheInit), f);
			ri->FS_Write(&SWeatherZone::mMarkedOutside, sizeof(SWeatherZone::mMarkedOutside), f);

			// Iterate Over All Weather Zones
			//--------------------------------
			for (int zone = 0; zone < mWeatherZones.size(); zone++)
			{
				SWeatherZone	wz = mWeatherZones[zone];

				ri->FS_Write(&wz.mDepth, sizeof(wz.mDepth), f);
				ri->FS_Write(&wz.mExtents, sizeof(wz.mExtents), f);
				ri->FS_Write(&wz.mHeight, sizeof(wz.mHeight), f);
				ri->FS_Write(&wz.mMarkedOutside, sizeof(wz.mMarkedOutside), f);
				ri->FS_Write(&wz.mSize, sizeof(wz.mSize), f);
				ri->FS_Write(&wz.mWidth, sizeof(wz.mWidth), f);

				uint32_t pointCahceSize = sizeof(uint32_t) * (wz.mWidth * wz.mHeight * wz.mDepth);
				//ri->FS_Write(&pointCahceSize, sizeof(uint32_t), f);
				ri->FS_Write(wz.mPointCache, pointCahceSize, f);

				if (wz.mMarkedOutside)
					numOutside++;
				else
					numInside++;
			}

			ri->FS_FCloseFile(f);

			ri->Printf(PRINT_ALL, "Saved %i weather zones to cache. %i are inside and %i are outside.\n", mWeatherZones.size(), numInside, numOutside);
		}
	}
#endif




public:
	////////////////////////////////////////////////////////////////////////////////////
	// PointOutside - Test to see if a given point is outside
	////////////////////////////////////////////////////////////////////////////////////
	inline	bool	PointOutside(const CVec3& pos)
	{
		if (r_weather->integer >= 2 || (JKA_WEATHER_ENABLED && !CONTENTS_INSIDE_OUTSIDE_FOUND)) return true;

		if (!mCacheInit)
		{
			return true;// ContentsOutside(ri->CM_PointContents(pos.v, 0));
		}
		for (int zone=0; zone<mWeatherZones.size(); zone++)
		{
			SWeatherZone	wz = mWeatherZones[zone];
			if (wz.mExtents.In(pos))
			{
				int		bit, x, y, z;
				wz.ConvertToCell(pos, x, y, z, bit);
				return wz.CellOutside(x, y, z, bit);
			}
		}
		return !(SWeatherZone::mMarkedOutside);

	}


	////////////////////////////////////////////////////////////////////////////////////
	// PointOutside - Test to see if a given bounded plane is outside
	////////////////////////////////////////////////////////////////////////////////////
	inline	bool	PointOutside(const CVec3& pos, float width, float height)
	{
		if (r_weather->integer >= 2 || (JKA_WEATHER_ENABLED && !CONTENTS_INSIDE_OUTSIDE_FOUND)) return true;

		for (int zone=0; zone<mWeatherZones.size(); zone++)
		{
			SWeatherZone	wz = mWeatherZones[zone];
			if (wz.mExtents.In(pos))
			{
				int		bit, x, y, z;
				wz.ConvertToCell(pos, x, y, z, bit);
				if (width<POINTCACHE_CELL_SIZE || height<POINTCACHE_CELL_SIZE)
				{
 					return (wz.CellOutside(x, y, z, bit));
				}

				mWCells = ((int)width  / POINTCACHE_CELL_SIZE);
				mHCells = ((int)height / POINTCACHE_CELL_SIZE);

				mXMax = x + mWCells;
				mYMax = y + mWCells;
				mZMax = bit + mHCells;

				for (mXCell=x-mWCells; mXCell<=mXMax; mXCell++)
				{
					for (mYCell=y-mWCells; mYCell<=mYMax; mYCell++)
					{
						for (mZBit=bit-mHCells; mZBit<=mZMax; mZBit++)
						{
							if (!wz.CellOutside(mXCell, mYCell, z, mZBit))
							{
								return false;
							}
						}
					}
				}
				return true;
			}
		}
		return !(SWeatherZone::mMarkedOutside);
	}
};
COutside			mOutside;
bool				COutside::SWeatherZone::mMarkedOutside = false;


void RE_AddWeatherZone(vec3_t mins, vec3_t maxs)
{
	mOutside.AddWeatherZone(mins, maxs);
}

bool R_IsOutside(vec3_t pos)
{
	return mOutside.PointOutside(pos);
}

bool R_IsShaking()
{
	return (mOutside.mOutsideShake && mOutside.PointOutside(backEnd.viewParms.ori.origin));
}

float R_IsOutsideCausingPain(vec3_t pos)
{
	return (mOutside.mOutsidePain && mOutside.PointOutside(pos));
}


////////////////////////////////////////////////////////////////////////////////////////
// Particle Cloud
////////////////////////////////////////////////////////////////////////////////////////
class	CWeatherParticleCloud
{
private:
	////////////////////////////////////////////////////////////////////////////////////
	// DYNAMIC MEMORY
	////////////////////////////////////////////////////////////////////////////////////
	image_t*	mImage;
	CWeatherParticle*	mParticles;

private:
	////////////////////////////////////////////////////////////////////////////////////
	// RUN TIME VARIANTS
	////////////////////////////////////////////////////////////////////////////////////
	float		mSpawnSpeed;
	CVec3		mSpawnPlaneNorm;
	CVec3		mSpawnPlaneRight;
	CVec3		mSpawnPlaneUp;
	SVecRange	mRange;

	CVec3		mCameraPosition;
	CVec3		mCameraForward;
	CVec3		mCameraLeft;
	CVec3		mCameraDown;
	CVec3		mCameraLeftPlusUp;
	CVec3		mCameraLeftMinusUp;


	int			mParticleCountRender;
	int			mGLModeEnum;

	bool		mPopulated;


public:
	////////////////////////////////////////////////////////////////////////////////////
	// CONSTANTS
	////////////////////////////////////////////////////////////////////////////////////
	bool		mOrientWithVelocity;
	float		mSpawnPlaneSize;
	float		mSpawnPlaneDistance;
	SVecRange	mSpawnRange;

	float		mGravity;			// How much gravity affects the velocity of a particle
	CVec4		mColor;				// RGBA color
	int			mVertexCount;		// 3 for triangle, 4 for quad, other numbers not supported

	float		mWidth;
	float		mHeight;

	int			mBlendMode;			// 0 = ALPHA, 1 = SRC->SRC
	int			mFilterMode;		// 0 = LINEAR, 1 = NEAREST

	float		mFade;				// How much to fade in and out 1.0 = instant, 0.01 = very slow

	SFloatRange	mRotation;
	float		mRotationDelta;
	float		mRotationDeltaTarget;
	float		mRotationCurrent;
	SIntRange	mRotationChangeTimer;
	int			mRotationChangeNext;

	SFloatRange	mMass;				// Determines how slowness to accelerate, higher number = slower
	float		mFrictionInverse;	// How much air friction does this particle have 1.0=none, 0.0=nomove

	int			mParticleCount;

	bool		mWaterParticles;




public:
	////////////////////////////////////////////////////////////////////////////////////
	// Initialize - Create Image, Particles, And Setup All Values
	////////////////////////////////////////////////////////////////////////////////////
	void	Initialize(int count, const char* texturePath, int VertexCount=4)
	{
		Reset();
		assert(mParticleCount==0 && mParticles==0);
		assert(mImage==0);

		// Create The Image
		//------------------
		mImage = R_FindImageFile(texturePath, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE);
		if (!mImage)
		{
			Com_Error(ERR_DROP, "CWeatherParticleCloud: Could not texture %s", texturePath);
		}

		GL_Bind(mImage);



		// Create The Particles
		//----------------------
		mParticleCount	= count;
		mParticles		= new CWeatherParticle[mParticleCount];



		CWeatherParticle*	part=0;
		for (int particleNum=0; particleNum<mParticleCount; particleNum++)
		{
			part = &(mParticles[particleNum]);
			part->mPosition.Clear();
			part->mVelocity.Clear();
			part->mAlpha	= 0.0f;
			mMass.Pick(part->mMass);
		}

		mVertexCount = VertexCount;

		mGLModeEnum = (mVertexCount==3)?(GL_TRIANGLES):(GL_QUADS);
	}


	////////////////////////////////////////////////////////////////////////////////////
	// Reset - Initializes all data to default values
	////////////////////////////////////////////////////////////////////////////////////
	void		Reset()
	{
		if (mImage)
		{
			// TODO: Free Image?
		}
		mImage				= 0;
		if (mParticleCount)
		{
			delete [] mParticles;
		}
		mParticleCount		= 0;
		mParticles			= 0;

		mPopulated			= 0;



		// These Are The Default Startup Values For Constant Data
		//========================================================
		mOrientWithVelocity = false;
		mWaterParticles		= false;

		mSpawnPlaneDistance	= 500;
		mSpawnPlaneSize		= 500;
		mSpawnRange.mMins	= -(mSpawnPlaneDistance*1.25f);
		mSpawnRange.mMaxs	=  (mSpawnPlaneDistance*1.25f);

		mGravity			= 300.0f;	// Units Per Second

		mColor				= 1.0f;

		mVertexCount		= 4;
		mWidth				= 1.0f;
		mHeight				= 1.0f;

		mBlendMode			= 0;
		mFilterMode			= 0;

		mFade				= 10.0f;

		mRotation.Clear();
		mRotationDelta		= 0.0f;
		mRotationDeltaTarget= 0.0f;
		mRotationCurrent	= 0.0f;
		mRotationChangeNext	= -1;
		mRotation.mMin		= -0.7f;
		mRotation.mMax		=  0.7f;
		mRotationChangeTimer.mMin = 500;
		mRotationChangeTimer.mMin = 2000;

		mMass.mMin			= 5.0f;
		mMass.mMax			= 10.0f;

		mFrictionInverse	= 0.7f;		// No Friction?
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Constructor - Will setup default values for all data
	////////////////////////////////////////////////////////////////////////////////////
	CWeatherParticleCloud()
	{
		mImage = 0;
		mParticleCount = 0;
		Reset();
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Initialize - Will setup default values for all data
	////////////////////////////////////////////////////////////////////////////////////
	~CWeatherParticleCloud()
	{
		Reset();
	}


	////////////////////////////////////////////////////////////////////////////////////
	// UseSpawnPlane - Check To See If We Should Spawn On A Plane, Or Just Wrap The Box
	////////////////////////////////////////////////////////////////////////////////////
	inline bool	UseSpawnPlane()
	{
		return (mGravity!=0.0f);
	}


	////////////////////////////////////////////////////////////////////////////////////
	// Update - Applies All Physics Forces To All Contained Particles
	////////////////////////////////////////////////////////////////////////////////////
	void		Update()
	{
		CWeatherParticle*	part=0;
		CVec3		partForce;
		CVec3		partMoved;
		CVec3		partToCamera;
		bool		partRendering;
		bool		partOutside;
		bool		partInRange;
		bool		partInView;
		int			particleNum;
		float		particleFade = (mFade * mSecondsElapsed);

/* TODO: Non Global Wind Zones
		CWindZone*	wind=0;
		int			windNum;
		int			windCount = mWindZones.size();
*/

		// Compute Camera
		//----------------
		{
			mCameraPosition = backEnd.viewParms.ori.origin;
			mCameraForward = backEnd.viewParms.ori.axis[0];
			mCameraLeft = backEnd.viewParms.ori.axis[1];
			mCameraDown = backEnd.viewParms.ori.axis[2];
			
			if (mRotationChangeNext!=-1)
			{
				if (mRotationChangeNext==0)
				{
					mRotation.Pick(mRotationDeltaTarget);
					mRotationChangeTimer.Pick(mRotationChangeNext);
					if (mRotationChangeNext<=0)
					{
						mRotationChangeNext = 1;
					}
				}
				mRotationChangeNext--;

				float	RotationDeltaDifference = (mRotationDeltaTarget - mRotationDelta);
				if (fabsf(RotationDeltaDifference)>0.01)
				{
					mRotationDelta += RotationDeltaDifference;		// Blend To New Delta
				}
                mRotationCurrent += (mRotationDelta * mSecondsElapsed);
				float s = sinf(mRotationCurrent);
				float c = cosf(mRotationCurrent);

				CVec3	TempCamLeft(mCameraLeft);

				mCameraLeft *= (c * mWidth);
				mCameraLeft.ScaleAdd(mCameraDown, (s * mWidth * -1.0f));

				mCameraDown *= (c * mHeight);
				mCameraDown.ScaleAdd(TempCamLeft, (s * mHeight));
			}
			else
			{
				mCameraLeft		*= mWidth;
 				mCameraDown		*= mHeight;
			}
		}


		// Compute Global Force
		//----------------------
		CVec3		force;
		{
			force.Clear();

			// Apply Gravity
			//---------------
			force[2] = -1.0f * mGravity;

			// Apply Wind Velocity
			//---------------------
			force    += mGlobalWindVelocity;
		}


		// Update Range
		//--------------
		{
			mRange.mMins = mCameraPosition + mSpawnRange.mMins;
			mRange.mMaxs = mCameraPosition + mSpawnRange.mMaxs;

			// If Using A Spawn Plane, Increase The Range Box A Bit To Account For Rotation On The Spawn Plane
			//-------------------------------------------------------------------------------------------------
			if (UseSpawnPlane())
			{
				for (int dim=0; dim<3; dim++)
				{
					if (force[dim]>0.01)
					{
						mRange.mMins[dim] -= (mSpawnPlaneDistance/2.0f);
					}
					else if (force[dim]<-0.01)
					{
						mRange.mMaxs[dim] += (mSpawnPlaneDistance/2.0f);
					}
				}
				mSpawnPlaneNorm	= force;
				mSpawnSpeed		= VectorNormalize(mSpawnPlaneNorm.v);
				MakeNormalVectors(mSpawnPlaneNorm.v, mSpawnPlaneRight.v, mSpawnPlaneUp.v);
				if (mOrientWithVelocity)
				{
					mCameraDown = mSpawnPlaneNorm;
					mCameraDown *= (mHeight * -1);
				}
			}

			// Optimization For Quad Position Calculation
			//--------------------------------------------
			if (mVertexCount==4)
			{
		 		mCameraLeftPlusUp  = (mCameraLeft - mCameraDown);
				mCameraLeftMinusUp = (mCameraLeft + mCameraDown);
			}
			else
			{
				mCameraLeftPlusUp  = (mCameraDown + mCameraLeft);		// should really be called mCamera Left + Down
			}
		}

		// Stop All Additional Processing
		//--------------------------------
		if (mFrozen)
		{
			return;
		}



		// Now Update All Particles
		//--------------------------
		mParticleCountRender = 0;
		for (particleNum=0; particleNum<mParticleCount; particleNum++)
		{
			part			= &mParticles[particleNum];

			if (!mPopulated)
			{
				mRange.Pick(part->mPosition);		// First Time Spawn Location
			}

			// Grab The Force And Apply Non Global Wind
			//------------------------------------------
			partForce = force;
			partForce /= part->mMass;


			// Apply The Force
			//-----------------
			part->mVelocity		+= partForce;
			part->mVelocity		*= mFrictionInverse;

			part->mPosition.ScaleAdd(part->mVelocity, mSecondsElapsed);

			partToCamera	= (part->mPosition - mCameraPosition);
			partRendering	= part->mFlags.get_bit(CWeatherParticle::FLAG_RENDER);
			partOutside		= mOutside.PointOutside(part->mPosition, mWidth, mHeight);
			partInRange		= mRange.In(part->mPosition);
			partInView		= (partOutside && partInRange && (partToCamera.Dot(mCameraForward)>0.0f));

			// Process Respawn
			//-----------------
			if (!partInRange && !partRendering)
			{
				part->mVelocity.Clear();

				// Reselect A Position On The Spawn Plane
				//----------------------------------------
				if (UseSpawnPlane())
				{
					part->mPosition		= mCameraPosition;
					part->mPosition		-= (mSpawnPlaneNorm* mSpawnPlaneDistance);
					part->mPosition		+= (mSpawnPlaneRight*WE_flrand(-mSpawnPlaneSize, mSpawnPlaneSize));
					part->mPosition		+= (mSpawnPlaneUp*   WE_flrand(-mSpawnPlaneSize, mSpawnPlaneSize));
				}

				// Otherwise, Just Wrap Around To The Other End Of The Range
				//-----------------------------------------------------------
				else
				{
					mRange.Wrap(part->mPosition, mSpawnRange);
				}
				partInRange = true;
			}

			// Process Fade
			//--------------
			{
				// Start A Fade Out
				//------------------
				if		(partRendering && !partInView)
				{
					part->mFlags.clear_bit(CWeatherParticle::FLAG_FADEIN);
					part->mFlags.set_bit(CWeatherParticle::FLAG_FADEOUT);
				}

				// Switch From Fade Out To Fade In
				//---------------------------------
				else if (partRendering && partInView && part->mFlags.get_bit(CWeatherParticle::FLAG_FADEOUT))
				{
					part->mFlags.set_bit(CWeatherParticle::FLAG_FADEIN);
					part->mFlags.clear_bit(CWeatherParticle::FLAG_FADEOUT);
				}

				// Start A Fade In
				//-----------------
				else if (!partRendering && partInView)
				{
					partRendering = true;
					part->mAlpha = 0.0f;
					part->mFlags.set_bit(CWeatherParticle::FLAG_RENDER);
					part->mFlags.set_bit(CWeatherParticle::FLAG_FADEIN);
					part->mFlags.clear_bit(CWeatherParticle::FLAG_FADEOUT);
				}

				// Update Fade
				//-------------
				if (partRendering)
				{

					// Update Fade Out
					//-----------------
					if (part->mFlags.get_bit(CWeatherParticle::FLAG_FADEOUT))
					{
						part->mAlpha -= particleFade;
						if (part->mAlpha<=0.0f)
						{
							part->mAlpha = 0.0f;
							part->mFlags.clear_bit(CWeatherParticle::FLAG_FADEOUT);
							part->mFlags.clear_bit(CWeatherParticle::FLAG_FADEIN);
							part->mFlags.clear_bit(CWeatherParticle::FLAG_RENDER);
							partRendering = false;
						}
					}

					// Update Fade In
					//----------------
					else if (part->mFlags.get_bit(CWeatherParticle::FLAG_FADEIN))
					{
						part->mAlpha += particleFade;
						if (part->mAlpha>=mColor[3])
						{
							part->mFlags.clear_bit(CWeatherParticle::FLAG_FADEIN);
							part->mAlpha = mColor[3];
						}
					}
				}
			}

			// Keep Track Of The Number Of Particles To Render
			//-------------------------------------------------
			if (part->mFlags.get_bit(CWeatherParticle::FLAG_RENDER))
			{
				mParticleCountRender ++;
			}
		}
		mPopulated = true;
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Render -
	////////////////////////////////////////////////////////////////////////////////////
	void		Render()
	{
		CWeatherParticle*	part=0;
		int			particleNum;

		switch (mBlendMode)
		{
		case 2:
			FBO_Bind(tr.renderFbo);
			break;
		case 0:
		case 1:
		default:
			break;
		}

		shaderProgram_t *shader = &tr.weatherShader;
		GLSL_BindProgram(shader);
		GLSL_SetUniformMatrix16(shader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		GLSL_SetUniformVec4(shader, UNIFORM_COLOR, colorWhite);

		GLSL_SetUniformInt(shader, UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GL_BindToTMU(mImage, TB_DIFFUSEMAP);

		vec3_t norm;
		VectorSubtract(vec3_origin, backEnd.viewParms.ori.axis[0], norm);
		VectorNormalize(norm);

		vec4_t l0;
		VectorSet4(l0, norm[0], norm[1], norm[2], (mBlendMode == 2) ? 1.0 : 0.0);
		GLSL_SetUniformVec4(shader, UNIFORM_LOCAL0, l0);

		// Set The GL State And Image Binding
		//------------------------------------
		//GL_Cull(CT_TWO_SIDED);
		GL_Cull(CT_FRONT_SIDED);

		uint32_t blendMode = 0;

		switch (mBlendMode)
		{
		case 1:
			blendMode = (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_LESS | GLS_ATEST_GT_0 /*| GLS_DEPTHMASK_TRUE*/);
			break;
		case 2:
			blendMode = (GLS_ALPHA | GLS_DEPTHFUNC_LESS | GLS_ATEST_GE_128 | GLS_DEPTHMASK_TRUE);
			break;
		case 0:
		default:
			blendMode = (GLS_ALPHA | GLS_DEPTHFUNC_LESS | GLS_ATEST_GT_0 /*| GLS_DEPTHMASK_TRUE*/);
			break;
		}
		
		GL_State(blendMode);

		// Enable And Disable Things
		//---------------------------

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mFilterMode == 0) ? (GL_LINEAR) : (GL_NEAREST));
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (mFilterMode == 0) ? (GL_LINEAR) : (GL_NEAREST));

		switch (mBlendMode)
		{
		case 2:
			qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			qglDepthMask(GL_TRUE);
			break;
		case 0:
		case 1:
		default:
			qglDepthMask(GL_FALSE);
			break;
		}

		tess.numVertexes = 0;
		tess.numIndexes = 0;
		tess.firstIndex = 0;

		tess.minIndex = 0;

		// Begin
		//-------
		for (particleNum = 0; particleNum < mParticleCount; particleNum++)
		{
			part = &(mParticles[particleNum]);

			if (!part->mFlags.get_bit(CWeatherParticle::FLAG_RENDER))
			{
				continue;
			}

			//vec4_t particleColor;

			// Blend Mode Zero -> Apply Alpha Just To Alpha Channel
			//------------------------------------------------------
			if (mBlendMode == 0)
			{
				qglColor4f(mColor[0], mColor[1], mColor[2], part->mAlpha);
			}
			// Let GLSL handle the alpha
			//---------------------------------------
			else if (mBlendMode == 2)
			{
				qglColor4f(mColor[0], mColor[1], mColor[2], mColor[3]);
			}
			// Otherwise Apply Alpha To All Channels
			//---------------------------------------
			else
			{
				qglColor4f(mColor[0] * part->mAlpha, mColor[1] * part->mAlpha, mColor[2] * part->mAlpha, mColor[3] * part->mAlpha);
			}

			if (mVertexCount == 3)
			{
				//-------------------
				// Render A Triangle
				//-------------------

				if (tess.numVertexes + 3 >= SHADER_MAX_VERTEXES || tess.numIndexes + 3 >= SHADER_MAX_INDEXES)
				{// Would go over the limit, render current queue and continue...
					switch (mBlendMode)
					{
					case 2:
						RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
						GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
						break;
					case 0:
					case 1:
					default:
						RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0);// | ATTR_COLOR | ATTR_NORMAL);
						GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0);// | ATTR_COLOR | ATTR_NORMAL);
						break;
					}
					
					R_DrawElementsVBO(tess.numIndexes, tess.firstIndex, tess.minIndex, tess.maxIndex, tess.numVertexes, qfalse);

					tess.numIndexes = 0;
					tess.numVertexes = 0;
					tess.firstIndex = 0;
					tess.minIndex = 0;
					tess.maxIndex = 0;
					tess.useInternalVBO = qfalse;
				}

				int ndx = tess.numVertexes;
				int idx = tess.numIndexes;
				vec2_t texCoords[3];
				vec4_t triVerts[3];
				//vec3_t normal;

				// triangle indexes for a simple tri
				tess.indexes[idx] = ndx;
				tess.indexes[idx + 1] = ndx + 1;
				tess.indexes[idx + 2] = ndx + 2;

				VectorSet2(texCoords[0], 1.0f, 0.0f);
				VectorSet4(triVerts[0],
					part->mPosition[0],
					part->mPosition[1],
					part->mPosition[2],
					1.0);

				VectorSet2(texCoords[1], 0.0f, 1.0f);
				VectorSet4(triVerts[1],
					part->mPosition[0] + mCameraLeft[0],
					part->mPosition[1] + mCameraLeft[1],
					part->mPosition[2] + mCameraLeft[2],
					1.0);

				VectorSet2(texCoords[2], 0.0f, 0.0f);
				VectorSet4(triVerts[2],
					part->mPosition[0] + mCameraLeftPlusUp[0],
					part->mPosition[1] + mCameraLeftPlusUp[1],
					part->mPosition[2] + mCameraLeftPlusUp[2],
					1.0);

#if 0
				// constant normal all the way around
				VectorSubtract(vec3_origin, backEnd.viewParms.ori.axis[0], normal);

				/*float* a = (float *)triVerts[0];
				float* b = (float *)triVerts[1];
				float* c = (float *)triVerts[2];
				vec3_t ba, ca;
				VectorSubtract(b, a, ba);
				VectorSubtract(c, a, ca);

				CrossProduct(ca, ba, normal);
				VectorNormalize(normal);

				normal[0] = normal[0] * 0.5 + 0.5;
				normal[1] = normal[1] * 0.5 + 0.5;
				normal[2] = normal[2] * 0.5 + 0.5;*/

				//VectorSubtract(triVerts[0], backEnd.refdef.vieworg, normal);
				VectorNormalize(normal);

				tess.normal[ndx] =
					tess.normal[ndx + 1] =
					tess.normal[ndx + 2] = R_VboPackNormal(normal);
#endif

				VectorCopy4(triVerts[0], tess.xyz[ndx]);
				VectorCopy4(triVerts[1], tess.xyz[ndx + 1]);
				VectorCopy4(triVerts[2], tess.xyz[ndx + 2]);

				// standard square texture coordinates
				VectorCopy2(texCoords[0], tess.texCoords[ndx][0]);
				VectorCopy2(texCoords[0], tess.texCoords[ndx][1]);

				VectorCopy2(texCoords[1], tess.texCoords[ndx + 1][0]);
				VectorCopy2(texCoords[1], tess.texCoords[ndx + 1][1]);

				VectorCopy2(texCoords[2], tess.texCoords[ndx + 2][0]);
				VectorCopy2(texCoords[2], tess.texCoords[ndx + 2][1]);

				// constant particleColor all the way around
				/*VectorCopy4(particleColor, tess.vertexColors[ndx]);
				VectorCopy4(particleColor, tess.vertexColors[ndx + 1]);
				VectorCopy4(particleColor, tess.vertexColors[ndx + 2]);*/

				tess.maxIndex = ndx + 3;

				tess.numVertexes += 3;
				tess.numIndexes += 3;
				tess.useInternalVBO = qtrue;
			}
			else
			{
				//-------------------
				// Render A Quad
				//-------------------

				if (tess.numVertexes + 4 >= SHADER_MAX_VERTEXES || tess.numIndexes + 6 >= SHADER_MAX_INDEXES)
				{// Would go over the limit, render current queue and continue...
					switch (mBlendMode)
					{
					case 2:
						RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
						GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
						break;
					case 0:
					case 1:
					default:
						RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0);// | ATTR_COLOR | ATTR_NORMAL);
						GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0);// | ATTR_COLOR | ATTR_NORMAL);
						break;
					}

					R_DrawElementsVBO(tess.numIndexes, tess.firstIndex, tess.minIndex, tess.maxIndex, tess.numVertexes, qfalse);

					tess.numIndexes = 0;
					tess.numVertexes = 0;
					tess.firstIndex = 0;
					tess.minIndex = 0;
					tess.maxIndex = 0;
					tess.useInternalVBO = qfalse;
				}

				int ndx = tess.numVertexes;
				int idx = tess.numIndexes;
				vec2_t texCoords[4];
				vec4_t quadVerts[4];
				//vec3_t normal;

				// triangle indexes for a simple quad
				tess.indexes[idx] = ndx;
				tess.indexes[idx + 1] = ndx + 1;
				tess.indexes[idx + 2] = ndx + 3;
				tess.indexes[idx + 3] = ndx + 3;
				tess.indexes[idx + 4] = ndx + 1;
				tess.indexes[idx + 5] = ndx + 2;

				// Left bottom.
				VectorSet2(texCoords[3], 0.0f, 0.0f);
				VectorSet4(quadVerts[1],
					part->mPosition[0] - mCameraLeftMinusUp[0],
					part->mPosition[1] - mCameraLeftMinusUp[1],
					part->mPosition[2] - mCameraLeftMinusUp[2],
					1.0);

				// Right bottom.
				VectorSet2(texCoords[0], 1.0f, 0.0f);
				VectorSet4(quadVerts[0],
					part->mPosition[0] - mCameraLeftPlusUp[0],
					part->mPosition[1] - mCameraLeftPlusUp[1],
					part->mPosition[2] - mCameraLeftPlusUp[2],
					1.0);

				// Right top.
				VectorSet2(texCoords[1], 1.0f, 1.0f);
				VectorSet4(quadVerts[3],
					part->mPosition[0] + mCameraLeftMinusUp[0],
					part->mPosition[1] + mCameraLeftMinusUp[1],
					part->mPosition[2] + mCameraLeftMinusUp[2],
					1.0);

				// Left top.
				VectorSet2(texCoords[2], 0.0f, 1.0f);
				VectorSet4(quadVerts[2],
					part->mPosition[0] + mCameraLeftPlusUp[0],
					part->mPosition[1] + mCameraLeftPlusUp[1],
					part->mPosition[2] + mCameraLeftPlusUp[2],
					1.0);

#if 0
				// constant normal all the way around
				VectorSubtract(vec3_origin, backEnd.viewParms.ori.axis[0], normal);

				/*
				float* a = (float *)quadVerts[2];
				float* b = (float *)quadVerts[1];
				float* c = (float *)quadVerts[0];
				vec3_t ba, ca;
				VectorSubtract(b, a, ba);
				VectorSubtract(c, a, ca);

				CrossProduct(ca, ba, normal);
				VectorNormalize(normal);

				normal[0] = normal[0] * 0.5 + 0.5;
				normal[1] = normal[1] * 0.5 + 0.5;
				normal[2] = normal[2] * 0.5 + 0.5;*/

				//VectorSubtract(quadVerts[0], backEnd.refdef.vieworg, normal);
				VectorNormalize(normal);

				tess.normal[ndx] =
					tess.normal[ndx + 1] =
					tess.normal[ndx + 2] =
					tess.normal[ndx + 3] = R_VboPackNormal(normal);
#endif

				VectorCopy4(quadVerts[0], tess.xyz[ndx]);
				VectorCopy4(quadVerts[1], tess.xyz[ndx + 1]);
				VectorCopy4(quadVerts[2], tess.xyz[ndx + 2]);
				VectorCopy4(quadVerts[3], tess.xyz[ndx + 3]);

				// standard square texture coordinates
				VectorCopy2(texCoords[0], tess.texCoords[ndx][0]);
				VectorCopy2(texCoords[0], tess.texCoords[ndx][1]);

				VectorCopy2(texCoords[1], tess.texCoords[ndx + 1][0]);
				VectorCopy2(texCoords[1], tess.texCoords[ndx + 1][1]);

				VectorCopy2(texCoords[2], tess.texCoords[ndx + 2][0]);
				VectorCopy2(texCoords[2], tess.texCoords[ndx + 2][1]);

				VectorCopy2(texCoords[3], tess.texCoords[ndx + 3][0]);
				VectorCopy2(texCoords[3], tess.texCoords[ndx + 3][1]);

				// constant particleColor all the way around
				/*VectorCopy4(particleColor, tess.vertexColors[ndx]);
				VectorCopy4(particleColor, tess.vertexColors[ndx + 1]);
				VectorCopy4(particleColor, tess.vertexColors[ndx + 2]);
				VectorCopy4(particleColor, tess.vertexColors[ndx + 3]);*/

				tess.maxIndex = ndx + 3;

				tess.numVertexes += 4;
				tess.numIndexes += 6;
				tess.useInternalVBO = qtrue;
			}
		}

		switch (mBlendMode)
		{
		case 2:
			RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
			GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0 | ATTR_NORMAL);
			break;
		case 0:
		case 1:
		default:
			RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0);// | ATTR_COLOR | ATTR_NORMAL);
			GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0);// | ATTR_COLOR | ATTR_NORMAL);
			break;
		}

		R_DrawElementsVBO(tess.numIndexes, tess.firstIndex, tess.minIndex, tess.maxIndex, tess.numVertexes, qfalse);

		//
		tess.numIndexes = 0;
		tess.numVertexes = 0;
		tess.firstIndex = 0;
		tess.minIndex = 0;
		tess.maxIndex = 0;
		tess.useInternalVBO = qfalse;

		mParticlesRendered += mParticleCountRender;

		GL_State(GLS_DEFAULT);
		GL_Cull(CT_FRONT_SIDED);
	}
};
ratl::vector_vs<CWeatherParticleCloud, MAX_PARTICLE_CLOUDS>	mParticleClouds;



////////////////////////////////////////////////////////////////////////////////////////
// Init World Effects - Will Iterate Over All Particle Clouds, Clear Them Out, And Erase
////////////////////////////////////////////////////////////////////////////////////////
void R_InitWorldEffects(void)
{
	srand(ri->Milliseconds());

	for (int i=0; i<mParticleClouds.size(); i++)
	{
		mParticleClouds[i].Reset();
	}
	mParticleClouds.clear();
	mWindZones.clear();
	mOutside.Reset();
}

////////////////////////////////////////////////////////////////////////////////////////
// Init World Effects - Will Iterate Over All Particle Clouds, Clear Them Out, And Erase
////////////////////////////////////////////////////////////////////////////////////////
void R_ShutdownWorldEffects(void)
{
	R_InitWorldEffects();
}

extern vec3_t  MAP_INFO_MINS;
extern vec3_t  MAP_INFO_MAXS;

qboolean WEATHER_KLUDGE_DONE = qfalse;

void RB_SetupGlobalWeatherZone(void)
{
	mOutside.AddWeatherZone(MAP_INFO_MINS, MAP_INFO_MAXS);
	WEATHER_KLUDGE_DONE = qtrue;
}

qboolean RB_WeatherEnabled(void)
{
	if (!r_weather->integer
		|| !tr.world
		|| (tr.refdef.rdflags & RDF_NOWORLDMODEL)
		|| (backEnd.refdef.rdflags & RDF_SKYBOXPORTAL)
		|| !mParticleClouds.size())
	{	//  no world rendering or no world or no particle clouds
		return qfalse;
	}

	return qtrue;
}

////////////////////////////////////////////////////////////////////////////////////////
// RB_RenderWorldEffects - If any particle clouds exist, this will update and render them
////////////////////////////////////////////////////////////////////////////////////////
void RB_RenderWorldEffects(void)
{
	if (!WEATHER_KLUDGE_DONE)
	{
		if (r_weather->integer >= 2 && !(JKA_WEATHER_ENABLED && !CONTENTS_INSIDE_OUTSIDE_FOUND))
		{// Make sure we always have a weather zone covering the whole map, if mapInfo says JKA weather is enabled or in forced mode (r_weather->integer >= 2)...
			RB_SetupGlobalWeatherZone();
		}

		WEATHER_KLUDGE_DONE = qtrue;
	}

	if (!tr.world ||
		(tr.refdef.rdflags & RDF_NOWORLDMODEL) ||
		(backEnd.refdef.rdflags & RDF_SKYBOXPORTAL) ||
		!mParticleClouds.size())
	{	//  no world rendering or no world or no particle clouds
		return;
	}

	// Calculate Elapsed Time For Scale Purposes
	//-------------------------------------------
	mMillisecondsElapsed = backEnd.refdef.floatTime;
	if (mMillisecondsElapsed<1)
	{
		mMillisecondsElapsed = 1.0f;
	}
	if (mMillisecondsElapsed>1000.0f)
	{
		mMillisecondsElapsed = 1000.0f;
	}
	mSecondsElapsed = (mMillisecondsElapsed / 1000.0f);


	// Make Sure We Are Always Outside Cached
	//----------------------------------------
	if (!mOutside.Initialized())
	{
		mOutside.Cache();
	}
	else
	{
		// Update All Wind Zones
		//-----------------------
		if (!mFrozen)
		{
			mGlobalWindVelocity.Clear();
			for (int wz=0; wz<mWindZones.size(); wz++)
			{
				mWindZones[wz].Update();
				if (mWindZones[wz].mGlobal)
				{
					mGlobalWindVelocity += mWindZones[wz].mCurrentVelocity;
				}
			}
			mGlobalWindDirection	= mGlobalWindVelocity;
			mGlobalWindSpeed		= VectorNormalize(mGlobalWindDirection.v);
		}

		// Update All Particle Clouds
		//----------------------------
		mParticlesRendered = 0;

		if (mParticleClouds.size())
		{
			FBO_Bind(tr.renderNoDepthFbo);
			//FBO_Bind(tr.renderFbo);
			SetViewportAndScissor();
			GL_SetModelviewMatrix(backEnd.viewParms.world.modelViewMatrix);

			for (int i = 0; i < mParticleClouds.size(); i++)
			{
				mParticleClouds[i].Update();
				mParticleClouds[i].Render();
			}
		}
		
		if (false)
		{
			ri->Printf( PRINT_ALL, "Weather: %d Particles Rendered\n", mParticlesRendered);
		}
	}
}


void R_WorldEffect_f(void)
{
	char temp[2048] = {0};
	ri->Cmd_ArgsBuffer( temp, sizeof( temp ) );
	RE_WorldEffectCommand( temp );
}

/*
===============
WE_ParseVector
===============
*/
qboolean WE_ParseVector( const char **text, int count, float *v ) {
	char	*token;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) {
		ri->Printf (PRINT_WARNING, "WARNING: missing parenthesis in weather effect\n" );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) {
			ri->Printf (PRINT_WARNING, "WARNING: missing vector element in weather effect\n" );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) {
		ri->Printf (PRINT_WARNING, "WARNING: missing parenthesis in weather effect\n" );
		return qfalse;
	}

	return qtrue;
}

void RE_WorldEffectCommand_REAL(const char *command, qboolean noHelp)
{
	if ( !command )
	{
		return;
	}

	COM_BeginParseSession ("RE_WorldEffectCommand");

	const char	*token;//, *origCommand;

	token = COM_ParseExt(&command, qfalse);

	if ( !token )
	{
		return;
	}


	//Die - clean up the whole weather system -rww
	if (Q_stricmp(token, "die") == 0)
	{
		R_ShutdownWorldEffects();
		return;
	}

	// Clear - Removes All Particle Clouds And Wind Zones
	//----------------------------------------------------
	else if (Q_stricmp(token, "clear") == 0)
	{
		for (int p=0; p<mParticleClouds.size(); p++)
		{
			mParticleClouds[p].Reset();
		}
		mParticleClouds.clear();
		mWindZones.clear();
	}

	// Freeze / UnFreeze - Stops All Particle Motion Updates
	//--------------------------------------------------------
	else if (Q_stricmp(token, "freeze") == 0)
	{
		mFrozen = !mFrozen;
	}

	// Add a zone
	//---------------
	else if (Q_stricmp(token, "zone") == 0)
	{
		vec3_t	mins;
		vec3_t	maxs;
		if (WE_ParseVector(&command, 3, mins) && WE_ParseVector(&command, 3, maxs))
		{
			mOutside.AddWeatherZone(mins, maxs);
		}
	}

	// Basic Wind
	//------------
	else if (Q_stricmp(token, "wind") == 0)
	{
		if (mWindZones.full())
		{
			return;
		}
		CWindZone& nWind = mWindZones.push_back();
		nWind.Initialize();
	}

	// Constant Wind
	//---------------
	else if (Q_stricmp(token, "constantwind") == 0)
	{
		if (mWindZones.full())
		{
			return;
		}
		CWindZone& nWind = mWindZones.push_back();
		nWind.Initialize();
		if (!WE_ParseVector(&command, 3, nWind.mCurrentVelocity.v))
		{
			nWind.mCurrentVelocity.Clear();
			nWind.mCurrentVelocity[1] = 800.0f;
		}
		nWind.mTargetVelocityTimeRemaining = -1;
	}

	// Gusting Wind
	//--------------
	else if (Q_stricmp(token, "gustingwind") == 0)
	{
		if (mWindZones.full())
		{
			return;
		}
		CWindZone& nWind = mWindZones.push_back();
		nWind.Initialize();
		nWind.mRVelocity.mMins				= -3000.0f;
		nWind.mRVelocity.mMins[2]			= -100.0f;
		nWind.mRVelocity.mMaxs				=  3000.0f;
		nWind.mRVelocity.mMaxs[2]			=  100.0f;

		nWind.mMaxDeltaVelocityPerUpdate	=  10.0f;

		nWind.mRDuration.mMin				=  1000;
		nWind.mRDuration.mMax				=  3000;

		nWind.mChanceOfDeadTime				=  0.5f;
		nWind.mRDeadTime.mMin				=  2000;
		nWind.mRDeadTime.mMax				=  4000;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "lightrain") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/world/vividrain.png", 4);
		nCloud.mHeight = 32.0;
		nCloud.mWidth = 32.0;
		nCloud.mGravity = 2800.0f;
		nCloud.mFilterMode = 0;
		nCloud.mBlendMode = 0;
		nCloud.mFade = 100.0f;
		nCloud.mColor = 3.0f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "rain") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/world/vividrain.png", 4);
		nCloud.mHeight = 48.0;
		nCloud.mWidth = 48.0;
		nCloud.mGravity = 2800.0f;
		nCloud.mFilterMode = 0;
		nCloud.mBlendMode = 0;
		nCloud.mFade = 100.0f;
		nCloud.mColor = 3.0f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "heavyrain") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(2000, "gfx/world/vividrain.png", 4);
		nCloud.mHeight = 96.0;
		nCloud.mWidth = 96.0;
		nCloud.mGravity = 2800.0f;
		nCloud.mFilterMode = 0;
		nCloud.mBlendMode = 0;
		nCloud.mFade = 100.0f;
		nCloud.mColor = 3.0f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "rainstorm") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(2000, "gfx/world/vividrain.png", 4);
		nCloud.mHeight = 128.0;
		nCloud.mWidth = 128.0;
		nCloud.mGravity = 2800.0f;
		nCloud.mFilterMode = 0;
		nCloud.mBlendMode = 0;
		nCloud.mFade = 100.0f;
		nCloud.mColor = 3.0f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

#if 0
	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "lightrain2") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/world/vividrain2.png", 4);
		nCloud.mHeight = 16.0;
		nCloud.mWidth = 8.0;
		nCloud.mGravity = 2000.0f;
		nCloud.mFilterMode = 0;
		nCloud.mBlendMode = 0;
		nCloud.mFade = 100.0f;
		nCloud.mColor = 3.0f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "rain2") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/world/vividrain2.png", 4);
		nCloud.mHeight = 18.0;
		nCloud.mWidth = 9.0;
		nCloud.mGravity = 2000.0f;
		nCloud.mFilterMode = 0;
		nCloud.mBlendMode = 0;
		nCloud.mFade = 100.0f;
		nCloud.mColor = 3.0f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "heavyrain2") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(2000, "gfx/world/vividrain2.png", 4);
		nCloud.mHeight = 24.0;
		nCloud.mWidth = 12.0;
		nCloud.mGravity = 2800.0f;
		nCloud.mFilterMode = 0;
		nCloud.mBlendMode = 0;
		nCloud.mFade = 100.0f;
		nCloud.mColor = 3.0f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "rainstorm2") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(2000, "gfx/world/vividrain2.png", 4);
		nCloud.mHeight = 28.0;
		nCloud.mWidth = 14.0;
		nCloud.mGravity = 2800.0f;
		nCloud.mFilterMode = 0;
		nCloud.mBlendMode = 0;
		nCloud.mFade = 100.0f;
		nCloud.mColor = 3.0f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}
#endif

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "oldlightrain") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(500, "gfx/world/rain.jpg", 3);
		nCloud.mHeight		= 80.0f;
		nCloud.mWidth		= 1.2f;
		nCloud.mGravity		= 2000.0f;
		nCloud.mFilterMode	= 1;
		nCloud.mBlendMode	= 1;
		nCloud.mFade		= 100.0f;
		nCloud.mColor		= 0.5f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "oldrain") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/world/rain.jpg", 3);
		nCloud.mHeight		= 80.0f;
		nCloud.mWidth		= 1.2f;
		nCloud.mGravity		= 2000.0f;
		nCloud.mFilterMode	= 1;
		nCloud.mBlendMode	= 1;
		nCloud.mFade		= 100.0f;
		nCloud.mColor		= 0.5f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "acidrain") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/world/rain.jpg", 3);
		nCloud.mHeight		= 80.0f;
		nCloud.mWidth		= 2.0f;
		nCloud.mGravity		= 2000.0f;
		nCloud.mFilterMode	= 1;
		nCloud.mBlendMode	= 1;
		nCloud.mFade		= 100.0f;

		nCloud.mColor[0]	= 0.34f;
		nCloud.mColor[1]	= 0.70f;
		nCloud.mColor[2]	= 0.34f;
		nCloud.mColor[3]	= 0.70f;

		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;

		mOutside.mOutsidePain = 0.1f;
	}

	// Create A Rain Storm
	//---------------------
	else if (Q_stricmp(token, "oldheavyrain") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/world/rain.jpg", 3);
		nCloud.mHeight		= 80.0f;
		nCloud.mWidth		= 1.2f;
		nCloud.mGravity		= 2800.0f;
		nCloud.mFilterMode	= 1;
		nCloud.mBlendMode	= 1;
		nCloud.mFade		= 15.0f;
		nCloud.mColor		= 0.5f;
		nCloud.mOrientWithVelocity = true;
		nCloud.mWaterParticles = true;
	}

	// Create Falling Leafs
	//---------------------
	else if (Q_stricmp(token, "fallingleafs") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}

#define FALLING_LEAFS_NUM				2//r_testvalue0->integer//20
#define FALLING_LEAFS_SIZE				4.0//r_testvalue1->value
#define FALLING_LEAFS_DELTA				0//r_testvalue2->value
#define FALLING_LEAFS_DELTA_TARGET		0//r_testvalue3->value

#define FALLING_LEAFS_GRAVITY			80.0//5.0//r_testshaderValue1->value//200
#define FALLING_LEAFS_COLOR				3.0//r_testshaderValue2->value//3.0f
#define FALLING_LEAFS_FADE				100.0//255.0//r_testshaderValue3->value//100.0f

		vec4_t wind;
		wind[0] = 50.0;// r_testshaderValue4->value;
		wind[1] = 50.0;// r_testshaderValue5->value;
		wind[2] = 50.0;// r_testshaderValue6->value;
		wind[3] = 10.0;// r_testshaderValue7->value;

		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(FALLING_LEAFS_NUM, "gfx/world/fallingleaf01.png", 4);
		nCloud.mHeight = FALLING_LEAFS_SIZE;
		nCloud.mWidth = FALLING_LEAFS_SIZE;
		nCloud.mGravity = FALLING_LEAFS_GRAVITY;
		nCloud.mFilterMode = 0;
		nCloud.mBlendMode = 2;
		nCloud.mFade = FALLING_LEAFS_FADE;
		nCloud.mColor = FALLING_LEAFS_COLOR;
		nCloud.mRotationChangeNext = 3;
		nCloud.mOrientWithVelocity = false;
		nCloud.mWaterParticles = false;
		nCloud.mRotationDelta = FALLING_LEAFS_DELTA;
		nCloud.mRotationDeltaTarget = FALLING_LEAFS_DELTA_TARGET;

		CWeatherParticleCloud& nCloud2 = mParticleClouds.push_back();
		nCloud2.Initialize(FALLING_LEAFS_NUM, "gfx/world/fallingleaf02.png", 4);
		nCloud2.mHeight = FALLING_LEAFS_SIZE;
		nCloud2.mWidth = FALLING_LEAFS_SIZE;
		nCloud2.mGravity = FALLING_LEAFS_GRAVITY;
		nCloud2.mFilterMode = 0;
		nCloud2.mBlendMode = 2;
		nCloud2.mFade = FALLING_LEAFS_FADE;
		nCloud2.mColor = FALLING_LEAFS_COLOR;
		nCloud2.mRotationChangeNext = 3;
		nCloud2.mOrientWithVelocity = false;
		nCloud2.mWaterParticles = false;
		nCloud2.mRotationDelta = FALLING_LEAFS_DELTA;
		nCloud2.mRotationDeltaTarget = FALLING_LEAFS_DELTA_TARGET;

		CWeatherParticleCloud& nCloud3 = mParticleClouds.push_back();
		nCloud3.Initialize(FALLING_LEAFS_NUM, "gfx/world/fallingleaf03.png", 4);
		nCloud3.mHeight = FALLING_LEAFS_SIZE;
		nCloud3.mWidth = FALLING_LEAFS_SIZE;
		nCloud3.mGravity = FALLING_LEAFS_GRAVITY;
		nCloud3.mFilterMode = 0;
		nCloud3.mBlendMode = 2;
		nCloud3.mFade = FALLING_LEAFS_FADE;
		nCloud3.mColor = FALLING_LEAFS_COLOR;
		nCloud3.mRotationChangeNext = 3;
		nCloud3.mOrientWithVelocity = false;
		nCloud3.mWaterParticles = false;
		nCloud3.mRotationDelta = FALLING_LEAFS_DELTA;
		nCloud3.mRotationDeltaTarget = FALLING_LEAFS_DELTA_TARGET;

		CWeatherParticleCloud& nCloud4 = mParticleClouds.push_back();
		nCloud4.Initialize(FALLING_LEAFS_NUM, "gfx/world/fallingleaf04.png", 4);
		nCloud4.mHeight = FALLING_LEAFS_SIZE;
		nCloud4.mWidth = FALLING_LEAFS_SIZE;
		nCloud4.mGravity = FALLING_LEAFS_GRAVITY;
		nCloud4.mFilterMode = 0;
		nCloud4.mBlendMode = 2;
		nCloud4.mFade = FALLING_LEAFS_FADE;
		nCloud4.mColor = FALLING_LEAFS_COLOR;
		nCloud4.mRotationChangeNext = 3;
		nCloud4.mOrientWithVelocity = false;
		nCloud4.mWaterParticles = false;
		nCloud4.mRotationDelta = FALLING_LEAFS_DELTA;
		nCloud4.mRotationDeltaTarget = FALLING_LEAFS_DELTA_TARGET;

		CWeatherParticleCloud& nCloud5 = mParticleClouds.push_back();
		nCloud5.Initialize(FALLING_LEAFS_NUM, "gfx/world/fallingleaf05.png", 4);
		nCloud5.mHeight = FALLING_LEAFS_SIZE;
		nCloud5.mWidth = FALLING_LEAFS_SIZE;
		nCloud5.mGravity = FALLING_LEAFS_GRAVITY;
		nCloud5.mFilterMode = 0;
		nCloud5.mBlendMode = 2;
		nCloud5.mFade = FALLING_LEAFS_FADE;
		nCloud5.mColor = FALLING_LEAFS_COLOR;
		nCloud5.mRotationChangeNext = 3;
		nCloud5.mOrientWithVelocity = false;
		nCloud5.mWaterParticles = false;
		nCloud5.mRotationDelta = FALLING_LEAFS_DELTA;
		nCloud5.mRotationDeltaTarget = FALLING_LEAFS_DELTA_TARGET;

		CWeatherParticleCloud& nCloud6 = mParticleClouds.push_back();
		nCloud6.Initialize(FALLING_LEAFS_NUM, "gfx/world/fallingleaf06.png", 4);
		nCloud6.mHeight = FALLING_LEAFS_SIZE;
		nCloud6.mWidth = FALLING_LEAFS_SIZE;
		nCloud6.mGravity = FALLING_LEAFS_GRAVITY;
		nCloud6.mFilterMode = 0;
		nCloud6.mBlendMode = 2;
		nCloud6.mFade = FALLING_LEAFS_FADE;
		nCloud6.mColor = FALLING_LEAFS_COLOR;
		nCloud6.mRotationChangeNext = 3;
		nCloud6.mOrientWithVelocity = false;
		nCloud6.mWaterParticles = false;
		nCloud6.mRotationDelta = FALLING_LEAFS_DELTA;
		nCloud6.mRotationDeltaTarget = FALLING_LEAFS_DELTA_TARGET;

		CWeatherParticleCloud& nCloud7 = mParticleClouds.push_back();
		nCloud7.Initialize(FALLING_LEAFS_NUM, "gfx/world/fallingleaf07.png", 4);
		nCloud7.mHeight = FALLING_LEAFS_SIZE;
		nCloud7.mWidth = FALLING_LEAFS_SIZE;
		nCloud7.mGravity = FALLING_LEAFS_GRAVITY;
		nCloud7.mFilterMode = 0;
		nCloud7.mBlendMode = 2;
		nCloud7.mFade = FALLING_LEAFS_FADE;
		nCloud7.mColor = FALLING_LEAFS_COLOR;
		nCloud7.mRotationChangeNext = 3;
		nCloud7.mOrientWithVelocity = false;
		nCloud7.mWaterParticles = false;
		nCloud7.mRotationDelta = FALLING_LEAFS_DELTA;
		nCloud7.mRotationDeltaTarget = FALLING_LEAFS_DELTA_TARGET;

		CWeatherParticleCloud& nCloud8 = mParticleClouds.push_back();
		nCloud8.Initialize(FALLING_LEAFS_NUM, "gfx/world/fallingleaf08.png", 4);
		nCloud8.mHeight = FALLING_LEAFS_SIZE;
		nCloud8.mWidth = FALLING_LEAFS_SIZE;
		nCloud8.mGravity = FALLING_LEAFS_GRAVITY;
		nCloud8.mFilterMode = 0;
		nCloud8.mBlendMode = 2;
		nCloud8.mFade = FALLING_LEAFS_FADE;
		nCloud8.mColor = FALLING_LEAFS_COLOR;
		nCloud8.mRotationChangeNext = 3;
		nCloud8.mOrientWithVelocity = false;
		nCloud8.mWaterParticles = false;
		nCloud8.mRotationDelta = FALLING_LEAFS_DELTA;
		nCloud8.mRotationDeltaTarget = FALLING_LEAFS_DELTA_TARGET;

		CWeatherParticleCloud& nCloud9 = mParticleClouds.push_back();
		nCloud9.Initialize(FALLING_LEAFS_NUM, "gfx/world/fallingleaf09.png", 4);
		nCloud9.mHeight = FALLING_LEAFS_SIZE;
		nCloud9.mWidth = FALLING_LEAFS_SIZE;
		nCloud9.mGravity = FALLING_LEAFS_GRAVITY;
		nCloud9.mFilterMode = 0;
		nCloud9.mBlendMode = 2;
		nCloud9.mFade = FALLING_LEAFS_FADE;
		nCloud9.mColor = FALLING_LEAFS_COLOR;
		nCloud9.mRotationChangeNext = 3;
		nCloud9.mOrientWithVelocity = false;
		nCloud9.mWaterParticles = false;
		nCloud9.mRotationDelta = FALLING_LEAFS_DELTA;
		nCloud9.mRotationDeltaTarget = FALLING_LEAFS_DELTA_TARGET;

		CWeatherParticleCloud& nCloud10 = mParticleClouds.push_back();
		nCloud10.Initialize(FALLING_LEAFS_NUM, "gfx/world/fallingleaf10.png", 4);
		nCloud10.mHeight = FALLING_LEAFS_SIZE;
		nCloud10.mWidth = FALLING_LEAFS_SIZE;
		nCloud10.mGravity = FALLING_LEAFS_GRAVITY;
		nCloud10.mFilterMode = 0;
		nCloud10.mBlendMode = 2;
		nCloud10.mFade = FALLING_LEAFS_FADE;
		nCloud10.mColor = FALLING_LEAFS_COLOR;
		nCloud10.mRotationChangeNext = 3;
		nCloud10.mOrientWithVelocity = false;
		nCloud10.mWaterParticles = false;
		nCloud10.mRotationDelta = FALLING_LEAFS_DELTA;
		nCloud10.mRotationDeltaTarget = FALLING_LEAFS_DELTA_TARGET;

		CWindZone& nWind = mWindZones.push_back();
		nWind.Initialize();
		nWind.mCurrentVelocity.v[0] = wind[1];// 20;
		nWind.mCurrentVelocity.v[1] = wind[2];// 100;
		nWind.mCurrentVelocity.v[2] = wind[3];//20;
		nWind.mCurrentVelocity.Clear();
		nWind.mCurrentVelocity[1] = wind[3];// 800.0f;
		nWind.mTargetVelocityTimeRemaining = -1;
	}

	// Create A Snow Storm
	//---------------------
	else if (Q_stricmp(token, "snow") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(1000, "gfx/effects/snowflake1.bmp");
		nCloud.mBlendMode			= 1;
		nCloud.mRotationChangeNext	= 0;
		nCloud.mColor		= 0.75f;
		nCloud.mWaterParticles = true;
	}

	// Create A Some stuff
	//---------------------
	else if (Q_stricmp(token, "spacedust") == 0)
	{
		int count;
		if (mParticleClouds.full())
		{
			return;
		}
		token = COM_ParseExt(&command, qfalse);
		count = atoi(token);

		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(count, "gfx/effects/snowpuff1.tga");
		nCloud.mHeight		= 1.2f;
		nCloud.mWidth		= 1.2f;
		nCloud.mGravity		= 0.0f;
		nCloud.mBlendMode			= 1;
		nCloud.mRotationChangeNext	= 0;
		nCloud.mColor		= 0.75f;
		nCloud.mWaterParticles = true;
		nCloud.mMass.mMax	= 30.0f;
		nCloud.mMass.mMin	= 10.0f;
		nCloud.mSpawnRange.mMins[0]	= -1500.0f;
		nCloud.mSpawnRange.mMins[1]	= -1500.0f;
		nCloud.mSpawnRange.mMins[2]	= -1500.0f;
		nCloud.mSpawnRange.mMaxs[0]	= 1500.0f;
		nCloud.mSpawnRange.mMaxs[1]	= 1500.0f;
		nCloud.mSpawnRange.mMaxs[2]	= 1500.0f;
	}

	// Create A Sand Storm
	//---------------------
	else if (Q_stricmp(token, "sand") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(400, "gfx/effects/alpha_smoke2b.tga");

		nCloud.mGravity		= 0;
 		nCloud.mWidth		= 70;
		nCloud.mHeight		= 70;
		nCloud.mColor[0]	= 0.9f;
		nCloud.mColor[1]	= 0.6f;
		nCloud.mColor[2]	= 0.0f;
		//nCloud.mColor[3]	= 0.5f;
		nCloud.mColor[3] = 0.05f;
		nCloud.mFade		= 5.0f;
		nCloud.mMass.mMax	= 30.0f;
		nCloud.mMass.mMin	= 10.0f;
		nCloud.mSpawnRange.mMins[2]	= -150;
		nCloud.mSpawnRange.mMaxs[2]	= 150;

		nCloud.mRotationChangeNext	= 0;
	}

	// Create Blowing Clouds Of Fog
	//------------------------------
	else if (Q_stricmp(token, "fog") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(60, "gfx/effects/alpha_smoke2b.tga");
		nCloud.mBlendMode	= 1;
		nCloud.mGravity		= 0;
 		nCloud.mWidth		= 70;
		nCloud.mHeight		= 70;
		//nCloud.mColor		= 0.2f;
		nCloud.mColor[0] = 0.2f;
		nCloud.mColor[1] = 0.2f;
		nCloud.mColor[2] = 0.2f;
		nCloud.mColor[3] = 0.05f;
		nCloud.mFade		= 5.0f;
		nCloud.mMass.mMax	= 30.0f;
		nCloud.mMass.mMin	= 10.0f;
		nCloud.mSpawnRange.mMins[2]	= -150;
		nCloud.mSpawnRange.mMaxs[2]	= 150;

		nCloud.mRotationChangeNext	= 0;
	}

	// Create Heavy Rain Particle Cloud
	//-----------------------------------
	else if (Q_stricmp(token, "heavyrainfog") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
 		nCloud.Initialize(70, "gfx/effects/alpha_smoke2b.tga");
		nCloud.mBlendMode	= 1;
		nCloud.mGravity		= 0;
 		nCloud.mWidth		= 100;
		nCloud.mHeight		= 100;
		//nCloud.mColor		= 0.3f;
		nCloud.mColor[0] = 0.3f;
		nCloud.mColor[1] = 0.3f;
		nCloud.mColor[2] = 0.3f;
		nCloud.mColor[3] = 0.05f;
		nCloud.mFade		= 1.0f;
		nCloud.mMass.mMax	= 10.0f;
		nCloud.mMass.mMin	= 5.0f;

		nCloud.mSpawnRange.mMins	= -(nCloud.mSpawnPlaneDistance*1.25f);
		nCloud.mSpawnRange.mMaxs	=  (nCloud.mSpawnPlaneDistance*1.25f);
		nCloud.mSpawnRange.mMins[2]	= -150;
		nCloud.mSpawnRange.mMaxs[2]	=  150;

		nCloud.mRotationChangeNext	= 0;
	}

	// Create Blowing Clouds Of Fog
	//------------------------------
	else if (Q_stricmp(token, "light_fog") == 0)
	{
		if (mParticleClouds.full())
		{
			return;
		}
		CWeatherParticleCloud& nCloud = mParticleClouds.push_back();
		nCloud.Initialize(40, "gfx/effects/alpha_smoke2b.tga");
		nCloud.mBlendMode	= 1;
		nCloud.mGravity		= 0;
 		nCloud.mWidth		= 100;
		nCloud.mHeight		= 100;
		//nCloud.mColor[0]	= 0.19f;
		//nCloud.mColor[1]	= 0.6f;
		//nCloud.mColor[2]	= 0.7f;
		//nCloud.mColor[3]	= 0.12f;
		nCloud.mColor[0] = 0.3f;
		nCloud.mColor[1] = 0.3f;
		nCloud.mColor[2] = 0.3f;
		nCloud.mColor[3] = 0.05f;
		nCloud.mFade		= 0.10f;
		nCloud.mMass.mMax	= 30.0f;
		nCloud.mMass.mMin	= 10.0f;
		nCloud.mSpawnRange.mMins[2]	= -150;
		nCloud.mSpawnRange.mMaxs[2]	= 150;

		nCloud.mRotationChangeNext	= 0;
	}

	else if (Q_stricmp(token, "outsideshake") == 0)
	{
		mOutside.mOutsideShake = !mOutside.mOutsideShake;
	}
	else if (Q_stricmp(token, "outsidepain") == 0)
	{
		mOutside.mOutsidePain = !mOutside.mOutsidePain;
	}
	else
	{
		if (!noHelp)
		{
			char heading[64] = "Weather";
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: ^3Weather Effect^5: Please enter a valid command.\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7clear\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7freeze\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7zone (mins) (maxs)\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7wind\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7constantwind (velocity)\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7gustingwind\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7windzone (mins) (maxs) (velocity)\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7lightrain^5 - warzone light rain\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7rain^5 - warzone rain\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7heavyrain^5 - warzone heavy rain\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7rainstorm^5 - warzone rain storm\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7acidrain^5 - original JKA acid rain\n", heading);
#if 0
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7lightrain2^5 - warzone light rain 2\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7rain2^5 - warzone rain 2\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7heavyrain2^5 - warzone heavy rain 2\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7rainstorm2^5 - warzone rain storm 2\n", heading);
#endif
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7oldlightrain^5 - original JKA light rain\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7oldrain^5 - original JKA rain\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7oldheavyrain^5 - original JKA heavy rain\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7snow^5 - original JKA snow\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7fallingleafs^5 - warzone falling leafs\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7spacedust\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7sand\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7fog\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7heavyrainfog\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7light_fog\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7outsideshake\n", heading);
			ri->Printf(PRINT_ALL, "^1*** ^3%s^5: 	^7outsidepain\n", heading);
		}
	}
}

void RE_WorldEffectCommand(const char *command)
{
	RE_WorldEffectCommand_REAL(command, qfalse);
}


float R_GetChanceOfSaberFizz()
{
 	float	chance = 0.0f;
	int		numWater = 0;
	for (int i=0; i<mParticleClouds.size(); i++)
	{
		if (mParticleClouds[i].mWaterParticles)
		{
			chance += (mParticleClouds[i].mGravity/20000.0f);
			numWater ++;
		}
	}
	if (numWater)
	{
		return (chance / numWater);
	}
	return 0.0f;
}

bool R_IsRaining()
{
	return !mParticleClouds.empty();
}

bool R_IsPuffing()
{ //Eh? Don't want surfacesprites to know this?
	return false;
}

#endif //__JKA_WEATHER__
