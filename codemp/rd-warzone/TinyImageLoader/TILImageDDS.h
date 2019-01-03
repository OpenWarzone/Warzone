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

#ifndef _TILIMAGEDDS_H_
#define _TILIMAGEDDS_H_

/*!
	\file TILImageDDS.h
	\brief A DDS image loader
*/

#include "TILImage.h"

#if (TIL_FORMAT & TIL_FORMAT_DDS)

namespace til
{

	// this seemingly pointless forward declaration
	// is necessary to fool doxygen into documenting
	// the class
	class DoxygenSaysWhat;

	//! Implementation of a DDS loader
	class ImageDDS : public Image
	{

	public:

		ImageDDS();
		~ImageDDS();

		uint32 GetFrameCount();
		byte* GetPixels(uint32 a_Frame = 0);

		uint32 GetWidth(uint32 a_Frame = 0);
		uint32 GetHeight(uint32 a_Frame = 0);

		uint32 GetPitchX(uint32 a_Frame = 0);
		uint32 GetPitchY(uint32 a_Frame = 0);

		bool Parse(uint32 a_ColorDepth);

	private:
		
		/*!
			@name Internal
			These functions are internal and shouldn't be called by developers.
		*/
		//@{

		typedef void (ImageDDS::*ColorFunc)(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha);

		void ColorFunc_A8B8G8R8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha);
		void ColorFunc_A8R8G8B8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha);
		void ColorFunc_B8G8R8A8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha);
		void ColorFunc_R8G8B8A8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha);
		void ColorFunc_B8G8R8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha);
		void ColorFunc_R8G8B8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha);
		void ColorFunc_B5G6R5(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha);
		void ColorFunc_R5G6B5(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha);

		ImageDDS::ColorFunc m_ColorFunc;

		void AddMipMap(uint32 a_Width, uint32 a_Height);
		void GetBlocks(uint32 a_Width, uint32 a_Height);
		void ConstructColors(color_16b a_Color0, color_16b a_Color1);

		void DecompressDXT1();
		void DecompressDXT5();
		bool DecompressUncompressed();

		struct MipMap
		{
			uint32 width, height;
			uint32 pitchx, pitchy;
			byte* data;
		};

		//@}

		uint32 m_Offset;
		uint32 m_Width, m_Height, m_Depth;
		uint32 m_PitchX, m_PitchY;
		MipMap* m_MipMap;
		uint32 m_MipMapSize;
		uint32 m_MipMapTotal;
		uint32 m_MipMapCurrent;
		uint32 m_CubeMap;
		uint32 m_Format;
		uint32 m_InternalDepth, m_InternalBPP;
		uint32 m_Size, m_Blocks;
		uint32 m_BlockSize;

		byte* m_Colors;
		byte* m_Alpha;

		byte* m_Data;
		byte* m_Pixels;

	}; // class ImageDDS

}; // namespace til

#endif

#endif