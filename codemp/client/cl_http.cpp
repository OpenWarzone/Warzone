#include "client.h"
#include "../lib/libcurl/curl/curl.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

CURL		*curl = NULL;
struct		curl_slist *list = NULL;

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);

  if(mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

void CURL_Init( void )
{
	if (curl) return;

	/* In windows, this will init the winsock stuff */ 
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */ 
	curl = curl_easy_init();

#ifdef __TTS_DEVELOPER__
	CURLcode	res;

	// NOW REQUIRES...
	// Content-Type: application/x-www-form-urlencoded; charset=UTF-8
	// Cookie: acabox=ouu8js0ke7tp15bmpeuct9f4g1

	//curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.80 Safari/537.36 OPR/33.0.1990.58");

	// EXTRAS - Just in case...
	// Referer:https://acapela-box.com/AcaBox/index.php
	// User-Agent:Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.80 Safari/537.36 OPR/33.0.1990.58

	if(curl) {
		struct MemoryStruct chunk;

		chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
		chunk.size = 0;    /* no data at this point */ 

		list = curl_slist_append(list, "Referer: https://acapela-box.com/AcaBox/index.php");
		list = curl_slist_append(list, "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2490.80 Safari/537.36 OPR/33.0.1990.58");
		list = curl_slist_append(list, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
		//list = curl_slist_append(list, "Cookie: acabox=ouu8js0ke7tp15bmpeuct9f4g1");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "acapela-cookie.txt");

		curl_easy_setopt(curl, CURLOPT_URL, "https://acapela-box.com/AcaBox/index.php");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		res = curl_easy_perform(curl);

		if(chunk.memory)
		{
			free(chunk.memory);
		}

		chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
		chunk.size = 0;    /* no data at this point */ 

		curl_easy_setopt(curl, CURLOPT_URL, "https://acapela-box.com/AcaBox/login.php");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		res = curl_easy_perform(curl);

		if(chunk.memory)
		{
			free(chunk.memory);
		}
	}
#endif //__TTS_DEVELOPER__
}

void CURL_Shutdown ( void )
{
	if (curl) 
	{
		curl_slist_free_all(list); /* free the list again */

		/* always cleanup */ 
		curl_easy_cleanup(curl);

		curl_global_cleanup();

		curl = NULL;
	}
}
 
size_t GetHttpPostData(char *address, char *poststr, char *recvdata)
{
	CURLcode	res;
	size_t		size = 0;

	struct MemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	if(curl) {
		/* First set the URL that is about to receive our POST. This URL can
		just as well be a https:// URL if that is what should receive the
		data. */ 
		curl_easy_setopt(curl, CURLOPT_URL, address);

		/* Now specify the POST data */ 
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, poststr);

		/* send all data to this function  */ 
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

		/* we pass our 'chunk' struct to the callback function */ 
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		
		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);

		/* Check for errors */ 
		if(res != CURLE_OK)
		{
			printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			return 0;
		}
	}

	if(chunk.memory)
	{
		size = chunk.size;

		if (size > 4096+1) size = 4096+1; // Our max size to return is always 4096+1 (for now)... No over-runs...
		memcpy(recvdata, chunk.memory, size);

		free(chunk.memory);
	}

	return size;
}

void GetHttpDownload(char *address, char *out_file)
{
	CURLcode	res;
	size_t		size = 0;

	struct MemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	if(curl) {
		/* First set the URL that is about to receive our POST. This URL can
		just as well be a https:// URL if that is what should receive the
		data. */ 
		curl_easy_setopt(curl, CURLOPT_URL, address);

		/* send all data to this function  */ 
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

		/* we pass our 'chunk' struct to the callback function */ 
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		
		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);

		/* Check for errors */ 
		if(res != CURLE_OK)
		{
			printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			return;
		}
	}

	if(chunk.memory)
	{
		FILE *fileOut = fopen(out_file, "wb");

		if (fileOut)
		{
			fwrite(chunk.memory, chunk.size, 1, fileOut);
			fclose(fileOut);

			//printf("TTS %s cached. Size %i.\n", out_file, chunk.size);
		}
		else
		{
			printf("Failed to write %s.\n", out_file);
		}

		free(chunk.memory);
	}
}
