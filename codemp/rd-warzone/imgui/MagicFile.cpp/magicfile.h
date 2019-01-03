#ifndef FILEAPI_H
#define FILEAPI_H

#include <stdio.h>
#include <stdlib.h>
#include "../ccall/ccall.h"

typedef struct ramfileinfo_s {
	char *filename;
	char *data;
	size_t size;
	int changed;
} ramfileinfo_t;

typedef struct ramfile_s {
	ramfileinfo_t *ramfileinfo;
	size_t position; // position for fseek 
} RAMFILE;

#ifdef __cplusplus
	#include <string>
	#include <sstream>
	#include <iostream>
	#include <map>
	extern std::map<std::string, ramfileinfo_t *> availableFiles;
#endif

CCALL ramfileinfo_t *file_get_buffer_by_name(char *filename);
CCALL int file_get_contents(const char *path, char **buffer, size_t *out_filesize);
CCALL int file_put_contents(const char *path, char *buffer, size_t filesize);
CCALL FILE *test_fopen();
CCALL void file_loaded(char *filename, char *buffer, int buffersize);

CCALL RAMFILE *HTML_fopen(char *filename, const char *mode);
CCALL size_t HTML_fplength(RAMFILE *file);
CCALL size_t HTML_fread(void *buffer, size_t element_size, size_t element_count, RAMFILE *htmlfile);
// not implemented, dont need it yet for anything...
CCALL size_t HTML_fwrite(const void *buffer, size_t element_size, size_t element_count, RAMFILE *htmlfile);
CCALL long HTML_ftell(RAMFILE *htmlfile);
CCALL int HTML_fseek(RAMFILE *htmlfile, long offset, int origin);
CCALL int HTML_fclose(RAMFILE *htmlfile);
CCALL int HTML_ferror(RAMFILE *htmlfile);
CCALL int HTML_fscanf(RAMFILE *f, char *fmt, ...);
CCALL int HTML_fprintf(RAMFILE *f, char *fmt, ...);
CCALL int file_dump_memory(char *filename, void *memory, size_t bytes);
CCALL int HTML_unlink(char *filename);

#endif