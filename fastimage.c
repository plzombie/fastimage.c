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

#if defined(WIN32)
#include <Windows.h>
#if !defined(__WATCOMC__)
#include <winhttp.h>
#endif
#else
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include "fastimage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

#define BMP_FILEHEADER_SIZE 14

typedef struct {
	int biSize;
	int biWidth;
	int biHeight;
	short biPlanes;
	short biBitCount;
} bmp_infoheader_min_t;


static void fastimageReadBmp(const fastimage_reader_t *reader, unsigned char *sign, fastimage_image_t *image)
{
	bmp_infoheader_min_t bmp_infoheader;
	
	(void)sign; // Unused
		
	if(!reader->seek(reader->context, BMP_FILEHEADER_SIZE)) {
		image->format = fastimage_error;

		return;
	}

	if(reader->read(reader->context, sizeof(bmp_infoheader_min_t), &bmp_infoheader) != sizeof(bmp_infoheader_min_t)) {
		image->format = fastimage_error;

		return;
	}

	image->width = bmp_infoheader.biWidth;
	image->height = bmp_infoheader.biHeight;
	switch(bmp_infoheader.biBitCount) {
		case 16:
		case 24:
		case 32:
			image->bitsperpixel = bmp_infoheader.biBitCount;
			break;
		case 1:
		case 4:
		case 8:
			image->bitsperpixel = 32;
			image->palette = bmp_infoheader.biBitCount;
			break;
		default:
			image->format = fastimage_error;
	}
	
	if(bmp_infoheader.biBitCount == 16)
		image->channels = 3;
	else
		image->channels = image->bitsperpixel / 8;
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
	image->channels = 3;
	image->bitsperpixel = 24;

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
			image->channels = 1;
			break;
		case 2:
			image->channels = 3;
			break;
		case 3:
			image->channels = 3;
			image->palette = png_bytes[8];
			break;
		case 4:
			image->channels = 2;
			break;
		case 6:
			image->channels = 4;
			break;
		default:
			goto PNG_ERROR;
	}

	if(image->palette)
		image->bitsperpixel = 24;
	else
		image->bitsperpixel = (unsigned int)(png_bytes[8]) * image->channels;

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
	image->bitsperpixel = 24;
	image->channels = 3;
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
			
		image->width = (size_t)(jpg_bytes[3])*256+jpg_bytes[4];
		image->height = (size_t)(jpg_bytes[1])*256+jpg_bytes[2];
		image->channels = jpg_bytes[5];
		image->bitsperpixel = image->channels * (unsigned int)(jpg_bytes[0]);
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

static size_t FASTIMAGE_APIENTRY fastimageFileRead(void *context, size_t size, void *buf)
{
	return fread(buf, 1, size, context);
}

