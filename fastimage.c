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

#include "fastimage.h"
#include <stdlib.h>
#include <string.h>

fastimage_image_t fastimageOpen(const fastimage_reader_t *reader)
{
	fastimage_image_t image;
	char sign[4];
	
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
	else if(sign[0] == 10 && sign[1] == 5 && sign[2] == 1)
		image.format = fastimage_pcx;
		
	// Try to detect TGA
	
	return image;
}

size_t cdecl fastimageFileRead(void *context, size_t size, void *buf)
{
	return fread(buf, 1, size, context);
}

bool cdecl fastimageFileSeek(void *context, int64_t pos)
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
