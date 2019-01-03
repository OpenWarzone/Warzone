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
	\file TILColors.h
	\brief Functions for constructing colors, converting colors and blending colors
*/

#ifndef _TILCOLORS_H_
#define _TILCOLORS_H_

#include "TILSettings.h"

namespace til
{

	// this seemingly pointless forward declaration
	// is necessary to fool doxygen into documenting
	// the class
	class DoxygenSaysWhat;

	/////////////////////////////////////////////////////
	/*!
		@name 16-bit RGB
	*/
	//@{
	/////////////////////////////////////////////////////


	//! Construct a 16-bit RGB color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255

		\return 16-bit color
	*/
	inline color_16b Construct_16b_R5G6B5(uint8 a_Red, uint8 a_Green, uint8 a_Blue)
	{
		return
			(((a_Red   * 0xF800) >> 8) & 0xF800) |
			(((a_Green * 0x07E0) >> 8) & 0x07E0) |
			(((a_Blue  * 0x001F) >> 8) & 0x001F);
	}

	//! Alpha blend a 16-bit RGB color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 16-bit RGB color

		Blends the RGB values with the alpha value.
	*/
	inline color_16b AlphaBlend_16b_R5G6B5(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		color_16b r = (((a_Red   * 0xF800) >> 8) & 0xF800);
		color_16b g = (((a_Green * 0x07E0) >> 8) & 0x07E0);
		color_16b b = (((a_Blue  * 0x001F) >> 8) & 0x001F);

		return (
			(((r * a_Alpha) >> 8) & 0xF800) |
			(((g * a_Alpha) >> 8) & 0x07E0) |
			(((b * a_Alpha) >> 8) & 0x001F)
		);
	}

	//! Blend between two 16-bit RGB colors
	/*!
		\param a_Left Color one
		\param a_Right Color two
		\param a_Factor The amount of blending, between 0 - 255

		\return 16-bit result

		Blends between colors. The higher the factor parameter, the more the blend is shifted towards the second color.
	*/
	inline color_16b Blend_16b_R5G6B5(color_16b a_Left, color_16b a_Right, uint8 a_Factor)
	{
		uint8 inv_fact = 255 - a_Factor;

		color_32b r = ((((a_Left & 0xF800) * a_Factor) + ((a_Right & 0xF800) * inv_fact)) >> 8) & 0xF800;
		color_32b g = ((((a_Left & 0x07E0) * a_Factor) + ((a_Right & 0x07E0) * inv_fact)) >> 8) & 0x07E0;
		color_32b b = ((((a_Left & 0x001F) * a_Factor) + ((a_Right & 0x001F) * inv_fact)) >> 8) & 0x001F;

		return (color_16b)(r | g | b);
	}

	//! Convert a 16-bit RGB color to a 32-bit ARGB color
	/*!
		\param a_Color a 16-bit RGB color

		\return 32-bit ARGB color
	*/
	inline color_32b Convert_From_16b_R5G6B5_To_32b_A8R8G8B8(color_16b a_Color)
	{
		return (
			(((a_Color & 0xF800) * 0xFF) / 0xF800) << 16 |
			(((a_Color & 0x07E0) * 0xFF) / 0x07E0) << 8  |
			(((a_Color & 0x001F) * 0xFF) / 0x001F)       |
			0xFF000000
		);	
	}

	//! Convert a 16-bit RGB color to a 32-bit ABGR color
	/*!
		\param a_Color a 16-bit RGB color

		\return 32-bit ABGR color
	*/
	inline color_32b Convert_From_16b_R5G6B5_To_32b_A8B8G8R8(color_16b a_Color)
	{
		return (
			(((a_Color & 0x001F) * 0xFF) / 0x001F) << 0  |
			(((a_Color & 0x07E0) * 0xFF) / 0x07E0) << 8  |
			(((a_Color & 0xF800) * 0xFF) / 0xF800) << 16 |
			0xFF000000
		);	
	}

	//@}


	/////////////////////////////////////////////////////
	/*!
		@name 16-bit BGR
	*/
	//@{
	/////////////////////////////////////////////////////


	//! Construct a 16-bit BGR color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255

		\return 16-bit BGR color
	*/
	inline color_16b Construct_16b_B5G6R5(uint8 a_Red, uint8 a_Green, uint8 a_Blue)
	{
		return
			(((a_Red   * 0x001F) >> 8) & 0x001F) |
			(((a_Green * 0x07E0) >> 8) & 0x07E0) |
			(((a_Blue  * 0xF800) >> 8) & 0xF800);
	}

