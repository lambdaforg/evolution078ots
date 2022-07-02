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

#ifndef _CONFIG_MANAGER_H
#define _CONFIG_MANAGER_H

#include <string>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


class ConfigManager {
public:
	ConfigManager();
	~ConfigManager();

	enum string_config_t {
		CONFIG_FILE = 0,
		DATA_DIRECTORY,
		MAP_FILE,
		MAP_STORE_FILE,
		HOUSE_STORE_FILE,
		HOUSE_RENT_PERIOD,
		MAP_KIND,
		BAN_FILE,
		LOGIN_MSG,
		SERVER_NAME,
		WORLD_NAME,
		OWNER_NAME,
		OWNER_EMAIL,
		URL,
		LOCATION,
		IP,
		MOTD,
		WORLD_TYPE,
		SQL_HOST,
		SQL_USER,
		SQL_PASS,
		SQL_DB,
		SQLITE_DB,
		SQL_TYPE,
		MAP_HOST,
		MAP_USER,
		MAP_PASS,
		MAP_DB,
		GUILD_SYSTEM,
		OTSERV_DB_HOST,
		ITEM_HOTKEYS,
		BATTLE_WINDOW_PLAYERS,
		#ifdef __XID_PREMIUM_SYSTEM__
		FREE_PREMIUM,
		#endif
		#ifdef __XID_CVS_MODS__
		REMOVE_AMMUNATION,
		REMOVE_RUNE_CHARGES,
		#endif
		#ifdef __XID_CONFIG_CAP__
		CAP_SYSTEM,
		#endif
		#ifdef __XID_LEARN_SPELLS__
		LEARN_SPELLS,
		#endif
		#ifdef __XID_ACCOUNT_MANAGER__
		ACCOUNT_MANAGER,
		#endif
		#ifdef __XID_SUMMONS_FOLLOW__
		SUMMONS_FOLLOW,
		#endif
		#ifdef __XID_PVP_FEATURES__
		ALLOW_OUTFIT_CHANGE,
		OUTFIT_DAMAGE,
		#endif
		LAST_STRING_CONFIG /* this must be the last one */
	};

	enum integer_config_t {
		LOGIN_TRIES = 0,
		RETRY_TIMEOUT,
		LOGIN_TIMEOUT,
		PORT,
		MOTD_NUM,
		MAX_PLAYERS,
		EXHAUSTED,
		EXHAUSTED_HEAL,
		EXHAUSTED_ADD,
		PZ_LOCKED,
		MIN_ACTIONTIME,
		ALLOW_CLONES,
		RATE_EXPERIENCE,
		RATE_EXPERIENCE_PVP,
		RATE_SKILL,
		RATE_LOOT,
		RATE_MAGIC,
		RATE_SPAWN,
		MAX_MESSAGEBUFFER,
		OTSERV_DB_ENABLED,
		MAX_SUMMONS,
		ACCESS_ENTER,
		ACCESS_PROTECT,
		ACCESS_HOUSE,
		ACCESS_TALK,
		ACCESS_MOVE,
		ACCESS_LOOK,
		#ifdef __TLM_SERVER_SAVE__
		SERVER_SAVE,
		#endif
		#ifdef __XID_CVS_MODS__
		BAN_UNJUST,
		RED_UNJUST,
		BAN_TIME,
		FRAG_TIME,
		WHITE_TIME,
		MAX_DEPOT_ITEMS,
		#endif
        #ifdef __PB_BUY_HOUSE__
		HOUSE_PRICE,
		HOUSE_LEVEL,
		#endif
		#ifdef __TR_ANTI_AFK__
		KICK_TIME,
		#endif
		#ifdef __JD_DEATH_LIST__
		MAX_DEATH_ENTRIES,
		#endif
		#ifdef __XID_PROTECTION_SYSTEM__
		PROTECTION_LIMIT,
		#endif
		LAST_INTEGER_CONFIG /* this must be the last one */
	};


	bool loadFile(const std::string& _filename);
	bool reload();
	std::string getString(int _what) { return m_confString[_what]; };
	int getNumber(int _what);
	bool setNumber(int _what, int _value);
	int getCriticalString(int _what) { return m_confCriticalString[_what-1]; };

private:
	std::string getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default="");
	int getGlobalNumber(lua_State* _L, const std::string& _identifier, const int _default=0);
	std::string getGlobalStringField(lua_State* _L, const std::string& _identifier, const int _key, const std::string& _default="");

	bool m_isLoaded;
	std::string m_confString[LAST_STRING_CONFIG];
	int m_confInteger[LAST_INTEGER_CONFIG];
	int m_confCriticalString[2];
};


#endif /* _CONFIG_MANAGER_H */
