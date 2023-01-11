# fastimage.c

Library to detect image size and type

## Supported formats

* bmp - full
* tga - full
* pcx - full
* png - full
* gif - full
* webp - detect only
* heic - detect and size only
* jpg - full
* avif - full
* qoi/qoy - full

## Supported data streams

* file - via filename or file handle
* http - via http(s) link (WinHTTP or libcurl)

### libcurl

To use libcurl define FASTIMAGE_USE_LIBCURL. For now the whole file is always downloaded.
