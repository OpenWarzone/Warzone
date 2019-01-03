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
	\file TILFileStreamStd.h
	\brief An ANSI C implementation of FileStream
*/

#ifndef _TILFILESTREAMSTD_H_
#define _TILFILESTREAMSTD_H_

#include "TILSettings.h"
#include "TILFileStream.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace til
{

	// this seemingly pointless forward declaration
	// is necessary to fool doxygen into documenting
	// the class
	class DoxygenSaysWhat;

	//! ANSI C implementation of FileStream
	/*!
		FileStream implementation that uses FILE pointers. Should work
		on all platforms with a valid ANSI C implementation, but details
		may differ.
	*/
	class FileStreamStd : public FileStream
	{
	
	public:
	
		FileStreamStd();
		~FileStreamStd();
	
		bool Open(const char* a_File, uint32 a_Options);

		bool Read(void* a_Dst, uint32 a_ElementSize, uint32 a_Count = 1);
		bool ReadByte(byte* a_Dst, uint32 a_Count = 1);
		bool ReadWord(word* a_Dst, uint32 a_Count = 1);
		bool ReadDWord(dword* a_Dst, uint32 a_Count = 1);

		bool Seek(uint32 a_Bytes, uint32 a_Options);

		bool EndOfFile();

		bool Close();

		bool IsReusable() { return false; }

	private:

		FILE* m_Handle;
	
	}; // class FileStreamStd

}; // namespace til
	
#endif