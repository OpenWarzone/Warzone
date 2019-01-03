#include "../ccall/ccall.h"
#include "magicfile.h"
//#include <kung/imgui/imgui_console.h>
#include <string.h>
#include <stdio.h>	 // all for vararg lol, had "undefined identifier va_alist"...
#include <stdlib.h>	 // all for vararg lol, had "undefined identifier va_alist"...
#include <stdarg.h>	 // all for vararg lol, had "undefined identifier va_alist"...
#ifdef _MSC_VER // neither emscripten nor linux gcc knows about varargs.h
#include <varargs.h> // all for vararg lol, had "undefined identifier va_alist"...
#endif
#include <string>
#include <sstream>
#include <iostream>
#include <map>

std::map<std::string, ramfileinfo_t *> availableFiles;

CCALL int fgc_test() {
	char *buf;
	size_t s;
	return file_get_contents("imgui.ini", &buf, &s);
}

CCALL int file_get_contents(const char *path_, char **buffer, size_t *out_filesize) {
	char path[512];
	// todo: something like node("./foo/bla") in C++, like in JS
	if (path_[0] != '.') {
		sprintf(path, "./%s", path_);
	} else {
		sprintf(path, "%s", path_);
	}
//#ifndef EMSCRIPTEN
#if 0 // sicne nodejs loads the files async, we gonna use availableFiles aswell
	char path_main[256];
	//snprintf(path_main, sizeof(path_main), "%s", path);
	snprintf(path_main, sizeof(path_main), "baseq3/%s", path);

	FILE *file = fopen(path_main, "rb");
	if (file) {
		size_t filesize;
		fseek(file, 0, SEEK_END);
		filesize = ftell(file);
		*buffer = (char *)malloc(filesize + 1); // no matter what, add \0 at end
		fseek(file, 0L, SEEK_SET);
		fread(*buffer, filesize, 1, file);
		fclose(file);
		(*buffer)[filesize] = 0; // end string
		if (out_filesize)
			*out_filesize = filesize;
		return 1;
	}
	*buffer = NULL;
	*out_filesize = 0;
	return 0;
#else


	auto bufferobject = availableFiles.find(std::string(path));
	if (bufferobject == availableFiles.end()) {
		//imgui_log("file %s not available\n", path);

		
		return 0;
	} else {
		ramfileinfo_t *buf = bufferobject->second;
		//imgui_log("buffer %p: data=%p size=%d\n", buf, buf->data, buf->size);
		*buffer = (char *)buf->data;
		*out_filesize = buf->size;
		return 1;
	}

#endif
	//PHYSFS_File *f = PHYSFS_openRead(path);
	//if (f == NULL) {
	//	imgui_log("file_get_contents(\"%s\"): Can't open/read file\n", path);
	//	return 0;
	//}
	//// lets say we have the file text.txt with the content "test"
    //int length = PHYSFS_fileLength(f); // the length would be 4
	//*out_filesize = length;
    //*buffer = (char *)calloc(length + 1, sizeof(char)); // but we are actually allocating 5 bytes
	//PHYSFS_read(f, *buffer, 1, length); // we read in the 4 letters
	//
	//// quite tricky, we have a pointer to a pointer, so first dereference it
	//(*buffer)[length] = '\0'; // and set buffer[4] to zero, to mark end of string
	//PHYSFS_close(f);
    //return 1;
}

CCALL ramfileinfo_t *file_get_buffer_by_name(char *filename) {
	auto bufferobject = availableFiles.find(std::string(filename));
	if (bufferobject == availableFiles.end())
		return NULL;
	return bufferobject->second;
}

CCALL int file_flush_disc(char *filename_) {
	char filename[512];
	if (filename_[0] == '.') {
		sprintf(filename, "%s", filename_);
	} else {
		sprintf(filename, "./%s", filename_);
	}
	ramfileinfo_t *filebuffer = file_get_buffer_by_name(filename);
	if (filebuffer == NULL)
		return 0;

#ifdef EMSCRIPTEN
	
			EM_ASM_({
			  callback_save_file($0, $1, $2);
			}, filebuffer->data, filebuffer->size, filename);
#else
		FILE *file = fopen(filename, "wb");
		if (file == NULL)
			return 0;
		fwrite(filebuffer->data, 1, filebuffer->size, file);
		fclose(file);	

		filebuffer->changed = 0; // on Desktop we can treat the file as saved, on WebAssembly we need to wait for success of the ajax callback
#endif
	return 1;
}

