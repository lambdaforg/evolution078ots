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

#include <string>
#include <sstream>
#include <utility>
#include <fstream>

#include <boost/tokenizer.hpp>
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

#include "commands.h"
#include "player.h"
#include "npc.h"
#include "monsters.h"
#include "game.h"
#include "actions.h"
#include "house.h"
#include "ioplayer.h"
#include "tools.h"
#include "ban.h"
#include "configmanager.h"
#include "town.h"
#include "spells.h"
#include "talkaction.h"
#include "movement.h"
#include "spells.h"
#include "weapons.h"
#include "raids.h"
#include "status.h"

#if defined USE_SQL_ENGINE
#include "database.h"
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern ConfigManager g_config;
extern Actions* g_actions;
extern Monsters g_monsters;
extern Ban g_bans;
extern TalkActions* g_talkactions;
extern MoveEvents* g_moveEvents;
extern Spells* g_spells;
extern Weapons* g_weapons;
extern Game g_game;

extern bool readXMLInteger(xmlNodePtr p, const char *tag, int &value);

//table of commands
s_defcommands Commands::defined_commands[] = { 
	{"/s",&Commands::placeNpc},
	{"/m",&Commands::placeMonster},
	{"/summon",&Commands::placeSummon},
	{"/B",&Commands::broadcastMessage},
	{"/b",&Commands::IPBanPlayer},
	{"/ban",&Commands::banPlayer},
	{"/t",&Commands::teleportMasterPos},
	{"/c",&Commands::teleportHere},
	{"/i",&Commands::createItemById},
	{"/n",&Commands::createItemByName},
	{"/q",&Commands::subtractMoney},
	{"/reload",&Commands::reloadInfo},
	{"/z",&Commands::testCommand},
	{"/goto",&Commands::teleportTo},
	{"/info",&Commands::getInfo},
	{"/closeserver",&Commands::closeServer},
	{"/openserver",&Commands::openServer},
	{"/getonline",&Commands::onlineList},
	{"/a",&Commands::teleportNTiles},
	{"/kick",&Commands::kickPlayer},
	{"/owner",&Commands::setHouseOwner},
	{"!sellhouse",&Commands::sellHouse},
	{"/gethouse",&Commands::getHouse},
	{"/bans",&Commands::bansManager},
	{"/town",&Commands::teleportToTown},
	{"!serverinfo",&Commands::serverInfo},
	{"/raid",&Commands::forceRaid},

	#ifdef __SKULLSYSTEM__
	{"!frags",&Commands::showFrags},
	#endif
	#ifdef __TLM_SERVER_SAVE__	
	{"/save",&Commands::saveServer},	
	#endif	
	#ifdef __XID_CVS_MODS__
	{"/addon",&Commands::addAddon},
	#endif	
	#ifdef __XID_PREMIUM_SYSTEM__
	{"/premium",&Commands::addPremium},
	#endif
	#ifdef __PB_BUY_HOUSE__
	{"!buyhouse",&Commands::buyHouse},
	#endif	
	#ifdef __XID_LEAVE_HOUSE__
	{"!leavehouse",&Commands::leaveHouse},
	#endif
	#ifdef __XID_CMD_EXT__
	{"/up",&Commands::goUp},
	{"/down",&Commands::goDown},
	{"/pos",&Commands::showPos},
	{"/pvp",&Commands::setWorldType},
	{"/send",&Commands::teleportPlayerTo},
	{"/max",&Commands::setMaxPlayers},
	{"!exp",&Commands::showExpForLvl},
	{"!mana",&Commands::showManaForLvl},
	{"!report",&Commands::report},
	{"!online",&Commands::whoIsOnline},
	{"!uptime",&Commands::showUptime},	
	#endif
	#ifdef __SILV_MC_CHECK__
	{"/check",&Commands::mcCheck},
	#endif
	#ifdef __TC_BROADCAST_COLORS__
	{"/bc",&Commands::broadcastColor},
	#endif
	#ifdef __YUR_SHUTDOWN__
	{"/shutdown",&Commands::shutdown},
	#endif
	#ifdef __YUR_CLEAN_MAP__
	{"/clean",&Commands::cleanMap},
	#endif
	#ifdef __TC_GM_INVISIBLE__
	{"/invisible", &Commands::setInvisible},
	#endif
	#ifdef __XID_ADD_SKILLLEVEL__
	{"/addskill",&Commands::addSkillLevel},
	#endif
	#ifdef __XID_BLESS_SYSTEM__
	{"/bless",&Commands::addBlessing},
	#endif
};


Commands::Commands(Game* igame):
game(igame),
loaded(false)
{
	//setup command map
	for(uint32_t i = 0; i < sizeof(defined_commands) / sizeof(defined_commands[0]); i++){
		Command* cmd = new Command;
		cmd->loaded = false;
		cmd->accesslevel = 1;
		cmd->f = defined_commands[i].f;
		std::string key = defined_commands[i].name;
		commandMap[key] = cmd;
	}
}

bool Commands::loadXml(const std::string& _datadir)
{	
	datadir = _datadir;
	std::string filename = datadir + "commands.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		loaded = true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"commands") != 0){
			xmlFreeDoc(doc);
			return false;
		}
	
		std::string strCmd;

		p = root->children;
        
		while (p){
			if(xmlStrcmp(p->name, (const xmlChar*)"command") == 0){
				if(readXMLString(p, "cmd", strCmd)){
					CommandMap::iterator it = commandMap.find(strCmd);
					int alevel;
					if(it != commandMap.end()){
						if(readXMLInteger(p,"access",alevel)){
							if(!it->second->loaded){
								it->second->accesslevel = alevel;
								it->second->loaded = true;
							}
							else{
								std::cout << "Duplicated command " << strCmd << std::endl;
							}
						}
						else{
							std::cout << "missing access tag for " << strCmd << std::endl;
						}
					}
					else{
						//error
						std::cout << "Unknown command " << strCmd << std::endl;
					}
				}
				else{
					std::cout << "missing cmd." << std::endl;
				}
			}
			p = p->next;
		}
		xmlFreeDoc(doc);
	}
	
	//
	for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it){
		if(it->second->loaded == false){
			std::cout << "Warning: Missing access level for command " << it->first << std::endl;
		}
		
		//register command tag in game
		g_game.addCommandTag(it->first.substr(0,1));
	}
	
	
	return this->loaded;
}

