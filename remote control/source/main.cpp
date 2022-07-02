//////////////////////////////////////////////////////////////////////
// OTAdmin - OpenTibia
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include <stdio.h>
#include <list>
#include <sstream>

#include "definitions.h"

#include "commands.h"
#include "networkmessage.h"

struct CommandLine
{
	CommandFunc function;
	char* params;
};

CommandLine* parseLine(char* line);
CommandFunc getCommand(char* name, bool internal = false);

typedef std::list<CommandLine*> COMMANDS_QUEUE;


COMMANDS_QUEUE commands_queue;
long next_command_delay = 0;
SOCKET g_socket = SOCKET_ERROR;
bool g_connected = false;

CommandFunc disconnect_function;
CommandFunc ping_function;

//OT PART


// MyInputBox.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "resource.h"

#define MAX_LOADSTRING 100
#define Show(Window)  RedrawWindow(Window,0,0,0);ShowWindow(Window,SW_SHOW);

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];								// The title bar text

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

HWND ParentHwnd;
bool textBreaking;
bool onLine = false;
#include "gui.h"

void ErrorMessage(const char* message);
void onStartServer(void *null);
LPSTR szMessage = "Waiting";

HWND  hIpEditBox = NULL;
HWND  hPortEditBox = NULL;
HWND  hStartServerButton = NULL;
HWND  hDisConnectButton = NULL;
HWND  hStatusWindow = NULL;
HWND  hMainConWindow = NULL;
HWND  hMainStatusWindow = NULL;
HWND  hMainCommandWindow = NULL;
HWND  hPlayerBox = NULL;
HWND  hComboBox = NULL;
HWND  hMainConsoleBox = NULL;
HWND  hMainConsoleWindow = NULL;
HWND  hStatusBox = NULL;
HWND  hErrorWindow = NULL;
HWND  hErrorWindowFrame = NULL;
HWND  hAutoRefreshButton = NULL;
HWND  hClearLogsButton = NULL;
HWND  hGetOnlineButton = NULL;

