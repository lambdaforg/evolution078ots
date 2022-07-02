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
#include "otpch.h"


#include "admin.h"
#include "game.h"
#include "networkmessage.h"
#include "configmanager.h"
#include "house.h"
#include "ban.h"
#include "tools.h"
#include "rsa.h"

#include "actions.h"
#include "commands.h"
#include "movement.h"
#include "raids.h"
#include "spells.h"
#include "status.h"
#include "talkaction.h"
#include "weapons.h"

#include "logger.h"

static void addLogLine(AdminConnection* conn, eLogType type, int level, std::string message);

extern ConfigManager g_config;
extern Actions* g_actions;
extern Ban g_bans;
extern Game g_game;
extern Commands commands;
extern Monsters g_monsters;
extern MoveEvents* g_moveEvents;
extern Spells* g_spells;
extern TalkActions* g_talkactions;
extern Vocations g_vocations;
extern Weapons* g_weapons;

AdminProtocolConfig* adminConfig = NULL;

AdminConnection::AdminConnection(SOCKET s)
{
	if(!adminConfig){
		adminConfig = new AdminProtocolConfig();
	}
	uint32_t ip = getIPSocket(s);
	char string_ip[32];
	formatIP(ip, string_ip);
	m_ip = string_ip;
	m_startTime = time(NULL);
	m_lastCommand = 0;
}

AdminConnection::~AdminConnection()
{
	//
}

long AdminConnection::getLastCommandTime()
{
	return m_lastCommand;
}

void AdminConnection::setLastCommandTime()
{
	m_lastCommand = time(NULL);
}

std::string AdminConnection::getIPString()
{
	return m_ip;
}

long AdminConnection::getStartTime()
{
	return m_startTime;
}

enum{
	NO_CONNECTED,
	ENCRYPTION_NO_SET,
	ENCRYPTION_OK,
	NO_LOGGED_IN,
	LOGGED_IN,
};

AdminProtocol::AdminProtocol(SOCKET s)
{
	m_socket = s;
	m_state = NO_CONNECTED;
	m_connection = new AdminConnection(s);
	m_loginTries = 0;
}

AdminProtocol::~AdminProtocol()
{
	delete m_connection;
}

enum{
	END_LOOP,
	CONTINUE_LOOP,
	NO_PACKET,
};

void AdminProtocol::receiveLoop()
{
	//is the remote admin protocol enabled?
	if(!adminConfig->isEnabled()){
		return;
	}
	
	m_state = NO_CONNECTED;
	//is allowed this ip?
	if(!adminConfig->allowIP(m_socket)){
		addLogLine(m_connection, LOGTYPE_EVENT, 1, "ip not allowed");
		return;
	}
	
	//max connections limit
	if(!adminConfig->addConnection()){
		addLogLine(m_connection, LOGTYPE_EVENT, 1, "can not add new connection");
		return;
	}
	
	addLogLine(m_connection, LOGTYPE_EVENT, 1, "start message loop");
	//read loop
	NetworkMessage msg;
	
	//send hello
	msg.AddByte(AP_MSG_HELLO);
	msg.AddU32(77); //version
	msg.AddString("Evolutions Remote-Control");
	msg.AddU16(adminConfig->getProtocolPolicy()); //security policy
	msg.AddU32(adminConfig->getProtocolOptions()); //protocol options(encryption, ...)
	msg.WriteToSocket(m_socket);
	
	m_connection->setLastCommandTime();
	msg.Reset();
	m_state = ENCRYPTION_NO_SET;
	
	NetworkMessage outputBuffer;
	//TODO. read from socket with timeout
	while(msg.ReadFromSocket(m_socket)){
		if(time(NULL) - m_connection->getLastCommandTime() > 60000){
			break;
		}
		if(parsePacket(msg, outputBuffer) == END_LOOP){
			break;
		}
	}
	addLogLine(m_connection, LOGTYPE_EVENT, 1, "end message loop");
	
	adminConfig->removeConnection();
	
	//close socket
	if(m_socket){
		closesocket(m_socket);
		m_socket = 0;
	}
}

