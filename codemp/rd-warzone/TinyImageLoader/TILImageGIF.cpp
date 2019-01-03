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

/*

	About this loader:

	I took this code almost verbatim from a folder I found at Breakpoint 2010. 
	You see, one of the patrons was kind enough to set up an FTP server. What
	he put up there (besides porn of course), was a folder called 'coding'.
	This turned out to be treasure trove of old, old code. The year of writing
	is 2010, and the code dates from early 1993 to 2004. Old stuff. Ancient
	stuff. As it turns out, so is the .gif file format. 

	In the folder "Demo tools" was a folder inconspicuously named
	"Neon 08.11.2004". In there, in Old\Despair, I found PIX.C. Which had a
	complete GIF loader! Like I said, the code was taken almost verbatim. Even
	though it was demoscene code and almost two decades old, it worked very
	well.

	I hacked animated GIF's in it myself.

	Something to think about: when PIX.C was written, Netscape hadn't even
	written their extension yet! .GIF's didn't support animation back then!

	My salutations go to you, Mr. Javier Baeza, may your code live forever.

	-knight666

*/

#include "TILImageGIF.h"
#include "TILInternal.h"

//#include <string.h>

#if (TIL_FORMAT & TIL_FORMAT_GIF)

#ifdef TIL_TARGET_DEBUG
	#define GIF_DEBUG(msg, ...)        TIL_PRINT_DEBUG("GIF: "msg, ##__VA_ARGS__)
#else
	#define GIF_DEBUG(msg, ...)
#endif

namespace til
{

#define GIF_TYPE(a, b, c, d)           (((d) << 24) + ((c) << 16) + ((b) << 8) + (a))

	ImageGIF::ImageGIF() : Image()
	{
		m_First = m_Current = NULL;
		m_Palette = NULL;
		m_Colors = NULL;
		m_CurrentColors = NULL;
		m_PrevBuffer = NULL;
		m_Delay = 0.f;
	}

	ImageGIF::~ImageGIF()
	{
		ReleaseMemory(m_First);
		if (m_First) { delete m_First; }
		if (m_PrevBuffer) { delete m_PrevBuffer; }
		if (m_Colors) { delete m_Colors; }
		if (m_Palette) { delete m_Palette; }
	}

	void ImageGIF::AddBuffer()
	{
		uint32 pitchx, pitchy;

		if (m_First)
		{
			m_Current->next = new BufferLinked;
			m_Current->next->next    = NULL;
			m_Current->next->buffer  = Internal::CreatePixels(m_Width, m_Height, m_BPP, pitchx, pitchy);
			m_Current = m_Current->next;
		}
		else
		{ 
			m_First = new BufferLinked;
			m_Current = m_First;
			m_Current->next    = NULL;
			m_Current->buffer  = Internal::CreatePixels(m_Width, m_Height, m_BPP, pitchx, pitchy);
		}
	}

