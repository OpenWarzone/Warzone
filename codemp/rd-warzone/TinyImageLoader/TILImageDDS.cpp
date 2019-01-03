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

	My implementation of the DDS loader is based on the specifications set out
	by the OpenGL standards board. You can find it here:

	http://www.opengl.org/registry/specs/EXT/texture_compression_s3tc.txt

	It describes in clear language the different compression techniques employed
	by DDS. It was very useful for figuring out how to decompress them.

	The basis for the loader is formed by code given to me by a colleague who
	calls himself Seniltai. For his game, he implemented a DDS loader, but he 
	didn't bother decompiling the images by hand.

	Many thanks also to the 

	-knight666

*/

#include "TILImageDDS.h"
#include "TILInternal.h"

#if (TIL_FORMAT & TIL_FORMAT_DDS)

#if (TIL_RUN_TARGET == TIL_TARGET_DEVEL)
	#define DDS_DEBUG(msg, ...)        TIL_PRINT_DEBUG("DDS: "msg, ##__VA_ARGS__)
#else
	#define DDS_DEBUG(msg, ...)
#endif

//#include <math.h>

namespace til
{

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	#define DDS_TYPE(a, b, c, d)       (((d) << 24) + ((c) << 16) + ((b) << 8) + (a))
	#define DDS_FOURCC(a, b, c, d)     (((d) << 24) + ((c) << 16) + ((b) << 8) + (a))

	static const uint32 DDS_FOURCC_DDS                = DDS_FOURCC('D', 'D', 'S', ' ');

	static const uint32 DDS_FOURCC_UNCOMPRESSED       = 0;
	static const uint32 DDS_FOURCC_DXT1               = DDS_FOURCC('D', 'X', 'T', '1');
	static const uint32 DDS_FOURCC_DXT2               = DDS_FOURCC('D', 'X', 'T', '2');
	static const uint32 DDS_FOURCC_DXT3               = DDS_FOURCC('D', 'X', 'T', '3');
	static const uint32 DDS_FOURCC_DXT4               = DDS_FOURCC('D', 'X', 'T', '4');
	static const uint32 DDS_FOURCC_DXT5               = DDS_FOURCC('D', 'X', 'T', '5');
	static const uint32 DDS_FOURCC_RXGB               = DDS_FOURCC('R', 'X', 'G', 'B');
	static const uint32 DDS_FOURCC_ATI1               = DDS_FOURCC('A', 'T', 'I', '1');
	static const uint32 DDS_FOURCC_ATI2               = DDS_FOURCC('A', 'T', 'I', '2');

	static const uint32 DDS_FOURCC_R16F               = 0x0000006F; // 16-bit float Red
	static const uint32 DDS_FOURCC_G16R16F            = 0x00000070; // 16-bit float Red/Green
	static const uint32 DDS_FOURCC_A16B16G16R16F      = 0x00000071; // 16-bit float RGBA
	static const uint32 DDS_FOURCC_R32F               = 0x00000072; // 32-bit float Red
	static const uint32 DDS_FOURCC_G32R32F            = 0x00000073; // 32-bit float Red/Green
	static const uint32 DDS_FOURCC_A32B32G32R32F      = 0x00000074; // 32-bit float RGBA

	#define DDSD_CAPS                    0x00000001
	#define DDSD_HEIGHT                  0x00000002
	#define DDSD_WIDTH                   0x00000004
	#define DDSD_PITCH                   0x00000008
	#define DDSD_PIXELFORMAT             0x00001000
	#define DDSD_MIPMAPCOUNT             0x00020000
	#define DDSD_LINEARSIZE              0x00080000
	#define DDSD_DEPTH                   0x00800000

	#define DDSCAPS_COMPLEX              0x00000008
	#define DDSCAPS_TEXTURE              0x00001000
	#define DDSCAPS_MIPMAP               0x00400000
		
