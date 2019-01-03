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

// change this for doxygen
/*!
	\file TILImageTemplate.h
	\brief Template for an image loader
*/

// change these defines
#ifndef _TILIMAGETEMPLATE_H_
#define _TILIMAGETEMPLATE_H_

#include "TILImage.h"

// change this to your format
#if (TIL_FORMAT & TIL_FORMAT_MYFORMAT)

namespace til
{

	// this seemingly pointless forward declaration
	// is necessary to fool doxygen into documenting
	// the class
	class DoxygenSaysWhat;

	//! Implementation of a Template loader
	class ImageTemplate : public Image
	{

	public:

		// make sure you support these functions
		uint32 GetFrameCount();
		byte* GetPixels(uint32 a_Frame = 0);

	private:

		ImageTemplate();
		~ImageTemplate();

		// this one does all the magic
		// check the cpp for the correct implementation
		bool Parse(uint32 a_ColorDepth);

		// return the amount of frames
		// or 1 if only one frame is allowed
		uint32 GetFrameCount();
		// return the pixels corresponding to the frame
		// or ignore the a_Frame parameter if multiple frames aren't allowed
		byte* GetPixels(uint32 a_Frame = 0);

		// return the width corresponding to the frame
		// or ignore the a_Frame parameter if multiple frames aren't allowed
		uint32 GetWidth(uint32 a_Frame = 0);
		// return the height corresponding to the frame
		// or ignore the a_Frame parameter if multiple frames aren't allowed
		uint32 GetHeight(uint32 a_Frame = 0);

	private:

		/*!
			@name Internal
			These functions are internal and shouldn't be called by developers.
		*/
		//@{

		// go wild, add as many members and functions as you need here

		//@}

	}; // class ImageTemplate

}; // namespace til

#endif

#endif