LPTSTR Text;
LPTSTR Text2;
    //char buffer[BUFSIZE]=""; 
    //char * lpPart[BUFSIZE]={NULL};

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_MYINPUTBOX, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_MYINPUTBOX);
    HWND hWndFocused;
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{  
        if(msg.message == WM_KEYDOWN)
        {
            switch(msg.wParam)
            {   
                case VK_TAB:
                {   
                    hWndFocused = GetFocus();
                    if (hWndFocused == hIpEditBox) SetFocus(hPortEditBox);
                    if (hWndFocused == hPortEditBox) SetFocus(hStartServerButton);
                    if (hWndFocused == hStartServerButton) SetFocus(hIpEditBox);
                    //if (hWndFocused == hDisConnectButton) SetFocus(hIpEditBox);                    
                }
                break;
            }
        }                         
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_MYINPUTBOX);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+7);
	wcex.lpszMenuName	= (LPCSTR)IDC_MYINPUTBOX;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
   RECT r;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX ,
   CW_USEDEFAULT, 0, 600, 570, NULL, NULL, hInstance, NULL);                              

   hMainConWindow = CreateWindowEx(WS_EX_CLIENTEDGE, "Static", "", WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
   0, 120, 220, 100, hWnd, 0, hInstance, 0); //Frame around the connection stuff
   
   hMainStatusWindow = CreateWindowEx(WS_EX_CLIENTEDGE, "Static", "", WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
   415, 120, 171, 380, hWnd, 0, hInstance, 0); //Frame around the player list     
   
   hStatusWindow = CreateWindowEx(WS_EX_CLIENTEDGE, "ListBox", "", WS_BORDER | WS_CHILD | WS_VISIBLE | WS_VSCROLL | SBS_VERT | WS_HSCROLL | SBS_HORZ | LBS_DISABLENOSCROLL | LBS_SORT,
   430, 153, 140, 317, hWnd, 0, hInstance, 0); //Player Online List

   hMainCommandWindow = CreateWindowEx(WS_EX_CLIENTEDGE, "Static", "", WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
   220, 120, 195, 100, hWnd, 0, hInstance, 0); //Player Command window ban,kick etc
   
   hMainConsoleBox = CreateWindowEx(WS_EX_CLIENTEDGE, "Static", "", WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
   0, 220, 415, 280, hWnd, 0, hInstance, 0);   //Console Frame
   
   hMainConsoleWindow = CreateWindowEx(WS_EX_CLIENTEDGE, "ListBox", "", WS_BORDER | WS_CHILD | WS_VISIBLE | WS_VSCROLL | SBS_VERT | WS_HSCROLL | SBS_HORZ | LBS_DISABLENOSCROLL ,
   3, 242, 408, 245, hWnd, 0, hInstance, 0); // The console itself
                 
   hStatusBox   = CreateWindowEx( WS_EX_CLIENTEDGE, "Static", "", WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKFRAME, 
   154, 190, 60, 20, hWnd, 0, hInstance, 0); //Frame around the connection status     

   hPlayerBox   = CreateWindowEx( WS_EX_CLIENTEDGE, "edit", "", WS_BORDER | WS_CHILD | WS_VISIBLE, 
   230, 130, 130, 25, hWnd, 0, hInstance, 0); //The Name box for the commands  

   hErrorWindow = CreateWindowEx(WS_EX_CLIENTEDGE, "ListBox", "", WS_BORDER | WS_CHILD | WS_VISIBLE | WS_VSCROLL | SBS_VERT | WS_HSCROLL | SBS_HORZ | LBS_DISABLENOSCROLL ,
   406, 8, 180, 112, hWnd, 0, hInstance, 0); // The console itself

   hErrorWindowFrame= CreateWindowEx(WS_EX_CLIENTEDGE, "Static", "", WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
   400, 0, 196, 120, hWnd, 0, hInstance, 0); // The console itself
      
   hAutoRefreshButton = CreateWindowEx(WS_EX_CLIENTEDGE, "Button", "  Auto Refresh", WS_VISIBLE | WS_CHILD | BS_CHECKBOX ,
   430, 4752, 140, 18, hWnd, 0, hInstance, 0); // Auto Refresh Checkbox

   hClearLogsButton = CreateWindowEx(WS_EX_CLIENTEDGE, "Button", "Clear Screen", WS_VISIBLE | WS_CHILD,
   320, 473, 90, 23, hWnd, 0, hInstance, 0); //Clear logs Button

   addLB (hErrorWindow, "Status Messages:");

   if (!hWnd){
      return FALSE;
   }
	
   ParentHwnd = hWnd;
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HINSTANCE m_hInst,m_hInst2;        
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR szHello[MAX_LOADSTRING];
	//LoadString(hInst, IDS_HELLO, szHello, MAX_LOADSTRING);
    CInputBox ibox(hWnd);
   
    Text = NULL;

#if defined WIN32 || defined __WINDOWS__
	WSADATA wsd;
	if(WSAStartup(MAKEWORD(2,2), &wsd) != 0){
		return 1;
	}
