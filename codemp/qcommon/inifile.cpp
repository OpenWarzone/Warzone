//#if defined(rd_warzone_x86_EXPORTS) || defined(DEDICATED) || defined(jampgamex86_EXPORTS) || defined(cgamex86_EXPORTS) || defined(uix86_EXPORTS)


//#define __INI_DEBUG__
//#define __INI_DEBUG_VERBOSE__


//
// Mixing C and C++ in one codebase is soooooo annoying!!!!
//
#include "inifile.h"   /* function prototypes in here */

#ifdef _WIN32
#define unlink _unlink
#include <windows.h>
#endif

#include <algorithm>
#include <string>

#if defined(rd_warzone_x86_EXPORTS)
#include "tr_local.h"
#include "../qcommon/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "../rd-common/tr_public.h"
#include "../rd-common/tr_common.h"
int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	if (mode == FS_WRITE)
		//return ri->FS_FOpenFileWrite(qpath, qfalse);
		return ri->FS_FOpenFileByMode(qpath, f, mode);
	else
		return ri->FS_FOpenFileRead(qpath, f, qfalse);
}
//#define FS_FOpenFileByMode ri->FS_FOpenFileByMode
#define FS_Read ri->FS_Read
#define FS_Write ri->FS_Write
#define FS_FCloseFile ri->FS_FCloseFile
#elif defined(jampgamex86_EXPORTS)
#include "game/g_local.h"
#define FS_FOpenFileByMode trap->FS_Open
#define FS_Read trap->FS_Read
#define FS_Write trap->FS_Write
#define FS_FCloseFile trap->FS_Close
#elif defined(cgamex86_EXPORTS)
#include "cgame/cg_local.h"
#define FS_FOpenFileByMode trap->FS_Open
#define FS_Read trap->FS_Read
#define FS_Write trap->FS_Write
#define FS_FCloseFile trap->FS_Close
#elif defined(uix86_EXPORTS)
#include "ui/ui_local.h"
#define FS_FOpenFileByMode trap->FS_Open
#define FS_Read trap->FS_Read
#define FS_Write trap->FS_Write
#define FS_FCloseFile trap->FS_Close
#else
#include "q_shared.h"
extern int FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode );
extern int FS_Read( void *buffer, int len, fileHandle_t f );
extern int FS_Write( const void *buffer, int len, fileHandle_t h );
extern void FS_FCloseFile( fileHandle_t f );
#endif

void DebugPrint(char *text)
{
#if defined(__INI_DEBUG_VERBOSE__) || defined(__INI_DEBUG__)

#if defined(rd_warzone_x86_EXPORTS)
	ri->Printf(PRINT_ALL, text);
#elif defined(_CGAME) || defined(_GAME) || defined(uix86_EXPORTS)
	trap->Print(text);
#elif defined(DEDICATED)
	Com_Printf(text);
#else
	Com_Printf(text);
#endif

#endif //defined(__INI_DEBUG_VERBOSE__) || defined(__INI_DEBUG__)
}

bool invalidChar(char c)
{
	//return !(c >= 0 && c <128);
	return !isprint(static_cast<unsigned char>(c));
}
void stripUnicode(std::string & str)
{
	str.erase(remove_if(str.begin(), str.end(), invalidChar), str.end());
}

char FS_GetC(fileHandle_t fp)
{
	char c = '\0';
	FS_Read(&c, sizeof(char), fp);
	return c;
}

/*****************************************************************
* Function:     read_line()
* Arguments:    <fileHandle_t> fp - a pointer to the file to be read from
*               <char *> bp - a pointer to the copy buffer
* Returns:      TRUE if successful FALSE otherwise
******************************************************************/
int read_line(fileHandle_t fp, char *bp)
{
	char c = '\0';
	int i = 0;

	/* Read one line from the source fileHandle_t */
	while ((c = FS_GetC(fp)) != '\n')
	{
		if (c == EOF || c == '\0')         /* return FALSE on unexpected EOF */
			return(0);

		bp[i] = c;
		i++;
	}

	bp[i] = '\0';

	/*#ifdef _GAME
		trap->Print("%s\n", bp);
		#endif*/

	return(1);
}

