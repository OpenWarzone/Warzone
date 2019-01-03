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
	\file TILImageGIF.h
	\brief A GIF image loader
*/

#ifndef _TILIMAGEGIF_H_
#define _TILIMAGEGIF_H_

#include "TILImage.h"

#if (TIL_FORMAT & TIL_FORMAT_GIF)

namespace til
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	struct BufferLinked
	{
		byte* buffer;
		BufferLinked* next;
	};

#endif

	/*!
		\brief til::Image implementation of a GIF loader.
	*/
	class ImageGIF : public Image
	{

	public:

		ImageGIF();
		~ImageGIF();

		bool Parse(uint32 a_Options);

		uint32 GetFrameCount();
		float GetDelay();
		byte* GetPixels(uint32 a_Frame = 0);

		uint32 GetWidth(uint32 a_Frame = 0);
		uint32 GetHeight(uint32 a_Frame = 0);

		uint32 GetPitchX(uint32 a_Frame = 0);
		uint32 GetPitchY(uint32 a_Frame = 0);

	private:

		/*!
			@name Internal
			These functions are internal and shouldn't be called by developers.
		*/
		//@{
		
		void AddBuffer();
		void CompileColors(bool a_LocalTable = true);
		void ReleaseMemory(BufferLinked* a_Buffer);

		void ColorFunc(byte* a_Dst, int32 a_Code);
		
		//@}

		color_16b* m_Dst16b;
		color_32b* m_Dst32b;

		BufferLinked* m_First;
		BufferLinked* m_Current;
		uint32 m_Frames;
		float m_Delay;

		byte m_Buffer[256];

		byte* m_PrevBuffer;

		byte* m_CurrentColors;
		
		uint32 m_ColorDepth;

		byte* m_Palette;
		byte* m_Colors;
		uint32 m_ColorTableSize;

		byte* m_GlobalPalette;
		byte* m_GlobalColorTable;
		uint32 m_GlobalColorTableSize;

		bool m_Transparency;
		uint8 m_TransparentIndex;

		uint32 m_Width, m_Height, m_Pitch;

		uint32 m_OffsetX, m_OffsetY;
		uint32 m_LocalWidth, m_LocalHeight, m_LocalPitch;
		uint32 m_LocalPitchX, m_LocalPitchY;
		uint32 m_TotalBytes;
		
	}; // class ImageGIF

}; // namespace til

#endif

#endif