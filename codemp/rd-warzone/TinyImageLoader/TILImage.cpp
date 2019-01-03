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
	\file TILImage.h
	\class til::Image "../SDK/headers/TILImage.h"
*/

#include "TILImage.h"
#include "TILInternal.h"

namespace til
{

	Image::Image()
	{
		m_FileName = NULL;
		m_Stream = NULL;
	}

	Image::~Image()
	{ 
		if (m_FileName) { delete m_FileName; }
	}

	void Image::Load(FileStream* a_Stream)
	{
		m_Stream = a_Stream;
	}

	bool Image::Close()
	{
		if (m_Stream)
		{
			m_Stream->Close();
			return true;
		}

		return false;
	}

	bool Image::SetBPP(uint32 a_Options)
	{
		switch (a_Options)
		{

		case TIL_DEPTH_A8R8G8B8:
		case TIL_DEPTH_A8B8G8R8:
		case TIL_DEPTH_R8G8B8A8:
		case TIL_DEPTH_B8G8R8A8:
			{
				m_BPP = 4;
				break;
			}
		case TIL_DEPTH_R8G8B8:
		case TIL_DEPTH_B8G8R8:
			{
				m_BPP = 4;
				break;
			}
			
		case TIL_DEPTH_R5G6B5:
		case TIL_DEPTH_B5G6R5:
			{
				m_BPP = 2;
				break;
			}
		default:
			{
				return false;
			}
			
		}	

		m_BPPIdent = (BitDepth)(a_Options >> 16);
		return true;
	}

}; // namespace til