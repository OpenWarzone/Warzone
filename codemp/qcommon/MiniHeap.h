#pragma once

#include "../qcommon/q_shared.h"

class IHeapAllocator
{
public:
	virtual ~IHeapAllocator() {}

	virtual void ResetHeap() = 0;
	virtual char *MiniHeapAlloc ( int size ) = 0;
};

class CMiniHeap : public IHeapAllocator
{
private:
	char	*mHeap;
	char	*mCurrentHeap;
	int		mSize;
public:

	// reset the heap back to the start
	void ResetHeap()
	{
		mCurrentHeap = mHeap;
	}

	// initialise the heap
	CMiniHeap (int size)
	{
		mHeap = (char *)malloc(size);
		mSize = size;
		if (mHeap)
		{
			ResetHeap();
		}
	}

	// free up the heap
	~CMiniHeap()
	{
		free(mHeap);
	}

	// give me some space from the heap please
	char *MiniHeapAlloc(int size)
	{
		if ((size_t)size < (mSize - ((size_t)mCurrentHeap - (size_t)mHeap)))
		{
			char *tempAddress =  mCurrentHeap;
			mCurrentHeap += size;
			return tempAddress;
		}
		return NULL;
	}

};

// this is in the parent executable, so access ri->GetG2VertSpaceServer() from the rd backends!
extern IHeapAllocator *G2VertSpaceServer;
extern IHeapAllocator *G2VertSpaceClient;
