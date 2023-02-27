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

#if defined(_DEBUG) && defined(USE_STB_LEAKCHECK)
#define STB_LEAKCHECK_IMPLEMENTATION
#include "third_party/stb_leakcheck.h"
#endif

#include "fastimage.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
	fastimage_image_t image;
#if defined(_WIN32)
	wchar_t *link_type, *link_path;
#else
	char *link_type, *link_path;
#endif

	if(argc < 2 || argc > 3) {
		printf("test.exe [type] input\n"
		       "\ttype = file - file input\n"
			   "\ttype = http - http url\n");
		
		return 0;
	}

	if(argc == 3) {
		link_type = argv[1];
		link_path = argv[2];
	} else {
		link_path = argv[1];
#if defined(_WIN32)
		if(!wcsncmp(link_path, L"http://", 7) || !wcsncmp(link_path, L"https://", 8))
			link_type = L"http";
		else
			link_type = L"file";
#else
		if(!strncmp(link_path, "http://", 7) || !strncmp(link_path, "https://", 8))
			link_type = "http";
		else
			link_type = "file";
#endif
	}
	
#if defined(_WIN32)
	if(!wcscmp(link_type, L"file")) {
		image = fastimageOpenFileW(link_path);
	} else if(!wcscmp(link_type, L"http")) {
		image = fastimageOpenHttpW(link_path, true);
#else
	if(!strcmp(link_type, "file")) {
		image = fastimageOpenFileA(link_path);
	} else if(!strcmp(link_type, "http")) {
		image = fastimageOpenHttpA(link_path, true);
#endif
	} else {
		printf("Unknown input type\n");

		return 0;
	}

	printf("format: ");
	switch(image.format) {
		case fastimage_error:
			printf("error\n");
			break;
		case fastimage_unknown:
			printf("unknown\n");
			break;
		case fastimage_bmp:
			printf("bmp\n");
			break;
		case fastimage_tga:
			printf("tga\n");
			break;
		case fastimage_pcx:
			printf("pcx\n");
			break;
		case fastimage_png:
			printf("png\n");
			break;
		case fastimage_gif:
			printf("gif\n");
			break;
		case fastimage_webp:
			printf("webp\n");
			break;
		case fastimage_heic:
			printf("heic\n");
			break;
		case fastimage_jpg:
			printf("jpg\n");
			break;
		case fastimage_avif:
			printf("avif\n");
			break;
		case fastimage_miaf:
			printf("miaf\n");
			break;
		case fastimage_qoi:
			printf("qoi\n");
			break;
		case fastimage_qoy:
			printf("qoy\n");
			break;
		case fastimage_ani:
			printf("ani\n");
			break;
		case fastimage_ico:
			printf("ico\n");
			break;
		default:
			printf("other\n");		
	}
		
	printf("width: %u\n", (unsigned int)image.width);
	printf("height: %u\n", (unsigned int)image.height);
		
	if(image.palette)
		printf("palette: %u bits, palette element %u bits %u channels\n", image.palette, image.bitsperpixel, image.channels);
	else
		printf("bits per pixel: %u, %u channel\n", image.bitsperpixel, image.channels);	      


#if defined(_DEBUG) && defined(USE_STB_LEAKCHECK)
	stb_leakcheck_dumpmem();
#endif

	return 0;
}