	void ImageGIF::CompileColors(bool a_LocalTable)
	{
		byte* palette = m_Palette;
		byte* colors = m_Colors;

		if (m_Palette) { delete m_Palette; }
		m_Palette = new byte[m_ColorTableSize * 3];
		m_Stream->ReadByte(m_Palette, m_ColorTableSize * 3);

		if (m_Colors) { delete m_Colors; }
		m_Colors = new byte[m_ColorTableSize * m_BPP];
		Internal::MemSet(m_Colors, 0, m_ColorTableSize * m_BPP);

		switch (m_BPPIdent)
		{

		case BPP_32B_A8R8G8B8:
			{
				color_32b* dst = (color_32b*)m_Colors;

				for (uint32 i = 0; i < m_ColorTableSize; i++)
				{
					*dst++ = Construct_32b_A8R8G8B8(
						m_Palette[(i * 3) + 0],
						m_Palette[(i * 3) + 1],
						m_Palette[(i * 3) + 2],
						255
					);
				}

				break;
			}

		case BPP_32B_A8B8G8R8:
			{
				color_32b* dst = (color_32b*)m_Colors;

				for (uint32 i = 0; i < m_ColorTableSize; i++)
				{
					*dst++ = Construct_32b_A8B8G8R8(
						m_Palette[(i * 3) + 0],
						m_Palette[(i * 3) + 1],
						m_Palette[(i * 3) + 2],
						255
					);
				}

				break;
			}

		case BPP_32B_R8G8B8A8:
			{
				color_32b* dst = (color_32b*)m_Colors;

				for (uint32 i = 0; i < m_ColorTableSize; i++)
				{					
					*dst++ = Construct_32b_R8G8B8A8(
						m_Palette[(i * 3) + 0],
						m_Palette[(i * 3) + 1],
						m_Palette[(i * 3) + 2],
						255
					);
				}

				break;
			}

		case BPP_32B_B8G8R8A8:
			{
				color_32b* dst = (color_32b*)m_Colors;

				for (uint32 i = 0; i < m_ColorTableSize; i++)
				{					
					*dst++ = Construct_32b_B8G8R8A8(
						m_Palette[(i * 3) + 0],
						m_Palette[(i * 3) + 1],
						m_Palette[(i * 3) + 2],
						255
					);
				}

				break;
			}

		case BPP_32B_R8G8B8:
			{
				byte* dst = m_Colors;

				for (uint32 i = 0; i < m_ColorTableSize; i++)
				{
					dst[3] = m_Palette[(i * 3) + 0];
					dst[2] = m_Palette[(i * 3) + 1];
					dst[1] = m_Palette[(i * 3) + 2];

					dst += m_BPP;
				}

				break;
			}

		case BPP_32B_B8G8R8:
			{
				byte* dst = m_Colors;

				for (uint32 i = 0; i < m_ColorTableSize; i++)
				{
					dst[3] = m_Palette[(i * 3) + 2];
					dst[2] = m_Palette[(i * 3) + 1];
					dst[1] = m_Palette[(i * 3) + 0];

					dst += m_BPP;
				}

				break;
			}

		case BPP_16B_R5G6B5:
			{
				color_16b* dst = (color_16b*)m_Colors;

				for (uint32 i = 0; i < m_ColorTableSize; i++)
				{
					*dst++ = Construct_16b_R5G6B5(
						m_Palette[(i * 3) + 0],
						m_Palette[(i * 3) + 1],
						m_Palette[(i * 3) + 2]
					);
				}

				break;
			}

		case BPP_16B_B5G6R5:
			{
				color_16b* dst = (color_16b*)m_Colors;

				for (uint32 i = 0; i < m_ColorTableSize; i++)
				{
					*dst++ = Construct_16b_B5G6R5(
						m_Palette[(i * 3) + 0],
						m_Palette[(i * 3) + 1],
						m_Palette[(i * 3) + 2]
					);
				}

				break;
			}

		default:
			{
				TIL_ERROR_EXPLAIN("Unhandled color format: %i", m_BPPIdent);
				break;
			}
		}

		m_CurrentColors = m_Colors;
	}

