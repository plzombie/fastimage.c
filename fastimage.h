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

#ifndef FASTIMAGE_H
#define FASTIMAGE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <wchar.h>

enum fastimage_image_format {
	fastimage_error,
	fastimage_unknown,
	fastimage_bmp,
	fastimage_tga,
	fastimage_pcx,
	fastimage_png,
	fastimage_gif,
	fastimage_webp,
	fastimage_heif,
	fastimage_jpg
};

typedef struct {
	int format;
	size_t width;
	size_t height;
	unsigned int channels;
	unsigned int bitsperpixel;
	unsigned int palette;
} fastimage_image_t;

typedef size_t (cdecl * fastimage_readfunc_t)(void *context, size_t size, void *buf);
typedef bool (cdecl * fastimage_seekfunc_t)(void *context, int64_t pos);

typedef struct {
	void *context;
	fastimage_readfunc_t read;
	fastimage_seekfunc_t seek;
} fastimage_reader_t;

extern fastimage_image_t fastimageOpen(const fastimage_reader_t *reader);
extern fastimage_image_t fastimageOpenFile(FILE *f);
extern fastimage_image_t fastimageOpenFileA(const char *filename);
extern fastimage_image_t fastimageOpenFileW(const wchar_t *filename);
extern fastimage_image_t fastimageOpenHttpW(const wchar_t *url, bool support_proxy);

#endif
