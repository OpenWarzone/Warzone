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

	The following PNG loader is entirely based on the works of Sean Barrett 
	a.k.a. nothings. You can download the original stb_image.c from his website 
	at http://www.nothings.org/stb_image.c

	stb_image is a legendary compact file loader that implements multiple file
	formats in a single C file. It does so in an opaque, but easy to use way.

	The original idea for TinyImageLoader came from reimplementing stb_image.c
	in such a way that it would be readable to me. When I was done I started
	implementing other formats as well. Without stb_image, no TinyImageLoader.

	What I changed:
	* Support for different color depths
	* Added Animated PNG support

	Everything else is Mr. Barrett's code implemented in a different manner.

	-knight666

*/

#include "TILImagePNG.h"
#include "TILInternal.h"

#if (TIL_FORMAT & TIL_FORMAT_PNG)

#if (TIL_RUN_TARGET == TIL_TARGET_DEVEL)
	#define PNG_DEBUG(msg, ...)        TIL_PRINT_DEBUG("PNG: "msg, ##__VA_ARGS__)
#else
	#define PNG_DEBUG(msg, ...)
#endif

namespace til
{

	// fast-way is faster to check than jpeg huffman, but slow way is slower
	#define ZFAST_BITS     9 // accelerate all cases in default tables
	#define ZFAST_MASK     ((1 << ZFAST_BITS) - 1)

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	int paeth(int a, int b, int c)
	{
		int p = a + b - c;
		
		int pa = (p - a);
		int pb = (p - b);
		int pc = (p - c);
		pa *= pa;
		pb *= pb;
		pc *= pc;

		//if (pa > pb || pa > pc) { return (pb > pc) ? c : b; }
		//return a;

		return ((pa > pb || pa > pc) ? ((pb > pc) ? c : b) : a);
	}

	int paeth_left(int a)
	{
		/*int b = 0;
		int c = 0;

		int p = a + b - c;
		int pa = (p - a);
		int pb = (p - b);
		int pc = (p - c);
		if (pa > pb || pa > pc) { return 0; }
		return a;*/

		if (a > 0) { return a; }
		return 0;
	}
	int paeth_middle(int b)
	{
		/*int a = 0;
		int c = 0;

		int p = b;
		int pa = p;
		int pb = 0;
		int pc = p;*/
		//if (b > 0 || b > b) { return (pc < 0) ? 0 : b; }

		if (b > 0) { return b; }
		return 0;
	}

#endif

	#define F_none         0
	#define F_sub          1
	#define F_up           2
	#define F_avg          3
	#define F_paeth        4
	#define F_avg_first    5 
	#define F_paeth_first  6
	#define F_paeth_pixel  7

	static uint8 first_row_filter[5] =
	{
		F_none, F_sub, F_none, F_avg_first, F_paeth_first
	};

	static uint8 first_pixel_filter[8] =
	{
		F_none, F_none, F_up, F_avg_first, F_paeth_first
	};

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	typedef uint8 (*FilterFunc)(uint8*, uint8*, uint8*, int, int);

	uint8 FilterFuncNone       (uint8* a_Cur, uint8* a_Target, uint8* a_Prior, int a_Index, int a_Size) 
	{ 
		return a_Target[a_Index]; 
	}
	uint8 FilterFuncSub        (uint8* a_Cur, uint8* a_Target, uint8* a_Prior, int a_Index, int a_Size) 
	{ 
		return a_Target[a_Index] + a_Cur[a_Index - a_Size]; 
	}
	uint8 FilterFuncUp         (uint8* a_Cur, uint8* a_Target, uint8* a_Prior, int a_Index, int a_Size) 
	{ 
		return a_Target[a_Index] + a_Prior[a_Index]; 
	}
	uint8 FilterFuncAvg        (uint8* a_Cur, uint8* a_Target, uint8* a_Prior, int a_Index, int a_Size) 
	{ 
		return a_Target[a_Index] + ((a_Prior[a_Index] + a_Cur[a_Index - a_Size]) >> 1); 
	}
	uint8 FilterFuncPaeth      (uint8* a_Cur, uint8* a_Target, uint8* a_Prior, int a_Index, int a_Size) 
	{ 
		return (uint8)(a_Target[a_Index] + paeth(a_Cur[a_Index - a_Size], a_Prior[a_Index], a_Prior[a_Index - a_Size]));
	}
	uint8 FilterFuncAvgFirst   (uint8* a_Cur, uint8* a_Target, uint8* a_Prior, int a_Index, int a_Size) 
	{ 
		return a_Target[a_Index] + (a_Cur[a_Index - a_Size] >> 1); 
	}
	uint8 FilterFuncAvgPixel   (uint8* a_Cur, uint8* a_Target, uint8* a_Prior, int a_Index, int a_Size) 
	{ 
		return a_Target[a_Index] + (a_Prior[a_Index] >> 1); 
	}
	uint8 FilterFuncPaethFirst (uint8* a_Cur, uint8* a_Target, uint8* a_Prior, int a_Index, int a_Size) 
	{ 
		return (uint8)(a_Target[a_Index] + paeth(a_Cur[a_Index - a_Size], 0, 0)); 
		//return (uint8)(a_Target[a_Index] + paeth_left(a_Cur[a_Index - a_Size])); 
	}
	uint8 FilterFuncPaethPixel (uint8* a_Cur, uint8* a_Target, uint8* a_Prior, int a_Index, int a_Size) 
	{ 
		return (uint8)(a_Target[a_Index] + paeth(0, a_Prior[a_Index], 0));
		//return (uint8)(a_Target[a_Index] + paeth_middle(a_Prior[a_Index]));
	}
	
	static FilterFunc g_Filter[7] = {
		FilterFuncNone,
		FilterFuncSub,
		FilterFuncUp,
		FilterFuncAvg,
		FilterFuncPaeth,
		FilterFuncAvgFirst,
		FilterFuncPaethFirst
	};

	static FilterFunc g_FilterFirst[7] = {
		FilterFuncNone,            // None
		FilterFuncNone,            // Sub
		FilterFuncUp,              // Up
		FilterFuncAvgPixel,        // Avg
		FilterFuncPaethPixel,      // Paeth 
		FilterFuncNone,            // AvgFirst
		FilterFuncNone             // PaethFirst
	};

	void ImagePNG::ColorFunc_A8R8G8B8(uint8* a_Dst, uint8* a_Src)
	{
		color_32b* dst = (color_32b*)a_Dst;
		*dst = Construct_32b_A8R8G8B8(a_Src[0], a_Src[1], a_Src[2], a_Src[3]);
	}