/**************************************************************************
* Function:     get_private_profile_string()
* Arguments:    <char *> section - the name of the section to search for
*               <char *> entry - the name of the entry to find the value of
*               <char *> def - default string in the event of a failed read
*               <char *> buffer - a pointer to the buffer to copy into
*               <int> buffer_len - the max number of characters to copy
*               <char *> file_name - the name of the .ini file to read from
* Returns:      the number of characters copied into the supplied buffer
***************************************************************************/
int get_private_profile_string(char *section, char *entry, char *def, char *buffer, int buffer_len, char *file_name)
{
	fileHandle_t fp = NULL;
	char buff[MAX_LINE_LENGTH] = { 0 };
	char *ep;
	char t_section[MAX_LINE_LENGTH] = { 0 };
	int len = strlen(entry);
	
	int fpLen = FS_FOpenFileByMode(file_name, &fp, FS_READ);

	if (!fp)
	{
#ifdef __INI_DEBUG_VERBOSE__
		DebugPrint(">> File not found, or zero length.\n");
#endif //__INI_DEBUG_VERBOSE__
		return(0);
	}

	if (!fpLen)
	{
#ifdef __INI_DEBUG_VERBOSE__
		DebugPrint(">> File not found, or zero length.\n");
#endif //__INI_DEBUG_VERBOSE__
		FS_FCloseFile(fp);               /* Clean up and return the amount copied */
		return(0);
	}

	sprintf(t_section, "[%s]", section);    /* Format the section name */

#ifdef __INI_DEBUG_VERBOSE__
	DebugPrint(va(">> Searching for: [section] %s [key] %s\n", section, entry));
#endif //__INI_DEBUG_VERBOSE__

	/*  Move through file 1 line at a time until a section is matched or EOF */
	do
	{
		if (!read_line(fp, buff))
		{
#ifdef __INI_DEBUG__
			DebugPrint(">> File end looking for section.\n");
#endif //__INI_DEBUG__

			FS_FCloseFile(fp);
			strncpy(buffer, def, buffer_len);
			return(strlen(buffer));
		}

#ifdef __INI_DEBUG_VERBOSE__
		DebugPrint(va(">> Line: %s\n", buff));
#endif //__INI_DEBUG_VERBOSE__

	} while (strncmp(buff, t_section, strlen(t_section)));

	/* Now that the section has been found, find the entry.
	 * Stop searching upon leaving the section's area. */
	do
	{
		if (!read_line(fp, buff) || buff[0] == '\0')
		{
#ifdef __INI_DEBUG__
			DebugPrint(">> File end looking for key.\n");
#endif //__INI_DEBUG__

			FS_FCloseFile(fp);
			strncpy(buffer, def, buffer_len);
			return(strlen(buffer));
		}

#ifdef __INI_DEBUG_VERBOSE__
		DebugPrint(va(">> Line: %s\n", buff));
#endif //__INI_DEBUG_VERBOSE__

	} while (strncmp(buff, entry, len));

	ep = strrchr(buff, '=');    /* Parse out the equal sign */
	ep++;

	/* Copy up to buffer_len chars to buffer */
	
	//if (/*ep[strlen(ep)] == '\0' ||*/ ep[strlen(ep)] == '\n')
	//	strncpy(buffer, ep, strlen(ep) - 1); // -1 to remove the trailing \n
	//else
	//	strncpy(buffer, ep, strlen(ep)); // WTF OJK, Why does the render version strip the newlines... the others don't! lol

	std::string str = ep;
	str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
	str.erase(std::remove(str.begin(), str.end(), '\0'), str.end());
	stripUnicode(str);
	strncpy(buffer, str.c_str(), str.length());

	FS_FCloseFile(fp);               /* Clean up and return the amount copied */

#ifdef __INI_DEBUG_VERBOSE__
	DebugPrint(va(">> [ep] %s. [count] %i.\n", ep, strlen(ep) - 1));
#endif //__INI_DEBUG_VERBOSE__
#ifdef __INI_DEBUG__
	DebugPrint(va(">> Found: %s\n", buffer));
#endif //__INI_DEBUG__

	return(strlen(buffer));
}

