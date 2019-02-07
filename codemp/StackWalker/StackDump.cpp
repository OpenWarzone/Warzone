/*
======================
    ReportCrash
======================
*/
 
#if defined (_WIN32) && !defined(_DEBUG) && !defined(DEDICATED)
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <array>
#include <string>
#include <filesystem>

#include "win32\resource.h"
#include "win32\win_local.h"
#include "qcommon\q_shared.h"
#include "qcommon\qcommon.h"
#include <DbgHelp.h>

extern WinVars_t g_wv;

typedef struct
{
    DWORD excThreadId;
   
    DWORD excCode;
    PEXCEPTION_POINTERS pExcPtrs;
   
    int numIgnoreThreads;
    DWORD ignoreThreadIds[16];
   
    int numIgnoreModules;
    HMODULE ignoreModules[16];
} dumpParams_t;
 
typedef struct dumpCbParams_t
{
    dumpParams_t*   p;
    HANDLE          hModulesFile;
} dumpCbParams_t;
 
static BOOL CALLBACK MiniDumpCallback( PVOID CallbackParam, const PMINIDUMP_CALLBACK_INPUT CallbackInput, PMINIDUMP_CALLBACK_OUTPUT CallbackOutput )
{
    const dumpCbParams_t* params = ( dumpCbParams_t* )CallbackParam;

	//MessageBox(NULL, va("CallbackInput->CallbackType %i", CallbackInput->CallbackType), "Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1);
   
    switch( CallbackInput->CallbackType )
    {
        case IncludeThreadCallback:
        {
            int i;
           
            for( i = 0; i < params->p->numIgnoreThreads; i++ )
            {
                if( CallbackInput->IncludeThread.ThreadId == params->p->ignoreThreadIds[i] )
                {
                    return FALSE;
                }
            }
        }
        return TRUE;
       
        case IncludeModuleCallback:
        {
            int i;
           
            for( i = 0; i < params->p->numIgnoreModules; i++ )
            {
                if( ( HMODULE )CallbackInput->IncludeModule.BaseOfImage == params->p->ignoreModules[i] )
                {
                    return FALSE;
                }
            }
        }
        return TRUE;
       
        case ModuleCallback:
            if( params->hModulesFile )
            {
                PWCHAR packIncludeMods[] = { L"Warzone.x86.exe", L"Warzoneded.x86.exe", L"rd-warzone_x86.dll", L"cgamex86.dll", L"jampgamex86.dll", L"uix86.dll" };
               
                wchar_t* path = CallbackInput->Module.FullPath;
                size_t len = wcslen( path );
                size_t i, c;
               
                //path is fully qualified - this won't trash
                for( i = len; i--; )
                {
                    if( path[i] == L'\\' )
                    {
                        break;
                    }
                }

				//MessageBox(NULL, "Writing...", "Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1);
               
                c = i + 1;
               
                for( i = 0; i < 6 /* lengthof(packIncludeMods[]) */; i++ )
                    if( wcsicmp( path + c, packIncludeMods[i] ) == 0 )
                    {
                        DWORD dummy;

						//MessageBox(NULL, va("Writing... %s", path), "Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1);
                       
                        WriteFile( params->hModulesFile, path, sizeof( wchar_t ) * len, &dummy, NULL );
                        WriteFile( params->hModulesFile, L"\n", sizeof( wchar_t ), &dummy, NULL );
                        break;
                    }
            }
            return TRUE;
           
        default:
            return TRUE;
    }
}
 
typedef BOOL ( WINAPI* MiniDumpWriteDumpFunc_t )(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
);
 
/*
==============
CreatePath
==============
*/
static void CreatePath( char* OSPath )
{
    char* ofs;
   
    for( ofs = OSPath + 1; *ofs; ofs++ )
    {
        if( *ofs == '\\' || *ofs == '/' )
        {
            // create the directory
            *ofs = 0;
           
            CreateDirectory( OSPath, NULL );
           
            *ofs = '\\';
        }
    }
   
    if( ofs[-1] != '\\' )
    {
        CreateDirectory( OSPath, NULL );
    }
}
 
