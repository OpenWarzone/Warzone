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

#ifndef _TINYIMAGELOADER_H_
#define _TINYIMAGELOADER_H_

/*! \file TinyImageLoader.h
    \brief The main include for using TinyImageLoader in your project.
    
    Details.
*/

#include "TILSettings.h"
#include "TILFileStream.h"
#include "TILImage.h"

/*! 
	\namespace til
	\brief TinyImageLoader namespace.

	Everything related to TinyImageLoader is contained in this namespace.
*/

namespace til
{

	//! Initializes TinyImageLoader. 
	/*! 
		\param a_Settings Settings for TinyImageLoader.

		Starts TinyImageLoader and determines vital settings.

		Valid settings are a combination of:

		Line endings:
		- #TIL_FILE_CRLF
		- #TIL_FILE_CR
		- #TIL_FILE_LF
	*/
	void TIL_Init(uint32 a_Settings = TIL_SETTINGS);

	//! Shuts down TinyImageLoader
	/*!
		Makes sure all memory allocated by TinyImageLoader (outside of til::Image objects) is deallocated.
	*/
	void TIL_ShutDown();

	//! Main interface for loading images.
	/*!
		\param a_Stream A FileStream handle that does file reading.
		\param a_Options A combination of loading options.

		\return An instance of til::Image or NULL on failure.

		A color depth is required, or the function returns NULL.

		And a color depth option:
		- #TIL_DEPTH_A8R8G8B8
		- #TIL_DEPTH_A8B8G8R8
		- #TIL_DEPTH_R8G8B8A8
		- #TIL_DEPTH_B8G8R8A8
		- #TIL_DEPTH_R8G8B8
		- #TIL_DEPTH_B8G8R8
		- #TIL_DEPTH_R5G6B5
		- #TIL_DEPTH_B5G6R5

		/code
		MyStream* stream = new MyStream();
		stream->Open("MyFile.png", TIL_ADDWORKINGDIR);
		til::Image* load = til::TIL_Load(stream, TIL_DEPTH_A8B8G8R8);
		/endcode
	*/
	Image* TIL_Load(FileStream* a_Stream, uint32 a_Options);

	//! Main interface for loading images.
	/*!
		\param a_FileName A string containing either a path to an image.
		\param a_Options A combination of loading options.

		\return An instance of til::Image or NULL on failure.

		When #TIL_FILE_ABSOLUTEPATH is specified, a_FileName is treated as a complete path.
		If #TIL_FILE_ADDWORKINGDIR is specified, the working directory is appended.

		A color depth is required, or the function returns NULL.

		Valid options include a combination of a file option:
		- #TIL_FILE_ABSOLUTEPATH
		- #TIL_FILE_ADDWORKINGDIR

		And a color depth option:
		- #TIL_DEPTH_A8R8G8B8
		- #TIL_DEPTH_A8B8G8R8
		- #TIL_DEPTH_R8G8B8A8
		- #TIL_DEPTH_B8G8R8A8
		- #TIL_DEPTH_R8G8B8
		- #TIL_DEPTH_B8G8R8
		- #TIL_DEPTH_R5G6B5
		- #TIL_DEPTH_B5G6R5

		/code
		til::Image* load = til::TIL_Load("MyFile.png", TIL_ADDWORKINGDIR | TIL_DEPTH_A8B8G8R8);
		/endcode
	*/
	Image* TIL_Load(const char* a_FileName, uint32 a_Options = (TIL_FILE_ABSOLUTEPATH | TIL_DEPTH_A8R8G8B8));

	//! Releases the handle to a til::Image
	/*!
		\param a_Image The handle to the til::Image

		\return True on success, false on failure

		This is the recommended way to close a handle to a til::Image.
	*/
	bool TIL_Release(Image* a_Image);

	//! Set the internal working directory
	/*!
		\param a_Path The path to set the working directory to.
		\param a_Length The length of the path string.

		\return The length of the string saved.

		This function sets the internal working directory to the value specified.
		When you load an image using #TIL_FILE_ADDWORKINGDIR, this is the string
		that is prepended to the path.

		On Windows systems, this value is set to the value returned by 
		GetModuleFileNameA, which is the path to folder the executable is located.

		On other systems, this value is left blank.
	*/
	size_t TIL_SetWorkingDirectory(const char* a_Path, size_t a_Length);

	//! Adds working directory to a path
	/*!
		\param a_Dst Where to put it
		\param a_MaxLength The length of the destination buffer
		\param a_Path The path to add
		
		Adds the working directory to a path and puts it in the destination buffer.
		If the destination buffer is too small, the path is truncated to fit.
	*/
	void TIL_AddWorkingDirectory(char* a_Dst, size_t a_MaxLength, const char* a_Path);

	//! Set the callback for creating FileStream's.
	/*!
		\param a_Func The creation function

		Used for implementing your own file handling system. Especially useful on embedded platforms.
		Whenever an Image opens a file, it goes through a FileStream. Write your own implementation
		and attach it to TinyImageLoader using this function.
	*/
	void TIL_SetFileStreamFunc(FileStreamFunc a_Func);

	//! Get the version as a string.
	/*!
		\param a_Target The string to write to.
		\param a_MaxLength The size of the destination buffer.

		Get the library version as a string.

		\code
		char version[32];
		TIL_GetVersion(version, 32); // version now contains "1.7.0"
		\endcode
	*/
	void TIL_GetVersion(char* a_Target, size_t a_MaxLength);

	//! Set the callback for the internal error logging function
	/*!
		Whenever a error message is posted, this function will be called.
		This is useful for capturing messages generated by TinyImageLoader.
	*/
	void TIL_SetErrorFunc(MessageFunc a_Func);

	//! Get the errors as a string
	/*!
		\return Error string.

		\note This function will return NULL if a custom callback is attached.

		If no errors were reported, the string is empty.
		An error is in indication something went wrong while loading an image. 

		The messages are in the following format:

		\code
		TinyImageLoader - Error: Something terrible happened!
		\endcode
	*/
	char* TIL_GetError();

	//! Get the length of the error string
	/*!
		\return Length.

		\note This function will return 0 if a custom callback is attached.

		If no errors were reported, this function returns 0.
	*/
	size_t TIL_GetErrorLength();

	//! Set the callback for the internal debug logging function
	/*!
		Whenever a debug message is posted, this function will be called.
		This is useful for capturing messages generated by TinyImageLoader.
	*/
	void TIL_SetDebugFunc(MessageFunc a_Func);

	//! Get the debug messages as a string
	/*!
		\return Debug string.

		If no debug messages were reported, the string is empty. 
		Debug messages are for the developers of this library, not for an end-user.
		Only when compiled under debug mode does the program generate these messages.

		The messages are in the following format:

		\code
		TinyImageLoader - Debug: I figured you'd like this.
		\endcode
	*/
	char* TIL_GetDebug();

	//! Get the length of the debug string
	/*!
		\return Length in characters.

		If no debug messages were posted, this function returns 0.
	*/
	size_t TIL_GetDebugLength();

	//! Clears the debug string
	/*!
		The debug string can get very large. Call this function to clear it.
	*/
	void TIL_ClearDebug();

	//! Set the pitch function to be used internally
	/*!
		\param a_Func The pitch function to attach

		Standard, the width and height of an image are the same as its horizontal
		and vertical pitch. However, in some cases you might want to have a
		different policy in place. For instance, when you're loading textures
		on a platform that doesn't support non-power-of-two textures, you can 
		attach a pitch function that always sets the pitch to a power of two.
	*/
	void TIL_SetPitchFunc(PitchFunc a_Func);

}; // namespace til

#endif