	bool ImageGIF::Parse(uint32 a_Options/*= TIL_DEPTH_A8R8G8B8*/)
	{
		m_Stream->ReadByte(m_Buffer, 6);

		if (*(uint32*)m_Buffer != GIF_TYPE('G', 'I', 'F', '8'))
		{
			TIL_ERROR_EXPLAIN("No GIF header found.", 0);
			return false;
		}

		m_Stream->Read(m_Buffer, 7, 1);

		GIF_DEBUG("Options: %x %x %x %x %x %x %x",
			m_Buffer[0], m_Buffer[1], m_Buffer[2], 
			m_Buffer[3], m_Buffer[4], m_Buffer[5], m_Buffer[6]);

		bool colortable = ((m_Buffer[4] & 0x80) == 0x80);
		m_ColorTableSize = 2 << (m_Buffer[4] & 0x07);
		CompileColors();

		m_Transparency = false;

		bool animation = false;
		bool transparancy = false;
		bool looping = false;

		m_Stream->ReadByte(m_Buffer, 1);

		/* It's an extension */
		while (m_Buffer[0] == 0x21)
		{   
			m_Stream->ReadByte(m_Buffer, 1);
			
			switch (m_Buffer[0]) 
			{

				// Animation
			case 0xFF:
				{
					GIF_DEBUG("Block: animation");

					m_Stream->ReadByte(m_Buffer, 1);  // rest of header
					m_Stream->ReadByte(m_Buffer, 11); // NETSCAPE2.0
					m_Stream->ReadByte(m_Buffer, 2);  // data follows

					uint16 set_looping;
					m_Stream->ReadWord(&set_looping);

					looping = (set_looping & 0xFFFF) ? true : false;

					GIF_DEBUG("Looping: %s", looping ? "true" : "false");

					m_Stream->ReadByte(m_Buffer, 1); // end;

					animation = true;

					break;
				}

				// Graphic Control Extension
			case 0xF9: 
				{
					GIF_DEBUG("Block: Graphic Control Extension");

					m_Stream->ReadByte(m_Buffer, 2); // rest of header

					if (m_Buffer[1] & 0x01) 
					{
						GIF_DEBUG("Transparent: true");
					}

					uint16 delay;
					m_Stream->ReadWord(&delay);

					GIF_DEBUG("Delay: %i ms", delay);
					m_Delay = (float)(delay) / 1000.f;

					m_Stream->ReadByte(m_Buffer, 2);
					
					//b->transparent = m_Buffer[4];            /* Index in colortable */
					break;
				}

				// Other extension
			default: 
				{
					GIF_DEBUG("Block: Unknown (%x)", m_Buffer[0]);

					m_Stream->ReadByte(m_Buffer, 1);
					
					while (m_Buffer[0] != 0) 
					{
						m_Stream->ReadByte(m_Buffer, m_Buffer[0]);
						m_Stream->ReadByte(m_Buffer);
					}
				}

			}

			m_Stream->ReadByte(m_Buffer);
		}

		if (m_Buffer[0] != 0x2c) 
		{
			TIL_ERROR_EXPLAIN("No image descriptor found.", 0);
			return false;
		}

		m_Stream->ReadByte(m_Buffer, 9);

		m_OffsetX = 0;
		m_OffsetY = 0;

		m_Width  = ((int)m_Buffer[5] * 256) | (int)m_Buffer[4];
		m_Height = ((int)m_Buffer[7] * 256) | (int)m_Buffer[6];

		m_LocalWidth = m_Width;
		m_LocalHeight = m_Height;
		Internal::CreatePixels(m_Width, m_Height, m_BPP, m_LocalPitchX, m_LocalPitchY);
		//m_LocalPitch = m_Width * m_BPP;
		m_LocalPitch = m_LocalPitchX * m_BPP;

		//m_TotalBytes = m_Width * m_Height * m_BPP;
		m_TotalBytes = m_LocalPitchX * m_LocalPitchY * m_BPP;
		m_PrevBuffer = new byte[m_TotalBytes];
		Internal::MemSet(m_PrevBuffer, 0, m_TotalBytes);

		GIF_DEBUG("Width: %i", m_Width);
		GIF_DEBUG("Height: %i", m_Height);
		
		//bool global_colortable = (m_Buffer[8] & 0x80) == 0x80;
		//bool interlaced = (m_Buffer[8] & 0x40) == 0x40;
		//int global_colortable_size = 2 << (m_Buffer[8] & 0x07);

		// =========================================
		// LZW
		// =========================================

		int32 min_code_size, clear_code, eoi_code, free_code, code_size, code_mask;
		int32 work_number;
		uint32 bufidx, buflen, workbits;
		int32 prev_code, first, sp;

		int32 stack[4096];
		int32 prefix[4096];
		int32 extnsn[4096];

		byte* target;

		m_Frames = 0;

		while (1)
		{
			m_Frames++;
			GIF_DEBUG("Frame: %i", m_Frames);

			AddBuffer();
			target = m_Current->buffer;

			Internal::MemCpy(target, m_PrevBuffer, m_TotalBytes);
			//memcpy(target, m_PrevBuffer, m_TotalBytes);

			uint32 xy = (m_OffsetY * m_Width) + (m_OffsetX);

			m_Pitch = m_LocalPitch;
			byte* dst = target + (m_OffsetY * m_LocalPitch) + (m_OffsetX * m_BPP);
			int width = m_LocalWidth;

			uint32 pixels_total = m_Width * m_Height;
			uint32 pixels_found = 0;

			m_Stream->ReadByte(m_Buffer);

			min_code_size = m_Buffer[0];
			GIF_DEBUG("Min code size: %i", min_code_size);

			clear_code = 1 << min_code_size;
			eoi_code = clear_code + 1;
			free_code = clear_code + 2;
			code_size = min_code_size + 1;
			code_mask = (1 << code_size) - 1;

			work_number = 0;
			bufidx = 0;
			buflen = 0;
			workbits = 0;
			prev_code = -2;
			first = 0;
			sp = 0;

			bool zerolength = false;

			/* Decode until we find an End-of-Information code */
			int code = -1;
			while (code != eoi_code) 
			{
				/* Add bytes until we have enough bits for next code */
				while (work_number < code_size) 
				{
					/* Read new data block if needed */
					if (bufidx == buflen) 
					{
						m_Stream->ReadByte(m_Buffer);
						if (m_Buffer[0] == 0)
						{
							GIF_DEBUG("Buffer without a length, found enough pixels.");
							zerolength = true;
							//return false;
							break;
						}
						else
						{
							buflen = m_Buffer[0];
							m_Stream->ReadByte(m_Buffer, buflen);
							bufidx = 0;
						}
					}
					workbits |= ((uint32)m_Buffer[bufidx++]) << work_number;
					work_number += 8;
				}

				if (zerolength)
				{
					GIF_DEBUG("Pixels: %i vs %i.", pixels_found, pixels_total);
					if (pixels_found != pixels_total)
					{
						TIL_ERROR_EXPLAIN("Didn't find enough pixels: %i vs %i.", pixels_found, pixels_total);
						return false;
					}

					break;
				}

				/* Get next code */
				code = workbits & code_mask;
				workbits >>= code_size;
				work_number -= code_size;

				/* Force first code of image to be a clear code */
				if (prev_code == -2) 
				{
					code = clear_code;
				}

				/* Branch on type of code */
				
				/* Reset the decoder */
				if (code == clear_code) 
				{
					code_size = min_code_size + 1;      /* Original code size */
					code_mask = (1 << code_size) - 1;   /* Corresponding mask */
					free_code = clear_code + 2;         /* First pos. in tables */
					prev_code = -1;                     /* Next code is a root code */
				} 
				/* End of Information */
				else if (code == eoi_code) 
				{         
					GIF_DEBUG("End of info.");
					GIF_DEBUG("Pixels: %i vs %i.", pixels_found, pixels_total);
					m_Stream->ReadByte(m_Buffer);
					break;
				} 
				/* 1st code after clearcode */
				else if (prev_code == -1) 
				{
					/* Add to image */

					if (width-- == 0)
					{
						target += m_LocalPitch;
						int32 xy = (int32)((m_OffsetY * m_Pitch) + (m_OffsetX * m_BPP));
						dst = target + xy;
						width = m_LocalWidth - 1;
					}

					//if (code != 127)
					if (!m_Transparency || code != m_TransparentIndex)
					{
						ColorFunc(dst, code);
					}

					first = code;
					prev_code = code;

					//first = prev_code = code;

					dst += m_BPP;
					pixels_found++;
				} 
				/* We've got a normal code */
				else 
				{                
					/* It's a new code */
					if (code >= free_code) 
					{            
						stack[sp++] = first;
						first = prev_code;
					} 
					/* It's an existing code */
					else    
					{
						first = code;
					}

					/* Push string of pixels */
					while (first >= clear_code)
					{       
						stack[sp++] = extnsn[first];
						first = prefix[first];
					}
					/* Push string's root code */
					stack[sp++] = first;

					/* Now add pixels to image */
					while (sp != 0) 
					{
						if (width-- == 0)
						{
							target += m_LocalPitch;
							
							int32 xy = (int32)((m_OffsetY * m_Pitch) + (m_OffsetX * m_BPP));
							dst = target + xy;
							width = m_LocalWidth - 1;
						}

						sp--;
						if (!m_Transparency || stack[sp] != m_TransparentIndex)
						{
							byte* pixel_dst = dst;
							byte* src = &m_CurrentColors[stack[sp] * m_BPP];
							for (int i = 0; i < m_BPP; i++) { *pixel_dst++ = *src++; }
						}

						dst += m_BPP;
						pixels_found++;
					}
					prefix[free_code] = prev_code;
					extnsn[free_code++] = first;
					prev_code = code;

					/* Check if code_size needs to increase */
					if (free_code > code_mask && code_size != 12) 
					{
						code_size++;
						code_mask = (1 << code_size) - 1;
					}
				}
			}

			// image block identifier: 21 F9 04
			uint32 header = 0;
			m_Stream->Read(&header, 1, 3);
			if (header != 0x04F921)
			{
				//fread(m_Buffer, 100, 1, m_Handle);

				GIF_DEBUG("End of file.");
				break;
			}

			m_Stream->ReadByte(m_Buffer, 6);

			m_Transparency = (m_Buffer[0] & 0x01) == 0x01;
			GIF_DEBUG("Transparancy: %s", (m_Transparency) ? "true" : "false");
			if (m_Transparency)
			{
				m_TransparentIndex = m_Buffer[3];

				GIF_DEBUG("Transparant index: %i", m_TransparentIndex);
			}

			if (m_Buffer[5] != 0x2c)
			{
				TIL_ERROR_EXPLAIN("No start of data. (%x %x %x %x %x %x)", 
					m_Buffer[0], m_Buffer[1], m_Buffer[2], 
					m_Buffer[3], m_Buffer[4], m_Buffer[5] 
				);
				break;
			}

			// get header of next block
			m_Stream->ReadByte(m_Buffer, 9);

			m_OffsetX = (m_Buffer[0]) + (m_Buffer[1] << 8);
			m_OffsetY = (m_Buffer[2]) + (m_Buffer[3] << 8);

			m_LocalWidth = (m_Buffer[4]) + (m_Buffer[5] << 8);
			m_LocalHeight = (m_Buffer[6]) + (m_Buffer[7] << 8);

			GIF_DEBUG(
				"Dimensions: (%i, %i)(%i, %i)", 
				m_OffsetX, m_OffsetY, 
				m_LocalWidth, m_LocalHeight
			);

			// get the new colors
			bool localtable = (m_Buffer[8] & (1 << 7)) == (1 << 7);
			bool interlaced = (m_Buffer[8] & (1 << 6)) == (1 << 6);

			GIF_DEBUG("Bitfield: %i", m_Buffer[8] & (1 << 7));

			if (localtable)
			{ 
				m_ColorTableSize = 2 << (m_Buffer[8] & 0x07);
				CompileColors(); 
			}
			else
			{
				GIF_DEBUG("TODO: Use global color table");
			}

			// copy the current frame to be used as the background
			// for the next frame
			//MemCpy(m_PrevBuffer, m_Current->buffer, m_TotalBytes);
			memcpy(m_PrevBuffer, m_Current->buffer, m_TotalBytes);
		}

		return true;
	}