/*
==============
DoGenerateCrashDump
==============
*/
static DWORD WINAPI DoGenerateCrashDump( LPVOID pParams )
{
    dumpParams_t* params = ( dumpParams_t* )pParams;
   
    static int nDump = 0;
   
    HANDLE hFile, hIncludeFile;
    DWORD dummy;
    char basePath[MAX_PATH], path[MAX_PATH];
   
    MiniDumpWriteDumpFunc_t MiniDumpWriteDump;
   
    {
        HMODULE hDbgHelp = LoadLibrary( "DbgHelp.dll" );
       
        if( !hDbgHelp )
        {
            return 1;
        }
       
        MiniDumpWriteDump = ( MiniDumpWriteDumpFunc_t )GetProcAddress( hDbgHelp, "MiniDumpWriteDump" );
       
        if( !MiniDumpWriteDump )
        {
            return 2;
        }
       
        params->ignoreModules[params->numIgnoreModules++] = hDbgHelp;
    }
   
    {
        char homePath[ MAX_OSPATH ];
		strcpy(homePath, Sys_DefaultHomePath());
        
        if( !homePath[0] )
        {
            return 3;
        }
       
        Com_sprintf( basePath, sizeof( basePath ), "%s/crashDumps", homePath );
    }
   
    CreatePath( basePath );

	//
	// Create a crash dump filename based on date/time...
	//
	std::array<char, 64> buffer;
	buffer.fill(0);
	time_t rawtime;
	time(&rawtime);
	const auto timeinfo = localtime(&rawtime);
	strftime(buffer.data(), sizeof(buffer), "%Y-%m-%d-%H-%M-%S", timeinfo);
	std::string timeStr(buffer.data());
	std::string crashDumpFilename = "crashdump-" + timeStr;
   
	//MessageBox(NULL, va("Dump path: %s.", crashDumpFilename.c_str()), "Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1);

	// Using date/time should not require finding a free file to use...
	Com_sprintf(path, sizeof(path), "%s/%s.dmp", basePath, crashDumpFilename.c_str());
	hFile = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);

   
    Com_sprintf( path, sizeof( path ), "%s/%s.include", basePath, crashDumpFilename.c_str() );
    hIncludeFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
   
    {
        dumpCbParams_t cbParams;
       
        MINIDUMP_EXCEPTION_INFORMATION excInfo;
        MINIDUMP_CALLBACK_INFORMATION cbInfo;
        MINIDUMP_TYPE flags;
       
        excInfo.ThreadId = params->excThreadId;
        excInfo.ExceptionPointers = params->pExcPtrs;
        excInfo.ClientPointers = TRUE;
       
        cbParams.p = params;
        cbParams.hModulesFile = hIncludeFile;
       
        cbInfo.CallbackParam = &cbParams;
        cbInfo.CallbackRoutine = MiniDumpCallback;
       
        MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hFile,
                           flags, &excInfo, NULL, &cbInfo );
    }
   
    CloseHandle( hFile );

    {
        /*
            Write any additional include files here. Remember,
            the include file is unicode (wchar_t), has one
            file per line, and lines are seperated by a single LF (L'\n').
        */
    }
    CloseHandle( hIncludeFile );

#if 1
    Com_sprintf( path, sizeof( path ), "%s/%s.build", basePath, crashDumpFilename.c_str() );
    hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
   
    {
        HMODULE hMod;
       
        HRSRC rcVer, rcBuild, rcMachine;
        HGLOBAL hgVer, hgBuild, hgMachine;
       
        DWORD lVer, lBuild, lMachine;
        char* pVer, *pBuild, *pMachine;
       
        hMod = GetModuleHandle( NULL );
       
		// TODO: Have no version infos in wz atm...
		rcVer = 0;// FindResource(hMod, MAKEINTRESOURCE(IDR_INFO_SVNSTAT), "INFO");
        rcBuild = 0;//FindResource( hMod, MAKEINTRESOURCE( IDR_INFO_BUILDCONFIG ), "INFO" );
        rcMachine = 0;//FindResource( hMod, MAKEINTRESOURCE( IDR_INFO_BUILDMACHINE ), "INFO" );
       
        hgVer = LoadResource( hMod, rcVer );
        hgBuild = LoadResource( hMod, rcBuild );
        hgMachine = LoadResource( hMod, rcMachine );
       
        lVer = SizeofResource( hMod, rcVer );
        pVer = ( char* )LockResource( hgVer );
       
        lBuild = SizeofResource( hMod, rcBuild );
        pBuild = ( char* )LockResource( hgBuild );
       
        lMachine = SizeofResource( hMod, rcMachine );
        pMachine = ( char* )LockResource( hgMachine );
       
        WriteFile( hFile, pBuild, lBuild, &dummy, NULL );
        WriteFile( hFile, pMachine, lMachine, &dummy, NULL );
        WriteFile( hFile, pVer, lVer, &dummy, NULL );
    }
   
    CloseHandle( hFile );
#endif

#if 1
    Com_sprintf( path, sizeof( path ), "%s/%s.info", basePath, crashDumpFilename.c_str() );
    hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
   
    {
        /*
            The .info contains one line of bug title and the rest is all bug description.
        */
       
        const char* msg;
        switch( params->excCode )
        {
       
            case 1: //SEH_CAPTURE_EXC:
                msg = ( const char* )params->pExcPtrs->ExceptionRecord->ExceptionInformation[0];
                break;
                //*/
               
            default:
                msg = "Crash Dump\nWarzone crashed, see the attached dump for more information.";
                break;
        }
       
        WriteFile( hFile, msg, strlen( msg ), &dummy, NULL );
    }
   
    CloseHandle( hFile );