	void ImagePNG::ColorFunc_A8B8G8R8(uint8* a_Dst, uint8* a_Src)
	{
		color_32b* dst = (color_32b*)a_Dst;
		*dst = Construct_32b_A8B8G8R8(a_Src[0], a_Src[1], a_Src[2], a_Src[3]);
	}

	void ImagePNG::ColorFunc_R8G8B8A8(uint8* a_Dst, uint8* a_Src)
	{
		color_32b* dst = (color_32b*)a_Dst;
		*dst = Construct_32b_R8G8B8A8(a_Src[0], a_Src[1], a_Src[2], a_Src[3]);
	}

	void ImagePNG::ColorFunc_B8G8R8A8(uint8* a_Dst, uint8* a_Src)
	{
		color_32b* dst = (color_32b*)a_Dst;
		*dst = Construct_32b_B8G8R8A8(a_Src[0], a_Src[1], a_Src[2], a_Src[3]);
	}

	void ImagePNG::ColorFunc_R8G8B8(uint8* a_Dst, uint8* a_Src)
	{
		color_32b* dst = (color_32b*)a_Dst;
		*dst = AlphaBlend_32b_R8G8B8(a_Src[0], a_Src[1], a_Src[2], a_Src[3]);
	}

	void ImagePNG::ColorFunc_B8G8R8(uint8* a_Dst, uint8* a_Src)
	{
		color_32b* dst = (color_32b*)a_Dst;
		*dst = AlphaBlend_32b_B8G8R8(a_Src[0], a_Src[1], a_Src[2], a_Src[3]);
	}

	void ImagePNG::ColorFunc_R5G6B5(uint8* a_Dst, uint8* a_Src)
	{
		color_16b* dst = (color_16b*)a_Dst;
		*dst = Construct_16b_R5G6B5(a_Src[0], a_Src[1], a_Src[2]);
	}

	void ImagePNG::ColorFunc_B5G6R5(uint8* a_Dst, uint8* a_Src)
	{
		color_16b* dst = (color_16b*)a_Dst;
		*dst = Construct_16b_B5G6R5(a_Src[0], a_Src[1], a_Src[2]);
	}

	//static ColorFunc g_ColorFuncPNG = NULL;

#endif

	static int length_base[31] = {
		3,4,5,6,7,8,9,10,11,13,
		15,17,19,23,27,31,35,43,51,59,
		67,83,99,115,131,163,195,227,258,0,0 
	};

	static int length_extra[31] = { 
		0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 
	};

	static int dist_base[32] = { 
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0
	};

	static int dist_extra[32] = { 
		0,  0,  0,  0,  1,  1,  2,  2,
		3,  3,  4,  4,  5,  5,  6,  6, 
		7,  7,  8,  8,  9,  9, 10, 10,
		11, 11, 12, 12, 13, 13
	};

	#define PNG_COLOR_MASK_PALETTE     0x00000001
	#define PNG_COLOR_MASK_COLOR       0x00000002
	#define PNG_COLOR_MASK_ALPHA       0x00000004

	#define PNG_COLOR_TYPE_GRAY        0
	#define PNG_COLOR_TYPE_PALETTE     (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
	#define PNG_COLOR_TYPE_RGB         (PNG_COLOR_MASK_COLOR)
	#define PNG_COLOR_TYPE_RGB_ALPHA   (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
	#define PNG_COLOR_TYPE_GRAY_ALPHA  (PNG_COLOR_MASK_ALPHA)

	// Animated PNG defines

	#define APNG_DISPOSE_OP_NONE        0
	#define APNG_DISPOSE_OP_BACKGROUND  1
	#define APNG_DISPOSE_OP_PREVIOUS    2

	#define APNG_BLEND_OP_SOURCE        0
	#define APNG_BLEND_OP_OVER          1

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	int32 BitReverse16(int32 a_Number)
	{
		a_Number  = ((a_Number & 0xAAAA) >> 1) | ((a_Number & 0x5555) << 1);
		a_Number  = ((a_Number & 0xCCCC) >> 2) | ((a_Number & 0x3333) << 2);
		a_Number  = ((a_Number & 0xF0F0) >> 4) | ((a_Number & 0x0F0F) << 4);
		a_Number  = ((a_Number & 0xFF00) >> 8) | ((a_Number & 0x00FF) << 8);
		return a_Number;
	}

	int32 BitReverse(int32 a_Number, int32 a_Bits)
	{
		//assert(bits <= 16);
		// to bit reverse n bits, reverse 16 and shift
		// e.g. 11 bits, bit reverse and shift away 5
		return BitReverse16(a_Number) >> (16 - a_Bits);
	}

#endif

	// =========================================
	// Huffman
	// =========================================

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	class Huffman
	{

	public:

		Huffman() 
		{
		}
		~Huffman() 
		{
			int i = 0;
		}

		bool Build(uint8* a_SizeList, uint32 a_Amount)
		{
			int code, next_code[16];

			int sizes[17] = { 0 };

			// DEFLATE spec for generating codes
			memset(sizes, 0, sizeof(sizes));
			memset(fast, 255, sizeof(fast));

			//if (a_Amount > )

			for (uint32 i = 0; i < a_Amount; ++i) { ++sizes[a_SizeList[i]]; }
			sizes[0] = 0;
			//for (int i = 1; i < 16; ++i) { assert(sizes[i] <= (1 << i)); }
			code = 0;

			uint32 k = 0;
			for (uint32 i = 1; i < 16; ++i) 
			{
				next_code[i] = code;
				firstcode[i] = (uint16)code;
				firstsymbol[i] = (uint16)k;
				code = (code + sizes[i]);
				if (sizes[i] && code - 1 >= (1 << i))
				{
					TIL_ERROR_EXPLAIN("Bad codelengths.");
					return false;
				}
				maxcode[i] = code << (16 - i); // preshift for inner loop
				code <<= 1;
				k += sizes[i];
			}

			maxcode[16] = 0x10000; // sentinel
			for (uint32 i = 0; i < a_Amount; ++i) 
			{
				int s = a_SizeList[i];
				if (s > 0) 
				{
					int c = next_code[s] - firstcode[s] + firstsymbol[s];
					size[c] = (uint8)s;
					value[c] = (uint16)i;
					if (s <= ZFAST_BITS) 
					{
						int k = BitReverse(next_code[s], s);
						while (k < (1 << ZFAST_BITS)) 
						{
							fast[k] = (uint16) c;
							k += (1 << s);
						}
					}
					++next_code[s];
				}
			}

			return true;
		}

		uint16 fast[1 << ZFAST_BITS];
		uint16 firstcode[16];
		int maxcode[17];
		uint16 firstsymbol[16];
		uint8  size[288];
		uint16 value[288];

	}; // class Huffman
	
#endif

