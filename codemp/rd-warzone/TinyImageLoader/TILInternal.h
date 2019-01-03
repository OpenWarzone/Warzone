/*
	TinyImageLoader - load images, just like that

	Copyright (C) 2010 - 2011 by Quinten Lansu
	
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	
	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

/*!
	\file TILInternal.h
	\brief Internal functions
*/

#ifndef _TILINTERNAL_H_
#define _TILINTERNAL_H_

#include "TILSettings.h"

namespace til
{
	// =========================================
	// Internal functions
	// =========================================

	extern void TIL_AddWorkingDirectory(char* a_Dst, size_t a_MaxLength, const char* a_Path);

	class FileStream;

	namespace Internal
	{

		/*!
			@name Internal
			These functions are internal and shouldn't be called by developers.
		*/
		//@{

		//! Adds an error to the logging stack
		/*!
			\param a_Message The message to post
			\param a_File The file it originated from
			\param a_Line The line it originated from

			\note Internal method. 

			An implementation of printf for errors. 
			The parameters in a_Message are parsed and sent to the attached logging function.
			This can be the internal logger or one of your own.
		*/
		extern void AddError(char* a_Message, char* a_File, int a_Line, ...);
		//! Adds a debug message to the logging stack
		/*!
			\param a_Message The message to post
			\param a_File The file it originated from
			\param a_Line The line it originated from

			\note Internal method. 

			An implementation of printf for debug messages. 
			The parameters in a_Message are parsed and sent to the attached logging function.
			This can be the internal logger or one of your own.
		*/
		extern void AddDebug(char* a_Message, char* a_File, int a_Line, ...);

		//! Default FileStream function.
		/*!
			\param a_Path File path
			\param a_Options Options to load with

			\return FileStream handle

			\note Internal method.

			Opens a file and returns a FileStream handle. If unsuccessful, it returns NULL.
		*/
		extern FileStream* OpenStreamDefault(const char* a_Path, uint32 a_Options);

		//! Opens a FileStream.
		/*!
			\note Internal method.	

			This function pointer is called by implementations to get a handle to a file.
		*/
		static FileStreamFunc g_FileFunc = OpenStreamDefault;

		extern bool GetPitch(uint32 a_Width, uint32 a_Height, uint8 a_BPP, uint32& a_PitchX, uint32& a_PitchY);
		extern byte* CreatePixels(uint32 a_Width, uint32 a_Height, uint8 a_BPP, uint32& a_PitchX, uint32& a_PitchY);

		extern void CreatePixelsDefault(uint32 a_Width, uint32 a_Height, uint8 a_BPP, uint32& a_PitchX, uint32& a_PitchY);
		static PitchFunc g_PixelFunc = CreatePixelsDefault;

		/*inline void SetPitch(uint32 a_Options, uint32 a_Width, uint32 a_Height, uint32& a_PitchX, uint32& a_PitchY)
		{
			uint32 options = a_Options & TIL_PITCH_MASK;
			if (options == 0) { options = TIL_PITCH_DEFAULT; }

			switch (options)
			{

			case TIL_PITCH_DEFAULT:
				{
					a_PitchX = a_Width;
					a_PitchY = a_Height;

					break;
				}

			case TIL_PITCH_POWER_OF_TWO:
				{
					uint32 closest = 0;
					while (a_Width >>= 1) { closest++; }

					a_PitchX = 1 << (closest + 1);
					a_PitchY = a_Height;

					break;
				}

			case TIL_PITCH_SQUARE:
				{
					a_PitchX = (a_Width > a_Height) ? a_Width : a_Height;
					a_PitchY = a_PitchX;

					break;
				}

			case TIL_PITCH_SQUARE_POWER_OF_TWO:
				{
					uint32 high = (a_Width > a_Height) ? a_Width : a_Height;

					uint32 closest = 0;
					while (high >>= 1) { closest++; }

					a_PitchX = 1 << (closest + 1);
					a_PitchY = a_PitchX;

					break;
				}

			default:
				{
					TIL_ERROR_EXPLAIN("Unknown pitch option: %i", a_Options);
				}

				TIL_PRINT_DEBUG("Dimensions: (%i x %i) Pitch: (%i x %i)", a_Width, a_Height, a_PitchX, a_PitchY);	
			}	
		}*/

		inline void MemCpy(uint8* a_Dst, uint8* a_Src, uint32 a_Size)
		{
			for (uint32 i = 0; i < a_Size; i++) { *a_Dst++ = *a_Src++; }
		}

		inline void MemSet(byte* a_Dst, byte a_Value, uint32 a_Size)
		{
			for (uint32 i = 0; i < a_Size; i++) { *a_Dst++ = a_Value; }
		}

		//@}

	}; // namespace Internal

}; // namespace til

#ifndef DOXYGEN_SHOULD_SKIP_THIS
	#if (TIL_PLATFORM == TIL_PLATFORM_WINDOWS && TIL_RUN_TARGET == TIL_TARGET_DEVEL)
		#undef _CRTDBG_MAP_ALLOC
		#define _CRTDBG_MAP_ALLOC
		#include <crtdbg.h>

		#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
		#define new DEBUG_NEW
	#endif
#endif

#endif