bool Commands::reload()
{
	this->loaded = false;
	for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it){
		it->second->accesslevel = 1;
		it->second->loaded = false;
	}
	g_game.resetCommandTag();
	this->loadXml(datadir);
	return true;
}

bool Commands::exeCommand(Creature* creature, const std::string& cmd)
{	
	std::string str_command;
	std::string str_param;
	
	std::string::size_type loc = cmd.find( ' ', 0 );
	if( loc != std::string::npos && loc >= 0){
		str_command = std::string(cmd, 0, loc);
		str_param = std::string(cmd, (loc+1), cmd.size()-loc-1);
	}
	else {
		str_command = cmd;
		str_param = std::string(""); 
	}
	
	//find command
	CommandMap::iterator it = commandMap.find(str_command);
	if(it == commandMap.end()){
		return false;
	}

	Player* player = creature->getPlayer();
	//check access for this command
	if(player && player->getAccessLevel() < it->second->accesslevel){
		player->sendTextMessage(MSG_STATUS_SMALL, "You can not execute this command.");
		return false;
	}

	//execute command
	CommandFunc cfunc = it->second->f;
	(this->*cfunc)(creature, str_command, str_param);
	if(player && player->getAccessLevel() != 0){
		#if defined USE_SQL_ENGINE
		time_t ltime;
		time(&ltime);
		std::string time = ctime(&ltime);

		Database* mysql = Database::instance();
		DBQuery query;
		DBResult result;
	
		if(!mysql->connect()){
			return false;
		}

		DBTransaction trans(mysql);
		if(!trans.start())
			return false;

		std::stringstream command;
		DBSplitInsert query_insert(mysql);
    	query_insert.setQuery("INSERT INTO `commands` (`id` , `date` , `player` , `command`) VALUES ");
		command << "(" << 0 << ",'" << time << "','" << creature->getName() << "','" << Database::escapeString(cmd) << "')";

		if(!query_insert.addRow(command.str()))
			return false;

		if(!query_insert.executeQuery())
			return false;
		#else
		time_t ticks = time(0);
		tm* now = localtime(&ticks);
        
		char buf[32];
		char buf2[32];

		strftime(buf, sizeof(buf), "%d/%m/%Y", now);
		strftime(buf2,sizeof(buf2), "%H:%M", now);
		std::ofstream out("data/logs/commands.log", std::ios::app);
		out << '[' << buf << "] " << creature->getName() << ": " << cmd << std::endl;
		out.close();
		#endif
   
		player->sendTextMessage(MSG_STATUS_CONSOLE_RED, cmd.c_str());
	}

	return true;
}

bool Commands::placeNpc(Creature* creature, const std::string& cmd, const std::string& param)
{
	Npc* npc = new Npc(param);
	if(!npc->isLoaded()){
		delete npc;
		return true;
	}

	// Place the npc
	if(g_game.placeCreature(creature->getPosition(), npc)){
		g_game.addMagicEffect(creature->getPosition(), NM_ME_MAGIC_BLOOD);
		return true;
	}
	else{
		delete npc;
		Player* player = creature->getPlayer();
		if(player){
			player->sendCancelMessage(RET_NOTENOUGHROOM);
			g_game.addMagicEffect(creature->getPosition(), NM_ME_PUFF);
		}
		return true;
	}

	return false;
}

bool Commands::placeMonster(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();

	Monster* monster = Monster::createMonster(param);
	if(!monster){
		if(player){
			player->sendCancelMessage(RET_NOTPOSSIBLE);
			g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		}
		return false;
	}

	// Place the monster
	if(g_game.placeCreature(creature->getPosition(), monster)){
		g_game.addMagicEffect(creature->getPosition(), NM_ME_MAGIC_BLOOD);
		return true;
	}
	else{
		delete monster;
		if(player){
			player->sendCancelMessage(RET_NOTENOUGHROOM);
			g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		}
	}

	return false;
}

ReturnValue Commands::placeSummon(Creature* creature, const std::string& name)
{
	Monster* monster = Monster::createMonster(name);
	if(!monster){
		return RET_NOTPOSSIBLE;
	}
	
	// Place the monster
	creature->addSummon(monster);
	if(!g_game.placeCreature(creature->getPosition(), monster)){
		creature->removeSummon(monster);
		return RET_NOTENOUGHROOM;
	}

	return RET_NOERROR;
}

bool Commands::placeSummon(Creature* creature, const std::string& cmd, const std::string& param)
{
	ReturnValue ret = placeSummon(creature, param);

	if(ret != RET_NOERROR){
		if(Player* player = creature->getPlayer()){
			player->sendCancelMessage(ret);
			g_game.addMagicEffect(player->getPosition(), NM_ME_PUFF);
		}
	}

	return (ret == RET_NOERROR);
}

bool Commands::broadcastMessage(Creature* creature, const std::string& cmd, const std::string& param)
{
	if(!creature)
		return false;

	if(Player* player = creature->getPlayer()){
		g_game.playerBroadcastMessage(player, param);
	}
	else{
		g_game.anonymousBroadcastMessage(param);
	}
	
	return true;
}

bool Commands::IPBanPlayer(Creature* creature, const std::string& cmd, const std::string& param)
{	
	Player* playerBan = g_game.getPlayerByName(param);
	if(playerBan){
		Player* player = creature->getPlayer();
		if(player && player->getAccessLevel() <= playerBan->getAccessLevel()){
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You cannot ban this player.");
			return true;
		}

		playerBan->sendTextMessage(MSG_STATUS_CONSOLE_RED, "You have been banned.");
		unsigned long ip = playerBan->lastip;
		if(ip > 0){
			g_bans.addIpBan(ip, 0xFFFFFFFF, 0, "nothing");
		}

		playerBan->kickPlayer();
		return true;
	}

	return false;
}

