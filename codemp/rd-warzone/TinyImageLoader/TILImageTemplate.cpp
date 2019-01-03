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

#include "TILImageTemplate.h"
#include "TILInternal.h"

// change this to your format
#if (TIL_FORMAT & TIL_FORMAT_MYFORMAT)

// change this to your format
#ifdef TIL_PRINT_DEBUG
	#define TEMPLATE_DEBUG(msg, ...)        TIL_PRINT_DEBUG("Template: "msg, __VA_ARGS__)
#endif

namespace til
{

	ImageTemplate::ImageTemplate()
	{
		// tip: set variables you will new to NULL

		// m_Example = NULL;
	}

	ImageTemplate::~ImageTemplate()
	{
		// make sure you delete any memory you new'd!

		// if (m_Example) { delete m_Example; }
	}

	bool ImageTemplate::Parse(uint32 a_ColorDepth)
	{
		// if you want to output debug info, use the *_DEBUG macro, like so:
	
		/*
			TEMPLATE_DEBUG("Is this thing on?");
		*/

		// m_Stream can be used to read data from the file.
	
		/*
			dword header;         m_Stream->ReadDWord(&header);
		*/

		// if anything goes wrong, you use TIL_ERROR_EXPLAIN to explain what went wrong
		// and return false

		/*
			if (header != 0xAABBCCDD)
			{
				TIL_ERROR_EXPLAIN("File is of the wrong format.");
				return false;
			}
		*/

		// before you can begin parsing, a bit about colors:

		// m_BPP has already been set for you, which is the amount of bytes per pixel
		// this is either 4 (R8G8B8, A8R8G8B8 or equivalent) or 2 (R5G6B5 or equivalent)

		// you need to make sure you have enough space for your pixel data
		// usually you will have a byte* called m_Pixels
		// but this isn't necessarily the case with image formats that contain multiple images

		/*
			m_Pixels = new byte[m_Width * m_Height * m_BPP];
		*/

		// a_ColorDepth tells you the color format, these are the TIL_DEPTH_* defines

		/*
			switch (a_ColorDepth)
			{
				case TIL_DEPTH_A8R8G8B8:
					break;
				case TIL_DEPTH_R5G6B5:
					break;
				default:
					break;
			}
		*/

		// now you can begin parsing your image

		/*
			PARSE HERE
		*/

		// if everything went well, return true

		// because this function is not yet implemented we'll return false

		return false;
	}

	uint32 ImageTemplate::GetFrameCount()
	{
		// return the amount of frames your data contains
		// this is only necessary for formats that contain multiple images
		// otherwise, return 1

		return 1;
	}

	byte* ImageTemplate::GetPixels(uint32 a_Frame /*= 0*/)
	{
		// return your pixel data
		// if an invalid parameter has been specified, return NULL
		// otherwise return a pointer to your data in byte's that has a length of:
		//
		// width * height * bytesperpixel
		//
		// bytes.

		return NULL;
	}

	uint32 ImageTemplate::GetWidth(uint32 a_Frame /*= 0*/)
	{
		// return the width of the frame
		// if only 1 frame is allowed, disregard the a_Frame parameter

		return 0;
	}

	uint32 ImageTemplate::GetHeight(uint32 a_Frame /*= 0*/)
	{
		// return the height of the frame
		// if only 1 frame is allowed, disregard the a_Frame parameter

		return 0;
	}

}; // namespace til

#endif