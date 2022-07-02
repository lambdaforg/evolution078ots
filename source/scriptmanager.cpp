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

#include "scriptmanager.h"
#include "luascript.h"
#include "configmanager.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h> 

#include "actions.h"
#include "talkaction.h"
#include "spells.h"
#include "movement.h"
#include "weapons.h"

extern Actions* g_actions;
extern TalkActions* g_talkactions;
extern Spells* g_spells;
extern MoveEvents* g_moveEvents;
extern Weapons* g_weapons;

extern ConfigManager g_config;
extern void ErrorMessage(const char* message);

ScriptingManager* ScriptingManager::_instance = NULL;

ScriptingManager::ScriptingManager()
{
	//
}

ScriptingManager::~ScriptingManager()
{
	//
}

ScriptingManager* ScriptingManager::getInstance()
{
	if(_instance == NULL){
		_instance = new ScriptingManager();
	}
	return _instance;
}


bool ScriptingManager::loadScriptSystems()
{	
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	
	//load weapons data
	std::cout << ":: Loading weapons.xml...";
	g_weapons = new Weapons();
	if(!g_weapons->loadFromXml(datadir)){
		ErrorMessage("Unable to load Weapons!");
		return false;
	}

	g_weapons->loadDefaults();
	std::cout << "[done]" << std::endl;

	//load spells data	
	g_spells = new Spells();
	std::cout << ":: Loading spells.xml...";
	if(!g_spells->loadFromXml(datadir)){
		ErrorMessage("Unable to load Spells!");
		return false;
	}
	std::cout << "[done]" << std::endl;

	//load actions data
	g_actions = new Actions();
	std::cout << ":: Loading actions.xml...";
	if(!g_actions->loadFromXml(datadir)){
		ErrorMessage("Unable to load Actions!");
		return false;
	}
	std::cout << "[done]" << std::endl;
	
	//load talkactions data
	g_talkactions = new TalkActions();
	std::cout << ":: Loading talkactions.xml...";
	if(!g_talkactions->loadFromXml(datadir)){
		ErrorMessage("Unable to load Talkactions!");
		return false;
	}
	std::cout << "[done]" << std::endl;
	
	//load moveEvents
	g_moveEvents = new MoveEvents();
	std::cout << ":: Loading movements.xml...";
	if(!g_moveEvents->loadFromXml(datadir)){
		ErrorMessage("Unable to load MoveEvents!");
		return false;
	}
	std::cout << "[done]" << std::endl;
    	
	return true;
}