bool Commands::banPlayer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	std::string ban = param;
	int pos = param.find(",");

    Player* banPlayer = game->getPlayerByName(ban.substr(0, pos).c_str());

	if(!player || !banPlayer)
		return false;

	if(player && player->getAccessLevel() <= banPlayer->getAccessLevel()){
		player->sendTextMessage(MSG_EVENT_DEFAULT, "You cannot ban this player.");
		return true;
	}

    if(banPlayer){
        std::string name = banPlayer->getName();   
        ban.erase(0, pos+1);
        int bantime = atoi(ban.c_str());

        if(bantime < 0 || bantime > 999){
            bantime = 1;
        }
        
        char buffer[4];
        itoa (bantime, buffer, 10);
        bantime = (bantime * 86400);
        if(param.find(",") && bantime > 0){
            g_bans.addPlayerBan(banPlayer->getName(), std::time(NULL) + bantime, "nothing");
            banPlayer->kickPlayer();
            std::stringstream banMessage;
            banMessage << "" << name << " has been banned for " << buffer << " day(s)."; 
            player->sendTextMessage(MSG_EVENT_DEFAULT, banMessage.str().c_str());

            return true;         
        }
        else{
            g_bans.addPlayerBan(banPlayer->getName(), 0xFFFFFFFF, "nothing");
            banPlayer->kickPlayer();
            std::stringstream banMessage;
            banMessage << "" << name << " has been banned forever.";
            player->sendTextMessage(MSG_EVENT_DEFAULT, banMessage.str().c_str());

            return true;         
        }
    }
    
    return false;
}

bool Commands::teleportMasterPos(Creature* creature, const std::string& cmd, const std::string& param)
{
	Position destPos = creature->masterPos;
	Tile* tile = g_game.getTile(destPos.x, destPos.y, destPos.z);		
	if(tile && tile->creatures.size() != 0){
		destPos.x++;
    }

	if(g_game.internalTeleport(creature, destPos) == RET_NOERROR){
		g_game.addMagicEffect(destPos, NM_ME_ENERGY_AREA);
		return true;
	}

	return false;
}

bool Commands::teleportHere(Creature* creature, const std::string& cmd, const std::string& param)
{
	Creature* paramCreature = g_game.getCreatureByName(param);
	if(paramCreature){
		Position destPos = creature->getPosition();
		Tile* tile = g_game.getTile(destPos.x, destPos.y, destPos.z);		
		if(tile && tile->creatures.size() != 0){
			destPos.x++;
    	}
    	
		if(g_game.internalTeleport(paramCreature, destPos) == RET_NOERROR){
			g_game.addMagicEffect(destPos, NM_ME_ENERGY_AREA);
			return true;
		}
	}

	return false;
}

bool Commands::createItemById(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	std::string type;
	int itemCount = 0;
	
	std::istringstream in(param.c_str());
	std::getline(in, type, ' ');
	in >> itemCount;
	
	int itemType = atoi(type.c_str());
	Item* newItem = Item::CreateItem(itemType, itemCount);
	
	if(!newItem)
		return false;

	ReturnValue ret = g_game.internalAddItem(player, newItem);
	if(ret != RET_NOERROR){
		ret = g_game.internalAddItem(player->getTile(), newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);

		if(ret != RET_NOERROR){
			delete newItem;
			return false;
		}
	}
	
	g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_POISON);
	
	return true;
}

bool Commands::createItemByName(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	std::string::size_type pos1 = param.find("\"");
	pos1 = (std::string::npos == pos1 ? 0 : pos1 + 1);

	std::string::size_type pos2 = param.rfind("\"");
	if(pos2 == pos1 || pos2 == std::string::npos){
		pos2 = param.rfind(' ');

		if(pos2 == std::string::npos){
			pos2 = param.size();
		}
	}
	
	std::string itemName = param.substr(pos1, pos2 - pos1);

	int count = 1;
	if(pos2 < param.size()){
		std::string itemCount = param.substr(pos2 + 1, param.size() - (pos2 + 1));
		count = std::min(atoi(itemCount.c_str()), 100);
	}

	int itemId = Item::items.getItemIdByName(itemName);
	if(itemId == -1){
		player->sendTextMessage(MSG_STATUS_CONSOLE_RED, "Item could not be summoned.");
		return false;
	}
				
	Item* newItem = Item::CreateItem(itemId, count);
	if(!newItem)
		return false;

	ReturnValue ret = g_game.internalAddItem(player, newItem);
	
	if(ret != RET_NOERROR){
		ret = g_game.internalAddItem(player->getTile(), newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);

		if(ret != RET_NOERROR){
			delete newItem;
			return false;
		}
	}
	
	g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_POISON);
	return true;
}

bool Commands::subtractMoney(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
				
	int count = atoi(param.c_str());
	uint32_t money = g_game.getMoney(player);
	if(!count){
		std::stringstream info;
		info << "You have " << money << " gold.";
		player->sendCancel(info.str().c_str());
		return true;
	}
	else if(count > (int)money){
		std::stringstream info;
		info << "You have " << money << " gold and is not sufficient.";
		player->sendCancel(info.str().c_str());
		return true;
	}

	if(!g_game.removeMoney(player, count)){
		std::stringstream info;
		info << "Can not subtract money!";
		player->sendCancel(info.str().c_str());
	}

	return true;
}

bool Commands::reloadInfo(Creature* creature, const std::string& cmd, const std::string& param)
{	
	if(param == "actions"){
		g_actions->reload();
	}
	else if(param == "commands"){
		this->reload();
	}
	else if(param == "monsters"){
		g_monsters.reload();
	}
	else if(param == "config"){
		g_config.reload();
	}
	else if(param == "talk"){
		g_talkactions->reload();
	}
	else if(param == "move"){
		g_moveEvents->reload();
	}
	else if(param == "spells"){
		g_spells->reload();
		g_monsters.reload();
	}
	else if(param == "raids"){
		Raids::getInstance()->reload();
		Raids::getInstance()->startup();
	}
	else{
		Player* player = creature->getPlayer();
		if(player)
			player->sendCancel("Option not found.");
	}

	return true;
}

bool Commands::testCommand(Creature* creature, const std::string& cmd, const std::string& param)
{
	int color = atoi(param.c_str());
	Player* player = creature->getPlayer();
	if(player) {
		player->sendMagicEffect(player->getPosition(), color);
	}

	return true;
}

bool Commands::teleportToTown(Creature* creature, const std::string& cmd, const std::string& param)
{
	std::string tmp = param;
	Player* player = creature->getPlayer();
    
	if(!player)
		return false;
            
	Town* town = Towns::getInstance().getTown(tmp);
	if(town){
		Position destPos = town->getTemplePosition();
		Tile* tile = g_game.getTile(destPos.x, destPos.y, destPos.z);		
		if(tile && tile->creatures.size() != 0){
			destPos.x++;
    	}
    	
        if(g_game.internalTeleport(creature, destPos) == RET_NOERROR) {
            g_game.addMagicEffect(destPos, NM_ME_ENERGY_AREA);
            return true;
        }
    }
    
    player->sendCancel("Could not find the town.");
    
    return false;    
}

