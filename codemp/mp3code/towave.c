#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>		/* file open flags */
#include <sys/types.h>		/* someone wants for port */
#include <sys/stat.h>		/* forward slash for portability */
#include "mhead.h"		/* mpeg header structure, decode protos */

#include "port.h"

// JDW
#ifdef __linux__
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <errno.h>
#endif
// JDW

#include "mp3struct.h"
#include <assert.h>


#if !defined(MACOS_X) && !defined(byte) && !defined (__linux__)
typedef unsigned char byte;
#endif


// the entire decode mechanism uses this now...
//
MP3STREAM _MP3Stream;
LP_MP3STREAM pMP3Stream = &_MP3Stream;
int bFastEstimateOnly = 0;	// MUST DEFAULT TO THIS VALUE!!!!!!!!!

qboolean C_MP3_IsMusic(void *pvData, int iDataLen, int bStereoDesired)
{
//	char sTemp[1024];	/////////////////////////////////////////////////
	unsigned int iRealDataStart;
	MPEG_HEAD head;
	int iBitRate;
	int iFrameBytes;

	memset(pMP3Stream,0,sizeof(*pMP3Stream));

	iFrameBytes = head_info3( (unsigned char *)pvData, iDataLen/2, &head, &iBitRate, &iRealDataStart);
	if (iFrameBytes == 0)
	{
		return qfalse;
	}

	// check for files with bad frame unpack sizes (that would crash the game), or stereo files.
	//
	// although the decoder can convert stereo to mono (apparently), we want to know about stereo files
	//	because they're a waste of source space...
	//
	if (head.mode != 3 && !bStereoDesired)	//3 seems to mean mono
	{
		if (iDataLen > 98000) {	// we'll allow it for small files even if stereo
			return qtrue;
		}
	}
	
	// file seems to be valid...
	//
	return qfalse;
}
