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
	\file TILFileStream.h
	\brief Virtual interface for loading data
*/

#ifndef _TILFILESTREAM_H_
#define _TILFILESTREAM_H_

#include "TILSettings.h"

namespace til
{

	// this seemingly pointless forward declaration
	// is necessary to fool doxygen into documenting
	// the class
	class DoxygenSaysWhat;

	//! The virtual interface for reading data from files
	/*!
		Reads data from a file in bytes. Platform-specific implementations can be made by
		inheriting from this class and attaching a new FileStreamFunc to TinyImageLoader.

		\code
		FileStream* OpenStreamMyDevice(const char* a_Path, uint32 a_Options)
		{
			FileStream* result = new FileStreamMyDevice();
			if (result->Open(a_Path, a_Options)) { return result; }

			return NULL;
		}
		\endcode
	*/
	
	class FileStream
	{
	
	public:
	
		FileStream();
		~FileStream();
	
		//! Open a handle to a file
		/*!
			\param a_File The path to the file to be loaded
			\param a_Options The options to consider

			\return True on success, false on failure

			Valid options are a file option:
			- #TIL_FILE_ABSOLUTEPATH
			- #TIL_FILE_ADDWORKINGDIR

			\note If a file cannot be found, return false
		*/
		virtual bool Open(const char* a_File, uint32 a_Options) = 0;

		//! Read data
		/*!
			\param a_Dst The destination buffer to write to
			\param a_ElementSize The size of each element
			\param a_Count The number of elements to read

			\return True on success, false on failure

			\note If more data is requested than is left in the file, return false
		*/
		virtual bool Read(void* a_Dst, uint32 a_ElementSize, uint32 a_Count = 1) = 0;

		//! Read a number of bytes
		/*!
			\param a_Dst The destination buffer to write to
			\param a_Count The number of bytes to read

			\return True on success, false on failure

			\note If more data is requested than is left in the file, return false
		*/
		virtual bool ReadByte(byte* a_Dst, uint32 a_Count = 1) = 0;

		//! Read a number of words
		/*!
			\param a_Dst The destination buffer to write to
			\param a_Count The number of words to read

			\return True on success, false on failure

			\note If more data is requested than is left in the file, return false
		*/
		virtual bool ReadWord(word* a_Dst, uint32 a_Count = 1) = 0;

		//! Read a number of dwords
		/*!
			\param a_Dst The destination buffer to write to
			\param a_Count The number of dwords to read

			\return True on success, false on failure

			\note If more data is requested than is left in the file, return false
		*/
		virtual bool ReadDWord(dword* a_Dst, uint32 a_Count = 1) = 0;

		//! Seek to a position in the file
		/*!
			\param a_Offset The number of bytes to skip
			\param a_Options The location in the file

			Valid options are:
			-#TIL_FILE_SEEK_START
			-#TIL_FILE_SEEK_CURR
			-#TIL_FILE_SEEK_END

			\return True on success, false on failure

			\note If the requested offset is too great, clamp to the nearest edge 
			(either the start or the end of the file) and return false
		*/
		virtual bool Seek(uint32 a_Offset, uint32 a_Options) = 0;

		//! Check for end of file
		/*!
			\return True if no more bytes can be read, otherwise false
		*/
		virtual bool EndOfFile() = 0;

		//! Close the stream
		/*!
			Close the handle to the file.

			\return Success
		*/
		virtual bool Close() = 0;

		//! Returns whether the stream can be reused
		/*!
			\note When a stream is used for more than one file it should 
			not be deleted. It is then up to to the developer to release
			the handle to the stream.

			\return True if reusable, otherwise false
		*/
		virtual bool IsReusable() = 0; 

		//! Get the full path
		/*!
			\note This string should always end with an image file extension, 
			otherwise TinyImageLoader can't load it.

			\return String with the full path to the file
		*/
		const char* GetFilePath();

	protected:

		char* m_FilePath; //!< The full path to the file
	
	}; // class FileStream

}; // namespace til
	
#endif