	//! Alpha blend a 16-bit BGR color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 16-bit BGR color

		Blends the RGB values with the alpha value.
	*/
	inline color_16b AlphaBlend_16b_B5G6R5(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		color_16b b = (((a_Blue  * 0xF800) >> 8) & 0xF800);
		color_16b g = (((a_Green * 0x07E0) >> 8) & 0x07E0);
		color_16b r = (((a_Red   * 0x001F) >> 8) & 0x001F);
		
		return (
			(((b * a_Alpha) >> 8) & 0xF800) |
			(((g * a_Alpha) >> 8) & 0x07E0) |
			(((r * a_Alpha) >> 8) & 0x001F)
		);
	}

	//! Blend between two 16-bit BGR colors
	/*!
		\param a_Left Color one
		\param a_Right Color two
		\param a_Factor The amount of blending, between 0 - 255

		\return 16-bit result

		Blends between colors. The higher the factor parameter, the more the blend is shifted towards the second color.
	*/
	inline color_16b Blend_16b_B5G6R5(color_16b a_Left, color_16b a_Right, uint8 a_Factor)
	{
		uint8 inv_fact = 255 - a_Factor;

		color_32b r = ((((a_Left & 0x001F) * a_Factor) + ((a_Right & 0x001F) * inv_fact)) >> 8) & 0x001F;
		color_32b g = ((((a_Left & 0x07E0) * a_Factor) + ((a_Right & 0x07E0) * inv_fact)) >> 8) & 0x07E0;
		color_32b b = ((((a_Left & 0xF800) * a_Factor) + ((a_Right & 0xF800) * inv_fact)) >> 8) & 0xF800;
		
		return (color_16b)(b | g | r);
	}

	//! Convert a 16-bit BGR color to a 32-bit ARGB color
	/*!
		\param a_Color a 16-bit BGR color

		\return 32-bit ARGB color
	*/
	inline color_32b Convert_From_16b_B5G6R5_To_32b_A8R8G8B8(color_16b a_Color)
	{
		return (
			(((a_Color & 0x001F) * 0xFF) / 0x001F) << 16 |
			(((a_Color & 0x07E0) * 0xFF) / 0x07E0) << 8  |
			(((a_Color & 0xF800) * 0xFF) / 0xF800)       |
			0xFF000000
		);
	}

	//! Convert a 16-bit BGR color to a 32-bit ABGR color
	/*!
		\param a_Color a 16-bit BGR color

		\return 32-bit ABGR color
	*/
	inline color_32b Convert_From_16b_B5G6R5_To_32b_A8B8G8R8(color_16b a_Color)
	{
		return (
			(((a_Color & 0xF800) * 0xFF) / 0xF800)       |
			(((a_Color & 0x07E0) * 0xFF) / 0x07E0) << 8  |
			(((a_Color & 0x001F) * 0xFF) / 0x001F) << 16 |
			0xFF000000
		);
	}

	//@}

	/////////////////////////////////////////////////////
	/*!
		@name 16-bit BGRA
	*/
	//@{
	/////////////////////////////////////////////////////

	//! Convert a 16-bit BGRA color to a 32-bit ARGB color
	/*!
		\param a_Color a 16-bit BGRA color

		\return 32-bit ARGB color
	*/

	inline color_32b Convert_From_16b_B5G5R5A1_To_32b_A8R8G8B8(color_16b a_Color)
	{
		return (
			0xFF000000 |
			(((a_Color & 0x001F) * 0xFF) / 0x001F)       |
			(((a_Color & 0x03E0) * 0xFF) / 0x03E0) << 8  |
			(((a_Color & 0x7c00) * 0xFF) / 0x7c00) << 16
		);
	}

	//! Convert a 16-bit BGRA color to a 32-bit ABGR color
	/*!
		\param a_Color a 16-bit BGRA color

		\return 32-bit ABGR color
	*/

	inline color_32b Convert_From_16b_B5G5R5A1_To_32b_A8B8G8R8(color_16b a_Color)
	{
		return (
			0xFF000000 |
			(((a_Color & 0x001F) * 0xFF) / 0x001F) << 16 |
			(((a_Color & 0x03E0) * 0xFF) / 0x03E0) << 8  |
			(((a_Color & 0x7c00) * 0xFF) / 0x7c00)
		);
	}

	//! Convert a 16-bit BGRA color to a 32-bit RGBA color
	/*!
		\param a_Color a 16-bit RGBA color

		\return 32-bit RGBA color
	*/

