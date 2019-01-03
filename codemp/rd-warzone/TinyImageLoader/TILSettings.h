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

#ifndef _TILSETTINGS_H_
#define _TILSETTINGS_H_

/*! 
	\file TILSettings.h
	\brief Settings and global defines.

	Details
*/

// =========================================
// Defines
// =========================================

#define TIL_VERSION_MAJOR                 1 //!< The version major
#define TIL_VERSION_MINOR                 7 //!< The version minor
#define TIL_VERSION_BUGFIX                0 //!< The bugfix version

#define TIL_PLATFORM_WINDOWS              0 //!< Windows platform
#define TIL_PLATFORM_WINMO                1 //!< Windows Mobile platform
#define TIL_PLATFORM_LINUX                2 //!< Linux platform
#define TIL_PLATFORM_ANDROID              4 //!< Android platform
#define TIL_PLATFORM_PSP                  5 //!< PSP platform

//! The platform TinyImageLoader should be built for
/*!
	If no platform was defined in the preprocessor, it's assumed to be on Windows.
*/
#ifndef TIL_PLATFORM
	#define TIL_PLATFORM                  TIL_PLATFORM_WINDOWS
#endif

#define TIL_COMPILER_MSVC                 0 //!< Microsoft Visual Studio compiler
#define TIL_COMPILER_GPP                  1 //!< GNU C++ compiler

//! The compiler to build with
/*!
	If no compiler was defined in the preprocessor, TinyImageLoader assumes MSVC is being used.
*/
#ifndef TIL_COMPILER
	#define TIL_COMPILER                  TIL_COMPILER_MSVC
#endif

#define TIL_TARGET_DEBUG                  1 //!< Debug target
#define TIL_TARGET_RELEASE                2 //!< Release target
#define TIL_TARGET_DEVEL                  3 //!< Development target

//! The target to build for
/*!
	If no target was set in the preprocessor, the library will compile for release mode.

	Targets:
	* TIL_TARGET_DEBUG
	* TIL_TARGET_RELEASE
	* TIL_TARGET_DEVEL
*/
#ifndef TIL_RUN_TARGET
	#define TIL_RUN_TARGET                TIL_TARGET_RELEASE
#endif

//! Internal define used to extract file options from the options
#define TIL_FILE_MASK                     0x0000FFFF

//! The image path is absolute.
/*!
	Useful for loading images from a networked drive.

	\code
	til::Image* load = TIL_Load("\\my-share\\texture.png", TIL_DEPTH_A8B8G8R8 | TIL_FILE_ABSOLUTEPATH);
	\endcode
*/
#define TIL_FILE_ABSOLUTEPATH             0x00000001
//! The image path is relative to the working directory.
/*!
	The most common way to load an image.

	\code
	til::Image* load = TIL_Load("media\\texture.png", TIL_DEPTH_A8B8G8R8 | TIL_FILE_ADDWORKINGDIR);
	\endcode
*/
#define TIL_FILE_ADDWORKINGDIR            0x00000002
//! The specified path is unicode.
#define TIL_FILE_WIDEPATH                 0x00000004
//! Add \\r\\n as the line ending
#define TIL_FILE_CRLF                     0x00000008
//! Add \\r as the line ending
#define TIL_FILE_CR                       0x00000010
//! Add \\n as the line ending
#define TIL_FILE_LF                       0x00000020

//! Seek from the start of the file
#define TIL_FILE_SEEK_START               0x00000100
//! Seek from the current position in the file
#define TIL_FILE_SEEK_CURR                0x00000200
//! Seek from the end of the file
#define TIL_FILE_SEEK_END                 0x00000400

//! Internal define used to extract debug options from the options
#define TIL_DEBUG_MASK                    0xFFFF0000

//! Default settings for initialization
#ifndef TIL_SETTINGS
	#define TIL_SETTINGS                  (TIL_FILE_CRLF)
#endif

//! Internal define used to extract format options from the options
#define TIL_FORMAT_MASK                   0x0000FFFF
#define TIL_FORMAT_PNG                    0x00000001 //!< PNG format
#define TIL_FORMAT_GIF                    0x00000002 //!< GIF format
#define TIL_FORMAT_BMP                    0x00000004 //!< BMP format
#define TIL_FORMAT_TGA                    0x00000008 //!< TGA format
#define TIL_FORMAT_ICO                    0x00000010 //!< ICO format
#define TIL_FORMAT_DDS                    0x00000020 //!< DDS format

//! Internal define used to extract color depth options from the options
#define TIL_DEPTH_MASK                    0x00FF0000
#define TIL_DEPTH_A8R8G8B8                0x00010000 //!< 32-bit ARGB color depth
#define TIL_DEPTH_A8B8G8R8                0x00020000 //!< 32-bit ABGR color depth
#define TIL_DEPTH_R8G8B8A8                0x00030000 //!< 32-bit RGBA color depth
#define TIL_DEPTH_B8G8R8A8                0x00040000 //!< 32-bit BGRA color depth
#define TIL_DEPTH_R8G8B8                  0x00050000 //!< 32-bit RGB color depth
#define TIL_DEPTH_B8G8R8                  0x00060000 //!< 32-bit BGR color depth
#define TIL_DEPTH_R5G6B5                  0x00070000 //!< 16-bit RGB color depth
#define TIL_DEPTH_B5G6R5                  0x00080000 //!< 16-bit BGR color depth