#endif
   
#if 0
    Com_sprintf( path, sizeof( path ), "%s/%s.con", basePath, crashDumpFilename.c_str(), nDump );
    hFile = CreateFile( path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
   
    {
        const char* conDump = Con_GetText( 0 );
        WriteFile( hFile, conDump, strlen( conDump ), &dummy, NULL );
    }
   
    CloseHandle( hFile );
#endif
   
    return 0;
}
 
/*
==============
DialogProc
==============
*/
static INT_PTR CALLBACK DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDOK:
                    PostQuitMessage( 0 );
                    break;
            }
            return TRUE;
           
        default:
            return FALSE;
    }
}
 
/*
==============
DoReportCrashGUI
==============
*/
static DWORD WINAPI DoReportCrashGUI( LPVOID pParams )
{
    dumpParams_t* params = ( dumpParams_t* )pParams;
   
    HWND dlg;
    HANDLE h;
    DWORD tid;
    MSG msg;
    BOOL mRet, ended;
   
    if( IsDebuggerPresent() )
    {
        if( MessageBox( NULL, "An exception has occurred: press Yes to debug, No to create a crash dump.",
                        "Warzone Error", MB_ICONERROR | MB_YESNO | MB_DEFBUTTON1 ) == IDYES )
            return EXCEPTION_CONTINUE_SEARCH;
    }
   
    h = CreateThread( NULL, 0, DoGenerateCrashDump, pParams, CREATE_SUSPENDED, &tid );
    params->ignoreThreadIds[params->numIgnoreThreads++] = tid;
   
    dlg = CreateDialog( g_wv.hInstance, MAKEINTRESOURCE( IDD_CRASH_REPORT ), NULL, DialogProc );
    ShowWindow( dlg, SW_SHOWNORMAL );
   
    ended = FALSE;
    ResumeThread( h );
   
    while( ( mRet = GetMessage( &msg, 0, 0, 0 ) ) != 0 )
    {
        if( mRet == -1 )
        {
            break;
        }
       
        TranslateMessage( &msg );
        DispatchMessage( &msg );
       
        if( !ended && WaitForSingleObject( h, 0 ) == WAIT_OBJECT_0 )
        {
            HWND btn = GetDlgItem( dlg, IDOK );
           
            ended = TRUE;
           
            if( btn )
            {
                EnableWindow( btn, TRUE );
                UpdateWindow( btn );
                InvalidateRect( btn, NULL, TRUE );
            }
            else
            {
                break;
            }
        }
    }
   
    if( !ended )
    {
        WaitForSingleObject( h, INFINITE );
    }
   
    {
        DWORD hRet;
        GetExitCodeThread( h, &hRet );
       
        if( hRet == 0 )
        {
            MessageBox( NULL, "There was an error. Please submit a bug report.", "Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1 );
        }
        else
        {
            char msg[1024];
           
            _snprintf( msg, sizeof( msg ),
                       "There was an error. Please submit a bug report.\n\n"
                       "No error info was saved (code:0x%X).", hRet );
                       
            MessageBox( NULL, msg, "Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1 );
        }
    }
   
    DestroyWindow( dlg );
   
    switch( params->excCode )
    {
   
        case 1: //SEH_CAPTURE_EXC:
            return ( DWORD )EXCEPTION_CONTINUE_EXECUTION;
           
           
        default:
            return ( DWORD )EXCEPTION_EXECUTE_HANDLER;
    }
}
 
/*
==============
ReportCrash
==============
*/
static int ReportCrashDump( DWORD excCode, PEXCEPTION_POINTERS pExcPtrs )
{
    /*
        Launch off another thread to get the crash dump.
   
        Note that this second thread will pause this thread so *be certain* to
        keep all GUI stuff on it and *not* on this thread.
    */
   
    dumpParams_t params;
   
    DWORD tid, ret;
    HANDLE h;
   
    Com_Memset( &params, 0, sizeof( params ) );
    params.excThreadId = GetCurrentThreadId();
    params.excCode = excCode;
    params.pExcPtrs = pExcPtrs;
   
    h = CreateThread( NULL, 0, DoReportCrashGUI, &params, CREATE_SUSPENDED, &tid );
    params.ignoreThreadIds[params.numIgnoreThreads++] = tid;
   
    ResumeThread( h );
    WaitForSingleObject( h, INFINITE );
   
    GetExitCodeThread( h, &ret );
    return ( int )ret;
}

int ReportCrash(DWORD excCode, PEXCEPTION_POINTERS pExcPtrs)
{
	return ReportCrashDump(excCode, pExcPtrs);
}
#endif
