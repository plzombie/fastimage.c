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

int wmain(int argc, wchar_t **argv)
{
	if(argc != 3) {
		printf("test.exe type input\n"
		       "\ttype = file - file input\n");
		
		return 0;
	}
	
	if(!wcscmp(argv[1], L"file")) {
		fastimage_image_t image;
		
		image = fastimageOpenFileW(argv[2]);
		
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
			case fastimage_heif:
				printf("heic\n");
				break;
			case fastimage_jpg:
				printf("jpg\n");
				break;
			default:
				printf("other\n");
				
		}
		
		printf("width: %u\n", (unsigned int)image.width);
		printf("height: %u\n", (unsigned int)image.height);
		
		if(image.palette)
			printf("palette: %u bits, palette element %u bytes\n", image.palette, image.bpp);
		else
			printf("bytes per pixel: %u\n", image.bpp);
	}
		      

	return 0;
}
