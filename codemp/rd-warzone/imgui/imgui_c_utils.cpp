#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imgui_c_utils.h"

#ifndef EMSCRIPTEN
// https://stackoverflow.com/questions/1258550/why-should-you-use-strncpy-instead-of-strcpy
size_t strlcpy(char *dst, const char *src, size_t dstsize)
{
  size_t len = strlen(src);
  if(dstsize) {
    size_t bl = (len < dstsize-1 ? len : dstsize-1);
    ((char*)memcpy(dst, src, bl))[bl] = 0;
  }
  return len;
}
#endif

const char *get_filename_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}