CCALL int file_put_contents(const char *filename_, char *buffer, size_t filesize) {
	char filename[512];
	if (filename_[0] == '.') {
		sprintf(filename, "%s", filename_);
	} else {
		sprintf(filename, "./%s", filename_);
	}
	ramfileinfo_t *filebuffer = file_get_buffer_by_name(filename);
	
	// todo: create filebuffer if it doesnt exist yet 
	if (buffer) {
		filebuffer->data = (char *)realloc(filebuffer->data, filesize);
		memcpy(filebuffer->data, buffer, filesize);
		filebuffer->changed++;

		file_flush_disc(filename);
	}

	return 1;
}
CCALL FILE *test_fopen() {
	return fopen("test.txt", "r");
}

CCALL void file_loaded(char *filename, char *buffer, int buffersize) {
	//printf("filename=%d buffer=%d buffersize=%d\n", filename, buffer, buffersize);
	//printf("filename=%s buffer=%s buffersize=%d\n", filename, buffer, buffersize);
	int n = strlen(filename);
	for (int i=0; i<n; i++) {
		if (filename[i] == '\\')
			filename[i] = '/';
	}
	//imgui_log("filename=%s buffer=%d buffersize=%d\n", filename, buffer, buffersize);
	//js_eval(buffer);
	ramfileinfo_t *buf = new ramfileinfo_t();
	//buf->filename = filename;
	buf->data = buffer;
	buf->size = buffersize;
	buf->filename = strdup(filename);
	availableFiles[std::string(filename)] = buf;
}



CCALL int HTML_debug() {

	//imgui_log("availableFiles.size() = %d\n", availableFiles.size());
	for (auto file : availableFiles) {
		//imgui_log("file: %s\n", file.first.c_str());
	}
	return availableFiles.size();
}

int HTML_unlink(char *filename_) {


	char filename[512];
	if (filename_[0] == '.') {
		sprintf(filename, "%s", filename_);
	} else {
		sprintf(filename, "./%s", filename_);
	}

	auto bufferobject = availableFiles.find(std::string(filename));
	// hashmap contains the string?
	if (bufferobject != availableFiles.end()) {
		// then delete it and return 1
		availableFiles.erase(bufferobject);
		return 1;
	}
	// otherwise just return 0
	return 0;
}

CCALL RAMFILE *HTML_fopen(char *filename_, const char *mode) {
	//imgui_log("bla1\n");
	//printf("bla1\n");
	RAMFILE *htmlfile = (RAMFILE *) malloc(sizeof(RAMFILE));
	memset(htmlfile, 0, sizeof(RAMFILE));
	htmlfile->position = 0;
	

	char filename[512];
	if (filename_[0] == '.') {
		sprintf(filename, "%s", filename_);
	} else {
		sprintf(filename, "./%s", filename_);
	}
	

	//printf("bla2\n");
	// make sure every path is using same separator, otherwise hash map lookup fails
	int n = strlen(filename);
	for (int i=0; i<n; i++) {
		if (filename[i] == '\\')
			filename[i] = '/';
	}
	

	if (mode[0] == 'w') {
			// create a buffer for the file
			ramfileinfo_t *buf = new ramfileinfo_t();
			buf->size = 0;
			buf->data = (char *)malloc(0);
			buf->filename = strdup(filename);

			// add buffer to availableFiles
			availableFiles[std::string(filename)] = buf;

			// return buffer data as HTML_FILE
			htmlfile->ramfileinfo = buf;
			htmlfile->position = 0;
			return htmlfile;
	}

	//if (strcmp(filename, ".\\baseq3\\killtube\\textures\\calm_pool.jpg")==0) {
	//if (strstr(filename, "pool")!=0) {
	//
	//__asm int 3;
	//}

	//printf("bla3\n");
	auto bufferobject = availableFiles.find(std::string(filename));

	// if not available yet, load the file
	if (bufferobject == availableFiles.end()) {

// we cant use fopen() etc. when running from emscripten, we use fetch() before calling main(), which calls file_loaded(), which adds the buffer to availableFiles
// so all files are basically just written to HEAP8 from javascript as plain memory cache
#ifdef __EMSCRIPTEN__
		return NULL;
#else
		//printf("HTML_fopen: %s\n", filename);


		FILE *file = fopen(filename, "rb");
		if (file) {
			// read in file
			size_t filesize;
			char *data = NULL;
			fseek(file, 0, SEEK_END);
			filesize = ftell(file);
			data = (char *)malloc(filesize + 1); // no matter what, add \0 at end
			fseek(file, 0L, SEEK_SET);
			fread(data, filesize, 1, file);
			fclose(file);

			// create a buffer for the file
			ramfileinfo_t *buf = new ramfileinfo_t();
			buf->size = filesize;
			buf->data = data;
			buf->data[filesize] = 0; // end data, might be a string
			buf->filename = strdup(filename);

			// add buffer to availableFiles
			availableFiles[std::string(filename)] = buf;

			// return buffer data as HTML_FILE
			htmlfile->ramfileinfo = buf;
			return htmlfile;
		}
		//*buffer = NULL;
		//*out_filesize = 0;
		return NULL;
		//int ret = file_get_contents(filename, (char **) &htmlfile->data, &htmlfile->size);
		//if (ret == 0)
		//	return NULL;


		
		printf("file done\n");
#endif		
	} else {
		ramfileinfo_t *buf = bufferobject->second;
		htmlfile->ramfileinfo = buf;
		return htmlfile;
	}

	return htmlfile;
}


