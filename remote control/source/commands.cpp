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
#include <string>

#include "commands.h"
#include "networkmessage.h"
#include "rsa.h"
#include "stdafx.h"
#include "gui.h"
#include "resource.h"
#include "definitions.h"

extern long next_command_delay;
extern SOCKET g_socket;
extern bool g_connected;
extern HWND hStatusWindow;

bool sendCommand(char commandByte, char* command);
bool sendMsg(NetworkMessage& msg, uint32_t* key = NULL);

std::string serverHost;
uint16_t serverPort;

//server localhost 7171
int setServer(char* params)
{
	char host[256];
	int port;
	if(!params){
		addConsoleMessage("You need to add parameters.");
		return -1;
	}
        
	if(strlen(params) > 255){
		addConsoleMessage("You've entered a too long parameter.");
		return -1;
	}
	if(sscanf(params, "%s %d", &host, &port) != 2){
		addConsoleMessage("You've entered an invalid host or port.");
		return -1;
	}
	else{
		serverHost = host;
		serverPort = port;
		return 1;
	}
}

//connect test
int cmdConnect(char* params)
{
	std::stringstream ss;
	ss << "Connecting to host: "<< serverHost <<", port: "<< serverPort <<".";
	addConsoleMessage(ss.str());

	if(g_connected == true){
		addConsoleMessage("You are already connected.");
		return -1;
	}

	if(!params){
		addConsoleMessage("You need to enter a password.");
		return -1;
	}

	char password[128];
	if(strlen(params) > 127){
		addConsoleMessage("You've entered a too long password.");
		return -1;
	}
	if(sscanf(params, "%s", &password) != 1){
		addConsoleMessage("You've entered an invalid password.");
		return -1;
	}

	g_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	uint32_t remote_ip = inet_addr(serverHost.c_str());
	if(remote_ip == INADDR_NONE){
		struct hostent* hp = gethostbyname(serverHost.c_str());
		if(hp != 0){
			remote_ip = *(long*)hp->h_addr;
		}
		else{
			closesocket(g_socket);
			ss.str("");
			ss << "Error! Could not resolve host: " << serverHost << " - " << WSAGetLastError() << ".";
			addConsoleMessage(ss.str());
			return -1;
		}
	}

	sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = remote_ip;
	serveraddr.sin_port = htons(serverPort);
	
	if(connect(g_socket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR){
		closesocket(g_socket);
		ss.str("");
		ss << "Error! Could not connect to host: " << serverHost << " - " << WSAGetLastError() << ".";
		addConsoleMessage(ss.str());
		return -1;
	}

	NetworkMessage msg;
	msg.AddByte(0xFE);
	if(!msg.WriteToSocket(g_socket)){
		closesocket(g_socket);
		addConsoleMessage("Error! Could not send first byte to server.");
		return -1;
	}
	msg.Reset();
	//read server hello

	if(!msg.ReadFromSocket(g_socket)){
		closesocket(g_socket);
		addConsoleMessage("Error! No response from server.");
		return -1;
	}

	char byte = msg.GetByte();
	if(byte != AP_MSG_HELLO){
		closesocket(g_socket);
		addConsoleMessage("Error! Invalid response from server.");
		return -1;
	}
	msg.GetU32();
	std::string strversion = msg.GetString();
	uint16_t security = msg.GetU16();
	uint32_t options = msg.GetU32();
	if(security & REQUIRE_ENCRYPTION){
		strversion = strversion + " encryption";
		if(options & ENCRYPTION_RSA1024XTEA){
			strversion = strversion + "(RSA1024XTEA)";
		}
		else{
			strversion = strversion + "(Not supported)";
		}
	}
	if(security & REQUIRE_LOGIN){
		strversion = strversion + " login";
	}

	//set encryption
	if(security & REQUIRE_ENCRYPTION){
		if(options & ENCRYPTION_RSA1024XTEA){
			//get public key
			msg.Reset();
			msg.AddByte(AP_MSG_KEY_EXCHANGE);
			msg.AddByte(ENCRYPTION_RSA1024XTEA);

			if(!sendMsg(msg)){
				closesocket(g_socket);
				addConsoleMessage("Error! Failed to receive key.");
				return -1;
			}

			char ret_code = msg.GetByte();
			if(ret_code == AP_MSG_KEY_EXCHANGE_OK){
				addConsoleMessage("Key has been exchanged successfully.");
			}
			else if(ret_code == AP_MSG_KEY_EXCHANGE_FAILED){
				std::string error_desc = msg.GetString();
				closesocket(g_socket);
				addConsoleMessage("Error! Could not receive public key: " + error_desc + ".");
				return -1;
			}
			else{
				closesocket(g_socket);
				addConsoleMessage("Error! Unknown response when requesting key.");
				return -1;
			}

			unsigned char key_type = msg.GetByte();
			if(key_type != ENCRYPTION_RSA1024XTEA){
				closesocket(g_socket);
				addConsoleMessage("Error! Invalid key has been returned.");
				return -1;
			}
			
			//the public key is 128 bytes
			uint32_t rsa_mod[32];
			for(unsigned int i = 0; i < 32; ++i){
				rsa_mod[i] = msg.GetU32();
			}
			RSA::getInstance()->setPublicKey((char*)rsa_mod, "65537");

			
			uint32_t random_key[32];
			for(unsigned int i = 0; i < 32; ++i){
				random_key[i] = 0x1111112;
			}
			
			msg.setRSAInstance(RSA::getInstance());
			msg.Reset();
			msg.AddByte(AP_MSG_ENCRYPTION);
			msg.AddByte(ENCRYPTION_RSA1024XTEA);
			//build the 128 bytes block
			msg.AddByte(0);
			for(unsigned int i = 0; i < 31; ++i){
				msg.AddU32(random_key[i]);
			}
			msg.AddByte(0);
			msg.AddByte(0);
			msg.AddByte(0);
			//
			msg.RSA_encrypt();

			if(!sendMsg(msg, random_key)){
				closesocket(g_socket);
				addConsoleMessage("Error! Could not send private key.");
				return -1;
			}

			ret_code = msg.GetByte();
			if(ret_code == AP_MSG_ENCRYPTION_OK){
				addConsoleMessage("Successfully encrypted server.");
			}
			else if(ret_code == AP_MSG_ENCRYPTION_FAILED){
				std::string error_desc = msg.GetString();
				closesocket(g_socket);
				addConsoleMessage("Error! Could not set private key: " + error_desc + ".");
				return -1;
			}
			else{
				closesocket(g_socket);
				addConsoleMessage("Error! Unknown response while setting private key.");
				return -1;
			}
			
		}
		else{
			closesocket(g_socket);
			addConsoleMessage("Error! Could not initiate encryption.");
			return -1;
		}
	}

	
	//login
	if(security & REQUIRE_LOGIN){
		msg.Reset();
		msg.AddByte(AP_MSG_LOGIN);
		msg.AddString(std::string(password));

		if(!sendMsg(msg)){
			closesocket(g_socket);
			addConsoleMessage("Error! Could not send login to server.");
			return -1;
		}

		char ret_code = msg.GetByte();
		if(ret_code == AP_MSG_LOGIN_OK){
			addConsoleMessage("Successfully logged in");
		}
		else if(ret_code == AP_MSG_LOGIN_FAILED){
			std::string error_desc = msg.GetString();
			closesocket(g_socket);
			addConsoleMessage("Error! Failed to login: " + error_desc + ".");
			return -1;
		}
		else{
			closesocket(g_socket);
			addConsoleMessage("Error! Unknown response when requesting login.");
			return -1;
		}
	}

	g_connected = true;

	return 1;
}

//disconnect
int cmdDisconnect(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return 1;
	}

	closesocket(g_socket);
	g_socket = SOCKET_ERROR;

	addConsoleMessage("You are now disconnected from the server.");
	g_connected = false;
	return 1;
}