bool Commands::teleportTo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Creature* paramCreature = g_game.getCreatureByName(param);
	if(paramCreature){
		Position destPos = paramCreature->getPosition();
		Tile* tile = g_game.getTile(destPos.x, destPos.y, destPos.z);		
		if(tile && tile->creatures.size() != 0){
			destPos.x++;
    	}
    	
		if(g_game.internalTeleport(creature, destPos) == RET_NOERROR){
			if(!creature->gmInvisible){
				g_game.addMagicEffect(destPos, NM_ME_ENERGY_AREA);
			}
			
			return true;
		}
	}
	
	return false;
}

bool Commands::getInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return true;
	
	Player* paramPlayer = g_game.getPlayerByName(param);
	if(paramPlayer) {
		std::stringstream info;

		unsigned char ip[4];
		*(unsigned long*)&ip = paramPlayer->lastip;
		info << "name:   " << paramPlayer->getName() << std::endl <<
		        "account: " << paramPlayer->getAccount() << std::endl <<
		        "access: " << paramPlayer->getAccessLevel() << std::endl <<
		        "level:  " << paramPlayer->getPlayerInfo(PLAYERINFO_LEVEL) << std::endl <<
		        "maglvl: " << paramPlayer->getPlayerInfo(PLAYERINFO_MAGICLEVEL) << std::endl <<
		        "speed:  " <<  paramPlayer->getSpeed() <<std::endl <<
		        "position " << paramPlayer->getPosition() << std::endl << 
				"ip: " << ipText(ip);
				
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, info.str().c_str());
	}
	else{
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Player not found.");
	}

	return true;
}

bool Commands::closeServer(Creature* creature, const std::string& cmd, const std::string& param)
{
	g_game.setGameState(GAME_STATE_CLOSED);
	//kick players with access = 0
	AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
	while(it != Player::listPlayer.list.end())
	{
		if((*it).second->getAccessLevel() < ACCESS_PROTECT){
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

bool Commands::openServer(Creature* creature, const std::string& cmd, const std::string& param)
{
	g_game.setGameState(GAME_STATE_NORMAL);
	return true;
}

bool Commands::onlineList(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
	
	std::stringstream players;
	players << "name   level   mag" << std::endl;
	
	int i, n = 0;
	AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
	for(;it != Player::listPlayer.list.end();++it)
	{
		if((*it).second->getAccessLevel() < ACCESS_PROTECT){
			players << (*it).second->getName() << "   " << 
				(*it).second->getPlayerInfo(PLAYERINFO_LEVEL) << "    " <<
				(*it).second->getPlayerInfo(PLAYERINFO_MAGICLEVEL) << std::endl;
			n++;
			i++;
		}
		if(i == 10){
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, players.str().c_str());
			players.str("");
			i = 0;
		}
	}
	if(i != 0)
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, players.str().c_str());
	
	players.str("");
	players << "Total: " << n << " player(s)" << std::endl;
	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, players.str().c_str());
	return true;
}

bool Commands::teleportNTiles(Creature* creature, const std::string& cmd, const std::string& param)
{				
	int ntiles = atoi(param.c_str());
	if(ntiles != 0)
	{
		Position newPos = creature->getPosition();
		switch(creature->getDirection()){
		case NORTH:
			newPos.y = newPos.y - ntiles;
			break;
		case SOUTH:
			newPos.y = newPos.y + ntiles;
			break;
		case EAST:
			newPos.x = newPos.x + ntiles;
			break;
		case WEST:
			newPos.x = newPos.x - ntiles;
			break;
		default:
			break;
		}

		Tile* tile = g_game.getTile(newPos.x, newPos.y, newPos.z);		
		if(tile && tile->creatures.size() != 0){
			++newPos.x;
    	}

		if(g_game.internalTeleport(creature, newPos) == RET_NOERROR){
			if(!creature->gmInvisible){
				g_game.addMagicEffect(newPos, NM_ME_ENERGY_AREA);
			}
			
			return true;
		}
	}

	return false;
}

bool Commands::kickPlayer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* playerKick = g_game.getPlayerByName(param);
	if(playerKick){
		Player* player = creature->getPlayer();
		if(player && player->getAccessLevel() <= playerKick->getAccessLevel()){
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You cannot kick this player.");
			return true;
		}

		playerKick->kickPlayer();
		return true;
	}
	return false;
}

bool Commands::setHouseOwner(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player){
		if(player->getTile()->hasFlag(TILESTATE_HOUSE)){
			HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile());
			if(houseTile){
				
				std::string real_name = param;
				if(param == "none"){
					houseTile->getHouse()->setHouseOwner("");
				}
				else if(IOPlayer::instance()->playerExists(real_name)){
					houseTile->getHouse()->setHouseOwner(real_name);
				}
				else{
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Player not found.");
				}
				return true;
			}
		}
	}
	return false;
}

bool Commands::sellHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player){
		House* house = Houses::getInstance().getHouseByPlayerName(player->getName());
		if(!house){
			player->sendCancel("You do not own a house");	
			return false;
		}
		
		Player* tradePartner = g_game.getPlayerByName(param);
		if(!(tradePartner && tradePartner != player)){
			player->sendCancel("Trade player not found.");
			return false;
		}
		
		if(tradePartner->getPlayerInfo(PLAYERINFO_LEVEL) < 1){
			player->sendCancel("Trade player level is too low.");
			return false;
		}
		
		if(Houses::getInstance().getHouseByPlayerName(tradePartner->getName())){
			player->sendCancel("Trade player already owns a house.");
			return false;
		}
		
		if(!Position::areInRange<2,2,0>(tradePartner->getPosition(), player->getPosition())){
			player->sendCancel("Trade player is too far away.");
			return false;
		}
		
		Item* transferItem = house->getTransferItem();
		if(!transferItem){
			player->sendCancel("You can not trade this house.");
			return false;
		}
		
		transferItem->getParent()->setParent(player);
		if(g_game.internalStartTrade(player, tradePartner, transferItem)){
			return true;
		}
		else{
			house->resetTransferItem();
		}
	}
	return false;
}