	#define DDSCAPS2_CUBEMAP             0x00000200
	#define DDSCAPS2_CUBEMAP_POSITIVEX   0x00000400
	#define DDSCAPS2_CUBEMAP_NEGATIVEX   0x00000800
	#define DDSCAPS2_CUBEMAP_POSITIVEY   0x00001000
	#define DDSCAPS2_CUBEMAP_NEGATIVEY   0x00002000
	#define DDSCAPS2_CUBEMAP_POSITIVEZ   0x00004000
	#define DDSCAPS2_CUBEMAP_NEGATIVEZ   0x00008000
	#define DDSCAPS2_VOLUME              0x00200000

	struct DDPixelFormat
	{
		int size;
		int flags;
		int fourCC;
		int bpp;
		int redMask;
		int greenMask;
		int blueMask;
		int alphaMask;
	};

	struct DDSCaps
	{
		int caps;
		int caps2;
		int caps3;
		int caps4;
	};

	struct DDColorKey
	{
		int lowVal;
		int highVal;
	};

	struct DDSurfaceDesc
	{
		int size;
		int flags;
		int height;
		int width;
		int pitch;
		int depth;
		int mipMapLevels;
		int alphaBitDepth;
		int reserved;
		int surface;

		DDColorKey ckDestOverlay;
		DDColorKey ckDestBlt;
		DDColorKey ckSrcOverlay;
		DDColorKey ckSrcBlt;

		DDPixelFormat format;
		DDSCaps caps;

		int textureStage;
	};

	struct DataDXT1
	{
		byte c0_lo, c0_hi, c1_lo, c1_hi;
		byte bits_0, bits_1, bits_2, bits_3;
	};

	struct DataDXT5
	{
		byte alpha0, alpha1;
		byte a_bits_0, a_bits_1, a_bits_2, a_bits_3, a_bits_4, a_bits_5;

		byte c0_lo, c0_hi, c1_lo, c1_hi;
		byte bits_0, bits_1, bits_2, bits_3;
	};

#define GETCODE(x, y, data) (data & ((2 * (4 * y + x)) + 1))

	typedef void (*ColorFunc)(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha);

	void ImageDDS::ColorFunc_A8B8G8R8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha)
	{
		color_32b* dst = (color_32b*)&a_Dst[a_DstIndex];
		byte* src = &a_Src[a_SrcIndex * 4];

		*dst = Construct_32b_A8B8G8R8(src[0], src[1], src[2], a_Alpha);
	}

	void ImageDDS::ColorFunc_A8R8G8B8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha)
	{
		color_32b* dst = (color_32b*)&a_Dst[a_DstIndex];
		byte* src = &a_Src[a_SrcIndex * 4];

		*dst = Construct_32b_A8R8G8B8(src[0], src[1], src[2], a_Alpha);
	}

	void ImageDDS::ColorFunc_B8G8R8A8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha)
	{
		color_32b* dst = (color_32b*)&a_Dst[a_DstIndex];
		byte* src = &a_Src[a_SrcIndex * 4];

		*dst = Construct_32b_B8G8R8A8(src[0], src[1], src[2], a_Alpha);
	}

	void ImageDDS::ColorFunc_R8G8B8A8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha)
	{
		color_32b* dst = (color_32b*)&a_Dst[a_DstIndex];
		byte* src = &a_Src[a_SrcIndex * 4];

		*dst = Construct_32b_R8G8B8A8(src[0], src[1], src[2], a_Alpha);
	}

	void ImageDDS::ColorFunc_B8G8R8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha)
	{
		color_32b* dst = (color_32b*)&a_Dst[a_DstIndex];
		byte* src = &a_Src[a_SrcIndex * 4];

		*dst = Construct_32b_B8G8R8(src[0], src[1], src[2]);
	}

	void ImageDDS::ColorFunc_R8G8B8(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha)
	{
		color_32b* dst = (color_32b*)&a_Dst[a_DstIndex];
		byte* src = &a_Src[a_SrcIndex * 4];

		*dst = Construct_32b_R8G8B8(src[0], src[1], src[2]);
	}

	void ImageDDS::ColorFunc_B5G6R5(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha)
	{
		color_16b* dst = (color_16b*)&a_Dst[a_DstIndex];
		byte* src = &a_Src[a_SrcIndex * 4];

		*dst = Construct_16b_B5G6R5(src[0], src[1], src[2]);
	}

	void ImageDDS::ColorFunc_R5G6B5(byte* a_Dst, uint32 a_DstIndex, byte* a_Src, uint32 a_SrcIndex, byte& a_Alpha)
	{
		color_16b* dst = (color_16b*)&a_Dst[a_DstIndex];
		byte* src = &a_Src[a_SrcIndex * 4];

		*dst = Construct_16b_R5G6B5(src[0], src[1], src[2]);
	}