//sleep 10000
int sleep(char* params)
{
	int delay;
	if(sscanf(params, "%d", &delay) != 1){
		addConsoleMessage("You've entered an invalid delay.");
		return -1;
	}
	else{
		next_command_delay = delay;
		addConsoleMessage("Successfully sleeping server.");
		return 1;
	}
}

//broadcast hello all
int commandBroadcast(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!params){
		addConsoleMessage("You need to enter a message.");
		return -1;
	}

	char message[128];
	strcpy(message, params);

	if(!sendCommand(CMD_BROADCAST, message)){
		addConsoleMessage("Error! Could not broadcast message.");
		return -1;
	}

	addConsoleMessage("You've successfully broadcasted the message.");

	return 1;
}

//closeserver
int commandCloseServer(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_CLOSE_SERVER, NULL)){
		addConsoleMessage("Error! Could not close server.");
		return -1;
	}

	addConsoleMessage("Successfully closed server.");

	return 1;
}

//shutdown
int commandShutdown(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_SHUTDOWN_SERVER, NULL)){
		addConsoleMessage("Error! Could not shutdown server.");
		return -1;
	}

	addConsoleMessage("Succesfully shutdown server.");

	return 1;
}

//internal use
int ping(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	NetworkMessage msg;
	msg.AddByte(AP_MSG_PING);

	if(!sendMsg(msg)){
		addConsoleMessage("Error! Could not send ping.");
		return -1;
	}

	char ret_code = msg.GetByte();
	if(ret_code != AP_MSG_PING_OK){
		addConsoleMessage("Error! Failed to ping server.");
		return -1;
	}
	
	return 1;
}

