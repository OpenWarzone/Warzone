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

#include "TILFileStreamStd.h"
#include "TILInternal.h"

namespace til
{

	FileStreamStd::FileStreamStd() : FileStream()
	{

	}

	FileStreamStd::~FileStreamStd()
	{

	}

	bool FileStreamStd::Open(const char* a_File, uint32 a_Options)
	{
		char path[TIL_MAX_PATH] = { 0 };
		size_t length = strlen(a_File);

		if (a_Options == TIL_FILE_ABSOLUTEPATH)
		{
			strcpy(path, a_File);
		}
		else if (a_Options == TIL_FILE_ADDWORKINGDIR)
		{
			TIL_AddWorkingDirectory(path, TIL_MAX_PATH, a_File);

			TIL_PRINT_DEBUG("Final path: %s", path);
		}

		length = strlen(path);
		m_FilePath = new char[length + 1];
		strcpy(m_FilePath, path);

		m_Handle = fopen(m_FilePath, "rb");
		if (!m_Handle)
		{
			TIL_ERROR_EXPLAIN("Could not open '%s'.", path);
			delete m_FilePath;
			return false;
		}

		return true;
	}

	bool FileStreamStd::Read(void* a_Dst, uint32 a_Size, uint32 a_Count)
	{
		size_t result = fread(a_Dst, a_Size, a_Count, m_Handle);
		return (result == a_Count * a_Size);
	}

	bool FileStreamStd::ReadByte(byte* a_Dst, uint32 a_Count)
	{
		size_t result = fread(a_Dst, sizeof(byte), a_Count, m_Handle);
		return (result == a_Count * sizeof(byte));
	}

	bool FileStreamStd::ReadWord(word* a_Dst, uint32 a_Count)
	{
		size_t result = fread(a_Dst, sizeof(word), a_Count, m_Handle);
		return (result == a_Count * sizeof(word));
	}

	bool FileStreamStd::ReadDWord(dword* a_Dst, uint32 a_Count)
	{
		size_t result = fread(a_Dst, sizeof(dword), a_Count, m_Handle);
		return (result == a_Count * sizeof(dword));
	}

	bool FileStreamStd::Seek(uint32 a_Bytes, uint32 a_Options)
	{
		if (a_Options & TIL_FILE_SEEK_START) 
		{ 
			fseek(m_Handle, a_Bytes, SEEK_SET); 
		}
		else if (a_Options & TIL_FILE_SEEK_CURR) 
		{ 
			fseek(m_Handle, a_Bytes, SEEK_CUR); 
		}
		else if (a_Options & TIL_FILE_SEEK_END) 
		{ 
			fseek(m_Handle, a_Bytes, SEEK_END);
		}

		return true;
	}

	bool FileStreamStd::EndOfFile()
	{
		return false;
	}

	bool FileStreamStd::Close()
	{
		if (m_Handle)
		{
			fclose(m_Handle);
			delete m_FilePath;
			return true;
		}

		return false;
	}

}