bool Commands::getHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
	
	std::string real_name = param;
	if(IOPlayer::instance()->playerExists(real_name)){
		House* house = Houses::getInstance().getHouseByPlayerName(real_name);
		std::stringstream str;
		str << real_name;
		if(house){
			str << " owns house: " << house->getName() << ".";
		}
		else{
			str << " does not own any house.";
		}

		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
	}
	return false;
}

bool Commands::serverInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
	
	std::stringstream text;
	text << "SERVER INFO:";
	text << "\nExp Rate: " << g_config.getNumber(ConfigManager::RATE_EXPERIENCE);
	text << "\nSkill Rate: " << g_config.getNumber(ConfigManager::RATE_SKILL);
	text << "\nMagic Rate: " << g_config.getNumber(ConfigManager::RATE_MAGIC);
	text << "\nLoot Rate: " << g_config.getNumber(ConfigManager::RATE_LOOT);
	
	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, text.str().c_str());
	
	return true;
}

void showTime(std::stringstream& str, unsigned long time)
{
	if(time == 0xFFFFFFFF){
		str << "permanent";
	}
	else if(time == 0){
		str << "shutdown";
	}
	else{
		time_t tmp = time;
		const tm* tms = localtime(&tmp);
		if(tms){
			str << tms->tm_hour << ":" << tms->tm_min << ":" << tms->tm_sec << "  " << 
				tms->tm_mday << "/" << tms->tm_mon + 1 << "/" << tms->tm_year + 1900;
		}
		else{
			str << "UNIX Time : " <<  time;
		}
	}
}

unsigned long parseTime(const std::string& time)
{
	if(time == ""){
		return 0;
	}
	if(time == "permanent"){
		return 0xFFFFFFFF;
	}
	else{
		boost::char_separator<char> sep("+");
		tokenizer timetoken(time, sep);
		tokenizer::iterator timeit = timetoken.begin();
		if(timeit == timetoken.end()){
			return 0;
		}
		unsigned long number = atoi(timeit->c_str());
		unsigned long multiplier = 0;
		++timeit;
		if(timeit == timetoken.end()){
			return 0;
		}
		if(*timeit == "m") //minute
			multiplier = 60;
		if(*timeit == "h") //hour
			multiplier = 60*60;
		if(*timeit == "d") //day
			multiplier = 60*60*24;
		if(*timeit == "w") //week
			multiplier = 60*60*24*7;
		if(*timeit == "o") //month
			multiplier = 60*60*24*30;
		if(*timeit == "y") //year
			multiplier = 60*60*24*365;
			
		uint32_t currentTime = std::time(NULL);
		return currentTime + number*multiplier;
	}
}

std::string parseParams(tokenizer::iterator &it, tokenizer::iterator end)
{
	std::string tmp;
	if(it == end){
		return "";
	}
	else{
		tmp = *it;
		++it;
		if(tmp[0] == '"'){
			tmp.erase(0,1);
			while(it != end && tmp[tmp.length() - 1] != '"'){
				tmp += " " + *it;
				++it;
			}
			tmp.erase(tmp.length() - 1);
		}
		return tmp;
	}
}

bool Commands::bansManager(Creature* creature, const std::string& cmd, const std::string& param)
{
	std::stringstream str;
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	boost::char_separator<char> sep(" ");
	tokenizer cmdtokens(param, sep);
	tokenizer::iterator cmdit = cmdtokens.begin();
	
	if(cmdit != cmdtokens.end() && *cmdit == "add"){
		unsigned char type;
		++cmdit;
		if(cmdit == cmdtokens.end()){
			str << "Parse error.";
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
			return true;
		}
		type = cmdit->c_str()[0];
		++cmdit;
		if(cmdit == cmdtokens.end()){
			str << "Parse error.";
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
			return true;
		}
		std::string param1;
		std::string param2;
		std::string param3;
		param1 = parseParams(cmdit, cmdtokens.end());
		param2 = parseParams(cmdit, cmdtokens.end());
		param3 = parseParams(cmdit, cmdtokens.end());
		long time = 0;
		switch(type){
		case 'i':
		{
			str << "Add IP-ban is not implemented.";
			break;
		}
		
		case 'p':
		{
			std::string playername = param1;
			if(!IOPlayer::instance()->playerExists(playername)){
				str << "Player not found.";
				break;
			}
			time = parseTime(param2);
			g_bans.addPlayerBan(playername, time, "nothing");
			str << playername <<  " banished.";
			break;
		}
		
		case 'a':
		{
			long account = atoi(param1.c_str());
			time = parseTime(param2);
			if(account != 0){
				g_bans.addAccountBan(account, time, "nothing");
				str << "Account " << account << " banished.";
			}
			else{
				str << "Not a valid account.";
			}
			break;
		}
		
		case 'b':
		{
			Player* playerBan = g_game.getPlayerByName(param1);
			if(!playerBan){
				str << "Player is not online.";
				break;
			}
			time = parseTime(param2);
			long account = playerBan->getAccount();
			if(account){
				g_bans.addAccountBan(account, time, "nothing");
				str << "Account banished.";
			}
			else{
				str << "Not a valid account.";
			}
			break;
		}

		default:
			str << "Unknown ban type.";
			break;
		}
	}
	else if(cmdit != cmdtokens.end() && *cmdit == "rem"){
		unsigned char type;
		++cmdit;
		if(cmdit == cmdtokens.end()){
			str << "Parse error.";
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
			return true;
		}
		type = cmdit->c_str()[0];
		++cmdit;
		if(cmdit == cmdtokens.end()){
			str << "Parse error.";
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
			return true;
		}
		unsigned long number = atoi(cmdit->c_str());
		bool ret = false;
		bool typeFound = true;
		switch(type){
		case 'i':
			ret = g_bans.removeIpBan(number);
			break;
		case 'p':
			ret = g_bans.removePlayerBan(number);
			break;
		case 'a':
			ret = g_bans.removeAccountBan(number);
			break;
		default:
			str << "Unknown ban type.";
			typeFound = false;
			break;
		}

		if(typeFound){
			if(!ret){
				str << "Error while removing ban "<<  number <<".";
			}
			else{
				str << "Ban removed.";
			}
		}
	}
	else{
		uint32_t currentTime = std::time(NULL);
		//ip bans
		{
			str << "IP bans: " << std::endl;
			const IpBanList ipBanList = g_bans.getIpBans();
			IpBanList::const_iterator it;
			unsigned char ip[4];
			unsigned char mask[4];
			unsigned long n = 0;
			for(it = ipBanList.begin(); it != ipBanList.end(); ++it){
				n++;
				if(it->time != 0 && it->time < currentTime){
					str << "*";
				}
				str << n << " : ";
				*(unsigned long*)&ip = it->ip;
				*(unsigned long*)&mask = it->mask;
				str << ipText(ip) << " " << ipText(mask) << " ";
				showTime(str, it->time);
				str << std::endl;
				if(str.str().size() > 200){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
					str.str("");
				}
			}
		}
		//player bans
		{
			str << "Player bans: " << std::endl;
			const PlayerBanList playerBanList = g_bans.getPlayerBans();
			PlayerBanList::const_iterator it;
			unsigned long n = 0;
			for(it = playerBanList.begin(); it != playerBanList.end(); ++it){
				n++;
				if(it->time != 0 && it->time < currentTime){
					str << "*";
				}
				str << n << " : ";
				if(IOPlayer::instance()->playerExists(it->name)){
					str << it->name << " ";
					showTime(str, it->time);
				}
				str << std::endl;
				if(str.str().size() > 200){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
					str.str("");
				}
			}
		}
		//account bans
		{
			str << "Account bans: " << std::endl;
			const AccountBanList accountBanList = g_bans.getAccountBans();
			AccountBanList::const_iterator it;
			unsigned long n = 0;
			for(it = accountBanList.begin(); it != accountBanList.end(); ++it){
				n++;
				if(it->time != 0 && it->time < currentTime){
					str << "*";
				}
				str << n << " : ";
				str << it->id << " ";
				showTime(str, it->time);
				str << std::endl;
				if(str.str().size() > 200){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
					str.str("");
				}
			}
		}
	}
	if(str.str().size() > 0){
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
	}
	return true;
}