//reload scripts
int commandKickPlayer(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_KICK, params)){
		addConsoleMessage("Failed to kick player.");
		return -1;
	}

		addConsoleMessage("Successfully kicked player.");

	return 1;
}

//add premium days
int commandPremiumPlayer(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_PREMIUM, params)){
		addConsoleMessage("Failed to add premium to player.");
		return -1;
	}

	addConsoleMessage("Successfully given premium days to player.");

	return 1;
}

//give addons
int commandAddonPlayer(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_ADDONS, params)){
		addConsoleMessage("Failed to give addons to player.");
		return -1;
	}

	addConsoleMessage("Successfully given addons to player.");

	return 1;
}

//ban player
int commandBanPlayer(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_BAN_PLAYER, params)){
		addConsoleMessage("Failed to ban player.");
		return -1;
	}

	addConsoleMessage("Successfully banned player.");

	return 1;
}

//ban account
int commandBanAccount(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_BAN_ACCOUNT, params)){
		addConsoleMessage("Failed ban account.");
		return -1;
	}

	addConsoleMessage("Successfully banned account.");

	return 1;
}

//ban IP
int commandBanIP(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_BAN_IP, params)){
		addConsoleMessage("Failed to ban IP.");
		return -1;
	}

	addConsoleMessage("Successfully banned IP.");

	return 1;
}

//reload commands
int commandReloadCommands()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_RELOAD_COMMANDS, NULL)){
		addConsoleMessage("Error! Could not reload commands.");
		return -1;
	}
	
	addConsoleMessage("Successfully reloaded commands.");	

	return 1;
}

//reload config
int commandReloadConfig()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_RELOAD_CONFIG, NULL)){
		addConsoleMessage("Error! Could not reload config.lua.");
		return -1;
	}
	
	addConsoleMessage("Successfully reloaded config.lua.");	

	return 1;
}

//reload monsters
int commandReloadMonsters()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_RELOAD_MONSTERS, NULL)){
		addConsoleMessage("Error! Could not reload monsters.");
		return -1;
	}
	
	addConsoleMessage("Successfully reloaded monsters.");	

	return 1;
}

//reload movements
int commandReloadMovements()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_RELOAD_MOVEMENTS, NULL)){
		addConsoleMessage("Error! Could not reload movements.");
		return -1;
	}
	
	addConsoleMessage("Successfully reloaded movements.");	

	return 1;
}

//reload raids
int commandReloadRaids()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_RELOAD_RAIDS, NULL)){
		addConsoleMessage("Error! Could not reload raids.");
		return -1;
	}
	
	addConsoleMessage("Successfully reloaded raids.");	

	return 1;
}

//reload spells
int commandReloadSpells()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_RELOAD_SPELLS, NULL)){
		addConsoleMessage("Error! Could not reload spells.");
		return -1;
	}
	
	addConsoleMessage("Successfully reloaded spells.");	

	return 1;
}

//reload talkactions
int commandReloadTalkactions()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_RELOAD_TALKACTIONS, NULL)){
		addConsoleMessage("Error! Could not reload talkactions.");
		return -1;
	}
	
	addConsoleMessage("Successfully reloaded talkactions.");	

	return 1;
}

//reload weapons
int commandReloadWeapons()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_RELOAD_WEAPONS, NULL)){
		addConsoleMessage("Error! Could not reload weapons.");
		return -1;
	}
	
	addConsoleMessage("Successfully reloaded weapons.");	

	return 1;
}

//reload everything
int commandReloadEverything()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_RELOAD_EVERYTHING, NULL)){
		addConsoleMessage("Error! Could not reload all.");
		return -1;
	}
	
	addConsoleMessage("Successfully reloaded all.");	

	return 1;
}

//set non pvp
int commandSetWorldType(char worldType)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(worldType, NULL)){
		addConsoleMessage("Error! Could not set World-Type.");
		return -1;
	}
	
	addConsoleMessage("Successfully set World-Type.");	

	return 1;
}

//execute raid
int commandExecuteRaid(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_EXECUTE_RAID, params)){
		addConsoleMessage("Error! Could not execute raid.");
		return -1;
	}

	addConsoleMessage("Successfully executed raid.");

	return 1;
}

//execute raid
int commandCleanMap()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_CLEAN_MAP, NULL)){
		addConsoleMessage("Error! Could not clean map.");
		return -1;
	}

	addConsoleMessage("Successfully cleaned map.");

	return 1;
}

//execute raid
int commandSetPlayerLimit(char* params)
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}
	
	std::string newtext = params;
	for(int i = 0; i < newtext.length(); i++){
		if(!isdigit(newtext[i]) && !isalpha(newtext[i]) && newtext[i] != ' ') {
			addConsoleMessage("You've enter an invalid amount.");
			return -1;
		}
	}

	if(!sendCommand(CMD_PLAYER_LIMIT, params)){
		addConsoleMessage("Error! Could not set new player limit.");
		return -1;
	}

	addConsoleMessage("Successfully set new player limit.");

	return 1;
}

