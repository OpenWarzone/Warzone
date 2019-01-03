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
	\file TILImageICO.h
	\brief An ICO image loader
*/

#ifndef _TILIMAGEICO_H_
#define _TILIMAGEICO_H_

#include "TILImage.h"

#if (TIL_FORMAT & TIL_FORMAT_ICO)

namespace til
{

	// this seemingly pointless forward declaration
	// is necessary to fool doxygen into documenting
	// the class
	class DoxygenSaysWhat;

	/*!
		\brief til::Image implementation of an ICO loader.
	*/
	class ImageICO : public Image
	{

	public:

		ImageICO();
		~ImageICO();

		bool Parse(uint32 a_ColorDepth);

		uint32 GetFrameCount();
		byte* GetPixels(uint32 a_Frame = 0);

		uint32 GetWidth(uint32 a_Frame = 0);
		uint32 GetHeight(uint32 a_Frame = 0);

		uint32 GetPitchX(uint32 a_Frame = 0);
		uint32 GetPitchY(uint32 a_Frame = 0);

#ifndef DOXYGEN_SHOULD_SKIP_THIS

		struct BufferICO
		{
			byte* buffer;
			uint32 width, height, pitch;
			uint32 pitchy;
			uint32 readpx, readpy;
			uint32 datasize, offset;
			uint32 bytespp, bitspp;
			byte* colors;
			uint16 palette;
			byte* andmask;
			BufferICO* next;
		};

#endif

	private:

		/*!
			@name Internal
			These functions are internal and shouldn't be called by developers.
		*/
		//@{

		typedef void (ImageICO::*ColorFunc)(uint8*, uint8*, uint32, uint32);

		void ColorFunc_A8R8G8B8(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint32 a_BPP);
		void ColorFunc_A8B8G8R8(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint32 a_BPP);
		void ColorFunc_R8G8B8A8(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint32 a_BPP);
		void ColorFunc_B8G8R8A8(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint32 a_BPP);
		void ColorFunc_R8G8B8(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint32 a_BPP);
		void ColorFunc_B8G8R8(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint32 a_BPP);
		void ColorFunc_R5G6B5(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint32 a_BPP);
		void ColorFunc_B5G6R5(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint32 a_BPP);

		ColorFunc m_ColorFunc;

		void AddBuffer(uint32 a_Width, uint32 a_Height);
		void ReleaseMemory(BufferICO* a_Buffer);
		void ExpandPalette(BufferICO* a_Buffer);
		void WriteColumnAnd(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint8* a_Palette, uint8* a_AndMask);
		void WriteColumnPalette(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint8* a_Palette);
		void WriteColumnUncompressed(uint8* a_Dst, uint8* a_Src, uint32 a_Width, uint32 a_BPP);
		
		//@}

		uint32 m_Images;

		byte* m_Pixels;
		uint32 m_Width, m_Height, m_Pitch;

#ifdef OLDMETHOD
		FILE* m_Handle;
#endif

		BufferICO* m_First;
		BufferICO* m_Current;

	}; // class ImageICO

}; // namespace til

#endif

#endif