	// ================================
	// zbuf
	// ================================

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	zbuf::zbuf()
	{
		for (int i = 0; i <= 287; i++)
		{
			if (i <= 143)      { default_length[i] = 8; }
			else if (i <= 255) { default_length[i] = 9; }
			else if (i <= 279) { default_length[i] = 7; }
			else               { default_length[i] = 8; }
		}

		for (int i = 0; i <= 31; ++i) { default_distance[i] = 5; }

		buffer = NULL;
		bigger_data = NULL;
		z_length = NULL;
		z_distance = NULL;
	}

	zbuf::~zbuf()
	{
		//delete buffer;
		//if (buffer) { delete buffer; }
		if (bigger_data) { delete bigger_data; }
		if (z_length) { delete z_length; }
		if (z_distance) { delete z_distance; }

		int i = 0;
	}


	uint32 zbuf::GetLength()
	{
		return (uint32)(zout - zout_start);
	}

	byte* zbuf::GetData()
	{
		return (byte*)zout_start;
	}

	int32 zbuf::GetByte()
	{
		if (zbuffer >= zbuffer_end) { return 0; }
		return *zbuffer++;
	}

	uint32 zbuf::GetCode( uint32 a_Amount )
	{
		if (num_bits < a_Amount) 
		{				
			do 
			{
				if (code_buffer >= (1U << num_bits))
				{
					TIL_ERROR_EXPLAIN("Something went terribly wrong.");
					return -1;
				}

				code_buffer |= (GetByte() << num_bits);
				num_bits += 8;
			} 
			while (num_bits <= 24);
		}

		uint32 result = code_buffer & ((1 << a_Amount) - 1);
		code_buffer >>= a_Amount;
		num_bits -= a_Amount;

		return result;
	}

	int zbuf::HuffmanDecode( Huffman* a_Huffman )
	{
		if (num_bits < 16) 
		{
			do 
			{
				if (code_buffer >= (1U << num_bits))
				{
					TIL_ERROR_EXPLAIN("Something went terribly wrong.");
					return -1;
				}

				code_buffer |= (GetByte() << num_bits);
				num_bits += 8;
			} 
			while (num_bits <= 24);
		}

		int b = a_Huffman->fast[code_buffer & ZFAST_MASK];
		if (b < 0xffff) 
		{
			int size = a_Huffman->size[b];
			code_buffer >>= size;
			num_bits -= size;
			return a_Huffman->value[b];
		}

		// not resolved by fast table, so compute it the slow way
		// use jpeg approach, which requires MSbits at top
		int k = BitReverse(code_buffer, 16);
		int s;
		for (s = ZFAST_BITS + 1; ; ++s)
		{
			if (k < a_Huffman->maxcode[s])
			{
				break;
			}
		}

		if (s == 16) 
		{
			TIL_ERROR_EXPLAIN("Invalid s: 16.");
			return -1; // invalid code!
		}
		// code size is s, so:
		b = (k >> (16 - s)) - a_Huffman->firstcode[s] + a_Huffman->firstsymbol[s];
		if (a_Huffman->size[b] != s)
		{
			TIL_ERROR_EXPLAIN("Huffman size doesn't match code size.");
			return -1;
		}
		code_buffer >>= s;
		num_bits -= s;

		return (a_Huffman->value[b]);
	}

	bool zbuf::Expand( int32 a_Amount )
	{
		if (!z_expandable) 
		{
			TIL_ERROR_EXPLAIN("Could not expand ZLib buffer, reached limit.");
			return false;
		}

		int cur   = (int)(zout     - zout_start);
		int limit = (int)(zout_end - zout_start);

		// double until greater
		while (cur + a_Amount > limit) { limit *= 2; }

		bigger_data = new char[limit];
		memcpy(bigger_data, zout_start, limit / 2);
		delete zout_start;

		zout_start = bigger_data;
		zout       = bigger_data + cur;
		zout_end   = bigger_data + limit;

		return true;
	}