//list players
int commandListPlayers()
{
	/*if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}
	
	NetworkMessage msg;
	msg.AddByte(AP_MSG_PLAYERLIST);

	if(!sendMsg(msg)){
		addConsoleMessage("Failed to receive player list.");
		return -1;
	}

	char ret_code = msg.GetByte();
	if(ret_code == AP_MSG_LOGIN_OK){
		addConsoleMessage("Succeeded to receive player list.");
		resetLB(hStatusWindow);
		uint8_t playerCount = msg.GetByte();
		
		if(playerCount > 0){
			for(int i=1; i < playerCount+1; i++){
				std::string onlinePlayer = msg.GetString();
				addConsoleMessage(onlinePlayer);
				addLB(hStatusWindow, onlinePlayer.c_str());
			}
		}
		else{
			addConsoleMessage("There are no players online.");
		}
	}
	else if(ret_code == AP_MSG_LOGIN_FAILED){
		std::string error_desc = msg.GetString();
		addConsoleMessage("Failed to receive player list: " + error_desc);
		return -1;
	}
	else{
		addConsoleMessage("Failed to receive player list: Unknown Byte received");
		return -1;
	}*/

	return 1;
}

//server save
int commandServerSave()
{
	if(g_connected != true){
		addConsoleMessage("You are not connected to any server.");
		return -1;
	}

	if(!sendCommand(CMD_SERVER_SAVE, NULL)){
		addConsoleMessage("Error! Could not save server.");
		return -1;
	}
	
	addConsoleMessage("Succeeded to save server.");	

	return 1;
}

//dummy function
int last(char* params)
{
	return 1;
}

//help functions

bool sendCommand(char commandByte, char* command)
{
	NetworkMessage msg;
	msg.AddByte(AP_MSG_COMMAND);
	msg.AddByte(commandByte);
	if(command){
		msg.AddString(command);
	}

	if(!sendMsg(msg)){
		addConsoleMessage("Error! Could not send message to server.");
		return false;
	}

	char ret_code = msg.GetByte();
	if(ret_code == AP_MSG_COMMAND_OK){
		return true;
	}
	else if(ret_code == AP_MSG_COMMAND_FAILED){
		std::string error_desc = msg.GetString();
		addConsoleMessage("Error! Could not execute command:" + error_desc + ".");
		return false;
	}
	else{
		addConsoleMessage("Error! Unknown byte received.");
		return false;
	}
}

bool sendMsg(NetworkMessage& msg, uint32_t* key /*= NULL*/)
{
#if defined WIN32 || defined __WINDOWS__
	// Set the socket I/O mode; iMode = 0 for blocking; iMode != 0 for non-blocking
	unsigned long mode = 0;
	ioctlsocket(g_socket, FIONBIO, &mode);
#else
	int flags = fcntl(g_socket, F_GETFL);
	fcntl(g_socket, F_SETFL, flags & (~O_NONBLOCK));
#endif
	bool ret = true;

	if(!msg.WriteToSocket(g_socket)){
		addConsoleMessage("Error! Could not send message to server.");
		ret = false;
	}

	msg.Reset();

	if(ret){
		if(key){
			msg.setEncryptionState(true);
			msg.setEncryptionKey(key);
		}
		if(!msg.ReadFromSocket(g_socket)){
			addConsoleMessage("Error! Could not receive message from server.");
			ret = false;
		}
		else{
			char ret_code = msg.InspectByte();
			if(ret_code == AP_MSG_ERROR){
				msg.GetByte();
				std::string error_desc = msg.GetString();
				addConsoleMessage("Error! Unknown message received: " + error_desc + ".");
				ret = false;
			}
		}
	}

#if defined WIN32 || defined __WINDOWS__
	mode = 1;
	ioctlsocket(g_socket, FIONBIO, &mode);
#else
	flags = fcntl(g_socket, F_GETFL);
	fcntl(g_socket, F_SETFL, flags | O_NONBLOCK);
#endif
	return ret;
}


//commands list

defcommands commands[] = {
	{"server", &setServer},
	{"connect", &cmdConnect},
	{"sleep", &sleep},
	{"broadcast", &commandBroadcast},
	{"closeserver", &commandCloseServer},
	{"shutdown", &commandShutdown},
	{"disconnect", &cmdDisconnect},
	//{"reload", &commandReloadScripts},
	{"LAST", &last},
	//internal commands
	{"ping", &ping},
	{"", NULL},
};

defcommands* getCommadsList()
{
	return commands;
}