	inline color_32b Convert_From_16b_B5G5R5A1_To_32b_R8G8B8A8(color_16b a_Color)
	{
		return (
			(((a_Color & 0x7c00) * 0xFF) / 0x7c00) << 24  |
			(((a_Color & 0x03E0) * 0xFF) / 0x03E0) << 16  |
			(((a_Color & 0x001F) * 0xFF) / 0x001F) << 8   |
			0x000000FF
		);
	}

	//! Convert a 16-bit BGRA color to a 32-bit BGRA color
	/*!
		\param a_Color a 16-bit BGRA color

		\return 32-bit BGRA color
	*/

	inline color_32b Convert_From_16b_B5G5R5A1_To_32b_B8G8R8A8(color_16b a_Color)
	{
		return (
			(((a_Color & 0x7c00) * 0xFF) / 0x7c00) << 8   |
			(((a_Color & 0x03E0) * 0xFF) / 0x03E0) << 16  |
			(((a_Color & 0x001F) * 0xFF) / 0x001F) << 24  |
			0x000000FF
		);
	}

	//! Convert a 16-bit BGRA color to a 32-bit RGB color
	/*!
		\param a_Color a 16-bit BGRA color

		\return 32-bit RGB color
	*/

	inline color_32b Convert_From_16b_B5G5R5A1_To_32b_R8G8B8(color_16b a_Color)
	{
		return (
			(((a_Color & 0x7c00) * 0xFF) / 0x7c00) << 16  |
			(((a_Color & 0x03E0) * 0xFF) / 0x03E0) << 8   |
			(((a_Color & 0x001F) * 0xFF) / 0x001F)	
		);
	}

	//! Convert a 16-bit BGRA color to a 32-bit BGR color
	/*!
		\param a_Color a 16-bit BGRA color

		\return 32-bit BGR color
	*/

	inline color_32b Convert_From_16b_B5G5R5A1_To_32b_B8G8R8(color_16b a_Color)
	{
		return (
			(((a_Color & 0x001F) * 0xFF) / 0x001F) << 16  |
			(((a_Color & 0x03E0) * 0xFF) / 0x03E0) << 8   |
			(((a_Color & 0x7c00) * 0xFF) / 0x7c00)
		);
	}

	//! Convert a 16-bit BGRA color to a 16-bit RGB color
	/*!
		\param a_Color a 16-bit BGRA color

		\return 16-bit RGB color
	*/

	inline color_16b Convert_From_16b_B5G5R5A1_To_16b_R5G6B5(color_16b a_Color)
	{
		color_16b r = ((( (((a_Color & 0x001F) * 0xFF) / 0x001F) * 0xF800) >> 8) & 0xF800);
		color_16b g = ((( (((a_Color & 0x03E0) * 0xFF) / 0x03E0) * 0x07E0) >> 8) & 0x07E0);
		color_16b b = ((( (((a_Color & 0x7c00) * 0xFF) / 0x7c00) * 0x001F) >> 8) & 0x001F);

		return r | g | b;
	}

	//! Convert a 16-bit BGRA color to a 16-bit BGR color
	/*!
		\param a_Color a 16-bit BGRA color

		\return 16-bit BGR color
	*/

	inline color_16b Convert_From_16b_B5G5R5A1_To_16b_B5G6R5(color_16b a_Color)
	{
		color_16b r = ((( (((a_Color & 0x001F) * 0xFF) / 0x001F ) * 0x001F) >> 8) & 0x001F);
		color_16b g = ((( (((a_Color & 0x03E0) * 0xFF) / 0x03E0 ) * 0x07E0) >> 8) & 0x07E0);
		color_16b b = ((( (((a_Color & 0x7c00) * 0xFF) / 0x7c00 ) * 0xF800) >> 8) & 0xF800);

		return r | g | b;
	}

	//@}
	
	/////////////////////////////////////////////////////
	/*!
		@name 32-bit RGB
	*/
	//@{
	/////////////////////////////////////////////////////

	//! Construct a 32-bit RGB color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha Not used

		\return 32-bit color