bool Commands::forceRaid(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player){
		return false;
	}

	Raid* raid = Raids::getInstance()->getRaidByName(param);
	if(!raid || !raid->isLoaded()){
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "No such raid exists.");
		return false;
	}

	if(Raids::getInstance()->getRunning()){
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Another raid is already being executed.");
		return false;
	}

	Raids::getInstance()->setRunning(raid);
	RaidEvent* event = raid->getNextRaidEvent();

	if(!event){
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "The raid does not contain any data.");
		return false;
	}

	raid->setState(RAIDSTATE_EXECUTING);
	g_game.addEvent(makeTask(event->getDelay(), boost::bind(&Raid::executeRaidEvent, raid, event)));

	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Raid started.");
	return true;
}

#ifdef __SKULLSYSTEM__
bool Commands::showFrags(Creature* creature, const std::string &cmd, const std::string &param)
{
	Player* player = creature->getPlayer();

	if(player){
        if(player->redSkullTicks > 0){
            int skullKills = player->redSkullTicks / g_config.getNumber(ConfigManager::FRAG_TIME);        

		    std::ostringstream info;
		    info << "You have " << skullKills + 1 << " unjustified kills. You will lose your unjustified kills in " <<  tickstr(player->redSkullTicks) << ".";

		    player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, info.str().c_str());
        }
        else
		    player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You have 0 unjustified kills.");
	}

	return true;
}
#endif

#ifdef __TLM_SERVER_SAVE__
bool Commands::saveServer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	
    if(player)
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Global save complete.");
	
    g_game.saveServer();
}
#endif

#ifdef __XID_CVS_MODS__
bool Commands::addAddon(Creature* creature, const std::string &cmd, const std::string &param)
{
	uint32_t lookType, addon = 0;
	std::string name;
	std::istringstream in(param.c_str());

	std::getline(in, name, ',');
	in >> lookType >> addon;	

	Player* player = g_game.getPlayerByName(name);
	if(player){
		if(addon < 0 || addon > 3){
			addon = 3;
		}
		
		g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_POISON);
		player->addAddon(lookType, addon);
		
		return true;
	}
    
	return false;
}
#endif

#ifdef __XID_PREMIUM_SYSTEM__
bool Commands::addPremium(Creature* creature, const std::string& cmd, const std::string& param)
{
	uint32_t premiumTime = 0;
	std::string name;
	std::istringstream in(param.c_str());

	std::getline(in, name, ',');
	in >> premiumTime;	

	Player* player = g_game.getPlayerByName(name);
	if(player){
		if(premiumTime < 0 || premiumTime > 999){
			premiumTime = 1;
		}
		
		if(g_game.savePremium(player, premiumTime*86400, false)){
			g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_POISON);
			return true;
		}
	}
    
	return false;
}
#endif

#ifdef __PB_BUY_HOUSE__
bool Commands::buyHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
	Position pos = player->getPosition();
	switch(player->getDirection()){
		case NORTH:
			pos.y--;
			break;
		case SOUTH:
			pos.y++;
			break;
		case WEST:
			pos.x--;
			break;
		case EAST:
			pos.x++;
			break;
	}
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); it++){
		House* otherHouse = it->second;
		if(otherHouse->getHouseOwner() == player->getName()){ //why is he trying to buy a house? he alreadly has one...
			player->sendCancel("You already have a house.");
			return false;
		}
	}
	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(tile){
		HouseTile* houseTile = dynamic_cast<HouseTile*>(tile);
		if(houseTile){
			House* house = houseTile->getHouse();
			if(house){
				Door* door = house->getDoorByPosition(pos);
				if(door){
					int price = 0;
					if(player->getLevel() < g_config.getNumber(ConfigManager::HOUSE_LEVEL)){
						player->sendCancel("You do not have the required level.");
						return false;
					}
					if(house->getHouseOwner() != ""){
						player->sendCancel("This house already has an owner.");
						return false;
					}

					for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); it++){
						price += g_config.getNumber(ConfigManager::HOUSE_PRICE);
					}
					if(price <= 0)
						return false;
					uint32_t money = g_game.getMoney(player);
					if(money < price){
						player->sendCancel("You do not have enough money.");
						return false;
					}
					if(!g_game.removeMoney(player, price)){
						player->sendCancel("You do not have enough money.");
						return false;
					}

					house->setHouseOwner(player->getName());
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Now you own this house.");
				}
			}
		}
	}

	return true;
}
#endif

