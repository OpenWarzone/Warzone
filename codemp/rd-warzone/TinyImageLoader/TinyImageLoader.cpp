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

#include "TinyImageLoader.h"
#include "TILInternal.h"

#if (TIL_FORMAT & TIL_FORMAT_PNG)
	#include "TILImagePNG.h"
#endif

#if (TIL_FORMAT & TIL_FORMAT_GIF)
	#include "TILImageGIF.h"
#endif

#if (TIL_FORMAT & TIL_FORMAT_TGA)
	#include "TILImageTGA.h"
#endif

#if (TIL_FORMAT & TIL_FORMAT_BMP)
	#include "TILImageBMP.h"
#endif

#if (TIL_FORMAT & TIL_FORMAT_ICO)
	#include "TILImageICO.h"
#endif

#if (TIL_FORMAT & TIL_FORMAT_DDS)
	#include "TILImageDDS.h"
#endif

#include "TILFileStreamStd.h"

namespace til
{

	static uint32 g_Options;
	static char* g_LineFeed = NULL;

	static char* g_WorkingDir = NULL;
	static size_t g_WorkingDirLength = 0;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	void AddErrorDefault(MessageData* a_Data);
	static MessageFunc g_ErrorFunc = AddErrorDefault;
	static char* g_Error = NULL;
	static char* g_ErrorTemp = NULL;
	static size_t g_ErrorMaxSize = 1024;
#define TIL_ERROR_MAX_SIZE 1024

	void AddDebugDefault(MessageData* a_Data);
	static MessageFunc g_DebugFunc = AddDebugDefault;
	static char* g_Debug = NULL;
	static char* g_DebugTemp = NULL;
	static size_t g_DebugMaxSize = 2048;
#define TIL_DEBUG_MAX_SIZE 1024

#endif

	static MessageData g_Msg;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	void InitLineFeed()
	{
		if (!g_LineFeed)
		{
			g_LineFeed = new char[4];
			memset(g_LineFeed, 0, 4);

			if (g_Options & TIL_FILE_CRLF)
			{
				strcpy(g_LineFeed, "\r\n");
			}
			else if (g_Options & TIL_FILE_LF)
			{
				strcpy(g_LineFeed, "\n");
			}
			else if (g_Options & TIL_FILE_CR)
			{
				strcpy(g_LineFeed, "\r");
			}
		}
	}

#endif

	void TIL_Init(uint32 a_Settings)
	{
		g_Options = a_Settings;

		g_WorkingDir = new char[TIL_MAX_PATH];

		if (!g_Error) 
		{
			g_Error = new char[g_ErrorMaxSize];
			Internal::MemSet((byte*)g_Error, 0, g_ErrorMaxSize);

			InitLineFeed();
		}

#if (TIL_RUN_TARGET == TIL_TARGET_DEVEL)

		if (!g_Debug) 
		{
			g_Debug = new char[g_DebugMaxSize];
			Internal::MemSet((byte*)g_Debug, 0, g_DebugMaxSize);

			InitLineFeed();
		}

#endif

#if (TIL_PLATFORM == TIL_PLATFORM_WINDOWS)

		char path[TIL_MAX_PATH];
		char dir[TIL_MAX_PATH];

		GetModuleFileNameA(NULL, dir, TIL_MAX_PATH);
		char* lastslash = strrchr(dir, '\\');

		strncpy(path, dir, lastslash - dir + 1);
		path[lastslash - dir + 1] = 0;

		TIL_SetWorkingDirectory(path, strlen(path));

#else

		// do nothing, for now

#endif

	}

	void TIL_ShutDown()
	{
		if (g_WorkingDir)
		{
			delete g_WorkingDir;
			g_WorkingDir = NULL;
		}
		if (g_Error)
		{
			delete g_Error;
			g_Error = NULL;
			delete g_ErrorTemp;
			g_ErrorTemp = NULL;
		}

#if (TIL_RUN_TARGET == TIL_TARGET_DEVEL)

		if (g_Debug) 
		{
			delete g_Debug;
			g_Debug = NULL;
			delete g_DebugTemp;
			g_DebugTemp = NULL;
		}

#endif

		delete g_LineFeed;
		g_LineFeed = NULL;
	}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	void AddErrorDefault(MessageData* a_Data)
	{
		sprintf(
			g_ErrorTemp,
			"%s (in file %s at line %i)", 
			a_Data->message, 
			a_Data->source_file, 
			a_Data->source_line
		);
		strcat(g_ErrorTemp, g_LineFeed);

		bool resize = false;
		while (strlen(g_ErrorTemp) + strlen(g_Error) >= g_ErrorMaxSize) { g_ErrorMaxSize *= 2; resize = true; }
		if (resize)
		{
			char* move = new char[g_ErrorMaxSize];
			strcpy(move, g_Error);
			g_Error = move;
		}

		strcat(g_Error, g_ErrorTemp);
	}

#endif

	void TIL_SetErrorFunc( MessageFunc a_Func )
	{
		g_ErrorFunc = a_Func;
	}

	char* TIL_GetError()
	{
		return g_Error;
	}