	Huffman* zbuf::ZLibDecode(uint8* a_Data, uint32 a_Length)
	{
		const int32 initial_size = 16384;

		buffer = new char[initial_size];

		zbuffer = (uint8*)a_Data;
		zbuffer_end = (uint8*)a_Data + a_Length;

		zout_start    = buffer;
		zout          = buffer;
		zout_end      = buffer + initial_size;
		z_expandable  = 1;

		// check header	

		int cmf   = GetByte();
		int cm    = cmf & 15;
		int flg   = GetByte();
		if ((cmf * 256 + flg) % 31 != 0) 
		{
			TIL_ERROR_EXPLAIN("Bad ZLib header.", 0);
			return NULL;
		}
		if (flg & 32) 
		{
			TIL_ERROR_EXPLAIN("Preset dictionary not allowed.", 0);
			return NULL;
		}
		if (cm != 8)
		{
			TIL_ERROR_EXPLAIN("Bad compression.", 0);
			return NULL;
		}

		num_bits = 0;
		code_buffer = 0;

		int final = 0;

		Huffman* result = NULL;

		do 
		{
			final = GetCode(1);
			int type = GetCode(2);

			if (type == 0) 
			{
				ParseUncompressedBlock();
				return NULL;
			} 
			// indexed color
			else if (type == 3) 
			{
				TIL_ERROR_EXPLAIN("TODO: Type == 3.", 0);
				return NULL;
			} 
			else 
			{
				if (type == 1) 
				{
					//InitDefaults();

					z_length->Build(default_length, 288);
					z_distance->Build(default_distance, 32);
				} 
				else 
				{
					static uint8 length_dezigzag[19] = { 16, 17, 18,  0,  8,  7,  9,  6, 10, 5, 11,  4, 12,  3, 13,  2, 14,  1, 15 };
					uint8 lencodes[286 + 32 + 137]; //padding for maximum single op
					uint8 codelength_sizes[19];

					int hlit  = GetCode(5) + 257;
					int hdist = GetCode(5) + 1;
					int hclen = GetCode(4) + 4;

					memset(codelength_sizes, 0, sizeof(codelength_sizes));
					for (int32 i = 0; i < hclen; ++i)
					{
						int s = GetCode(3);
						codelength_sizes[length_dezigzag[i]] = (uint8)s;
					}

					result = new Huffman();
					if (!result->Build(codelength_sizes, 19))
					{
						TIL_ERROR_EXPLAIN("Failed to build Huffman thing.", 0);
						return NULL;
					}

					int n = 0;
					while (n < hlit + hdist) 
					{
						//int c = zhuffman_decode(a, &z_codelength);
						int c = this->HuffmanDecode(result);
						//assert(c >= 0 && c < 19);
						if (c < 0 || c >= 19)
						{
							TIL_ERROR_EXPLAIN("Couldn't Huffman decode data.", 0);
							return NULL;
						}
						if (c < 16)
						{
							lencodes[n++] = (uint8)c;
						}
						else if (c == 16) 
						{
							c = GetCode(2) + 3;
							memset(lencodes + n, lencodes[n - 1], c);
							n += c;
						} 
						else if (c == 17) 
						{
							c = GetCode(3) + 3;
							memset(lencodes + n, 0, c);
							n += c;
						} 
						else 
						{
							if (c != 18)
							{
								TIL_ERROR_EXPLAIN("C is too big.", 0);
								return NULL;
							}
							c = GetCode(7) + 11;
							memset(lencodes + n, 0, c);
							n += c;
						}
					}

					if (n != hlit + hdist) 
					{
						TIL_ERROR_EXPLAIN("Bad codelength.", 0);
						return NULL;
					}

					z_length = new Huffman();
					if (!z_length->Build(lencodes, hlit))
					{
						TIL_ERROR_EXPLAIN("Could not build z_length.", 0);
					}
					z_distance = new Huffman();
					z_distance->Build(lencodes + hlit, hdist);
				}

				// parse block

				while (1)
				{
					int z = HuffmanDecode(z_length);
					if (z < 256)
					{
						if (z < 0) 
						{
							TIL_ERROR_EXPLAIN("Bad Huffman code. (%i)", z);
							return NULL;
						}

						if (zout >= zout_end) 
						{
							PNG_DEBUG("Expanding buffer.");
							if (!Expand(1))
							{
								TIL_ERROR_EXPLAIN("Could not expand stuff.", 0);
								return 0;
							}
						}
						*zout++ = (char)z;
					} 
					else 
					{
						if (z == 256) 
						{ 
							break;
							//return 1; 
						}

						z -= 257;
						int len = length_base[z];
						if (length_extra[z]) 
						{
							len += GetCode(length_extra[z]);
						}

						z = HuffmanDecode(z_distance); 
						if (z < 0) 
						{
							TIL_ERROR_EXPLAIN("Bad Huffman code. (%i)", z);
							return NULL;
						}

						int dist = dist_base[z];
						if (dist_extra[z]) 
						{
							dist += GetCode(dist_extra[z]);
						}
						if (zout - zout_start < dist) 
						{
							TIL_ERROR_EXPLAIN("Bad distance. (%i)", dist);
							return NULL;
						}
						if (zout + len > zout_end) 
						{
							if (!Expand(len)) 
							{
								TIL_ERROR_EXPLAIN("Could not expand stuff.", 0);
								return NULL;
							}
						}

						uint8* p = (uint8*)(zout - dist);
						while (len--)
						{
							*zout++ = *p++;
						}
					}
				}
			}
		} 
		while (!final);

		//return result;
		delete result;

		return NULL;
	}

	bool zbuf::ParseUncompressedBlock()
	{
		uint8 header[4];
		int len, nlen, k;
		if (num_bits & 7)
		{
			GetCode(num_bits & 7); // discard
		}
		// drain the bit-packed data into header
		k = 0;
		while (num_bits > 0) 
		{
			header[k++] = (uint8)(code_buffer & 255); // wtf this warns?
			code_buffer >>= 8;
			num_bits -= 8;
		}

		// now fill header the normal way
		while (k < 4)
		{
			header[k++] = (uint8)GetByte();
		}
		len  = header[1] * 256 + header[0];
		nlen = header[3] * 256 + header[2];
		if (nlen != (len ^ 0xffff))
		{
			TIL_ERROR_EXPLAIN("Corrupt PNG: ZLib corrupt", 0);
			return false;
		}
		if (zbuffer + len > zbuffer_end)
		{
			TIL_ERROR_EXPLAIN("Corrupt PNG: read past buffer", 0);
			return false;
		}
		if (zout + len > zout_end)
		{
			if (!Expand(len)) 
			{
				return false;
			}
		}

		memcpy(zout, zbuffer, len);
		zbuffer += len;
		zout += len;

		return true;
	}

#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

	bool ImagePNG::Decompile(
		byte* a_Dst, byte* a_Src, 
		uint32 a_Width, uint32 a_Height, uint32 a_Pitch, 
		int a_Depth, 
		int a_OffsetX, int a_OffsetY
	)
	{
		byte* out = new byte[a_Width * a_Height * 4];
		uint8* cur = out;

		uint32 pitch_dst = a_Pitch;
		uint32 pitch_src = a_Width * 4;

		// NOTE: Magic number 4
		a_Dst += (pitch_dst * a_OffsetY) + (a_OffsetX * 4);

		for (uint32 j = 0; j < a_Height; ++j) 
		{
			byte* dst = a_Dst;
			uint8* src = cur;
			uint8* prior = cur - pitch_src;

			int filter = *a_Src++;
			if (filter > 4) 
			{
				TIL_ERROR_EXPLAIN("Invalid filter (%i).", filter);
				return false;								
			}

			// if first row, use special filter that doesn't sample previous row
			if (j == 0) 
			{
				filter = first_row_filter[filter];
			}

			// handle first pixel explicitly

			src[3] = 255;
			for (int k = 0; k < a_Depth; ++k)
			{
				src[k] = g_FilterFirst[filter](src, a_Src, prior, k, a_Depth);
			}

			(this->*m_ColorFunc)(dst, src);
			dst += 4;

			a_Src += a_Depth;
			src   += 4;
			prior += 4;

			for (int i = a_Width - 1; i >= 1; --i)
			{
				src[3] = 255;
				for (int k = 0; k < a_Depth; ++k)
				{
					src[k] = g_Filter[filter](src, a_Src, prior, k, 4);
				}
				(this->*m_ColorFunc)(dst, src);
				dst += 4;

				a_Src  += a_Depth;
				src    += 4;
				prior  += 4;
			}

			cur += pitch_src;
			a_Dst += pitch_dst;
		}

		delete out;

		return true;
	}

#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
	
	struct ImagePNG::AnimationData
	{