#endif
		             
	switch (message) 
	{             
        case WM_INITDIALOG: 
        { 
        }           
        case WM_CREATE:
        {
            m_hInst = GetModuleHandle(NULL);                                                
                  
            hStartServerButton = CreateWindowEx(WS_EX_CLIENTEDGE, "Button", "Shut Control", WS_VISIBLE | WS_CHILD,
            45, 190, 85, 20, hWnd, 0, m_hInst, 0);

            break;
        }
		case WM_COMMAND:
        {                  
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
            switch (wmEvent)
            {
                case BN_CLICKED:
                {   
                    if ((HWND)lParam == hStartServerButton)  
                    {
                        if(onLine == false)
                        {
                            DestroyWindow(hWnd);
                        }  
                        else
                            MessageBox(0, "Server is already started.", "Error!", MB_ICONSTOP | MB_OK);
                        SetFocus(NULL);
                        break;                  
                    }                                                
                    if ((HWND)lParam == hClearLogsButton)  
                    {
                        resetLB(hMainConsoleWindow); 
                        SetFocus(NULL);
                        break;                  
                    }                                                                                                                                              
                    break;
                }
                case LBN_SELCHANGE:
                {         
                    static char    text[20];                  
                    std::strcpy(text,(char*)getLB(hStatusWindow));             
                    SetWindowText(hPlayerBox, text);            
                    break;
                }                                                      
                
                break;
            }      			
			// Parse the menu selections:
			switch (wmId)
			{               
				case IDM_ABOUT:
				   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				   break;			   
				case IDM_EXIT:
                {     
                   DestroyWindow(hWnd);			   
				   break;
                }
				case IDM_SET_SERVER:
					if(ibox.DoModal("Set Server", "Please enter the IP-Address and Server Port you want to get a connection with when you connect.\n Example: \"localhost 7171\"")){
						if(ibox.Text != NULL){
							if(setServer(ibox.Text) == 1){
								addConsoleMessage("Successfully set server.");
							}
						}
						else{
							MessageBox(hWnd, "Please enter a message.", "Action", 0);
						}
                    }
                    break;
				case IDM_CONNECT:
					if(ibox.DoModal("Connect Server", "Enter the security password to connect to the server.")){
						if(ibox.Text != NULL){
							if(cmdConnect(ibox.Text) == 1){
								addConsoleMessage("Connected to server.");
							}
						}
						else{
							MessageBox(hWnd, "Please enter a password.", "Action", 0);
						}
                    }
                    break;
				case IDM_DISCONNECT:
					cmdDisconnect(NULL);
                    break;
				case ID_BROADCAST_MESSAGE:
					if(ibox.DoModal("Broadcast", "Please enter the message you want to broadcast.")){
						if(ibox.Text != NULL){
							commandBroadcast(ibox.Text);
						}
						else{
							MessageBox(hWnd, "Please enter a message.", "Action", 0);
						}
                    }
                    break;
                    
				case ID_LIST_PLAYERS:
					commandListPlayers();
					break;
					
				case ID_KICK_PLAYER:
					if(ibox.DoModal("Kick Player", "Please enter the name of the player that want to kick.")){
						if(ibox.Text != NULL){    
							commandKickPlayer(ibox.Text);
						}
						else{
							MessageBox(hWnd, "Please enter a name.", "Action", 0);
						}
					}
					break;
					
				case ID_PREMIUM_ACCOUNT:
					if(ibox.DoModal("Premium Account", "Please enter the name of the player that want to give 7 days of premium.")){
						if(ibox.Text != NULL){    
							commandPremiumPlayer(ibox.Text);
						}
						else{
							MessageBox(hWnd, "Please enter a name.", "Action", 0);
						}
                    }
					break;
					
				case ID_OUTFIT_ADDON:
					if(ibox.DoModal("Outfit Addons", "Please enter the name of the player that want to give both addons.")){
						if(ibox.Text != NULL){    
							commandAddonPlayer(ibox.Text);
						}
						else{
                        	MessageBox(hWnd, "Please enter a name.", "Action", 0);
						}
					}
					break;
					
				case ID_BAN_PLAYER:
					if(ibox.DoModal("Ban Player", "Please enter the name of the player that want to ban for 7 days.")){
                    	if(ibox.Text != NULL){    
							commandBanPlayer(ibox.Text);
						} 
						else{
							MessageBox(hWnd, "Please enter a name.", "Action", 0);
						}
					}
					break;
					
				case ID_BAN_ACCOUNT:
					if(ibox.DoModal("Ban Account by Playername", "Please enter the name of the player of the account you want to ban for 7 days.")){
						if(ibox.Text != NULL){    
							commandBanAccount(ibox.Text);
						}
						else{
							MessageBox(hWnd, "Please enter a name.", "Action", 0);
						}
					}
					break;
					
				case ID_BAN_IP_ADDRESS:
					if(ibox.DoModal("Ban IP-Address by Playername", "Please enter the name of the player of the IP-Address you want to ban for 7 days.")){
                    	if(ibox.Text != NULL){    
							commandBanIP(ibox.Text);
						}
                    	else{
                        	MessageBox(hWnd, "Please enter a name.", "Action", 0);
						}
					}
					break;

				case ID_RELOAD_COMMANDS:
					commandReloadCommands();
					break;
					
				case ID_RELOAD_CONFIG:
					commandReloadConfig();
					addConsoleMessage("Reloaded config.");
					break;
					
				case ID_RELOAD_MONSTERS:
					commandReloadMonsters();
					break;
					
				case ID_RELOAD_MOVEMENTS:
					commandReloadMovements();
					break;
					
				case ID_RELOAD_RAIDS:
					commandReloadRaids();
					break;
					
				case ID_RELOAD_SPELLS:
					commandReloadSpells();
					break;
					
				case ID_RELOAD_TALKACTIONS:
					commandReloadTalkactions();
					break;
					
				case ID_RELOAD_WEAPONS:
					commandReloadWeapons();
					addConsoleMessage("Reloaded weapons.");
					break;
					
				case ID_RELOAD_EVERYTHING:
                    commandReloadEverything();
					break;
					
				case ID_WORLD_TYPE_PVP:
					commandSetWorldType(CMD_SET_PVP);
					break;
					
				case ID_WORLD_TYPE_NO_PVP:
					commandSetWorldType(CMD_SET_NON_PVP);
					break;
					
				case ID_WORLD_TYPE_PVP_ENFORCED:
					commandSetWorldType(CMD_SET_PVP_ENF);
					break;
					
				case ID_EXECUTE_RAID:
					if(ibox.DoModal("Execute Raid", "Please enter the name of the raid that you want to execute.")){
						if(ibox.Text != NULL){
							commandExecuteRaid(ibox.Text);
						} 
						else
							MessageBox(hWnd, "Please enter a name.", "Action", 0);
					}
					break;
					
				case ID_SERVER_SAVE:
					commandServerSave();
					break;
					
				case ID_CLEAN_MAP:
				{
					commandCleanMap();
					break;
				}

				case ID_PLAYER_LIMIT:
					if(ibox.DoModal("Change Player Limit", "Please enter the amount of the new player limit.")){
                    	if(ibox.Text != NULL){
							commandSetPlayerLimit(ibox.Text);
						}
                    	else{
                        	MessageBox(hWnd, "Please enter an amount.", "Action", 0);
						}
					}
					break;
					
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
            case WM_PAINT: 
            {
                hdc = BeginPaint(hWnd, &ps);
                HBITMAP foo = LoadBitmap (hInst, MAKEINTRESOURCE(IDB_BITMAP1));
                HDC memDC = CreateCompatibleDC (hdc);

                int save = SaveDC (memDC);

                SelectObject (memDC, foo);

                BitBlt (hdc, 0, 0, 400, 117, memDC, 0, 0, SRCCOPY);

                RestoreDC (memDC, save);
                EndPaint(hWnd, &ps);
            }
			break;
		case WM_DESTROY:
            PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
        }
   }
   
#if defined WIN32 || defined __WINDOWS__
	WSACleanup();
#endif
   
   return 0;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

void addConsoleMessage(std::string message) 
{
    char *FileExt = new char[message.size() + 1];   
    std::strcpy(FileExt, message.c_str());          
    addLB(hMainConsoleWindow, FileExt);
}

void ErrorMessage(const char* message)
{
    std::string lol = message;
    char* fileExt = new char(lol.size() +1);
    std::strcpy(fileExt,lol.c_str());
    addLB(hErrorWindow, fileExt);
}

CommandLine* parseLine(char* line)
{
	char* command = NULL;
	char* params = NULL;
	for(int i = 0; i < strlen(line); i++){
		if(line[i] == ' '){
			command = line;
			line[i] = 0;
			params = &line[i + 1];
			break;
		}
	}
	if(command == NULL){
		command = line;
	}
	
	CommandFunc f = getCommand(command);
	if(f){
		CommandLine* cLine = new CommandLine;
		cLine->function = f;
		char* tmp_params;
		if(params){
			int n = strlen(params);
			if(n > 0){
				tmp_params = new char[n + 1];
				strcpy(tmp_params, params);
			}
			else{
				tmp_params = NULL;
			}
		}
		else{
			tmp_params = NULL;
		}
		cLine->params = tmp_params;
		return cLine;
	}
	return NULL;
}



CommandFunc getCommand(char* name,  bool internal)
{
	defcommands* list = getCommadsList();
	for(int i = 0;  list[i].f != NULL; ++i){
		if(!internal){
			if(strcmp(list[i].name, "LAST") == 0){
				break;
			}
		}
		if(strcmp(list[i].name, name) == 0){
			return list[i].f;
		}
	}
	return NULL;
}