void DumpIniData(char *file_name, char *buffer)
{
	fileHandle_t wfp = NULL;

	int wfpLen = FS_FOpenFileByMode(file_name, &wfp, FS_WRITE);

	if (!wfp)
	{
		DebugPrint(va("unable to open output file.\n"));
		return;
	}

	FS_Write(buffer, strlen(buffer), wfp);
	FS_FCloseFile(wfp);
}

/*************************************************************************
 * Function:    write_private_profile_string()
 * Arguments:   <char *> section - the name of the section to search for
 *              <char *> entry - the name of the entry to find the value of
 *              <char *> buffer - pointer to the buffer that holds the string
 *              <char *> file_name - the name of the .ini file to read from
 * Returns:     TRUE if successful, otherwise FALSE
 *************************************************************************/
int write_private_profile_string(char *section, char *entry, char *buffer, char *file_name)
{
	fileHandle_t rfp = NULL;
	char buff[MAX_LINE_LENGTH] = { 0 };
	char t_section[MAX_LINE_LENGTH] = { 0 };
	int len = strlen(entry);
	int rfpLen;
	char out_buffer[524288] = { 0 };

	sprintf(t_section, "[%s]", section);/* Format the section name */

	rfpLen = FS_FOpenFileByMode(file_name, &rfp, FS_READ);

	if (!rfp || !rfpLen)  /* If the .ini file doesn't exist */
	{
		char sectionText[16384/*81*/] = { 0 };
		char entryText[16384/*1024*/] = { 0 };

		sprintf(sectionText, "%s\n", t_section);
		strcat(out_buffer, sectionText);

		sprintf(entryText, "%s=%s\n", entry, buffer);
		strcat(out_buffer, entryText);

		DumpIniData(file_name, out_buffer);
		return(1);
	}

	/* Move through the file one line at a time until a section is
	 * matched or until EOF. Copy to temp file as it is read. */

	do
	{
		char buffText[16384/*81*/] = { 0 };

		if (!read_line(rfp, buff))
		{
			char sectionText[16384/*81*/] = { 0 };
			char entryText[16384/*1024*/] = { 0 };
			char newlineText[2] = { 0 };

			/* Failed to find section, so add one to the end */
			sprintf(newlineText, "\n");
			strcat(out_buffer, sectionText);

			sprintf(sectionText, "%s\n", t_section);
			strcat(out_buffer, sectionText);

			sprintf(entryText, "%s=%s\n", entry, buffer);
			strcat(out_buffer, entryText);

			FS_FCloseFile(rfp);

			DumpIniData(file_name, out_buffer);
			return(1);
		}

		sprintf(buffText, "%s\n", buff);
		strcat(out_buffer, buffText);

	} while (strncmp(buff, t_section, strlen(t_section)));

	/* Now that the section has been found, find the entry. Stop searching
	 * upon leaving the section's area. Copy the file as it is read and
	 * create an entry if one is not found.  */
	while (1)
	{
		char buffText[16384/*81*/] = { 0 };

		if (!read_line(rfp, buff))
		{
			char entryText[16384/*1024*/] = { 0 };

			/* EOF without an entry so make one */
			sprintf(entryText, "%s=%s\n", entry, buffer);
			strcat(out_buffer, entryText);

			FS_FCloseFile(rfp);

			DumpIniData(file_name, out_buffer);
			return(1);
		}

		if (!strncmp(buff, entry, len) || buff[0] == '\n'/*'\0'*/)
			break;

		sprintf(buffText, "%s\n", buff);
		strcat(out_buffer, buffText);
	}

	if (buff[0] == '\n'/*'\0'*/)
	{
		char entryText[16384/*1024*/] = { 0 };

		sprintf(entryText, "%s=%s\n", entry, buffer);
		strcat(out_buffer, entryText);

		do
		{
			char buffText[16384/*81*/] = { 0 };
			sprintf(buffText, "%s\n", buff);
			strcat(out_buffer, buffText);
		} while (read_line(rfp, buff));
	}
	else
	{
		char entryText[16384/*1024*/] = { 0 };

		sprintf(entryText, "%s=%s\n", entry, buffer);
		strcat(out_buffer, entryText);

		while (read_line(rfp, buff))
		{
			char buffText[16384/*81*/] = { 0 };
			sprintf(buffText, "%s\n", buff);
			strcat(out_buffer, buffText);
		}
	}

	FS_FCloseFile(rfp);

	DumpIniData(file_name, out_buffer);
	return(1);
}

