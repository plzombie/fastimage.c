/*
BSD 2-Clause License

Copyright (c) 2022, Mikhail Morozov
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <Windows.h>

#include "fastimage.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
	short Xmin;
	short Ymin;
	short Xmax;
	short Ymax;
	short HRes;
	short VRes;
	unsigned char Colormap[48];
	unsigned char Reserved;
	unsigned char NPlanes;
} pcx_header_min_t;


static void fastimageReadBmp(const fastimage_reader_t *reader, unsigned char *sign, fastimage_image_t *image)
{
	BITMAPINFOHEADER bmp_infoheader;
	
	(void)sign; // Unused
		
	if(!reader->seek(reader->context, sizeof(BITMAPFILEHEADER))) {
		image->format = fastimage_error;

		return;
	}

	if(reader->read(reader->context, sizeof(BITMAPINFOHEADER), &bmp_infoheader) != sizeof(BITMAPINFOHEADER)) {
		image->format = fastimage_error;

		return;
	}

	image->width = bmp_infoheader.biWidth;
	image->height = bmp_infoheader.biHeight;
	switch(bmp_infoheader.biBitCount) {
		case 16:
		case 24:
		case 32:
			image->bpp = bmp_infoheader.biBitCount / 8;
			break;
		case 1:
		case 4:
		case 8:
			image->bpp = 4;
			image->palette = bmp_infoheader.biBitCount;
			break;
		default:
			image->format = fastimage_error;
	}
}

static void fastimageReadPcx(const fastimage_reader_t *reader, unsigned char *sign, fastimage_image_t *image)
{
	pcx_header_min_t pcx_header_min;

	(void)sign; // Unused

	if(reader->read(reader->context, sizeof(pcx_header_min_t), &pcx_header_min) != sizeof(pcx_header_min_t)) {
		image->format = fastimage_error;

		return;
	}

	image->width = (size_t)(pcx_header_min.Xmax - pcx_header_min.Xmin) + 1;
	image->height = (size_t)(pcx_header_min.Ymax - pcx_header_min.Ymin) + 1;
	image->bpp = 3;

	if(pcx_header_min.NPlanes == 1)
		image->palette = 8;
	else if(pcx_header_min.NPlanes != 3)
		image->format = fastimage_error;
}

static void fastimageReadPng(const fastimage_reader_t *reader, unsigned char *sign, fastimage_image_t *image)
{
	unsigned char png_bytes[10];
	int64_t png_curr_offt = 4;
	uint32_t png_chunk_size;
	
	(void)sign; // Unused
	
	// Read last part of signature
	if(reader->read(reader->context, 4, png_bytes) != 4)
		goto PNG_ERROR;
	
	png_curr_offt += 4;
	
	if(memcmp(png_bytes, "\x0d\x0a\x1a\x0a", 4))
		goto PNG_ERROR;
	
	// Skip to header chunk
	while(1) {
		unsigned char png_chunk_head[8];
		
		if(reader->read(reader->context, 8, png_chunk_head) != 8)
			goto PNG_ERROR;
		
		png_curr_offt += 8;
		
		png_chunk_size = (uint32_t)(png_chunk_head[0])*16777216+(uint32_t)(png_chunk_head[1])*65536+(uint32_t)(png_chunk_head[2])*256+png_chunk_head[3];
		
		if(!memcmp(png_chunk_head+4, "IHDR", 4))
			break;
			
		png_curr_offt += (int64_t)4 + png_chunk_size;
		
		if(!reader->seek(reader->context, png_curr_offt))
			goto PNG_ERROR;
	}
	
	if(png_chunk_size != 0xD)
		goto PNG_ERROR;

	if(reader->read(reader->context, 10, png_bytes) != 10)
		goto PNG_ERROR;

	image->width = (uint32_t)(png_bytes[0])*16777216+(uint32_t)(png_bytes[1])*65536+(uint32_t)(png_bytes[2])*256+png_bytes[3];
	image->height = (uint32_t)(png_bytes[4])*16777216+(uint32_t)(png_bytes[5])*65536+(uint32_t)(png_bytes[6])*256+png_bytes[7];
	
	switch(png_bytes[9]) {
		case 0:
			image->bpp = 1;
			break;
		case 2:
			image->bpp = 3;
			break;
		case 3:
			image->bpp = 3;
			image->palette = png_bytes[8];
			break;
		case 4:
			image->bpp = 2;
			break;
		case 6:
			image->bpp = 4;
			break;
		default:
			goto PNG_ERROR;
	}

	return;
	
PNG_ERROR:
	image->format = fastimage_error;
}

static void fastimageReadGif(const fastimage_reader_t *reader, unsigned char *sign, fastimage_image_t *image)
{
	unsigned short gif_header_min[3];
	
	(void)sign; // Unused
	
	// GIF87a or GIF89a
	if(reader->read(reader->context, 6, &gif_header_min) != 6) {
		image->format = fastimage_error;
		
		return;
	}
		
	if( (gif_header_min[0] != '7'+(unsigned short)('a')*256)
		&& (gif_header_min[0] != '9'+(unsigned short)('a')*256)) {
		image->format = fastimage_error;
			
		return;
	}
		
	image->width = gif_header_min[1];
	image->height = gif_header_min[2];
	image->bpp = 3;
	image->palette = 8;
}

static void fastimageReadJpeg(const fastimage_reader_t *reader, unsigned char *sign, fastimage_image_t *image)
{
	int64_t jpg_curr_offt = 4;
	unsigned short jpg_frame;
	int64_t jpg_segment_size;
		
	jpg_frame = sign[2]+(unsigned short)(sign[3])*256;
		
	// First of all, skipping segments we don't needed
	// TODO: there are segments without length (with empty or 2-byte data). Need to check them
	while(1) {
		unsigned char jpg_bytes[2];
			
		//printf("jpeg segment %hx\n", jpg_frame);
			
		// Read segment size
		if(reader->read(reader->context, 2, jpg_bytes) != 2) {
			image->format = fastimage_error;
				
			return;
		}
			
		jpg_curr_offt += 2;
			
		jpg_segment_size = (int64_t)(jpg_bytes[0])*256+jpg_bytes[1];
			
		if(jpg_segment_size < 2) {
			image->format = fastimage_error;
				
			return;
		}
			
		jpg_segment_size -= 2;
			
		if(jpg_frame == 0xC0FF || jpg_frame == 0xC1FF || jpg_frame == 0xC2FF) {
			break;
		}
			
		// Skip segment
		jpg_curr_offt += jpg_segment_size;
			
		if(!reader->seek(reader->context, jpg_curr_offt)) {
			image->format = fastimage_error;
				
			return;
		}
			
		// Read next segment signature
		if(reader->read(reader->context, 2, &jpg_frame) != 2) {
			image->format = fastimage_error;
				
			return;
		}
		
		jpg_curr_offt += 2;
	}
		
	if(jpg_frame == 0xC0FF || jpg_frame == 0xC1FF || jpg_frame == 0xC2FF) {
		// Found segment with header
		unsigned char jpg_bytes[6];
		
		if(jpg_segment_size < 6) {
			image->format = fastimage_error;
					
			return;
		}
				
		if(reader->read(reader->context, 6, jpg_bytes) != 6) {
			image->format = fastimage_error;
				
			return;
		}
			
		image->width = (size_t)(jpg_bytes[1])*256+jpg_bytes[2];
		image->height = (size_t)(jpg_bytes[3])*256+jpg_bytes[4];
		image->bpp = jpg_bytes[5];
	} else
		image->format = fastimage_error;
}

fastimage_image_t fastimageOpen(const fastimage_reader_t *reader)
{
	fastimage_image_t image;
	unsigned char sign[4];
	
	memset(&image, 0, sizeof(fastimage_image_t));
	
	if(reader->read(reader->context, 4, sign) != 4) {
		image.format = fastimage_error;
		
		return image;
	}
	
	image.format = fastimage_unknown;
	
	if(!memcmp(sign, "BM", 2))
		image.format = fastimage_bmp;
	if(!memcmp(sign, "\x89PNG", 4))
		image.format = fastimage_png;
	if(!memcmp(sign, "GIF8", 4)) // GIF87a or GIF89a
		image.format = fastimage_gif;
	if(!memcmp(sign, "RIFF", 4))
		image.format = fastimage_webp;
	else if(sign[0] == 0xFF && sign[1] == 0xD8)
		image.format = fastimage_jpg;
	else if(sign[0] == 10 && sign[1] == 5 && sign[2] == 1 && sign[3] == 8)
		image.format = fastimage_pcx;
		
	// Try to detect TGA
	// Here should be code
	
	// Try to detect HEIF
	// Here should be code
	
	// Read BMP meta
	if(image.format == fastimage_bmp)
		fastimageReadBmp(reader, sign, &image);
	
	// Read TGA meta
	// Here should be code
	
	// Read PCX meta
	if(image.format == fastimage_pcx)
		fastimageReadPcx(reader, sign, &image);
	
	// Read PNG meta
	if(image.format == fastimage_png)
		fastimageReadPng(reader, sign, &image);
	
	// Read GIF meta
	if(image.format == fastimage_gif)
		fastimageReadGif(reader, sign, &image);
	
	// Read WEBP meta
	// Here should be code
	
	// Read HEIF meta
	// Here should be code
	
	// Read JPG meta
	if(image.format == fastimage_jpg)
		fastimageReadJpeg(reader, sign, &image);
	
	return image;
}

static size_t cdecl fastimageFileRead(void *context, size_t size, void *buf)
{
	return fread(buf, 1, size, context);
}

static bool cdecl fastimageFileSeek(void *context, int64_t pos)
{
	return _fseeki64(context, pos, SEEK_SET) == 0;
}

fastimage_image_t fastimageOpenFile(FILE *f)
{
	fastimage_reader_t reader;
	
	reader.context = f;
	reader.read = fastimageFileRead;
	reader.seek = fastimageFileSeek;
	
	return fastimageOpen(&reader);
}

fastimage_image_t fastimageOpenFileA(const char *filename)
{
	FILE *f = 0;
	
	f = fopen(filename, "rb");
	if(!f) {
		fastimage_image_t image;
		
		memset(&image, 0, sizeof(fastimage_image_t));
		image.format = fastimage_error;
	
		return image;
	}
	
	return fastimageOpenFile(f);
}

fastimage_image_t fastimageOpenFileW(const wchar_t *filename)
{
	FILE *f = 0;
	
	f = _wfopen(filename, L"rb");
	if(!f) {
		fastimage_image_t image;
		
		memset(&image, 0, sizeof(fastimage_image_t));
		image.format = fastimage_error;
	
		return image;
	}
	
	return fastimageOpenFile(f);
}