CCALL size_t HTML_fplength(RAMFILE *ramfile) {
	return ramfile->ramfileinfo->size;
}

//#define fopen HTML_fopen;

CCALL size_t HTML_fread(void *buffer, size_t element_size, size_t element_count, RAMFILE *htmlfile) {
	size_t ret;

	
	
	size_t amount = element_size * element_count;
	size_t newpos = htmlfile->position + amount;
	if (newpos > htmlfile->ramfileinfo->size)
	{
		amount = htmlfile->ramfileinfo->size - htmlfile->position;
	}

	//ret = fread(buffer, element_size, element_count, htmlfile->original);
	memcpy(buffer, (void *)((size_t)htmlfile->ramfileinfo->data + htmlfile->position), amount);

	htmlfile->position += amount; // strip to end?

	return amount;

	//if (amount != ret) {
	//	//Com_Printf("need to be the same...\n");
	//}
	//return ret;
}

// not implemented, dont need it yet for anything...
CCALL size_t HTML_fwrite(const void *buffer, size_t element_size, size_t element_count, RAMFILE *htmlfile) {
	//return fwrite(buffer, element_size, element_count, htmlfile->original);
	return 0;
}

CCALL long HTML_ftell(RAMFILE *ramfile) {
	//return ftell(ramfile->original);
	return ramfile->position;
}

CCALL int HTML_fseek(RAMFILE *htmlfile, long offset, int origin) {
	int ret;
	
	if (origin == SEEK_END) {
		htmlfile->position = htmlfile->ramfileinfo->size;
	}
	if (origin == SEEK_SET) {
		htmlfile->position = offset;
	}


	return htmlfile->position;

	//ret = fseek(htmlfile->original, offset, origin);
	//
	//size_t testpos = ftell(htmlfile->original);
	//if (testpos != htmlfile->position) {
	//	Com_Printf("Wrong pos...");
	//}
	//
	//return ret;
}

CCALL int HTML_fclose(RAMFILE *htmlfile) {
	// data is only allocated once per file in a Buffer object, which needs to stay alive
	//free(htmlfile->data);
	//int ret = fclose(htmlfile->original);
	if (htmlfile->ramfileinfo->changed) {
		file_flush_disc(htmlfile->ramfileinfo->filename);
		htmlfile->ramfileinfo->changed = 0;
	}
	free(htmlfile);
	return 0;
}

CCALL int HTML_ferror(RAMFILE *htmlfile) {
	return 0;
	//return ferror(htmlfile->original);
}

CCALL int HTML_fscanf(RAMFILE *f, char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int ret = vsscanf((const char *)((size_t)f->ramfileinfo->data + f->position), fmt, args);
	va_end(args);
	f->position += ret;
	// meh no clue how to get %n shit via varargs... just jump to next line, works for my imgui .layout case
	// sooner or later is just wanna use JSON over JS-API nonetheless
	for (int i=f->position; i<f->ramfileinfo->size; i++) {
		if (((char *)f->ramfileinfo->data)[i] == '\n') {
			f->position = i + 1;
			break;
		}
	}
	return ret;
}

CCALL int HTML_fprintf(RAMFILE *f, char *fmt, ...) {
	char buf[4069];
	va_list args;
	va_start(args, fmt);
	int ret = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	// add the size to the allocated memory.... todo: check if we really need to add memory, might be seeked into existing memory
	f->ramfileinfo->size += ret;
	f->ramfileinfo->data = (char *) realloc(f->ramfileinfo->data, f->ramfileinfo->size);
	// now copy over the buf to the f->position
	memcpy((void *)((size_t)f->ramfileinfo->data + f->position), buf, ret);
	// and increase f->position by the amount we have written
	f->position += ret;
	f->ramfileinfo->changed++; // count every change
	return ret;
}

CCALL int file_dump_memory(char *filename, void *memory, size_t bytes) {
	FILE *file = fopen(filename, "wb");
	if (file == NULL)
		return 0;
	fwrite(memory, 1, bytes, file);
	fclose(file);
	return 1;
}