// ==============================================================================================================
// C++ Interface Layer Functions - For simple usage from C++ code...
// ==============================================================================================================

bool IniExists(char *file_name)
{
	fileHandle_t fp = NULL;

	int fpLen = FS_FOpenFileByMode(file_name, &fp, FS_READ);

	if (!fp)
	{
		return false;
	}

	if (!fpLen)
	{
		FS_FCloseFile(fp);
		return false;
	}

	FS_FCloseFile(fp);
	return true;
}

const char *IniReadCPP(char *aFilespec, char *aSection, char *aKey, char *aDefault)
{
	if (!aDefault || !*aDefault)
		aDefault = "";

	if (!aKey || !aKey[0])
		return "";

	char	szBuffer[512/*524288*/] = { 0 };					// Max ini file size is 65535 under 95 -- UQ1: Fuck windows 95!

	if (!get_private_profile_string(aSection, aKey, aDefault, szBuffer, sizeof(szBuffer), aFilespec))
	{
		return aDefault;
	}

	return va("%s", szBuffer);
}

void IniWriteCPP(char *aFilespec, char *aSection, char *aKey, char *aValue)
{
	write_private_profile_string(aSection, aKey, aValue, aFilespec);
}

// ==============================================================================================================
// C Interface Layer Functions - For simple access from Q3 code...
// ==============================================================================================================

/*
// Example usage
PLAYER_FACTION = IniRead("general.ini","PLAYER_SETTINGS","PLAYER_FACTION","imperial");
PLAYER_HEALTH = atoi(IniRead("general.ini","PLAYER_SETTINGS","PLAYER_HEALTH","100"));
*/

const char *IniRead(char *aFilespec, char *aSection, char *aKey, char *aDefault)
{
	try
	{
		const char *value = IniReadCPP(aFilespec, aSection, aKey, aDefault);

		if (value[0] == '\0' || !strcmp(value, "") || !strcmp(value, aDefault))
		{
#ifdef __INI_DEBUG__
			DebugPrint(va("[file] %s [section] %s [key] %s [default value] %s\n", aFilespec, aSection, aKey, aDefault));
#endif //__INI_DEBUG__
			return aDefault;
	}

#ifdef __INI_DEBUG__
		DebugPrint(va("[file] %s [section] %s [key] %s [value] %s\n", aFilespec, aSection, aKey, value));
#endif //__INI_DEBUG__

		return value;
	}
	catch (int code) 
	{
#if defined(rd_warzone_x86_EXPORTS)
		ri->Printf(PRINT_WARNING, "Error reading %s from section %s of ini file %s. Check your ini file. Returning default value. Error code %i.\n", aKey, aSection, aFilespec, code);
#elif defined(_CGAME) || defined(_GAME) || defined(uix86_EXPORTS)
		trap->Print("Error reading %s from section %s of ini file %s. Check your ini file. Returning default value. Error code %i.\n", aKey, aSection, aFilespec, code);
#elif defined(DEDICATED)
		Com_Printf("Error reading %s from section %s of ini file %s. Check your ini file. Returning default value. Error code %i.\n", aKey, aSection, aFilespec, code);
#else
		Com_Printf("Error reading %s from section %s of ini file %s. Check your ini file. Returning default value. Error code %i.\n", aKey, aSection, aFilespec, code);
#endif
		return aDefault;
	}
}

void IniWrite(char *aFilespec, char *aSection, char *aKey, char *aValue)
{
	IniWriteCPP(aFilespec, aSection, aKey, aValue);
}
//#endif //defined(rd_warzone_x86_EXPORTS) || defined(DEDICATED) || defined(jampgamex86_EXPORTS) || defined(cgamex86_EXPORTS) || defined(uix86_EXPORTS)