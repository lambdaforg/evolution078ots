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


#ifndef __OTSERV_COMMANDS_H__
#define __OTSERV_COMMANDS_H__

#include <string>
#include <map>
#include "creature.h"

class Game;
struct Command;
struct s_defcommands;

class Commands{
public:
	Commands():game(NULL),loaded(false){};
	Commands(Game* igame);

	bool loadXml(const std::string& _datadir);	
	bool isLoaded(){return loaded;}
	bool reload();

	bool exeCommand(Creature* creature, const std::string& cmd);

	static ReturnValue placeSummon(Creature* creature, const std::string& name);
	
protected:
	Game* game;
	bool loaded;
	std::string datadir;

	//commands
	bool placeNpc(Creature* creature, const std::string& cmd, const std::string& param);
	bool placeMonster(Creature* creature, const std::string& cmd, const std::string& param);
	bool placeSummon(Creature* creature, const std::string& cmd, const std::string& param);
	bool broadcastMessage(Creature* creature, const std::string& cmd, const std::string& param);
	bool IPBanPlayer(Creature* creature, const std::string& cmd, const std::string& param);
	bool banPlayer(Creature* creature, const std::string& cmd, const std::string& param);
	bool teleportMasterPos(Creature* creature, const std::string& cmd, const std::string& param);
	bool teleportHere(Creature* creature, const std::string& cmd, const std::string& param);
	bool teleportToTown(Creature* creature, const std::string& cmd, const std::string& param);
	bool teleportTo(Creature* creature, const std::string& cmd, const std::string& param);
	bool createItemById(Creature* creature, const std::string& cmd, const std::string& param);
	bool createItemByName(Creature* creature, const std::string& cmd, const std::string& param);
	bool subtractMoney(Creature* creature, const std::string& cmd, const std::string& param);
	bool reloadInfo(Creature* creature, const std::string& cmd, const std::string& param);
	bool testCommand(Creature* creature, const std::string& cmd, const std::string& param);
	bool getInfo(Creature* creature, const std::string& cmd, const std::string& param);
	bool closeServer(Creature* creature, const std::string& cmd, const std::string& param);
	bool openServer(Creature* creature, const std::string& cmd, const std::string& param);
	bool onlineList(Creature* creature, const std::string& cmd, const std::string& param);
	bool teleportNTiles(Creature* creature, const std::string& cmd, const std::string& param);
	bool kickPlayer(Creature* creature, const std::string& cmd, const std::string& param);
	bool setHouseOwner(Creature* creature, const std::string& cmd, const std::string& param);
	bool sellHouse(Creature* creature, const std::string& cmd, const std::string& param);
	bool getHouse(Creature* creature, const std::string& cmd, const std::string& param);
	bool bansManager(Creature* creature, const std::string& cmd, const std::string& param);
	bool serverInfo(Creature* creature, const std::string& cmd, const std::string& param);
	bool forceRaid(Creature* creature, const std::string& cmd, const std::string& param);

	#ifdef __SKULLSYSTEM__
	bool showFrags(Creature* creature, const std::string &cmd, const std::string &param);
	#endif
	#ifdef __TLM_SERVER_SAVE__
	bool saveServer(Creature* creature, const std::string& cmd, const std::string& param);
	#endif
	#ifdef __XID_CVS_MODS__
    bool addAddon(Creature* creature, const std::string& cmd, const std::string& param);
    #endif
	#ifdef __XID_PREMIUM_SYSTEM__
	bool addPremium(Creature* creature, const std::string& cmd, const std::string& param);
	#endif
	#ifdef __PB_BUY_HOUSE__
	bool buyHouse(Creature* creature, const std::string& cmd, const std::string& param);
	#endif
	#ifdef __XID_LEAVE_HOUSE__
	bool leaveHouse(Creature* creature, const std::string& cmd, const std::string& param);	
	#endif
	#ifdef __XID_CMD_EXT__
	bool goUp(Creature* creature, const std::string &cmd, const std::string &param);
	bool goDown(Creature* creature, const std::string &cmd, const std::string &param);
	bool showExpForLvl(Creature* creature, const std::string &cmd, const std::string &param);
	bool showManaForLvl(Creature* creature, const std::string &cmd, const std::string &param);
	bool report(Creature* creature, const std::string &cmd, const std::string &param);
	bool whoIsOnline(Creature* creature, const std::string &cmd, const std::string &param);
	bool setWorldType(Creature* creature, const std::string &cmd, const std::string &param);
	bool teleportPlayerTo(Creature* creature, const std::string &cmd, const std::string &param);
	bool showPos(Creature* creature, const std::string &cmd, const std::string &param);
	bool showUptime(Creature* creature, const std::string &cmd, const std::string &param);
	bool setMaxPlayers(Creature* creature, const std::string &cmd, const std::string &param);
	#endif
	#ifdef __SILV_PREMIUM_SYSTEM__
	bool addPremium(Creature* creature, const std::string& cmd, const std::string& param);
	#endif
	#ifdef __SILV_MC_CHECK__
	bool mcCheck(Creature* creature, const std::string& cmd, const std::string& param);
	#endif
	#ifdef __TC_BROADCAST_COLORS__
	bool broadcastColor(Creature* creature, const std::string &cmd, const std::string &param);
	#endif
	#ifdef __YUR_SHUTDOWN__
	bool shutdown(Creature* creature, const std::string &cmd, const std::string &param);
	#endif
	#ifdef __YUR_CLEAN_MAP__
	bool cleanMap(Creature* creature, const std::string &cmd, const std::string &param);
	#endif
	#ifdef __TC_GM_INVISIBLE__
	bool setInvisible(Creature* creature, const std::string &cmd, const std::string &param);
	#endif
	#ifdef __XID_ADD_SKILLLEVEL__
	bool addSkillLevel(Creature* creature, const std::string &cmd, const std::string &param);
	#endif
	#ifdef __XID_BLESS_SYSTEM__
	bool addBlessing(Creature* creature, const std::string &cmd, const std::string &param);
	#endif

	
	//table of commands
	static s_defcommands defined_commands[];
	
	typedef std::map<std::string,Command*> CommandMap;
	CommandMap commandMap;
};

typedef  bool (Commands::*CommandFunc)(Creature*,const std::string&,const std::string&);

struct Command{
	CommandFunc f;
	long accesslevel;
	bool loaded;
};

struct s_defcommands{
	char *name;
	CommandFunc f;
};

#endif
