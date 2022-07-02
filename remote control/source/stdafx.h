// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers


// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <string>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
// Local Header Files
enum eventFunctions {
    EVENT_ONLINELIST = 1,
    EVENT_CHANNELLOG = 2,
    EVENT_CONNECT    = 3,
    
};
enum redirectFunctions {
    REDIR_SERVERSTATE = 1,
    REDIR_WORLDSTATE  = 2,
    REDIR_ONLINELIST  = 3,
    REDIR_CHECKSTATUS = 4,
    
};

void addConsoleMessage(std::string);
void addLB (HWND, const char *);
void removeLB (HWND, char *);
void resetLB (HWND);
void getCountLB (HWND);
void setColumnLB (HWND idnr, int ltext);
char * getLB (HWND idnr);
char *BCX_TmpStr (size_t Bites);
void addCB (HWND, char *);
void removeCB (HWND, char *);
bool ReadMemory(HANDLE hProcess, LPCVOID Address, unsigned long Size, void *Buffer);
bool WriteMemory(HANDLE hProcess, LPVOID Address, unsigned long Size, void *Buffer);
void setState (HWND, int state);
LRESULT getState (HWND);
// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