#ifdef __XID_LEAVE_HOUSE__
bool Commands::leaveHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player){
		House* house = Houses::getInstance().getHouseByPlayerName(player->getName());
		if(!house){
			player->sendCancel("You do not own a house");	
			return false;
		}

		house->setHouseOwner("");
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You have successfully left your house.");
	}
	
	return false;
}
#endif

#ifdef __XID_CMD_EXT__
bool Commands::showExpForLvl(Creature* creature, const std::string &cmd, const std::string &param)
{
	if(Player* player = creature->getPlayer()){
		std::string msg = std::string("You need ") + str(player->getExpForLv(player->getLevel() + 1) - player->getExperience()) + 
			std::string(" experience points to gain level.");
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, msg.c_str());
	}
	
	return true;
}

bool Commands::showManaForLvl(Creature* creature, const std::string &cmd, const std::string &param)
{
	if(Player* player = creature->getPlayer()){
		std::string msg = std::string("You need to spent ") + str((long)player->vocation->getReqMana(player->getMagicLevel()+1) - player->getManaSpent()) + 
			std::string(" mana to gain magic level.");
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, msg.c_str());
	}

	return true;
}

bool Commands::report(Creature* creature, const std::string &cmd, const std::string &param)
{
	if(Player* player = creature->getPlayer()){
		char buf[64];
		time_t ticks = time(0);
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&ticks));	
		std::ofstream out("data/logs/report.log", std::ios::app);
		out << '[' << buf << "] " << player->getName() << ": " << param << std::endl;
		out.close();

		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Your report has been sent.");
	}

	return true;
}

bool Commands::setMaxPlayers(Creature* creature, const std::string &cmd, const std::string &param)
{
	if(!param.empty()){
		int newmax = atoi(param.c_str());
		if(newmax > 0){
			Status::instance()->playersmax = newmax;

			if(Player* player = creature->getPlayer()){
				player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, (std::string("Max number of players is now ")+param).c_str());
			}
		}
	}
	return true;
}

bool Commands::whoIsOnline(Creature* creature, const std::string &cmd, const std::string &param)
{
	if(Player* player = creature->getPlayer()){
		AutoList<Player>::listiterator iter = Player::listPlayer.list.begin();
		std::string info = "Players online: " + (*iter).second->getName();
		++iter;

		while(iter != Player::listPlayer.list.end()){
			if((*iter).second->getAccessLevel() < ACCESS_PROTECT){
				info += ", ";
				info += (*iter).second->getName();
			}
			
			++iter;
		}

		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, info.c_str());
	}

	return true;
}

bool Commands::showUptime(Creature* creature, const std::string &cmd, const std::string &param)
{
	if(Player* player = creature->getPlayer()){
		uint64_t uptime = (OTSYS_TIME() - Status::instance()->start)/1000;
		int h = (int)floor(uptime / 3600.0);
		int m = (int)floor((uptime - h*3600) / 60.0);
		int s = (int)(uptime - h*3600 - m*60);

		std::stringstream msg;
		msg << "This server has been running for " << h << (h != 1? " hours " : " hour ") <<
			m << (m != 1? " minutes " : " minute ") << s << (s != 1? " seconds. " : " second.") << std::ends;

		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, msg.str().c_str());
	}
	
	return true;
}

bool Commands::goUp(Creature* creature, const std::string &cmd, const std::string &param)
{
    Position newPos = creature->getPosition();
	newPos.z--;
	if(g_game.internalTeleport(creature, newPos) == RET_NOERROR){
		if(!creature->gmInvisible){
			g_game.addMagicEffect(newPos, NM_ME_ENERGY_AREA);
		}
		
		return true;
    }
    
    return false;
}

bool Commands::goDown(Creature* creature, const std::string &cmd, const std::string &param)
{
    Position newPos = creature->getPosition();
	newPos.z++;
	Tile* tile = g_game.getTile(newPos.x, newPos.y, newPos.z);		
	if(tile && tile->creatures.size() != 0){
		newPos.x++;
    }
    
	if(g_game.internalTeleport(creature, newPos) == RET_NOERROR){
		if(!creature->gmInvisible){
			g_game.addMagicEffect(newPos, NM_ME_ENERGY_AREA);
		}
		
		return true;
    }
    
    return false;
}

bool Commands::setWorldType(Creature* creature, const std::string &cmd, const std::string &param)
{
	Player* player = creature->getPlayer();

	if(player && !param.empty()){
		int type = atoi(param.c_str());

		if(type == 0){
			g_game.setWorldType(WORLD_TYPE_NO_PVP);
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "World-type set to Non-PvP.");
		}
		else if(type == 1){
			g_game.setWorldType(WORLD_TYPE_PVP);
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "World-type set to PvP.");
		}
		else if(type == 2){
			g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "World type set to PvP-Enforced.");
		}
	}

	return true;
}

bool Commands::teleportPlayerTo(Creature* creature, const std::string &cmd, const std::string &param)
{
    Position newPos = creature->getPosition();
	std::string name;
	std::istringstream in(param.c_str());

	std::getline(in, name, ',');
	in >> newPos.x >> newPos.y >> newPos.z;	

	Tile* tile = g_game.getTile(newPos.x, newPos.y, newPos.z);		
	if(tile && tile->creatures.size() != 0){
		newPos.x++;
	}

	if(Player* player = g_game.getPlayerByName(name)){
		if(g_game.internalTeleport(player, newPos) == RET_NOERROR){
			g_game.addMagicEffect(newPos, NM_ME_ENERGY_AREA);
			return true;
		}
	}
    
	return false;	
}

bool Commands::showPos(Creature* creature, const std::string &cmd, const std::string &param)
{
	if(Player* player = creature->getPlayer()){
		std::stringstream msg;
		msg << "Your current position is [X: " << player->getPosition().x << "] [Y: " << player->getPosition().y << "] [Z: " << player->getPosition().z << "]";
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, msg.str().c_str());
	}
	
	return true;
}
#endif

