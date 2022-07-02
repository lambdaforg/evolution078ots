//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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

#ifndef __OTSERV_ADMIN_H__
#define __OTSERV_ADMIN_H__

// -> server
// command(1 byte) | size(2 bytes) | parameters(size bytes)
// commands:
//	login
//		password(string)
//  encryption
//		encryption type(1 byte)
//			RSA1024+XTEA
//				:128 bytes encrypted using 1024 bytes public key
//				16 bytes XTEA key
//  key-exchange
//		public_key_type(1 byte)
//			RSA1024+XTEA
//  command
//		command + paramters(string)
//	no_operation/ping
//		nothing
//
// <- server
// ret-code(1 byte)| size(2 bytes) | parameters(size bytes)
// ret-codes:
//	hello
//		server_version(4 bytes)
//		server_string(string)
//		security_policy(2 bytes flags)
//			required_login
//			required_encryption
//		accepted_encryptions(4 bytes flags)
//			RSA1024+XTEA
//	key-exchange-ok
//		public_key_type(1 byte)
//			RSA1024+XTEA
//				:128 bytes public key modulus
//	key-exchange-failed
//		reason(string)
//  login-ok
//		nothing
//  login-failed
//		reason(string)
//  command-ok
//		command result(string)
//  command-failed
//		reason(string)
//  encryption-ok
//		nothing
//  encryption-failed
//		reason(string)
//	no_operation-ok
//		nothing
//	message
//		message(string)
//  error
//		message(string)
//

#include "otsystem.h"
#include "player.h"
#include "logger.h"
#include <string>

class NetworkMessage;
class RSA;

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

class AdminProtocolConfig{
public:
	AdminProtocolConfig();
	~AdminProtocolConfig();
	
	bool loadXMLConfig(const std::string& directory);
	
	bool isEnabled();
	
	bool onlyLocalHost();
	bool addConnection();
	void removeConnection();
	
	bool requireLogin();
	bool requireEncryption();
	
	uint16_t getProtocolPolicy();
	uint32_t getProtocolOptions();
	
	bool allowIP(SOCKET s);
	
	bool passwordMatch(std::string& password);
	
	RSA* getRSAKey(uint8_t type);
	
protected:
	bool m_enabled;
	bool m_onlyLocalHost;
	long m_maxConnections;
	long m_currrentConnections;
	
	std::string m_password;
	
	bool m_requireLogin;
	bool m_requireEncryption;
	
	RSA* m_key_RSA1024XTEA;
};

class AdminConnection{
public:
	AdminConnection(SOCKET s);
	~AdminConnection();
	
	std::string getIPString();
	long getStartTime();
	
	long getLastCommandTime();
	void setLastCommandTime();
	
protected:
	std::string m_ip;
	long m_startTime;
	long m_lastCommand;
};

class AdminProtocol{
public:
	AdminProtocol(SOCKET s);
	~AdminProtocol();
	
	void receiveLoop();
	
protected:
	
	long parsePacket(NetworkMessage &msg, NetworkMessage &outputBuffer);

	bool adminCommandCloseServer();
	bool adminCommandPayHouses();
	
	AdminConnection* m_connection;
	
	long m_loginTries;
	
	SOCKET m_socket;
	long m_state;
	
};

#endif
