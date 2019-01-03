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
	\file TILImageTGA.h
	\brief A TGA image loader
*/

#ifndef _TILIMAGETGA_H_
#define _TILIMAGETGA_H_

#include "TILImage.h"

#if (TIL_FORMAT & TIL_FORMAT_TGA)

namespace til
{

	/*!
		\brief til::Image implementation of a TGA loader.
	*/
	class ImageTGA : public Image
	{

		enum ColorType
		{
			COLOR_BLACKANDWHITE,
			COLOR_MAPPED,
			COLOR_TRUECOLOR
		};
		
		//! Compression type.
		enum Compression
		{
			COMP_NONE,
			COMP_RLE
		};

	public:

		ImageTGA();
		~ImageTGA();

		bool Parse(uint32 a_Options);

		uint32 GetFrameCount();
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

		typedef uint8* (ImageTGA::*ColorFunc)(uint8*, uint8*, uint32, int, int);

		uint8* ColorFunc_A8R8G8B8(uint8* a_Dst, uint8* a_Src, uint32 a_BPP, int a_Repeat, int a_Unique);
		uint8* ColorFunc_A8B8G8R8(uint8* a_Dst, uint8* a_Src, uint32 a_BPP, int a_Repeat, int a_Unique);
		uint8* ColorFunc_R8G8B8A8(uint8* a_Dst, uint8* a_Src, uint32 a_BPP, int a_Repeat, int a_Unique);
		uint8* ColorFunc_B8G8R8A8(uint8* a_Dst, uint8* a_Src, uint32 a_BPP, int a_Repeat, int a_Unique);
		uint8* ColorFunc_R8G8B8(uint8* a_Dst, uint8* a_Src, uint32 a_BPP, int a_Repeat, int a_Unique);
		uint8* ColorFunc_B8G8R8(uint8* a_Dst, uint8* a_Src, uint32 a_BPP, int a_Repeat, int a_Unique);
		uint8* ColorFunc_R5G6B5(uint8* a_Dst, uint8* a_Src, uint32 a_BPP, int a_Repeat, int a_Unique);
		uint8* ColorFunc_B5G6R5(uint8* a_Dst, uint8* a_Src, uint32 a_BPP, int a_Repeat, int a_Unique);

		ImageTGA::ColorFunc m_ColorFunc;

		//! Compile uncompressed image data to pixel information.
		bool CompileUncompressed();

		//! Compile compressed image data to pixel information.
		bool CompileRunLengthEncoded();

		//@}

		byte* m_Data;

		ColorType m_Type;
		Compression m_Comp;
		byte* m_Target;
		byte m_Src[4];
		byte m_Depth;

		uint32 m_Width, m_Height, m_Pitch;
		uint32 m_PitchX, m_PitchY;

	}; // class ImageTGA

}; // namespace til

#endif

#endif