		AnimationData()
		{
			chunk_idat = false;

			data = NULL;
			data_limit = 0;
			data_offset = 0;

			control = 0;
			curr = 0;
			num_plays = 0;
			index = 0;
			w = h = 0;
			ox = oy = 0;
			delay_num = delay_den = 0;
			dispose = blend = 0;
		}

		~AnimationData()
		{

		}

		void NextFrame(byte** a_Data)
		{
			if (data_offset > 0)
			{
				if (chunk_idat) { curr++; }
				chunk_idat = false;

				a_Data[curr] = Internal::CreatePixels(w, h, bpp, px, py);
			
				if (dispose == APNG_DISPOSE_OP_NONE)
				{
					PNG_DEBUG("Dispose method: %s", "APNG_DISPOSE_OP_NONE");

					if (curr > 1)
					{
						memcpy(a_Data[curr], a_Data[curr - 1], bytes_total);
					}
				}
				else if (dispose == APNG_DISPOSE_OP_PREVIOUS)
				{
					PNG_DEBUG("Dispose method: %s", "APNG_DISPOSE_OP_PREVIOUS");

					memcpy(a_Data[curr], frame_prev, bytes_total);
				}
				else if (dispose == APNG_DISPOSE_OP_BACKGROUND)
				{
					PNG_DEBUG("Dispose method: %s", "APNG_DISPOSE_OP_BACKGROUND");

					memset(a_Data[curr], 0, bytes_total);
				}
				else
				{
					PNG_DEBUG("Dispose method: %s", "Other");

					memset(a_Data[curr], 0, bytes_total);
				}

				PNG_DEBUG("Filling index %i", curr);

				zbuf decompress;
				decompress.ZLibDecode(data, data_offset);
				uint32 len = decompress.GetLength();
				byte* final = decompress.GetData();

				PNG_DEBUG("ZLib length: %i", len);
				PNG_DEBUG("Size: (%i, %i) Offset: (%i, %i)", w, h, ox, oy);

				instance->Decompile(a_Data[curr], final, w, h, pitchx * image_bpp, image_bpp, ox, oy);

				if (dispose != APNG_DISPOSE_OP_PREVIOUS)
				{
					memcpy(frame_prev, a_Data[curr], bytes_total);
				}

				curr++;
			}
			else
			{
				if (dispose == APNG_DISPOSE_OP_NONE)
				{
					memcpy(frame_prev, a_Data[curr], bytes_total);
				}
			}

			if (data) { delete data; }
			data = NULL;
			data_limit = 0;
			data_offset = 0;
		}

		byte* frame_prev;

		ImagePNG* instance;

		bool chunk_idat;
		uint8* data;
		uint32 data_limit, data_offset;

		uint32 control;
		uint32 num_plays;
		uint32 index, curr;

		uint8 bpp, image_bpp;
		uint32 w, h;
		uint32 px, py;
		uint32 ox, oy;

		uint32 bytes_total;
		uint32 pitchx, pitchy;

		word delay_num, delay_den;

		byte dispose, blend;
	};	

#endif

	// =========================================
	// ImagePNG
	// =========================================

	ImagePNG::ImagePNG() : Image()
	{
		m_Ani = NULL;
		m_Pixels = NULL;
		m_Huffman = NULL;
		m_Chunk = new chunk;

		idata = NULL;
		expanded = NULL;
		out = NULL;
	}

	ImagePNG::~ImagePNG()
	{
		delete m_Chunk;

		if (idata) { delete idata; }
		if (expanded) { delete expanded; }
		if (out) { delete out; }

		if (m_Ani) { delete m_Ani; }
		if (m_Pixels) { delete [] m_Pixels; }
		if (m_Huffman) { delete m_Huffman; }
	}

	byte ImagePNG::GetByte()
	{
		byte temp;
		m_Stream->ReadByte(&temp);
		return temp;
	}

	word ImagePNG::GetWord()
	{
		byte temp[2];
		m_Stream->ReadByte(temp, 2);
		return ((temp[0] << 8) | temp[1]);
	}