		Alpha is ignored, it's a parameter only to allow fast changing between this function and Construct_32b_R8G8B8A8.
	*/
	inline color_32b Construct_32b_R8G8B8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha = 0)
	{
		return
			((a_Red   & 0xFF) << 16) |
			((a_Green & 0xFF) << 8) |
			((a_Blue  & 0xFF));
	}

	//! Alpha blend a 32-bit RGB color
	/*!
		\param a_Color A 32-bit RGB color
		\param a_Amount A value between 0 - 255

		\return 32-bit color

		Alpha blends the color with the specified amount.
	*/
	inline color_32b AlphaBlend_32b_R8G8B8(color_32b a_Color, uint8 a_Amount)
	{
		const color_32b rb = (a_Color & 0x00FF00FF);
		const color_32b g = (a_Color & 0x0000FF00);

		return (
			(((rb * a_Amount) >> 8) & 0x00FF00FF) | 
			(((g * a_Amount) >> 8) & 0x0000FF00)
		);
	}

	//! Alpha blend a 32-bit RGB color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 32-bit color

		Alpha blends the color with the specified amount.
	*/
	inline color_32b AlphaBlend_32b_R8G8B8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		const color_32b rb  = ((a_Red << 16) | (a_Blue))  & 0x00FF00FF;
		const color_32b g   = (a_Green << 8)              & 0x0000FF00;

		return (
			(((rb * a_Alpha) >> 8) & 0x00FF00FF) | 
			(((g  * a_Alpha) >> 8) & 0x0000FF00)
		);
	}

	//! Blend between two 32-bit RGB colors
	/*!
		\param a_Left Color one
		\param a_Right Color two
		\param a_Factor The amount of blending, between 0 - 255

		\return 32-bit result

		Blends between colors. The higher the factor parameter, the more the blend is shifted towards the second color.
	*/
	inline color_32b Blend_32b_R8G8B8(color_32b a_Left, color_32b a_Right, uint8 a_Factor)
	{
		uint8 inv_fact = 255 - a_Factor;

		color_32b r = ((((a_Left & 0xFF0000) * a_Factor) + ((a_Right & 0xFF0000) * inv_fact)) >> 8) & 0xFF0000;
		color_32b g = ((((a_Left & 0x00FF00) * a_Factor) + ((a_Right & 0x00FF00) * inv_fact)) >> 8) & 0x00FF00;
		color_32b b = ((((a_Left & 0x0000FF) * a_Factor) + ((a_Right & 0x0000FF) * inv_fact)) >> 8) & 0x0000FF;

		return r | g | b;
	}

	//@}


	/////////////////////////////////////////////////////
	/*!
		@name 32-bit BGR
	*/
	//@{
	/////////////////////////////////////////////////////


	//! Construct a 32-bit BGR color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha Not used

		\return 32-bit BGR color

		Alpha is ignored, it's a parameter only to allow fast changing between this function and Construct_32b_R8G8B8A8.
	*/
	inline color_32b Construct_32b_B8G8R8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha = 0)
	{
		return
			((a_Blue  & 0xFF) << 16) |
			((a_Green & 0xFF) << 8 ) |
			((a_Red   & 0xFF)      );
	}

	//! Alpha blend a 32-bit BGR color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 32-bit BGR color

		Alpha blends the color with the specified amount.
	*/
	inline color_32b AlphaBlend_32b_B8G8R8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		const color_32b br = ((a_Blue << 16) | (a_Red))  & 0x00FF00FF;
		const color_32b g  = (a_Green << 8)              & 0x0000FF00;

		return (
			(((br * a_Alpha) >> 8) & 0x00FF00FF) | 
			(((g  * a_Alpha) >> 8) & 0x0000FF00)
		);
	}

	//@}


	/////////////////////////////////////////////////////
	/*!
		@name 32-bit ARGB
	*/
	//@{
	/////////////////////////////////////////////////////

	//! Construct a 32-bit ARGB color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 32-bit ARGB color
	*/
	inline color_32b Construct_32b_A8R8G8B8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		return (a_Alpha << 24) | (a_Red << 16) | (a_Green << 8) | (a_Blue);
	}

	//! Construct a 32-bit ARGB color
	/*!
		\param a_Color An ARGB color
		\param a_Alpha A value between 0 - 255

		\return 32-bit ARGB color
	*/
	inline color_32b Construct_32b_A8R8G8B8(color_32b a_Color, uint8 a_Alpha)
	{
		return (a_Alpha << 24) | (a_Color & 0x00FFFFFF);
	}

	//! Alpha blend a 32-bit ARGB color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 32-bit ARGB color

		Alpha blends the color with the specified amount and sets the alpha channel.
	*/
	inline color_32b AlphaBlend_32b_A8R8G8B8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		const color_32b rb  = ((a_Red << 16) | (a_Blue))  & 0x00FF00FF;
		const color_32b g   = (a_Green << 8)              & 0x0000FF00;

		return (
			((a_Alpha << 24)       & 0xFF000000) |
			(((rb * a_Alpha) >> 8) & 0x00FF00FF) | 
			(((g  * a_Alpha) >> 8) & 0x0000FF00)
		);
	}

	//@}


	/////////////////////////////////////////////////////
	/*!
		@name 32-bit ABGR
	*/
	//@{
	/////////////////////////////////////////////////////

	//! Construct a 32-bit ABGR color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 32-bit color
	*/
	inline color_32b Construct_32b_A8B8G8R8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		return (a_Alpha << 24) | (a_Blue << 16) | (a_Green << 8) | (a_Red);
	}

	//! Construct a 32-bit ABGR color
	/*!
		\param a_Color A color value
		\param a_Alpha A value between 0 - 255

		\return 32-bit color
	*/
	inline color_32b Construct_32b_A8B8G8R8(color_32b a_Color, uint8 a_Alpha)
	{
		return (a_Alpha << 24) | (a_Color & 0x00FFFFFF);
	}

	//! Alpha blend a 32-bit ABGR color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 32-bit color

		Alpha blends the color with the specified amount and sets the alpha channel.
	*/
	inline color_32b AlphaBlend_32b_A8B8G8R8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		const color_32b br  = ((a_Blue << 16) | (a_Red)) & 0x00FF00FF;
		const color_32b g   = (a_Green << 8)             & 0x0000FF00;

		const color_32b a    = (a_Alpha << 24) & 0xFF000000; 
		const color_32b br_a = ((br * a_Alpha) >> 8) & 0x00FF00FF;
		const color_32b g_a  = ((g  * a_Alpha) >> 8) & 0x0000FF00;

		return (a | br_a | g_a);
	}

	//@}


	/////////////////////////////////////////////////////
	/*!
		@name 32-bit RGBA
	*/
	//@{
	/////////////////////////////////////////////////////

	//! Construct a 32-bit RGBA color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 32-bit color

		Does not do alpha blending. Simply takes the a_Alpha parameter and puts it in the right place in the color.
	*/
	inline color_32b Construct_32b_R8G8B8A8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		return (a_Red << 24) | (a_Green << 16) | (a_Blue << 8) | (a_Alpha);
	}

	
	//! Alpha blend a 32-bit RGBA color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 32-bit color

		Alpha blends the color with the specified amount and sets the alpha channel.
	*/
	inline color_32b AlphaBlend_32b_R8G8B8A8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		const color_32b rb  = ((a_Red << 16) | (a_Blue)) & 0xFF00FF;
		const color_32b g   = (a_Green << 8);

		return (
			((rb * a_Alpha) & 0xFF00FF00) | 
			((g  * a_Alpha) & 0x00FF0000) | 
			(a_Alpha        & 0x000000FF)
		);
	}

	//@}


	/////////////////////////////////////////////////////
	/*!
		@name 32-bit BGRA
	*/
	//@{
	/////////////////////////////////////////////////////


	//! Construct a 32-bit BGRA color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 32-bit color
	*/
	inline color_32b Construct_32b_B8G8R8A8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		return (a_Blue << 24) | (a_Green << 16) | (a_Red << 8) | (a_Alpha);
	}

	//! Alpha blend a 32-bit BGRA color
	/*!
		\param a_Red A value between 0 - 255
		\param a_Green A value between 0 - 255
		\param a_Blue A value between 0 - 255
		\param a_Alpha A value between 0 - 255

		\return 32-bit color

		Alpha blends the color with the specified amount and sets the alpha channel.
	*/
	inline color_32b AlphaBlend_32b_B8G8R8A8(uint8 a_Red, uint8 a_Green, uint8 a_Blue, uint8 a_Alpha)
	{
		const color_32b br  = ((a_Blue << 16) | (a_Red)) & 0xFF00FF;
		const color_32b g   = (a_Green << 8);

		const color_32b br_a = (br * a_Alpha) & 0xFF00FF00;
		const color_32b g_a  = (g  * a_Alpha) & 0x00FF0000;
		const color_32b a    = a_Alpha        & 0x000000FF;

		return br_a | g_a | a;

		/*return (
			(((rb * a_Alpha) >> 8) & 0xFF00FF00) | 
			(((g  * a_Alpha) >> 8) & 0x00FF0000) |
			(a_Alpha               & 0x000000FF)
		);*/
	}

	//@}

}; // namespace til
	
#endif