long AdminProtocol::parsePacket(NetworkMessage &msg, NetworkMessage &outputBuffer)
{
	if(msg.getMessageLength() <= 0)
		return NO_PACKET;
	
	uint8_t recvbyte = msg.GetByte();
	
	outputBuffer.Reset();
	
	switch(m_state){
	case ENCRYPTION_NO_SET:
	{
		if(adminConfig->requireEncryption()){
			if((time(NULL) - m_connection->getStartTime()) > 30000){
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "encryption timeout");
				return END_LOOP;
			}
		
			if(recvbyte != AP_MSG_ENCRYPTION && recvbyte != AP_MSG_KEY_EXCHANGE){
				outputBuffer.AddByte(AP_MSG_ERROR);
				outputBuffer.AddString("encryption needed");
				outputBuffer.WriteToSocket(m_socket);
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "wrong command while ENCRYPTION_NO_SET");
				return CONTINUE_LOOP;
			}
			break;
		}
		else{
			m_state = NO_LOGGED_IN;
		}
	}
	case NO_LOGGED_IN:
	{
		if(adminConfig->requireLogin()){
			if((time(NULL) - m_connection->getStartTime()) > 30000){
				//login timeout
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "login timeout");
				return END_LOOP;
			}

			if(m_loginTries > 3){
				outputBuffer.AddByte(AP_MSG_ERROR);
				outputBuffer.AddString("too many login tries");
				outputBuffer.WriteToSocket(m_socket);
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "too many login tries");
				return END_LOOP;
			}

			if(recvbyte != AP_MSG_LOGIN){
				outputBuffer.AddByte(AP_MSG_ERROR);
				outputBuffer.AddString("you are not logged in");
				outputBuffer.WriteToSocket(m_socket);
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "wrong command while NO_LOGGED_IN");
				return CONTINUE_LOOP;
			}
			break;
		}
		else{
			m_state = LOGGED_IN;
		}
	}
	case LOGGED_IN:
	{
		//can execute commands
		break;
	}
	default:
		addLogLine(m_connection, LOGTYPE_ERROR, 1, "no valid connection state");
		return END_LOOP;
	}
	
	m_connection->setLastCommandTime();
	switch(recvbyte){
	case AP_MSG_LOGIN:
	{
		if(m_state == NO_LOGGED_IN && adminConfig->requireLogin()){
			std::string password = msg.GetString();
			if(adminConfig->passwordMatch(password)){
				m_state = LOGGED_IN;
				outputBuffer.AddByte(AP_MSG_LOGIN_OK);
				addLogLine(m_connection, LOGTYPE_EVENT, 1, "login ok");
			}
			else{
				m_loginTries++;
				outputBuffer.AddByte(AP_MSG_LOGIN_FAILED);
				outputBuffer.AddString("wrong password");
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "login failed.("+ password + ")");
			}
		}
		else{
			outputBuffer.AddByte(AP_MSG_LOGIN_FAILED);
			outputBuffer.AddString("can not login");
			addLogLine(m_connection, LOGTYPE_WARNING, 1, "wrong state at login");
		}
		break;
	}
	case AP_MSG_ENCRYPTION:
	{
		if(m_state == ENCRYPTION_NO_SET && adminConfig->requireEncryption()){
			uint8_t keyType = msg.GetByte();
			switch(keyType){
			case ENCRYPTION_RSA1024XTEA:
			{
				RSA* rsa = adminConfig->getRSAKey(ENCRYPTION_RSA1024XTEA);
				if(!rsa){
					outputBuffer.AddByte(AP_MSG_ENCRYPTION_FAILED);
					addLogLine(m_connection, LOGTYPE_WARNING, 1, "no valid server key type");
					break;
				}
				
				msg.setRSAInstance(rsa);
				if(msg.RSA_decrypt()){
					m_state = NO_LOGGED_IN;
					uint32_t k[4];
					k[0] = msg.GetU32();
					k[1] = msg.GetU32();
					k[2] = msg.GetU32();
					k[3] = msg.GetU32();
					
					msg.Reset();
					//use for in/out the new key we have
					msg.setEncryptionState(true);
					msg.setEncryptionKey(k);
					outputBuffer.setEncryptionState(true);
					outputBuffer.setEncryptionKey(k);
					
					outputBuffer.AddByte(AP_MSG_ENCRYPTION_OK);
					addLogLine(m_connection, LOGTYPE_EVENT, 1, "encryption ok");
				}
				else{
					outputBuffer.AddByte(AP_MSG_ENCRYPTION_FAILED);
					outputBuffer.AddString("wrong encrypted packet");
					addLogLine(m_connection, LOGTYPE_WARNING, 1, "wrong encrypted packet");
				}
				break;
			}
			default:
				outputBuffer.AddByte(AP_MSG_ENCRYPTION_FAILED);
				outputBuffer.AddString("no valid key type");
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "no valid client key type");
				break;
			}
		}
		else{
			outputBuffer.AddByte(AP_MSG_ENCRYPTION_FAILED);
			outputBuffer.AddString("can not set encryption");
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "can not set encryption");
		}
		break;
	}
	case AP_MSG_KEY_EXCHANGE:
	{
		if(m_state == ENCRYPTION_NO_SET && adminConfig->requireEncryption()){
			uint8_t keyType = msg.GetByte();
			switch(keyType){
			case ENCRYPTION_RSA1024XTEA:
			{
				RSA* rsa = adminConfig->getRSAKey(ENCRYPTION_RSA1024XTEA);
				if(!rsa){
					outputBuffer.AddByte(AP_MSG_KEY_EXCHANGE);
					addLogLine(m_connection, LOGTYPE_WARNING, 1, "no valid server key type");
					break;
				}
				
				outputBuffer.AddByte(AP_MSG_KEY_EXCHANGE_OK);
				outputBuffer.AddByte(ENCRYPTION_RSA1024XTEA);
				char RSAPublicKey[128];
				rsa->getPublicKey(RSAPublicKey);
				outputBuffer.AddBytes(RSAPublicKey, 128);
				break;
			}
			default:
				outputBuffer.AddByte(AP_MSG_KEY_EXCHANGE_FAILED);
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "no valid client key type");
				break;
			}
		}
		else{
			outputBuffer.AddByte(AP_MSG_KEY_EXCHANGE_FAILED);
			outputBuffer.AddString("can not get public key");
			addLogLine(m_connection, LOGTYPE_WARNING, 1, "can not get public key");
		}
		break;
	}
	case AP_MSG_COMMAND:
	{
		if(m_state != LOGGED_IN){
			addLogLine(m_connection, LOGTYPE_ERROR, 1, "recvbyte == AP_MSG_COMMAND && m_state != LOGGED_IN !!!");
			//never should reach this point!!
			break;
		}
		uint8_t command = msg.GetByte();
		switch(command){
		case CMD_BROADCAST:
		{
			std::string message = msg.GetString();
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "broadcast: " + message);
			g_game.anonymousBroadcastMessage(message);
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_CLOSE_SERVER:
		{
			if(adminCommandCloseServer()){
				addLogLine(m_connection, LOGTYPE_EVENT, 1, "close server ok");
				outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			}
			else{
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "close server fail");
				outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
				outputBuffer.AddString(" ");
			}
			break;
		}
		case CMD_PAY_HOUSES:
		{
			if(adminCommandPayHouses()){
				addLogLine(m_connection, LOGTYPE_EVENT, 1, "pay houses ok");
				outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			}
			else{
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "pay houses fail");
				outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
				outputBuffer.AddString(" ");
			}
			break;
		}
		case CMD_SHUTDOWN_SERVER:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "start server shutdown");
			g_game.setGameState(GAME_STATE_SHUTDOWN);
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			outputBuffer.WriteToSocket(m_socket);
			return END_LOOP;
			break;
		}
		case CMD_KICK:
		{
			std::string message = msg.GetString();
			if(Player* player = g_game.getPlayerByName(message)){
				player->kickPlayer();
				addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded kick player");
				outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			}
			else{
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "failed kick player");
				outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
				outputBuffer.AddString(" ");
			}
			break;
		}
		case CMD_PREMIUM:
		{
			std::string message = msg.GetString();
			if(Player* player = g_game.getPlayerByName(message)){
				g_game.savePremium(player, 7*86400, false);
				addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded add premium to player");
				outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			}
			else{
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "failed add premium to player");
				outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
				outputBuffer.AddString(" ");
			}
			break;
		}
		case CMD_ADDONS:
		{
			addLogLine(m_connection, LOGTYPE_WARNING, 1, "failed giving addons to player");
			outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
			outputBuffer.AddString("this function is disabled");
			break;
		}
		case CMD_BAN_PLAYER:
		{
			std::string message = msg.GetString();
			if(Player* player = g_game.getPlayerByName(message)){
				g_bans.addPlayerBan(player->getName(), std::time(NULL) + 60*60*24*7, "nothing");
				player->kickPlayer();
				addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded to ban player");
				outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			}
			else{
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "failed to ban player");
				outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
				outputBuffer.AddString(" ");
			}
			break;
		}
		case CMD_BAN_ACCOUNT:
		{
			std::string message = msg.GetString();
			if(Player* player = g_game.getPlayerByName(message)){
				g_bans.addAccountBan(player->getAccount(), std::time(NULL) + 60*60*24*7, "nothing");
				player->kickPlayer();
				addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded to ban account");
				outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			}
			else{
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "failed to ban account");
				outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
				outputBuffer.AddString(" ");
			}
			break;
		}
		case CMD_BAN_IP:
		{
			std::string message = msg.GetString();
			if(Player* player = g_game.getPlayerByName(message)){
				g_bans.addIpBan(player->getIP(), 0xFFFFFFFF, 0, "nothing");
				player->kickPlayer();
				addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded to ban IP");
				outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			}
			else{
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "failed to ban IP");
				outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
				outputBuffer.AddString(" ");
			}
			break;
		}
		case CMD_RELOAD_COMMANDS:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded reload commands");
			commands.reload();
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_RELOAD_CONFIG:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded reload config.lua");
			g_config.reload();
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_RELOAD_MONSTERS:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded reload monsters");
			g_monsters.reload();
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_RELOAD_MOVEMENTS:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded reload movements");
			g_moveEvents->reload();
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_RELOAD_RAIDS:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded reload raids");
			Raids::getInstance()->reload();
			Raids::getInstance()->startup();
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_RELOAD_TALKACTIONS:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded reload talkactions");
			g_talkactions->reload();
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_RELOAD_WEAPONS:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded reload weapons");
			g_weapons->reload();
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_RELOAD_EVERYTHING:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded reload everything");
			g_monsters.reload();
			commands.reload();
			g_config.reload();
			g_actions->reload();
			g_moveEvents->reload();
			g_talkactions->reload();
			g_spells->reload();
			g_weapons->reload();
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_SET_PVP:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded to set world-type to pvp");
			g_game.setWorldType(WORLD_TYPE_PVP);
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_SET_NON_PVP:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded to set world-type to non-pvp");
			g_game.setWorldType(WORLD_TYPE_NO_PVP);
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_SET_PVP_ENF:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded to set world-type to pvp-enforced");
			g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_EXECUTE_RAID:
		{
			std::string message = msg.GetString();
			Raid* raid = Raids::getInstance()->getRaidByName(message);
			if(!raid || !raid->isLoaded()){
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "no such raid exists");
				outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
				outputBuffer.AddString(" ");
				break;
			}

			if(Raids::getInstance()->getRunning()){
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "another raid is already being executed");
				outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
				outputBuffer.AddString(" ");
				break;
			}

			Raids::getInstance()->setRunning(raid);
			RaidEvent* event = raid->getNextRaidEvent();

			if(!event){
				addLogLine(m_connection, LOGTYPE_WARNING, 1, "this raid doesn't contain any data");
				outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
				outputBuffer.AddString(" ");
				break;
			}

			raid->setState(RAIDSTATE_EXECUTING);
			g_game.addEvent(makeTask(event->getDelay(), boost::bind(&Raid::executeRaidEvent, raid, event)));

			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded to execute raid");
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			
			break;
		}
		case CMD_CLEAN_MAP:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded to clean map");
			g_game.map->clean();
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_PLAYER_LIMIT:
		{
			std::string text = msg.GetString();
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded to set new player limit");
			int newmax = atoi(text.c_str());
			Status::instance()->playersmax = newmax;
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		case CMD_SERVER_SAVE:
		{
			addLogLine(m_connection, LOGTYPE_EVENT, 1, "succeeded to save server");
			g_game.saveServer();
			outputBuffer.AddByte(AP_MSG_COMMAND_OK);
			break;
		}
		default:
		{
			outputBuffer.AddByte(AP_MSG_COMMAND_FAILED);
			outputBuffer.AddString("not known server command");
			addLogLine(m_connection, LOGTYPE_WARNING, 1, "not known server command");
		}
		};
		break;
	}
	case AP_MSG_PING:
		outputBuffer.AddByte(AP_MSG_PING_OK);
		break;
	/*case AP_MSG_PLAYERLIST:
	{
		uint32_t i = (uint32_t)g_game.getPlayersOnline();		
		outputBuffer.AddByte(AP_MSG_LOGIN_OK);
		outputBuffer.AddByte(i);

		if(i > 0){
			for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
				msg.AddString((*it).second->getName());
			}
			
			break;
		}
		else{
			outputBuffer.AddByte(AP_MSG_LOGIN_FAILED);
			outputBuffer.AddString("There are no players online.");
		}
		
		break;
	}*/
	default:
		outputBuffer.AddByte(AP_MSG_ERROR);
		outputBuffer.AddString("not known command byte");
		addLogLine(m_connection, LOGTYPE_WARNING, 1, "not known command byte");
		break;
	};
	
	outputBuffer.WriteToSocket(m_socket);
	return CONTINUE_LOOP;
}