	size_t TIL_GetErrorLength()
	{
		if (g_Error) { return strlen(g_Error); }
		return 0;
	}

	void TIL_SetDebugFunc( MessageFunc a_Func )
	{
		g_DebugFunc = a_Func;
	}

	char* TIL_GetDebug()
	{
		return g_Debug;
	}

	size_t TIL_GetDebugLength()
	{
		if (g_Debug) { return strlen(g_Debug); }
		return 0;
	}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	void AddDebugDefault(MessageData* a_Data)
	{
		/*sprintf(g_DebugTemp, "%s", a_Data->message);
		strcat(g_DebugTemp, g_LineFeed);

		bool resize = false;*/

		/*while (strlen(g_DebugTemp) + strlen(g_Debug) >= g_DebugMaxSize) 
		{ 
			g_DebugMaxSize *= 2; 
			resize = true; 
		}
		if (resize)
		{
			g_Debug = (char*)realloc(g_Debug, g_DebugMaxSize);
		}*/

		/*if (strlen(g_DebugTemp) + strlen(g_Debug) >= g_DebugMaxSize) 
		{ 
			TIL_ClearDebug();
		}

		strcat(g_Debug, g_DebugTemp);*/

		sprintf(g_DebugTemp, "%s", a_Data->message);
		strcat(g_DebugTemp, g_LineFeed);

		if (strlen(g_DebugTemp) + strlen(g_Debug) >= g_DebugMaxSize) 
		{ 
			TIL_ClearDebug();
		}

		strcat(g_Debug, g_DebugTemp);

		/*sprintf(
			g_DebugTemp,
			"%s (in file %s at line %i)", 
			a_Data->message, 
			a_Data->source_file, 
			a_Data->source_line
		);
		strcat(g_DebugTemp, g_LineFeed);

		bool resize = false;
		while (strlen(g_DebugTemp) + strlen(g_Debug) >= g_DebugMaxSize) { g_DebugMaxSize *= 2; resize = true; }
		if (resize)
		{
			char* move = new char[g_DebugMaxSize];
			strcpy(move, g_Debug);
			delete g_Debug;
			g_Debug = move;
		}

		strcat(g_Debug, g_DebugTemp);*/
	}

	void PixelDefault(uint8 a_ColorWidth, uint8* a_Dst, uint8* a_Src, uint32 a_Index, uint32 a_Count)
	{
		for (uint32 i = 0; i < a_Count * a_ColorWidth; i++) { a_Dst[i] = a_Src[i]; }
	}

#endif

	void TIL_GetVersion(char* a_Target, size_t a_MaxLength)
	{
		sprintf(a_Target, "%i.%i.%i", TIL_VERSION_MAJOR, TIL_VERSION_MINOR, TIL_VERSION_BUGFIX);
	}

	Image* TIL_Load(FileStream* a_Stream, uint32 a_Options)
	{
		// i don't know the path at this point
		// oh well, good luck, have fun
		if (!a_Stream)
		{
			return NULL;
		}

		const char* filepath = a_Stream->GetFilePath();

		size_t end = strlen(filepath) - 4;
		if (end < 4)
		{
			TIL_ERROR_EXPLAIN("Filename isn't long enough.");
			return NULL;
		}

		TIL_PRINT_DEBUG("Filepath: %s", filepath);

		Image* result = NULL;

		// lol hack
		if (0) { }
#if (TIL_FORMAT & TIL_FORMAT_PNG)
		else if (!strncmp(filepath + end, ".png", 4)) 
		{ 
			result = new ImagePNG(); 
			TIL_PRINT_DEBUG("Found: PNG", 0);
		}
#endif
#if (TIL_FORMAT & TIL_FORMAT_GIF)
		else if (!strncmp(filepath + end, ".gif", 4)) 
		{ 
			result = new ImageGIF(); 
			TIL_PRINT_DEBUG("Found: GIF", 0);
		}
#endif
#if (TIL_FORMAT & TIL_FORMAT_TGA)
		else if (!strncmp(filepath + end, ".tga", 4)) 
		{ 
			result = new ImageTGA(); 
			TIL_PRINT_DEBUG("Found: TGA", 0);
		}
#endif
#if (TIL_FORMAT & TIL_FORMAT_BMP)
		else if (!strncmp(filepath + end, ".bmp", 4)) 
		{ 
			result = new ImageBMP();
			TIL_PRINT_DEBUG("Found: BMP", 0);
		}
#endif
#if (TIL_FORMAT & TIL_FORMAT_ICO)
		else if (!strncmp(filepath + end, ".ico", 4)) 
		{ 
			result = new ImageICO(); 
			TIL_PRINT_DEBUG("Found: ICO", 0);
		}
#endif
#if (TIL_FORMAT & TIL_FORMAT_DDS)
		else if (!strncmp(filepath + end, ".dds", 4)) 
		{ 
			result = new ImageDDS(); 
			TIL_PRINT_DEBUG("Found: DDS", 0);
		}
#endif
		else
		{
			TIL_PRINT_DEBUG("Filename: '%s' (end: '%s')", filepath, filepath + end);
			TIL_ERROR_EXPLAIN("Can't parse file: unknown format.");
			a_Stream->Close();
			delete a_Stream;
			return NULL;
		}

		result->Load(a_Stream);

		if (result && !result->SetBPP(a_Options & TIL_DEPTH_MASK))
		{
			TIL_ERROR_EXPLAIN("Invalid bit-depth option: %i.", a_Options & TIL_DEPTH_MASK);
			delete result;
			result = NULL;
		}

		if (result && !result->Parse(a_Options & (TIL_DEPTH_MASK)))
		{
			TIL_ERROR_EXPLAIN("Could not parse file.");
			delete result;
			result = NULL;
		}

		a_Stream->Close();
		if (!a_Stream->IsReusable())
		{
			delete a_Stream;
			a_Stream = NULL;
		}

		return result;
	}

