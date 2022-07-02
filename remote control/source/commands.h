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

#ifndef __OTADMIN_COMMANDS_H__
#define __OTADMIN_COMMANDS_H__

typedef int (*CommandFunc)(char* params);

struct defcommands{
	char *name;
	CommandFunc f;
};

defcommands* getCommadsList();


enum{
	//
	AP_MSG_LOGIN = 1,
	AP_MSG_ENCRYPTION = 2,
	AP_MSG_KEY_EXCHANGE = 3,
	AP_MSG_COMMAND = 4,
	AP_MSG_PING = 5,
	AP_MSG_PLAYERLIST = 6,
	//
	AP_MSG_HELLO = 1,
	AP_MSG_KEY_EXCHANGE_OK = 2,
	AP_MSG_KEY_EXCHANGE_FAILED = 3,
	AP_MSG_LOGIN_OK = 4,
	AP_MSG_LOGIN_FAILED = 5,
	AP_MSG_COMMAND_OK = 6,
	AP_MSG_COMMAND_FAILED = 7,
	AP_MSG_ENCRYPTION_OK = 8,
	AP_MSG_ENCRYPTION_FAILED = 9,
	AP_MSG_PING_OK = 10,
	AP_MSG_MESSAGE = 11,
	AP_MSG_ERROR = 12,
};

enum{
	//server state
	CMD_OPEN_SERVER = 1,
	CMD_CLOSE_SERVER = 2,
	CMD_SHUTDOWN_SERVER = 3,
	//players
	CMD_KICK = 4,
	CMD_PREMIUM = 5,
	CMD_ADDONS = 6,
	//bans
	CMD_BAN_PLAYER = 7,
	CMD_BAN_ACCOUNT = 8,
	CMD_BAN_IP = 9,
	//reload
	CMD_RELOAD_COMMANDS = 10,
	CMD_RELOAD_CONFIG = 11,
	CMD_RELOAD_MONSTERS = 12,
	CMD_RELOAD_MOVEMENTS = 13,
	CMD_RELOAD_RAIDS = 14,
	CMD_RELOAD_SPELLS = 15,
	CMD_RELOAD_TALKACTIONS = 16,
	CMD_RELOAD_WEAPONS = 17,
	CMD_RELOAD_EVERYTHING = 18,
	//pvp state
	CMD_SET_NON_PVP = 19,
	CMD_SET_PVP = 20,
	CMD_SET_PVP_ENF = 21,
	//others
	CMD_BROADCAST = 22,
	CMD_PAY_HOUSES = 23,
	CMD_SERVER_SAVE = 24,
	CMD_EXECUTE_RAID = 25,
	CMD_CLEAN_MAP = 26,
	CMD_PLAYER_LIMIT = 27,
};


enum{
	REQUIRE_LOGIN = 1,
	REQUIRE_ENCRYPTION = 2,
};

enum{
	ENCRYPTION_RSA1024XTEA = 1,
};

int setServer(char* params);
int cmdConnect(char* params);
int cmdDisconnect(char* params);

int commandKickPlayer(char* params);
int commandPremiumPlayer(char* params);
int commandAddonPlayer(char* params);

int commandBanPlayer(char* params);
int commandBanAccount(char* params);
int commandBanIP(char* params);

int commandReloadCommands();
int commandReloadConfig();
int commandReloadMonsters();
int commandReloadMovements();
int commandReloadRaids();
int commandReloadSpells();
int commandReloadTalkactions();
int commandReloadWeapons();
int commandReloadEverything();

int commandBroadcast(char* params);
int commandExecuteRaid(char* params);
int commandCleanMap();
int commandSetPlayerLimit(char* params);
int commandListPlayers();
int commandServerSave();
int commandSetWorldType(char worldType);

#endif