bool AdminProtocol::adminCommandCloseServer()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(g_game.gameLock, "AdminProtocol::adminCommandCloseServer");
	g_game.setGameState(GAME_STATE_CLOSED);
	AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
	while(it != Player::listPlayer.list.end()){
		if((*it).second->getAccessLevel() < ACCESS_ENTER){
			(*it).second->kickPlayer();
			it = Player::listPlayer.list.begin();
		}
		else{
			++it;
		}
	}
	
	g_game.saveServer();
	
	return true;
}

bool AdminProtocol::adminCommandPayHouses()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(g_game.gameLock, "AdminProtocol::adminCommandPayHouses");
	return Houses::getInstance().payHouses(true);
}

/////////////////////////////////////////////

AdminProtocolConfig::AdminProtocolConfig()
{
	m_enabled = true;
	m_onlyLocalHost = true;
	m_maxConnections = 1;
	m_currrentConnections = 0;
	m_password = "";
	m_key_RSA1024XTEA = NULL;
	m_requireLogin = true;
	m_requireEncryption = false;
}

AdminProtocolConfig::~AdminProtocolConfig()
{
	delete m_key_RSA1024XTEA;
}

bool AdminProtocolConfig::loadXMLConfig(const std::string& directory)
{	
	std::string filename = directory + "admin.xml";
	
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(!doc){
		return false;
	}

	xmlNodePtr root, p, q;
	root = xmlDocGetRootElement(doc);
	
	if(!xmlStrEqual(root->name,(const xmlChar*)"otadmin")){
		xmlFreeDoc(doc);
		return false;
	}
		
	int enabled;
	if(readXMLInteger(root, "enabled", enabled)){
		if(enabled){
			m_enabled = true;
		}
		else{
			m_enabled = false;
		}
	}
		
	int value;
	p = root->children;
	while(p){
		if(xmlStrEqual(p->name, (const xmlChar*)"security")){
			if(readXMLInteger(p, "onlylocalhost", value)){
				if(value){
					m_onlyLocalHost = true;
				}
				else{
					m_onlyLocalHost = false;
				}
			}
			if(readXMLInteger(p, "maxconnections", value) && value > 0){
				m_maxConnections = value;
			}
			if(readXMLInteger(p, "loginrequired", value)){
				if(value){
					m_requireLogin = true;
				}
				else{
					m_requireLogin = false;
				}
			}
			std::string password;
			if(readXMLString(p, "loginpassword", password)){
				m_password = password;
			}
			else{
				if(m_requireLogin){
					std::cout << "Security warning: require login but use default password." << std::endl;
				}
			}
		}
		else if(xmlStrEqual(p->name, (const xmlChar*)"encryption")){
			if(readXMLInteger(p, "required", value)){
				if(value){
					m_requireEncryption = true;
				}
				else{
					m_requireEncryption = false;
				}
			}
			q = p->children;
			while(q){
				if(xmlStrEqual(q->name, (const xmlChar*)"key")){
					std::string str;
					if(readXMLString(q, "type", str)){
						if(str == "RSA1024XTEA"){
							if(readXMLString(q, "file", str)){
								m_key_RSA1024XTEA = new RSA();
								if(!m_key_RSA1024XTEA->setKey(directory + str)){
									delete m_key_RSA1024XTEA;
									m_key_RSA1024XTEA = NULL;
									std::cout << "Can not load key from " << directory << str << std::endl;
								}
							}
							else{
								std::cout << "Missing file for RSA1024XTEA key." << std::endl;
							}
						}
						else{
							std::cout << str << " is not a valid key type." << std::endl;
						}
					}
				}
				q = q->next;
			}
		}
		p = p->next;
	}
	xmlFreeDoc(doc);

	return true;
}

