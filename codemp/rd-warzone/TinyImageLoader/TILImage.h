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
	\brief Virtual interface for loading images
*/

#ifndef _TILIMAGE_H_
#define _TILIMAGE_H_

#include "TILSettings.h"
#include "TILColors.h"
#include "TILFileStream.h"

namespace til
{

	// this seemingly pointless forward declaration
	// is necessary to fool doxygen into documenting
	// the class
	class DoxygenSaysWhat;

	/*!
		\brief The virtual interface for loading images and extracting image data.

		All image operations go through this interface. Every image loader is an implementation of this class.
	*/
	class Image
	{

	public:

		// TODO: Replace enum with defines

		//! The amount of bits per pixel and its arrangement.
		enum BitDepth
		{
			BPP_32B_A8R8G8B8 = 1, /**< 32-bit ARGB color */
			BPP_32B_A8B8G8R8 = 2, /**< 32-bit ABGR color */
			BPP_32B_R8G8B8A8 = 3, /**< 32-bit RGBA color */
			BPP_32B_B8G8R8A8 = 4, /**< 32-bit BGRA color */
			BPP_32B_R8G8B8   = 5, /**< 32-bit RGB color */
			BPP_32B_B8G8R8   = 6, /**< 32-bit BGR color */
			BPP_16B_R5G6B5   = 7, /**< 16-bit RGB color */
			BPP_16B_B5G6R5   = 8, /**< 16-bit BGR color */
		};

		Image();
		virtual ~Image();

		//! Sets the bit depth to convert to when parsing.
		/*!
			\param a_Options A bit depth option

			\return True on success, false on failure
		*/
		bool SetBPP(uint32 a_Options);

		//! Get the color depth as an enum
		BitDepth GetBitDepth() { return m_BPPIdent; }

		//! Load an image
		/*!
			\param a_Stream A handle to a FileStream

			The main entrypoint for #til::TIL_Load().
		*/
		void Load(FileStream* a_Stream);

		//! Closes the handle to the image file
		/*!
			Used internally by TinyImageLoader.
		*/
		bool Close();

		//! Parses the actual image data.
		/*!
			\param a_Options The options used for parsing
			
			This method is pure virtual and should be overwritten by an
			image loading implementation.
		*/
		virtual bool Parse(uint32 a_Options) = 0;

		//! Returns the amount of frames this image contains.
		/*!
			Used when dealing with formats that support animation or multiple images.

			\note There is never going to be support for other video formats.
			This is because TinyImageLoader is an *image* loader, not a video loader.
			The exception to the rule are GIF89 and APNG, because both concern an extension
			to a standard of an otherwise single-framed format.
		*/
		virtual uint32 GetFrameCount() { return 1; }

		//! Returns the delay between frames.
		/*!
			\return The delay in seconds between frames

			Used when dealing with formats that support animation.
		*/
		virtual float GetDelay() { return 0; }

		//! Get the pixel data from this image.
		/*!
			\param a_Frame The frame of an animation to return.

			\return Pixel array as a byte array.

			The data is encoded according to the color depth specified.
			For instance, when loading images as 32-bit RGBA, the stream
			of bytes must be converted to unsigned long before being
			used.

			\code
			til::Image* load = TIL_Load("media\\texture.png", TIL_DEPTH_A8B8G8R8 | TIL_FILE_ADDWORKINGDIR);
			unsigned long* pixels = (unsigned long*)load->GetPixels();
			\endcode
		*/
		virtual byte* GetPixels(uint32 a_Frame = 0) = 0;

		//! Get the width of a frame
		/*!
			\param a_Frame The frame of an animation or subimage to return

			\return Width in pixels

			Some formats support multiple frames or images with different dimensions.
			You can call this function with a frame number to get the correct dimensions.
		*/
		virtual uint32 GetWidth(uint32 a_Frame = 0) = 0;

		//! Get the height of a frame
		/*!
			\param a_Frame The frame of an animation or subimage to return

			\return Height in pixels

			Some formats support multiple frames or images with different dimensions.
			You can call this function with a frame number to get the correct dimensions.
		*/
		virtual uint32 GetHeight(uint32 a_Frame = 0) = 0;

		//! Get the horizontal pitch of a frame
		/*!
			\param a_Frame The frame of an animation or subimage to return

			\return Horizontal pitch in pixels

			The pitch can be different than the width of an image. This is the case
			when you've attached your own pitch function using #TIL_SetPitchFunc. 

			It is advised to use this function in favor of #GetWidth when you
			are using TinyImageLoader to load textures.
		*/
		virtual uint32 GetPitchX(uint32 a_Frame = 0) = 0;

		//! Get the vertical pitch of a frame
		/*!
			\param a_Frame The frame of an animation or subimage to return

			\return Vertical pitch in pixels

			The pitch can be different than the height of an image. This is the case
			when you've attached your own pitch function using #TIL_SetPitchFunc. 

			It is advised to use this function in favor of #GetHeight when you
			are using TinyImageLoader to load textures.
		*/
		virtual uint32 GetPitchY(uint32 a_Frame = 0) = 0;

	protected:

		FileStream* m_Stream; //!< The file interface
		char* m_FileName; //!< The filename
		BitDepth m_BPPIdent; //!< The bit depth to convert to
		uint8 m_BPP; //!< The amount of bytes per pixel

	}; // class Image

}; // namespace til

#endif