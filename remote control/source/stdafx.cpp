// stdafx.cpp : source file that includes just the standard includes
//	MyInputBox.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file
void addLB (HWND idnr, const char *ltext)
{
  SendMessage(idnr,(UINT)LB_ADDSTRING,(WPARAM)0,(LPARAM)ltext);
}

void removeLB (HWND idnr, char *ltext)
{
  SendMessage(idnr,(UINT)LB_DELETESTRING,(WPARAM)0,(LPARAM)ltext);
}

void resetLB (HWND idnr)
{
    SendMessage(idnr,(UINT)LB_RESETCONTENT ,(WPARAM)0,(LPARAM)0);     
}

void getCountLB (HWND idnr)
{
  SendMessage(idnr,(UINT)LB_GETCOUNT,(WPARAM)0,(LPARAM)0);
}

void setColumnLB (HWND idnr, int ltext)
{
    SendMessage(idnr,(UINT)LB_SETCOLUMNWIDTH ,(WPARAM)ltext,(LPARAM)0);     
}

char *BCX_TmpStr (size_t Bites)
{
  static int   StrCnt;
  static char *StrFunc[2048];
  StrCnt=(StrCnt + 1) & 2047;
  if(StrFunc[StrCnt]) free (StrFunc[StrCnt]);
  return StrFunc[StrCnt]=(char*)calloc(Bites+128,sizeof(char));
}

char * getLB (HWND idnr)
{
  static int      index;
  memset(&index,0,sizeof(index));
  static char buf[2048];
  memset(&buf,0,sizeof(buf));
  char *BCX_RetStr={0};
  index=SendMessage(idnr,(UINT)LB_GETCURSEL,(WPARAM)0,(LPARAM)0);
  SendMessage(idnr,(UINT)LB_GETTEXT,(WPARAM)index,(LPARAM)buf);
  BCX_RetStr=BCX_TmpStr(strlen(buf));
  strcpy(BCX_RetStr,buf);
  return BCX_RetStr;
}

void addCB (HWND idnr, char *ltext)
{
  SendMessage(idnr,(UINT)CB_ADDSTRING,(WPARAM)0,(LPARAM)ltext);
}

void removeCB (HWND idnr, char *ltext)
{
  SendMessage(idnr,(UINT)CB_DELETESTRING,(WPARAM)0,(LPARAM)ltext);
}

bool ReadMemory(HANDLE hProcess, LPCVOID Address, unsigned long Size, void *Buffer)
{
	return (bool)ReadProcessMemory(hProcess, Address, Buffer, Size, NULL);
}

bool WriteMemory(HANDLE hProcess, LPVOID Address, unsigned long Size, void *Buffer)
{
	return (bool)WriteProcessMemory(hProcess, Address, Buffer, Size, NULL);
}

void setState(HWND idnr, int state)
{
  SendMessage(idnr,(UINT)BM_SETCHECK,(WPARAM)state,(LPARAM)0);
  SetFocus(NULL);
}

LRESULT getState(HWND idnr)
{
  SendMessage(idnr,(UINT)BM_GETCHECK,(WPARAM)0,(LPARAM)0);
}