bool AdminProtocolConfig::isEnabled()
{
	return m_enabled;
}

bool AdminProtocolConfig::onlyLocalHost()
{
	return m_onlyLocalHost;
}

bool AdminProtocolConfig::addConnection()
{
	if(m_currrentConnections >= m_maxConnections){
		return false;
	}
	else{
		m_currrentConnections++;
		return true;
	}
}

void AdminProtocolConfig::removeConnection()
{
	if(m_currrentConnections > 0){
		m_currrentConnections--;
	}
}

bool AdminProtocolConfig::passwordMatch(std::string& password)
{
	//prevent empty password login
	if(m_password == ""){
		return false;
	}
	if(password == m_password){
		return true;
	}
	else{
		return false;
	}
}

bool AdminProtocolConfig::allowIP(SOCKET s)
{
	uint32_t ip = getIPSocket(s);
	if(ip == 0){
		return false;
	}
	if(m_onlyLocalHost){
		if(ip == 0x0100007F){ //127.0.0.1
			return true;
		}
		else{
			char str_ip[32];
			formatIP(ip, str_ip);
			addLogLine(NULL, LOGTYPE_WARNING, 1, std::string("forbidden connection try from ") + str_ip);
			return false;
		}
	}
	else{
		if(!g_bans.isIpDisabled(s)){
			return true;
		}
		else{
			return false;
		}
	}
}