	Image* TIL_Load(const char* a_FileName, uint32 a_Options)
	{
		FileStream* load = Internal::g_FileFunc(a_FileName, a_Options & TIL_FILE_MASK);
		if (!load) 
		{
			TIL_ERROR_EXPLAIN("Could not find file '%s'.", a_FileName);
			return NULL;
		}

		return TIL_Load(load, a_Options);
	}

	bool TIL_Release(Image* a_Image)
	{
		if (!a_Image) { return false; }
		delete a_Image;
		return true;
	}

	size_t TIL_SetWorkingDirectory(const char* a_Path, size_t a_Length )
	{
		g_WorkingDirLength = strlen(a_Path);
		if (g_WorkingDirLength > TIL_MAX_PATH - 1) { g_WorkingDirLength = TIL_MAX_PATH - 1; }
		strncpy(g_WorkingDir, a_Path, g_WorkingDirLength);
		g_WorkingDir[g_WorkingDirLength] = 0;

		return g_WorkingDirLength;
	}

	void TIL_AddWorkingDirectory( char* a_Dst, size_t a_MaxLength, const char* a_Path )
	{
		if (a_MaxLength < g_WorkingDirLength + strlen(a_Path)) { return; }

		strcpy(a_Dst, g_WorkingDir); 
		strcat(a_Dst, a_Path);

		int i = 0;
	}

	void TIL_SetFileStreamFunc(FileStreamFunc a_Func)
	{
		Internal::g_FileFunc = a_Func;
	}

	void TIL_SetPitchFunc(PitchFunc a_Func)
	{
		Internal::g_PixelFunc = a_Func;
	}

	void TIL_ClearDebug()
	{
		if (g_Debug) { delete g_Debug; }
		g_DebugMaxSize = 2048;
		g_Debug = new char[g_DebugMaxSize];
		Internal::MemSet((byte*)g_Debug, 0, g_DebugMaxSize);
	}

	namespace Internal
	{

		FileStream* OpenStreamDefault(const char* a_Path, uint32 a_Options)
		{
			FileStream* result = new FileStreamStd();
			if (result->Open(a_Path, a_Options)) { return result; }

			return NULL;
		}

		byte* CreatePixels(uint32 a_Width, uint32 a_Height, uint8 a_BPP, uint32& a_PitchX, uint32& a_PitchY)
		{
			g_PixelFunc(a_Width, a_Height, a_BPP, a_PitchX, a_PitchY);

			if (a_PitchX < a_Width)
			{
				TIL_ERROR_EXPLAIN("Horizontal pitch is smaller than width.");
				return NULL;
			}
			if (a_PitchY < a_Height)
			{
				TIL_ERROR_EXPLAIN("Vertical pitch is smaller than width.");
				return NULL;
			}

			uint32 total = a_PitchX * a_BPP * a_PitchY;	
			byte* result = new byte[total];
			memset(result, 0, total);

			return result;
		}

		void CreatePixelsDefault(uint32 a_Width, uint32 a_Height, uint8 a_BPP, uint32& a_PitchX, uint32& a_PitchY)
		{
			a_PitchX = a_Width;
			a_PitchY = a_Height;
		}

		void AddDebug(char* a_Message, char* a_File, int a_Line, ...)
		{
			va_list args;
			va_start(args, a_Line);
			if (!g_DebugTemp) { g_DebugTemp = new char[TIL_DEBUG_MAX_SIZE + 1]; }
			vsprintf(g_DebugTemp, a_Message, args);
			va_end(args);

			g_Msg.message = g_DebugTemp;
			g_Msg.source_file = a_File;
			g_Msg.source_line = a_Line;
			g_DebugFunc(&g_Msg);
		}

		void AddError( char* a_Message, char* a_File, int a_Line, ... )
		{
			va_list args;
			va_start(args, a_Line);
			if (!g_ErrorTemp) { g_ErrorTemp = new char[TIL_ERROR_MAX_SIZE]; }
			vsprintf(g_ErrorTemp, a_Message, args);
			va_end(args);

			g_Msg.message = g_ErrorTemp;
			g_Msg.source_file = a_File;
			g_Msg.source_line = a_Line;
			g_ErrorFunc(&g_Msg);
		}

	}

}; // namespace til