//! Determine which formats should be included in compilation.
/*!
	Define this macro in the preprocessor definitions to overwrite the default.

	If a format is not included in the macro, its code is not compiled in.
	The following formats are standard:
	- PNG
	- GIF
	- TGA
	- BMP
	- ICO
	- DDS
*/
#ifndef TIL_FORMAT
	#define TIL_FORMAT                    (TIL_FORMAT_PNG | TIL_FORMAT_GIF | TIL_FORMAT_TGA | TIL_FORMAT_BMP | TIL_FORMAT_ICO | TIL_FORMAT_DDS)
#endif

/*!
	\def TIL_PRINT_DEBUG
	\brief Print a debug message
	Debug info is only generated when compiled using #TIL_TARGET_DEVEL.

	\def TIL_ERROR_EXPLAIN
	\brief Prints an error message
	Error messages are always posted, even when compiled using TIL_TARGET_RELEASE.
*/
#if (TIL_RUN_TARGET == TIL_TARGET_DEVEL)
	#define TIL_PRINT_DEBUG(msg, ...)  til::Internal::AddDebug("TinyImageLoader - Debug: "msg" ", __FILE__, __LINE__, ##__VA_ARGS__)
#else
	#define TIL_PRINT_DEBUG(msg, ...)
#endif
#define TIL_ERROR_EXPLAIN(msg, ...)    til::Internal::AddError("TinyImageLoader - Error: "msg" ", __FILE__, __LINE__, ##__VA_ARGS__)

/*!
	\def TIL_MAX_PATH
	The maximum path length for the platform
*/
#if (TIL_PLATFORM == TIL_PLATFORM_WINDOWS || TIL_PLATFORM == TIL_PLATFORM_WINMO)
	#include <stdarg.h>
	#include <windows.h>

	#define TIL_MAX_PATH _MAX_PATH

	#if (TIL_PLATFORM == TIL_PLATFORM_WINMO)
		#define NULL 0
	#elif (TIL_PLATFORM == TIL_PLATFORM_WINDOWS)

	#endif
#else
	#define TIL_MAX_PATH 256

	#if (TIL_PLATFORM == TIL_PLATFORM_ANDROID)
		#include <cstddef>
		#include <stdarg.h>
		#include <string.h>
	#endif
#endif

namespace til
{

	// =========================================
	// Unsigned
	// =========================================

#if (TIL_COMPILER == TIL_COMPILER_MSVC)

	typedef unsigned __int64                  uint64; //!< 64-bit unsigned integer
	typedef unsigned long                     uint32; //!< 32-bit unsigned integer
	typedef unsigned short                    uint16; //!< 16-bit unsigned integer
	typedef unsigned char                     uint8;  //!< 8-bit unsigned integer

#elif (TIL_COMPILER == TIL_COMPILER_GPP)

	typedef unsigned long long                uint64; //!< 64-bit unsigned integer
	typedef unsigned long                     uint32; //!< 32-bit unsigned integer
	typedef unsigned short                    uint16; //!< 16-bit unsigned integer
	typedef unsigned char                     uint8;  //!< 8-bit unsigned integer

#endif

	// =========================================
	// Signed
	// =========================================

#if (TIL_COMPILER == TIL_COMPILER_MSVC)

	typedef __int64                           int64;  //!< 64-bit signed integer
	typedef long                              int32;  //!< 32-bit signed integer
	typedef short                             int16;  //!< 16-bit signed integer
	typedef char                              int8;   //!< 8-bit signed integer

#elif (TIL_COMPILER == TIL_COMPILER_GPP)

	typedef long long                         int64;  //!< 64-bit signed integer
	typedef long                              int32;  //!< 32-bit signed integer
	typedef short                             int16;  //!< 16-bit signed integer
	typedef char                              int8;   //!< 8-bit signed integer

#endif

	// =========================================
	// Datatypes
	// =========================================

#if (TIL_COMPILER == TIL_COMPILER_MSVC)

	typedef unsigned char                     byte;  //!< smallest chunk of data
	typedef unsigned short                    word;  //!< two bytes
	typedef unsigned long                     dword; //!< four bytes or two words

#elif (TIL_COMPILER == TIL_COMPILER_GPP)

	typedef unsigned char                     byte;  //!< smallest chunk of data
	typedef unsigned short                    word;  //!< two bytes
	typedef unsigned long                     dword; //!< four bytes or two words

#endif

	// =========================================
	// Colors
	// =========================================

	typedef uint8                             color_8b;  //!< 8-bit color
	typedef uint16                            color_16b; //!< 16-bit color
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	typedef struct { uint8 d[3]; }            color_24b; //!< 24-bit color
#endif
	typedef uint32                            color_32b; //!< 32-bit color

	// =========================================
	// Function pointers
	// =========================================

	//! Message structure
	struct MessageData
	{
		char* message;      /**< Contains the message provided by TinyImageLoader. */
		char* source_file;  /**< The file where the message originated. */
		int source_line;    /**< The line the message came from. */
	};

	//! Message function
	/*! 
		\param MessageData* A pointer containing the message data.

		Message functions are used for logging. You can create your own and attach them to TinyImageLoader.
	*/
	typedef void (*MessageFunc)(MessageData* a_Data);

	//! Pitch data function
	/*!
		\param a_Width The width of the image
		\param a_Height The height of the image
		\param a_BPP The amount of bytes per pixel
		\param a_PitchX The horizontal pitch
		\param a_PitchY The vertical pitch
	*/
	typedef void (*PitchFunc)(uint32 a_Width, uint32 a_Height, uint8 a_BPP, uint32& a_PitchX, uint32& a_PitchY);

	class FileStream;
	//! FileStream creation function
	/*!
		Opens a FileStream with the specified path and options. Used to create your own implementation of file handling.
	*/
	typedef FileStream* (*FileStreamFunc)(const char* a_Path, uint32 a_Options);
	
}; // namespace til

#endif