	dword ImagePNG::GetDWord()
	{
		byte temp[4];
		m_Stream->ReadByte(temp, 4);
		return ((temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | (temp[3]));
	}

	void ImagePNG::Skip( uint32 a_Bytes )
	{
		m_Stream->Seek(a_Bytes, TIL_FILE_SEEK_CURR);
	}

	chunk* ImagePNG::GetChunkHeader()
	{
		m_Chunk->length    = GetDWord();

		m_Chunk->header[0] = (char)GetByte();
		m_Chunk->header[1] = (char)GetByte();
		m_Chunk->header[2] = (char)GetByte();
		m_Chunk->header[3] = (char)GetByte();
		m_Chunk->header[4] = 0;

		m_Chunk->type = 
			(m_Chunk->header[0] << 24) | 
			(m_Chunk->header[1] << 16) | 
			(m_Chunk->header[2] << 8) | 
			(m_Chunk->header[3]);

		return m_Chunk;
	}

	bool ImagePNG::Compile()
	{
		return false;
	}

	bool ImagePNG::Parse(uint32 a_Options /*= TIL_DEPTH_A8R8G8B8*/)
	{
		req_comp = 1;
		expanded = NULL;
		idata = NULL;
		out = NULL;
		if (req_comp < 0 || req_comp > 4)
		{
			TIL_ERROR_EXPLAIN("Bad req_comp.", 0);
			return false;
		}

		uint8 palette[1024];
		pal_img_n = 0;
		has_trans = 0;
		uint8 tc[3];
		ioff = 0;
		uint32 idata_limit = 0;
		uint32 i;
		uint32 pal_len = 0;
		int32 first = 1;
		//int32 k;

		uint8 png_sig[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };

		for (int8 i = 0; i < 8; ++i)
		{
			int32 c = GetByte();
			if (c != png_sig[i])
			{
				TIL_ERROR_EXPLAIN("Bad PNG signature (0x%x vs 0x%x at %i)", png_sig[i], c, i);
				return false;
			}
		}

		switch (m_BPPIdent)
		{

		case BPP_32B_A8R8G8B8: 
			m_ColorFunc = &ImagePNG::ColorFunc_A8R8G8B8; 
			break;

		case BPP_32B_A8B8G8R8:
			m_ColorFunc = &ImagePNG::ColorFunc_A8B8G8R8;
			break;

		case BPP_32B_R8G8B8A8: 
			m_ColorFunc = &ImagePNG::ColorFunc_R8G8B8A8; 
			break;

		case BPP_32B_B8G8R8A8: 
			m_ColorFunc = &ImagePNG::ColorFunc_B8G8R8A8; 
			break;

		case BPP_32B_R8G8B8: 
			m_ColorFunc = &ImagePNG::ColorFunc_R8G8B8; 
			break;

		case BPP_32B_B8G8R8: 
			m_ColorFunc = &ImagePNG::ColorFunc_B8G8R8;
			break;

		case BPP_16B_R5G6B5: 
			m_ColorFunc = &ImagePNG::ColorFunc_R5G6B5; 
			break;

		case BPP_16B_B5G6R5: 
			m_ColorFunc = &ImagePNG::ColorFunc_B5G6R5; 
			break;

		default:
			TIL_ERROR_EXPLAIN("Unhandled color format: %i", m_BPPIdent);
			break;

		}

		// animations

		m_Pixels = NULL;
		m_Ani = NULL;
		m_Frames = 1;

		for (;;first = 0)
		{
			//chunk current = GetChunkHeader();
			m_Chunk = GetChunkHeader();

			if (first && m_Chunk->type != PNG_TYPE('I','H','D','R'))
			{
				TIL_ERROR_EXPLAIN("Could not find IHDR tag.", 0);
				return NULL;
			}

			switch (m_Chunk->type) 
			{

			case PNG_TYPE('I','H','D','R'): 
				{
					PNG_DEBUG("Found tag 'IHDR'", 0);

					int8 depth, color, interlace, compression, filter;

					if (!first)
					{
						TIL_ERROR_EXPLAIN("Multiple IHDR tags.", 0);
						return NULL;
					}

					if (m_Chunk->length != 13) 
					{
						TIL_ERROR_EXPLAIN("Bad IHDR length.", 0);
						return NULL;
					}

					m_Width = GetDWord(); 
					if (m_Width > (1 << 24)) 
					{
						TIL_ERROR_EXPLAIN("Very large width (%i), possibly corrupted image.", m_Width);
						return NULL;
					}
					else if (m_Width == 0) 
					{
						TIL_ERROR_EXPLAIN("Zero-length width.", 0);
						return NULL;
					}

					m_Height = GetDWord(); 
					if (m_Height > (1 << 24)) 
					{
						TIL_ERROR_EXPLAIN("Very large height (%i), possibly corrupted image.", m_Height);
						return NULL;
					}
					else if (m_Height == 0)
					{
						TIL_ERROR_EXPLAIN("Zero-length height.", 0);
						return NULL;
					}

					PNG_DEBUG("Width: %i", m_Width);
					PNG_DEBUG("Height: %i", m_Height);

					//Internal::SetPitch(a_Options, m_Width, m_Height, m_PitchX, m_PitchY);

					depth = GetByte(); 
					if (depth != 8)
					{
						TIL_ERROR_EXPLAIN("Only 8-bit images are supported, received: %i.", depth);
						return NULL;
					}

					color = GetByte();  
					if (color > 6)
					{
						TIL_ERROR_EXPLAIN("Unsupported colortype. (%i)", color);
						return NULL;
					}
					else if (color == 3) 
					{
						pal_img_n = 3; 
					}
					else if (color & 1) 
					{
						TIL_ERROR_EXPLAIN("Unsupported colortype. (%i)", color);
						return NULL;
					}

					int channels = 0;
					m_ColorType = color;

					switch (color)
					{

					case PNG_COLOR_TYPE_GRAY:
						PNG_DEBUG("Color type: grayscale", 0);

					case PNG_COLOR_TYPE_PALETTE:
						PNG_DEBUG("Color type: paletted", 0);
						channels = 1;
						break;

					case PNG_COLOR_TYPE_RGB:
						PNG_DEBUG("Color type: RGB", 0);
						channels = 3;
						break;

					case PNG_COLOR_TYPE_GRAY_ALPHA:
						PNG_DEBUG("Color type: grayscale with alpha", 0);
						channels = 2;
						break;

					case PNG_COLOR_TYPE_RGB_ALPHA:
						PNG_DEBUG("Color type: RGB with alpha", 0);
						channels = 4;
						break;

					}

					PNG_DEBUG("Color depth: %i", color * channels);

					compression = GetByte();
					if (compression) 
					{
						TIL_ERROR_EXPLAIN("Bad compression method. (%i)", compression);
						return NULL;
					}

					filter = GetByte();  
					if (filter) 
					{
						TIL_ERROR_EXPLAIN("Bad filter method (%i)", filter);
						return NULL;
					}

					interlace = GetByte(); 
					if (interlace) 
					{
						TIL_ERROR_EXPLAIN("Interlaced mode not supported.", 0);
						return NULL;
					}

					if (!pal_img_n) 
					{
						img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
						if ((1 << 30) / (m_Width * img_n) < m_Height) 
						{
							TIL_ERROR_EXPLAIN("Image too large to decode.", 0);
							return NULL;
						}
					}
					else 
					{
						// if paletted, then pal_n is our final components, and
						// img_n is # components to decompress/filter.
						img_n = 1;
						if ((1 << 30) / m_Width / 4 < m_Height) 
						{
							TIL_ERROR_EXPLAIN("Too much data.", 0);
							return NULL;
						}
						// if SCAN_header, have to scan to see if we have a tRNS
					}

					break;
				}

			case PNG_TYPE('a', 'c', 'T', 'L'):
				{
					// https://wiki.mozilla.org/APNG_Specification#.60acTL.60:_The_Animation_Control_Chunk

					PNG_DEBUG("Found chunk 'acTL' indicating APNG animation.", 0);			

					m_Frames = (uint32)GetDWord();

					PNG_DEBUG("Frames: %i", m_Frames);

					m_Ani = new AnimationData;
					m_Ani->bpp = m_BPP;
					m_Ani->frame_prev = Internal::CreatePixels(m_Width, m_Height, m_BPP, m_Ani->pitchx, m_Ani->pitchy);
					m_Ani->bytes_total = m_Ani->pitchx * m_Ani->pitchy * m_BPP;
					memset(m_Ani->frame_prev, 0, m_Ani->bytes_total);
					m_Ani->image_bpp = (uint8)img_n;
					m_Ani->num_plays = (uint32)GetDWord();
					m_Ani->instance = this;

					break;
				}

			case PNG_TYPE('P','L','T','E'):  
				{
					PNG_DEBUG("Found tag 'PLTE'", 0);

					if (m_Chunk->length > 256 * 3) 
					{
						TIL_ERROR_EXPLAIN("Invalid PLTE, chunk is too long: %i.", m_Chunk->length);
						return NULL;
					}

					pal_len = m_Chunk->length / 3;
					if (pal_len * 3 != m_Chunk->length) 
					{
						TIL_ERROR_EXPLAIN("Invalid PLTE.", 0);
						return NULL;
					}

					for (i = 0; i < pal_len; ++i) 
					{
						palette[i * 4 + 0] = (uint8)GetByte();
						palette[i * 4 + 1] = (uint8)GetByte();
						palette[i * 4 + 2] = (uint8)GetByte();
						palette[i * 4 + 3] = 255;
					}

					break;
				}

			case PNG_TYPE('t','R','N','S'): 
				{
					PNG_DEBUG("Found tag 'tRNS'", 0);

					if (idata) 
					{
						TIL_ERROR_EXPLAIN("tRNS after IDAT.", 0);
						return NULL;
					}
					if (pal_img_n) 
					{
						if (pal_len == 0) 
						{
							TIL_ERROR_EXPLAIN("tRNS before PLTE.", 0);
							return NULL;
						}
						if (m_Chunk->length > pal_len) 
						{
							TIL_ERROR_EXPLAIN("Chunk length is greater than specified. (%i > %i)", m_Chunk->length, pal_len);
							return NULL;
						}

						pal_img_n = 4;
						for (uint32 i = 0; i < m_Chunk->length; ++i)
						{
							palette[i * 4 + 3] = (uint8)GetByte();
						}
					} 
					else 
					{
						if (!(img_n & 1)) 
						{
							TIL_ERROR_EXPLAIN("tRNS should contain alpha channel.", 0);
							return NULL;
						}

						if (m_Chunk->length != (uint32)img_n * 2) 
						{
							TIL_ERROR_EXPLAIN("Chunk length does not equal specified length. (%i != %i)", m_Chunk->length, (uint32)img_n);
							return NULL;
						}

						has_trans = 1;
						for (int32 k = 0; k < img_n; ++k)
						{
							tc[k] = (uint8)GetWord(); // non 8-bit images will be larger
						}
					}
					break;
				}

			case PNG_TYPE('I','D','A','T'): 
				{
					PNG_DEBUG("Found tag 'IDAT'", 0);

					//if (m_Ani && m_Ani->index > 0) 
					if (m_Ani)
					{ 
						m_Ani->chunk_idat = true; 
					}

					if (pal_img_n && !pal_len) 
					{
						TIL_ERROR_EXPLAIN("IDAT tag before PLTE tag.", 0);
						return NULL;
					}

					if (ioff + m_Chunk->length > idata_limit) 
					{
						if (idata_limit == 0) 
						{ 
							idata_limit = m_Chunk->length > 4096 ? m_Chunk->length : 4096; 
						}
						while (ioff + m_Chunk->length > idata_limit)
						{
							idata_limit *= 2;
						}
						uint8* p = (uint8*)realloc(idata, idata_limit); 
						if (p == NULL)
						{
							TIL_ERROR_EXPLAIN("Out of memory.", 0);
							return NULL;
						}
						idata = p;
					}

					if (!m_Stream->ReadByte(idata + ioff, m_Chunk->length))
					{	
						TIL_ERROR_EXPLAIN("Not enough data.", 0);
						return NULL;
					}

					ioff += m_Chunk->length;

					break;
				}

			case PNG_TYPE('f', 'c', 'T', 'L'):
				{
					// https://wiki.mozilla.org/APNG_Specification#.60fcTL.60:_The_Frame_Control_Chunk

					PNG_DEBUG("Found chunk 'fcTL'", 0);

					if (!m_Ani)
					{
						TIL_ERROR_EXPLAIN("'fcTL' chunk before 'acTL' chunk.", 0);
						return NULL;
					}

					uint32 sequence_nr = (uint32)GetDWord();
					if (m_Ani->control++ == 0 && sequence_nr != 0)
					{
						TIL_ERROR_EXPLAIN("First 'fcTL' chunk did not have sequence number 0.", 0);
						return NULL;
					}
					if (m_Ani->index++ != sequence_nr)
					{
						TIL_ERROR_EXPLAIN("Chunks out of order, received index %i, expected %i.", m_Ani->index - 1, sequence_nr);
						return NULL;
					}

					PNG_DEBUG("Index: %i", sequence_nr);

					// if the index is greater than 1, that means 
					// we don't have a hacky image with the default 
					// image and the animation intertwined
					if (m_Ani->index > 1)
					{
						if (!m_Pixels)
						{
							m_Pixels = new byte*[m_Frames];
							m_Pixels[0] = Internal::CreatePixels(m_Width, m_Height, m_BPP, m_PitchX, m_PitchY);
						}

						if (m_Ani->index == 2 && m_Ani->chunk_idat)
						{
							Compose();
						}

						m_Ani->NextFrame(m_Pixels);
					}
					else
					{
						PNG_DEBUG("Hacky APNG detected.");

						if (ioff > 0)
						{
							if (!m_Pixels)
							{
								m_Frames++;
								m_Pixels = new byte*[m_Frames];
								m_Pixels[0] = Internal::CreatePixels(m_Width, m_Height, m_BPP, m_PitchX, m_PitchY);
							}

							Compose();
						}
					}
					PNG_DEBUG("Frame: (%i / %i)", m_Ani->curr + 1, m_Frames);

					//m_Ani->curr++;		

					m_Ani->w = (uint32)GetDWord();
					m_Ani->h = (uint32)GetDWord();
					m_Ani->ox = (uint32)GetDWord();
					m_Ani->oy = (uint32)GetDWord();

					if (m_Ani->w + m_Ani->ox > m_Width)
					{
						TIL_ERROR_EXPLAIN("Width and offset greater than image width. (%i vs %i)", m_Ani->w + m_Ani->ox, m_Width);
						return NULL;
					}
					if (m_Ani->h + m_Ani->oy > m_Height)
					{
						TIL_ERROR_EXPLAIN("Height and offset greater than image height. (%i vs %i)", m_Ani->h + m_Ani->oy, m_Height);
						return NULL;
					}

					m_Ani->delay_num = (word)GetWord();
					m_Ani->delay_den = (word)GetWord();

					float delay = (float)m_Ani->delay_num / (float)m_Ani->delay_den;

					PNG_DEBUG("Delay: %f", delay);

					m_Ani->dispose = (byte)GetByte();

					switch (m_Ani->dispose)
					{
					case APNG_DISPOSE_OP_NONE:
						PNG_DEBUG("Dispose: APNG_DISPOSE_OP_NONE", 0);
						break;
					case APNG_DISPOSE_OP_BACKGROUND:
						PNG_DEBUG("Dispose: APNG_DISPOSE_OP_BACKGROUND", 0);
						break;
					case APNG_DISPOSE_OP_PREVIOUS:
						PNG_DEBUG("Dispose: APNG_DISPOSE_OP_PREVIOUS", 0);
						break;
					default:
						TIL_ERROR_EXPLAIN("Unknown dispose operation: %i", m_Ani->dispose);
						break;
					}

					m_Ani->blend = (byte)GetByte();

					switch (m_Ani->blend)
					{
					case APNG_BLEND_OP_SOURCE:
						PNG_DEBUG("Blend: APNG_BLEND_OP_SOURCE", 0);
						break;
					case APNG_BLEND_OP_OVER:
						PNG_DEBUG("Blend: APNG_BLEND_OP_OVER", 0);
						break;
					default:
						TIL_ERROR_EXPLAIN("Unknown blend operation: %i", m_Ani->blend);
						break;
					}

					break;
				}

			case PNG_TYPE('f', 'd', 'A', 'T'):
				{
					PNG_DEBUG("Found chunk 'fdAT'.", 0);

					if (!m_Ani)
					{
						TIL_ERROR_EXPLAIN("'fcTL' chunk before 'acTL' chunk.", 0);
						return NULL;
					}

					uint32 sequence_nr = (uint32)GetDWord();

					if (m_Ani->index++ != sequence_nr)
					{
						TIL_ERROR_EXPLAIN("Chunks out of order, received index %i, expected %i.", m_Ani->index - 1, sequence_nr);
						return NULL;
					}

					PNG_DEBUG("Index: %i", sequence_nr);

					uint32 len = m_Chunk->length - 4;

					//Skip(m_Chunk->length - 4);

					if (m_Ani->data_offset + len > m_Ani->data_limit) 
					{
						if (m_Ani->data_limit == 0) 
						{ 
							 m_Ani->data_limit = len > 4096 ? m_Chunk->length : 4096; 
						}
						while (m_Ani->data_offset + len > m_Ani->data_limit)
						{
							m_Ani->data_limit *= 2;
						}
						uint8* p = (uint8*)realloc(m_Ani->data, m_Ani->data_limit); 
						if (p == NULL)
						{
							TIL_ERROR_EXPLAIN("Out of memory.", 0);
							return NULL;
						}
						m_Ani->data = p;
					}

					if (!m_Stream->ReadByte(m_Ani->data + m_Ani->data_offset, len))
					{	
						TIL_ERROR_EXPLAIN("Not enough data.", 0);
						return NULL;
					}

					m_Ani->data_offset += len;

					break;
				}

			case PNG_TYPE('I','E','N','D'): 
				{	
					PNG_DEBUG("Found tag 'IEND'", 0);

					if (!m_Pixels)
					{
						m_Pixels = new byte*[m_Frames];
						m_Pixels[0] = Internal::CreatePixels(m_Width, m_Height, m_BPP, m_PitchX, m_PitchY);

						Compose();
					}

					if (m_Ani) 
					{ 
						m_Ani->NextFrame(m_Pixels); 
					}

					return true;
				}

			default:
				{
					PNG_DEBUG("Skipping chunk: %s", m_Chunk->header);

					// if critical, fail
					if ((m_Chunk->type & (1 << 29)) == 0) 
					{
						TIL_ERROR_EXPLAIN("Corrupt chunk. (%s)", m_Chunk->header);
						return NULL;
					}

					Skip(m_Chunk->length);

					break;
				}
			}

			// skip CRC
			GetDWord();
		}
	}

	bool ImagePNG::Compose()
	{
		if (idata == NULL) 
		{
			TIL_ERROR_EXPLAIN("No IDAT tag.", 0);
			return false;
		}

		PNG_DEBUG("Composing IDAT image.", 0);

		m_ZBuffer.ZLibDecode(idata, ioff);
		m_RawLength = (uint32)(m_ZBuffer.zout - m_ZBuffer.zout_start);
		unsigned char* target = (unsigned char*)m_ZBuffer.zout_start;

		img_out_n = 4;

		// create png image

		if (img_out_n != img_n && img_out_n != img_n + 1)
		{
			TIL_ERROR_EXPLAIN("Wrong bitdepth.", 0);
			return false;
		}

		if (m_RawLength != (img_n * m_Width + 1) * m_Height) 
		{
			TIL_ERROR_EXPLAIN("Not enough pixels. (%i vs %i)", m_RawLength, (img_n * m_Width + 1) * m_Height);
			return false;
		}

		byte* write = m_Pixels[0]; // first is default image
		//byte* write = m_Data[0]->GetData();

		m_Pitch = m_PitchX * 4;
		Decompile(write, target, m_Width, m_Height, m_Pitch, img_n);

		if (has_trans)
		{
			TIL_ERROR_EXPLAIN("TODO: Compute transparancy.", 0);

			/*if (!compute_transparency(z, tc, s->img_out_n)) 
			{
				return 0;
			}*/

			return false;
		}

		if (pal_img_n) 
		{
			// pal_img_n == 3 or 4
			img_n = pal_img_n; // record the actual colors we had
			img_out_n = pal_img_n;
			if (req_comp >= 3) 
			{
				img_out_n = req_comp;
			}

			TIL_ERROR_EXPLAIN("TODO: Expand palette.", 0);

			return false;

			/*if (!expand_palette(z, palette, pal_len, img_out_n))
			{
				return 0;
			}*/
		}
					
		//delete out;

		return true;
	}

	uint32 ImagePNG::GetFrameCount()
	{
		return m_Frames;
	}

	byte* ImagePNG::GetPixels( uint32 a_Frame /*= 0*/ )
	{
		if (a_Frame > m_Frames - 1) { a_Frame = m_Frames - 1; }
		return m_Pixels[a_Frame];
		//return m_Data[a_Frame]->GetData();
	}

	til::uint32 ImagePNG::GetHeight(uint32 a_Frame /*= 0*/)
	{
		return m_Height;
	}

	til::uint32 ImagePNG::GetWidth(uint32 a_Frame /*= 0*/)
	{
		return m_Width;
	}

	til::uint32 ImagePNG::GetPitchX(uint32 a_Frame /*= 0*/)
	{
		return m_PitchX;
	}

	til::uint32 ImagePNG::GetPitchY(uint32 a_Frame /*= 0*/)
	{
		return m_PitchY;
	}

}; // namespace til

#endif