	byte* ImageGIF::GetPixels(uint32 a_Frame)
	{
		if (a_Frame >= 0 && a_Frame < m_Frames)
		{
			BufferLinked* current = m_First;
			for (uint32 i = 0; i < a_Frame; i++) { current = current->next; }
			return current->buffer;
		}

		return NULL;
	}

	uint32 ImageGIF::GetFrameCount()
	{
		return m_Frames;
	}

	float ImageGIF::GetDelay()
	{
		return m_Delay;
	}

	void ImageGIF::ReleaseMemory(BufferLinked* a_Buffer)
	{
		if (a_Buffer) 
		{
			if (a_Buffer->buffer)
			{
				delete a_Buffer->buffer;
				a_Buffer->buffer = NULL;
			}			
			
			if (a_Buffer->next)
			{
				ReleaseMemory(a_Buffer->next); 
				delete a_Buffer->next;
				a_Buffer->next = NULL;
			}
		}
	}

	void ImageGIF::ColorFunc(byte* a_Dst, int32 a_Code)
	{
		/*if (m_BPP == 2)
		{
			*(color_16b*)a_Dst = *(color_16b*)&m_CurrentColors[a_Code * m_BPP];
		}
		else if (m_BPP == 4)
		{
			*(color_32b*)a_Dst = *(color_32b*)&m_CurrentColors[a_Code * m_BPP];
		}*/

		memcpy(a_Dst, &m_CurrentColors[a_Code * m_BPP], m_BPP);
	}

	uint32 ImageGIF::GetWidth(uint32 a_Frame)
	{
		return m_Width;
	}
	uint32 ImageGIF::GetHeight(uint32 a_Frame)
	{
		return m_Height;
	}

	uint32 ImageGIF::GetPitchX(uint32 a_Frame /*= 0*/)
	{
		return m_LocalPitchX;
	}

	uint32 ImageGIF::GetPitchY(uint32 a_Frame /*= 0*/)
	{
		return m_LocalPitchY;
	}

}; // namespace til

#endif