#ifdef __SILV_MC_CHECK__
bool Commands::mcCheck(Creature* creature, const std::string& cmd, const std::string& param)
{
    std::stringstream info;
    unsigned char ip[4];
        
    if(Player* player = creature->getPlayer()){   
        info << "The following players are multiclienting: \n";
        info << "Name          IP" << "\n";
        for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
            Player* lol = (*it).second;                                                   
            for(AutoList<Player>::listiterator cit = Player::listPlayer.list.begin(); cit != Player::listPlayer.list.end(); ++cit){
                if((*cit).second != lol && (*cit).second->lastip == lol->lastip){
                *(unsigned long*)&ip = (*cit).second->lastip;
                    info << (*cit).second->getName() << "      " << (unsigned int)ip[0] << "." << (unsigned int)ip[1] << 
        "." << (unsigned int)ip[2] << "." << (unsigned int)ip[3] << "\n";       
                }                                                           
            }
        }      
        player->sendTextMessage(MSG_STATUS_CONSOLE_RED, info.str().c_str());        
    }
    else{
        return false;
    }
       
    return true;                                     
}
#endif

#ifdef __TC_BROADCAST_COLORS__
bool Commands::broadcastColor(Creature* creature, const std::string &cmd, const std::string &param)
{
    int a;
    int colorInt;
    Player* player = creature->getPlayer();
    std::string message = param.c_str();
    std::stringstream fullMessage;
    std::string color;
    MessageClasses mclass;
    
    for(a=0; a<param.length(); ++a){
       if(param[a] > 3 && param[a] == ' '){
         color = param;
         color.erase(a,1-param.length());
         message.erase(0,1+a);
         break;
       }
       else
          message = param.c_str();       
    }
    
    std::transform(color.begin(), color.end(), color.begin(), tolower);
    
    if(color == "blue")
       mclass = MSG_STATUS_CONSOLE_BLUE;
    else if(color == "red"){
       g_game.playerBroadcastMessage(player, param);
       return false;
    }
    else if(color == "red2")
       mclass = MSG_STATUS_CONSOLE_RED;
    else if(color == "orange")
       mclass = MSG_STATUS_CONSOLE_ORANGE;
    else if(color == "white")
       mclass = MSG_EVENT_ADVANCE; //Invasion
    else if(color == "white2")
       mclass = MSG_EVENT_DEFAULT;
    else if(color == "green")
       mclass = MSG_INFO_DESCR;
    else if(color == "small")
       mclass = MSG_STATUS_DEFAULT;
    else if(color == "yellow")
       mclass = MSG_STATUS_CONSOLE_YELLOW;
    else if(color == "private")
       mclass = MSG_EVENT_PRIVATE;                                       
    else{
       player->sendTextMessage(MSG_STATUS_SMALL, "Define a color, or use #B to broadcast red.");
       return false;
    }
    
    fullMessage << message.c_str()<<std::endl; //Name: Message
                      
    for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
       if(dynamic_cast<Player*>(it->second))
         (*it).second->sendTextMessage(mclass, fullMessage.str().c_str());
    }
    
    return true;
}
#endif

#ifdef __YUR_SHUTDOWN__
bool Commands::shutdown(Creature* creature, const std::string &cmd, const std::string &param)
{
	Player* player = creature->getPlayer();
	if(player && !param.empty()){
		g_game.sheduleShutdown(atoi(param.c_str()));
	}
		
	return true;
}
#endif

#ifdef __YUR_CLEAN_MAP__
bool Commands::cleanMap(Creature* creature, const std::string &cmd, const std::string &param)
{
	std::ostringstream info;
	long count = g_game.map->clean();

	std::cout << ":: server clean.. ";
	info << "Clean completed. Collected " << count << (count==1? " item." : " items.") << std::ends;
	std::cout << "[done]" << std::endl;

	if(Player* player = creature->getPlayer()){
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, info.str().c_str());
	}
	else{
		g_game.anonymousBroadcastMessage(info.str().c_str());
	}
	
	return true;
}
#endif

#ifdef __TC_GM_INVISIBLE__
bool Commands::setInvisible(Creature* creature, const std::string &cmd, const std::string &param)
{
	Player* gm = creature->getPlayer();
	if(!gm)
		return false;
   
	gm->gmInvisible = !gm->gmInvisible;
   
	SpectatorVec list;
	Cylinder* cylinder = creature->getTile();
	g_game.getSpectators(list, cylinder->getPosition(), true);
	uint32_t index = cylinder->__getIndexOfThing(creature);
   
	for(SpectatorVec::iterator it = list.begin(); it != list.end(); ++it){
		if((*it) != gm){
			Player* isP = (*it)->getPlayer();
			if(isP && isP->getAccessLevel() == 0){
				isP->sendCreatureHealth(gm);
				if(gm->gmInvisible)
					isP->sendCreatureDisappear(creature, index, true);
				else
					isP->sendCreatureAppear(creature, true);
			}
		}
	}

	creature->getTile()->onUpdateTile();
	if(gm->gmInvisible)
		gm->sendTextMessage(MSG_INFO_DESCR, "You are invisible.");
	else
		gm->sendTextMessage(MSG_INFO_DESCR, "You are visible again.");

	return true;
}
#endif

#ifdef __XID_ADD_SKILLLEVEL__
bool Commands::addSkillLevel(Creature* creature, const std::string& cmd, const std::string& param)
{
	std::string name, skillName;
	std::istringstream in(param.c_str());

	std::getline(in, name, ',');
	in >> skillName;	

	Player* player = g_game.getPlayerByName(name);
	if(player){
		uint32_t skillId = player->getSkillId(skillName);
		Vocation * vocation = player->vocation;
		
		//add skill level
		if(skillId >= 0 && skillId <= 6){
			uint32_t skillLevel = vocation->getReqSkillTries(skillId, player->getSkill((skills_t)skillId, SKILL_LEVEL) + 1);
			player->addSkillAdvance((skills_t)skillId, skillLevel);
		}
		//add experience level
		else if(skillId == 7){
			int64_t experience = player->getExpForLv(player->getLevel() + 1);
			player->addExperience(experience - player->getExperience());
		}
		//add magic level
		else if(skillId == 8){
			int64_t manaSpent = vocation->getReqMana(player->getMagicLevel() + 1);
			player->addManaSpent(manaSpent - player->getManaSpent());
		}
		
		g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_POISON);
		return true;
	}
    
	return false;
}
#endif

#ifdef __XID_ADD_SKILLLEVEL__
bool Commands::addBlessing(Creature* creature, const std::string& cmd, const std::string& param)
{
	std::string name;
	int32_t blessId = 0;
	std::istringstream in(param.c_str());

	std::getline(in, name, ',');
	in >> blessId;

	Player* player = g_game.getPlayerByName(name);
	if(player){
		player->addBlessing(blessId);
		g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_POISON);
		
		return true;
	}
    
	return false;
}
#endif