static bool FASTIMAGE_APIENTRY fastimageFileSeek(void *context, int64_t pos)
{
#if defined(WIN32)
	return _fseeki64(context, pos, SEEK_SET) == 0;
#else
	return fseeko64(context, pos, SEEK_SET) == 0;
#endif
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

#if !defined(WIN32)
FILE *_wfopen(const wchar_t* filename, const wchar_t* mode)
{
	size_t filename_len, mode_len;
	char* cfilename = 0, * cmode = 0;
	FILE *f = 0;

	filename_len = wcslen(filename);
	mode_len = wcslen(mode);

	cfilename = malloc(filename_len * MB_CUR_MAX + 1);
	cmode = malloc(mode_len * MB_CUR_MAX + 1);
	if (!cfilename || !cmode) {
		errno = ENOMEM;
		goto FINAL;
	}

	if (wcstombs(cfilename, filename, filename_len * MB_CUR_MAX + 1) == (size_t)(-1)) {
		errno = EINVAL;
		goto FINAL;
	}

	if (wcstombs(cmode, mode, mode_len * MB_CUR_MAX + 1) == (size_t)(-1)) {
		errno = EINVAL;
		goto FINAL;
	}

	f = fopen(cfilename, cmode);

FINAL:
	if (cfilename) free(cfilename);
	if (cmode) free(cmode);

	return f;
}
#endif

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

#if defined(__WATCOMC__) || !defined(WIN32)
fastimage_image_t fastimageOpenHttpW(const wchar_t *url, bool support_proxy)
{
	fastimage_image_t image;

	(void)url;
	(void)support_proxy;

	memset(&image, 0, sizeof(fastimage_image_t));
	image.format = fastimage_error;
	
	return image;
}
#else

typedef struct {
	HINTERNET request;
	int64_t offset;
} fastimage_http_context_t;

static size_t FASTIMAGE_APIENTRY fastimageHttpRead(void *context, size_t size, void* buf)
{
	fastimage_http_context_t* httpc;
	
	size_t total_downloaded = 0;

	httpc = (fastimage_http_context_t*)context;

	//printf("read %u\n", (unsigned int)size);

	while (1) {
		DWORD data_awailable = 0, data_downloaded = 0, bytes_to_read = 0;

		//printf("read data\n");

		if(!WinHttpQueryDataAvailable(httpc->request, &data_awailable))
			return total_downloaded;

		//printf("awailable %u\n", data_awailable);

		if(data_awailable > (size-total_downloaded))
			bytes_to_read = (DWORD)(size - total_downloaded);
		else
			bytes_to_read = data_awailable;

		if(data_awailable) {
			if(!WinHttpReadData(httpc->request, buf, bytes_to_read, &data_downloaded))
				return total_downloaded;

			//printf("downloaded %u\n", data_downloaded);

			httpc->offset += data_downloaded;
			total_downloaded += data_downloaded;
		}
		if(total_downloaded == size)
			break;
		if(data_awailable == 0)
			break;
	}

	return total_downloaded;
}

static bool FASTIMAGE_APIENTRY fastimageHttpSeek(void *context, int64_t pos)
{
	fastimage_http_context_t *httpc;
	int64_t bytes_to_read;
	char *buf;

	httpc = (fastimage_http_context_t *)context;

	if(pos < httpc->offset)
		return false;
	else if(pos == httpc->offset)
		return true;

	bytes_to_read = pos - httpc->offset;

	if(bytes_to_read > SIZE_MAX)
		return false;

	buf = malloc((size_t)bytes_to_read);
	if(!buf)
		return false;

	fastimageHttpRead(context, (size_t)bytes_to_read, buf);

	free(buf);

	if(pos == httpc->offset)
		return true;
	else
		return false;
}

fastimage_image_t fastimageOpenHttpW(const wchar_t *url, bool support_proxy)
{
	HINTERNET session = 0, connect = 0, request = 0;
	bool success = true;
	wchar_t *url_server = 0, *url_path = 0, *url_copy = 0;
	size_t url_len;
	fastimage_image_t image;
	
	url_len = wcslen(url);
	url_copy = malloc((url_len+1)*sizeof(wchar_t));
	if(url_copy) {
		memcpy(url_copy, url, url_len*2);
		url_copy[url_len] = 0;

		url_server = wcschr(url_copy, L':');
		if(!url_server) success = false;
		else if(*(url_server+1) == '/' && *(url_server+2) == '/') {
			url_server += 3;
			url_path = wcschr(url_server, L'/');
			if(!url_path) success = false;
			else {
				*url_path = 0;
				url_path += 1;
			}
		} else success = false;
	} else success = false;

	if(success) {
		session = WinHttpOpen(
			L"fastimage_c/1.0",
			support_proxy?(WINHTTP_ACCESS_TYPE_DEFAULT_PROXY):(WINHTTP_ACCESS_TYPE_NO_PROXY),
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0);
		if(!session) success = false;
	}

	if(success) {
		connect = WinHttpConnect(
			session,
			url_server,
			INTERNET_DEFAULT_PORT,
			0);
		if(!connect) success = false;
	}

	if(success) {
		request = WinHttpOpenRequest(
			connect,
			L"GET",
			url_path,
			NULL,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			0
		);
		if(!request) success = false;
	}

	if(success) {
		fastimage_http_context_t context;
		fastimage_reader_t reader;
		BOOL results;

		results = WinHttpSendRequest(
			request,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			WINHTTP_NO_REQUEST_DATA,
			0,
			0,
			0);

		if(results != FALSE)
			results = WinHttpReceiveResponse(request, NULL);

		if(results != FALSE) {
			context.offset = 0;
			context.request = request;

			reader.context = &context;
			reader.read = fastimageHttpRead;
			reader.seek = fastimageHttpSeek;

			image = fastimageOpen(&reader);
		} else

		if(results == FALSE) success = false;
	}
	
	if(!success) {
		memset(&image, 0, sizeof(fastimage_image_t));
		image.format = fastimage_error;
	}

	if(url_copy) free(url_copy);
	if(request) WinHttpCloseHandle(request);
	if(connect) WinHttpCloseHandle(connect);
	if(session) WinHttpCloseHandle(session);

	return image;
}
#endif