bool AdminProtocolConfig::requireLogin()
{
	return m_requireLogin;
}

bool AdminProtocolConfig::requireEncryption()
{
	return m_requireEncryption;
}

uint16_t AdminProtocolConfig::getProtocolPolicy()
{
	uint16_t policy = 0;
	if(requireLogin()){
		policy = policy | REQUIRE_LOGIN;
	}
	if(requireEncryption()){
		policy = policy | REQUIRE_ENCRYPTION;
	}
	return policy;
}

uint32_t AdminProtocolConfig::getProtocolOptions()
{
	uint32_t ret = 0;
	if(requireEncryption()){
		if(m_key_RSA1024XTEA){
			ret = ret | ENCRYPTION_RSA1024XTEA;
		}
	}
	return ret;
}

RSA* AdminProtocolConfig::getRSAKey(uint8_t type)
{
	switch(type){
	case ENCRYPTION_RSA1024XTEA:
		return m_key_RSA1024XTEA;
		break;
	default:
		return NULL;
	}
}

/////////////////////////////////////////////

static void addLogLine(AdminConnection* conn, eLogType type, int level, std::string message)
{
	std::string logMsg;
	if(conn){
		logMsg = "[" + conn->getIPString() + "] - ";
	}
	logMsg = logMsg + message;
	LOG_MESSAGE("OTADMIN", type, level, logMsg);
}