#endif

	ImageDDS::ImageDDS() : Image()
	{
		m_Data = NULL;
		m_Pixels = NULL;
		m_Colors = NULL;
		m_Alpha = NULL;
		m_ColorFunc = NULL;
		m_MipMap = NULL;
		m_MipMapCurrent = 0;
		m_MipMapTotal = 0;
	}

	ImageDDS::~ImageDDS()
	{
		if (m_Data) { delete m_Data; }
		if (m_Pixels) { delete [] m_Pixels; }
		if (m_Colors) { delete m_Colors; }
		if (m_Alpha) { delete m_Alpha; }
		if (m_MipMap) { delete [] m_MipMap; }
	}

	bool ImageDDS::Parse(uint32 a_ColorDepth)
	{
		dword header;
		m_Stream->ReadDWord(&header);

		if (header != DDS_FOURCC_DDS)
		{
			TIL_ERROR_EXPLAIN("%s is not a DDS file or header is invalid.", m_FileName);
			return false;
		}

		DDSurfaceDesc ddsd;
		m_Stream->Read(&ddsd, sizeof(DDSurfaceDesc));

		m_Format = 0;
		m_InternalBPP = 0;
		m_InternalDepth = 0;

		if (ddsd.format.fourCC != 0)
		{
			m_Format = ddsd.format.fourCC;

			m_BlockSize = 0;
			switch (m_Format)
			{

			case DDS_FOURCC_DXT1:
				{
					DDS_DEBUG("Format: DXT1");
					m_BlockSize = 8;
					break;
				}

			case DDS_FOURCC_ATI1:
				{
					DDS_DEBUG("Format: ATI1");
					m_BlockSize = 8;
					break;
				}
				
			case DDS_FOURCC_DXT2:
				{
					DDS_DEBUG("Format: DXT2");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_DXT3:
				{
					DDS_DEBUG("Format: DXT3");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_DXT4:
				{
					DDS_DEBUG("Format: DXT4");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_DXT5:
				{
					DDS_DEBUG("Format: DXT5");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_RXGB:
				{
					DDS_DEBUG("Format: RXGB");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_ATI2:
				{
					DDS_DEBUG("Format: ATI2");
					m_BlockSize = 16;
					break;
				}
			case DDS_FOURCC_A16B16G16R16F:
				{
					DDS_DEBUG("Format: A16B16G16R16F");
					m_BlockSize = 16;
					break;
				}
			default:
				{
					TIL_ERROR_EXPLAIN("Unknown format: 0x%x", ddsd.format.fourCC);
					return false;
				}
				
			}
		}
		else
		{
			DDS_DEBUG("Format: Uncompressed");

			m_Format = DDS_FOURCC_UNCOMPRESSED;

			m_InternalBPP = ddsd.format.bpp >> 3;
			m_BlockSize = m_InternalBPP;

			if (ddsd.format.bpp == 32)
			{
				if (ddsd.format.redMask == 0x00ff0000)
				{
					m_InternalDepth = TIL_DEPTH_A8R8G8B8;
					DDS_DEBUG("Depth: A8R8G8B8");
				}
				else
				{
					m_InternalDepth = TIL_DEPTH_A8B8G8R8;
					DDS_DEBUG("Depth: A8B8G8R8");
				}
			}
			else if (ddsd.format.bpp == 24)
			{
				if (ddsd.format.redMask == 0x00ff0000)
				{
					m_InternalDepth = TIL_DEPTH_R8G8B8;
					DDS_DEBUG("Depth: R8G8B8");
				}
				else
				{
					m_InternalDepth = TIL_DEPTH_B8G8R8;
					DDS_DEBUG("Depth: B8G8R8");
				}
			}
			else
			{
				TIL_ERROR_EXPLAIN("Unknown bit-depth: %d", ddsd.format.bpp);
				return false;
			}
		}

		m_Offset = 0;
		m_Width = ddsd.width;
		m_Height = ddsd.height;
		m_Depth = ddsd.depth;

		m_MipMapTotal = (ddsd.mipMapLevels >= 1 ? ddsd.mipMapLevels : 1);

		DDS_DEBUG("Dimensions: (%d, %d)", m_Width, m_Height);
		DDS_DEBUG("Depth: %d", m_Depth);
		DDS_DEBUG("Mipmaps: %d", m_MipMapTotal);

		m_CubeMap = 1;
		if ((ddsd.caps.caps2 & DDSCAPS2_CUBEMAP) == DDSCAPS2_CUBEMAP)
		{
			DDS_DEBUG("Cubemap");

			if (ddsd.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEX) { DDS_DEBUG("+X"); m_CubeMap++; }
			if (ddsd.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) { DDS_DEBUG("-X"); m_CubeMap++; }
			if (ddsd.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEY) { DDS_DEBUG("+Y"); m_CubeMap++; }
			if (ddsd.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) { DDS_DEBUG("-Y"); m_CubeMap++; }
			if (ddsd.caps.caps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) { DDS_DEBUG("+Z"); m_CubeMap++; }			
			if (ddsd.caps.caps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) { DDS_DEBUG("-Z"); m_CubeMap++; }

			m_CubeMap--;
		}

		m_MipMap = new MipMap[m_MipMapTotal * m_CubeMap];

		switch (m_BPPIdent)
		{

		case BPP_32B_A8B8G8R8:
			{
				m_ColorFunc = &ImageDDS::ColorFunc_A8B8G8R8;
				break;
			}
		case BPP_32B_A8R8G8B8:
			{
				m_ColorFunc = &ImageDDS::ColorFunc_A8R8G8B8;
				break;
			}
		case BPP_32B_B8G8R8A8:
			{
				m_ColorFunc = &ImageDDS::ColorFunc_B8G8R8A8;
				break;
			}
		case BPP_32B_R8G8B8A8:
			{
				m_ColorFunc = &ImageDDS::ColorFunc_R8G8B8A8;
				break;
			}
		case BPP_32B_B8G8R8:
			{
				m_ColorFunc = &ImageDDS::ColorFunc_B8G8R8;
				break;
			}
		case BPP_32B_R8G8B8:
			{
				m_ColorFunc = &ImageDDS::ColorFunc_R8G8B8;
				break;
			}
		case BPP_16B_B5G6R5:
			{
				m_ColorFunc = &ImageDDS::ColorFunc_B5G6R5;
				break;
			}
		case BPP_16B_R5G6B5:
			{
				m_ColorFunc = &ImageDDS::ColorFunc_R5G6B5;
				break;
			}
		default:
			{
				TIL_ERROR_EXPLAIN("Unknown color depth: %i", m_BPPIdent);
				return false;
			}
		}

		m_Pixels = Internal::CreatePixels(m_Width, m_Height, m_BPP, m_PitchX, m_PitchY);

		m_Data = new byte[m_Width * m_Height * m_BlockSize];
		m_Colors = new byte[m_BPP * 8];
		if (m_Format == DDS_FOURCC_DXT5)
		{
			m_Alpha = new byte[16 * sizeof(dword)];
		}

		// decompress

		for (uint32 j = 0; j < m_CubeMap; j++)
		{
			uint32 w = m_Width;
			uint32 h = m_Height;

			for (uint32 i = 0; i < m_MipMapTotal; i++)
			{
				w = (1 > w) ? 1 : w; 
				h = (1 > h) ? 1 : h;

				if (m_Format == DDS_FOURCC_DXT1 || m_Format == DDS_FOURCC_DXT3 || m_Format == DDS_FOURCC_DXT5)
				{
					m_MipMapSize = ((w + 3) >> 2) * ((h + 3) >> 2) * m_BlockSize;
				}
				else
				{
					m_MipMapSize = w * h * m_BlockSize;
				}

				if (m_MipMapSize > m_Width * m_Height * m_BlockSize)
				{
					TIL_ERROR_EXPLAIN("Write buffer not big enough.");
					return false;
				}

				GetBlocks(w, h);
				AddMipMap(w, h);

				DDS_DEBUG("Mipmap %i x %i - reading %i bytes", w, h, m_MipMapSize);

				m_Stream->ReadByte(m_Data, m_MipMapSize);

				if (m_Format == DDS_FOURCC_DXT1)
				{
					DecompressDXT1();
				}
				else if (m_Format == DDS_FOURCC_DXT5)
				{
					DecompressDXT5();
				}
				else if (m_Format == DDS_FOURCC_UNCOMPRESSED)
				{
					if (!DecompressUncompressed())
					{
						return false;
					}
				}
				else
				{
					TIL_ERROR_EXPLAIN("Unknown or unhandled compression algorithm: 0x%x", m_Format);
					return false;
				}

				m_MipMapCurrent++;

				w >>= 1;
				h >>= 1;
			}
		}

		return true;
	}

	void ImageDDS::GetBlocks(uint32 a_Width, uint32 a_Height)
	{
		int powres = 1 << m_MipMapTotal;
		int nW = a_Width / powres;
		int nH = a_Height / powres;
		int nD = m_Depth / powres;

		m_Blocks = 0;

		// 4x4 blocks, 8 bytes per block
		if (m_Format == DDS_FOURCC_DXT1)
		{
			//m_Size = (((nW + 3) / 4) * ((nH + 3) / 4) * 8);
			m_Blocks = (a_Width / 4) * (a_Height / 4);
		}
		// 4x4 blocks, 16 bytes per block
		else if (m_Format == DDS_FOURCC_DXT5 || m_Format == DDS_FOURCC_DXT3)
		{
			//m_Size = (((nW + 3) / 4) * ((nH + 3) / 4) * 16);
			m_Blocks = (a_Width / 4) * (a_Height / 4);
		}
		else
		{
			m_Blocks = a_Width * a_Height;
		}
	}

	void ImageDDS::AddMipMap(uint32 a_Width, uint32 a_Height)
	{
		MipMap* curr = &m_MipMap[m_MipMapCurrent];
		curr->width = a_Width;
		curr->height = a_Height;
		curr->data = Internal::CreatePixels(a_Width, a_Height, m_BPP, curr->pitchx, curr->pitchy);
	}

	void ImageDDS::DecompressDXT1()
	{
		uint32 curr = 0;

		DataDXT1* src = (DataDXT1*)m_Data;
		MipMap* dst = &m_MipMap[m_MipMapCurrent];

		uint32 pitch_blocks = dst->width / 4;

		uint32 glob_x = 0;
		uint32 glob_y = 0;
		uint32 pos_x = 0;
		uint32 pos_y = 0;

		for (uint32 i = 0; i < m_Blocks; i++)
		{
			color_16b color0 = (src->c0_lo + (src->c0_hi << 8));
			color_16b color1 = (src->c1_lo + (src->c1_hi << 8));
			dword bits = (src->bits_0) | (src->bits_1 << 8) | (src->bits_2 << 16) | (src->bits_3 << 24);

			ConstructColors(color0, color1);

			uint32 offset = (color0 > color1) ? 0 : 4;
		
			pos_y = glob_y;
			byte alpha = 255;

			for (int y = 0; y < 4; y++)
			{
				pos_x = glob_x;

				for (int x = 0; x < 4; x++)
				{
					uint32 curr = (2 * (4 * y + x));
					uint32 enabled = ((bits & (0x3 << curr)) >> curr) + offset;

					uint32 index = ((pos_y * dst->pitchx) + pos_x) * m_BPP;
					(this->*m_ColorFunc)(dst->data, index, m_Colors, enabled, alpha);

					pos_x++;
				}

				pos_y++;
			}

			glob_x += 4;
			if (++curr == pitch_blocks)
			{
				curr = 0;

				glob_y += 4;
				glob_x = 0;
			}

			src++;
		}
	}

	void ImageDDS::DecompressDXT5()
	{
		uint32 curr = 0;

		DataDXT5* src = (DataDXT5*)m_Data;
		MipMap* dst = &m_MipMap[m_MipMapCurrent];

		uint32 pitch_blocks = dst->width / 4;

		uint32 glob_x = 0;
		uint32 glob_y = 0;
		uint32 pos_x = 0;
		uint32 pos_y = 0;

		for (uint32 i = 0; i < m_Blocks; i++)
		{
			color_16b color0 = (src->c0_lo + (src->c0_hi << 8));
			color_16b color1 = (src->c1_lo + (src->c1_hi << 8));
			dword bits = (src->bits_0) | (src->bits_1 << 8) | (src->bits_2 << 16) | (src->bits_3 << 24);

			ConstructColors(color0, color1);

			// construct alpha

			uint64 a_bits_total = 
				((uint64)src->a_bits_0      ) | ((uint64)src->a_bits_1 << 8 ) | 
				((uint64)src->a_bits_2 << 16) | ((uint64)src->a_bits_3 << 24) |
				((uint64)src->a_bits_4 << 32) | ((uint64)src->a_bits_5 << 40);

			byte* alpha = (byte*)m_Alpha;

			alpha[0] = src->alpha0;
			alpha[1] = src->alpha1;
			alpha[2] = (6 * src->alpha0 + 1 * src->alpha1) / 7;
			alpha[3] = (5 * src->alpha0 + 2 * src->alpha1) / 7;
			alpha[4] = (4 * src->alpha0 + 3 * src->alpha1) / 7;
			alpha[5] = (3 * src->alpha0 + 4 * src->alpha1) / 7;
			alpha[6] = (2 * src->alpha0 + 5 * src->alpha1) / 7;
			alpha[7] = (1 * src->alpha0 + 6 * src->alpha1) / 7;

			alpha[8] = src->alpha0;
			alpha[9] = src->alpha1;
			alpha[10] = (4 * src->alpha0 + 1 * src->alpha1) / 5;
			alpha[11] = (3 * src->alpha0 + 2 * src->alpha1) / 5;
			alpha[12] = (2 * src->alpha0 + 3 * src->alpha1) / 5;
			alpha[13] = (1 * src->alpha0 + 4 * src->alpha1) / 5;
			alpha[14] = 0;
			alpha[15] = 255;

			pos_y = glob_y;

			int offset = 0;
			if (src->alpha0 <= src->alpha1)
			{
				offset = 8;
			}

			for (int y = 0; y < 4; y++)
			{
				pos_x = glob_x;

				for (int x = 0; x < 4; x++)
				{
					uint64 bits_alpha = 0;

					int curr_a = (3 * (4 * y + x));
					bits_alpha = (a_bits_total & ((uint64)0x7 << curr_a)) >> curr_a;

					uint32 curr = (2 * (4 * y + x));
					uint32 enabled = ((bits & (0x3 << curr)) >> curr);

					uint32 index = ((pos_y * dst->pitchx) + pos_x) * m_BPP;
					(this->*m_ColorFunc)(dst->data, index, m_Colors, enabled, alpha[bits_alpha + offset]);

					pos_x++;
				}

				pos_y++;
			}

			glob_x += 4;
			if (++curr == pitch_blocks)
			{
				curr = 0;

				glob_y += 4;
				glob_x = 0;
			}

			src++;
		}
	}

	bool ImageDDS::DecompressUncompressed()
	{
		byte* read = m_Data;
		byte src[4];
		MipMap* dst = &m_MipMap[m_MipMapCurrent];

		if (m_BPP == 4)
		{
			uint32 r, g, b, a;

			if (m_InternalDepth == TIL_DEPTH_A8R8G8B8)
			{
				r = 2;
				g = 1;
				b = 0;
				a = 3;
			}
			else
			{
				TIL_ERROR_EXPLAIN("Unsupported bit depth: %i", m_InternalDepth);
				return false;
			}

			for (uint32 y = 0; y < dst->height; y++)
			{
				uint32 index_y = (y * dst->pitchx);

				for (uint32 x = 0; x < dst->width; x++)
				{
					src[0] = read[r];
					src[1] = read[g];
					src[2] = read[b];
					src[3] = read[a];

					uint32 index = index_y + x;
					(this->*m_ColorFunc)(dst->data, index * m_BPP, src, 0, src[3]);

					read += 4;
				}
			}
		}
		else if (m_BPP == 2)
		{
			TIL_ERROR_EXPLAIN("Unsupported bit depth: %i", m_BPP);
			return false;
		}
		else
		{
			TIL_ERROR_EXPLAIN("Unsupported bit depth: %i", m_BPP);
			return false;
		}
		
		return true;
	}

	void ImageDDS::ConstructColors(color_16b a_Color0, color_16b a_Color1)
	{
		color_32b* colors = (color_32b*)m_Colors;

		colors[0] = Convert_From_16b_R5G6B5_To_32b_A8B8G8R8(a_Color0);
		colors[1] = Convert_From_16b_R5G6B5_To_32b_A8B8G8R8(a_Color1);
		colors[2] = Convert_From_16b_R5G6B5_To_32b_A8B8G8R8(Blend_16b_R5G6B5(a_Color0, a_Color1, 0xAA));
		colors[3] = Convert_From_16b_R5G6B5_To_32b_A8B8G8R8(Blend_16b_R5G6B5(a_Color0, a_Color1, 0x55));

		colors[4] = colors[0];
		colors[5] = colors[1];
		colors[6] = Convert_From_16b_R5G6B5_To_32b_A8B8G8R8(Blend_16b_R5G6B5(a_Color0, a_Color1, 0x80));
		colors[7] = 0;
	}

	uint32 ImageDDS::GetFrameCount()
	{
		return m_MipMapTotal * m_CubeMap;
	}

	byte* ImageDDS::GetPixels(uint32 a_Frame /*= 0*/)
	{
		return m_MipMap[a_Frame].data;
	}

	uint32 ImageDDS::GetWidth(uint32 a_Frame /*= 0*/)
	{
		return m_MipMap[a_Frame].width;
	}

	uint32 ImageDDS::GetHeight(uint32 a_Frame /*= 0*/)
	{
		return m_MipMap[a_Frame].height;
	}

	uint32 ImageDDS::GetPitchX(uint32 a_Frame /*= 0*/)
	{
		return m_MipMap[a_Frame].pitchx;
	}

	uint32 ImageDDS::GetPitchY(uint32 a_Frame /*= 0*/)
	{
		return m_MipMap[a_Frame].pitchy;
	}

}; // namespace til

#endif