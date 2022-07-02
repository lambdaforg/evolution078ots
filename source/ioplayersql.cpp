//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Player Loader/Saver based on MySQL
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

#include "ioplayer.h"
#include "ioplayersql.h"
#include "ioaccount.h"
#include "item.h"
#include "configmanager.h"
#include "tools.h"
#include "definitions.h"

#include <boost/tokenizer.hpp>
#include <iostream>
#include <iomanip>

extern ConfigManager g_config;
typedef std::vector<std::string> StringVector;
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

#ifndef __GNUC__
#pragma warning( disable : 4005)
#pragma warning( disable : 4996)
#endif

IOPlayerSQL::IOPlayerSQL()
{
	//
}

bool IOPlayerSQL::loadPlayer(Player* player, std::string name)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}

	query << "SELECT * FROM players WHERE name='" << Database::escapeString(name) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	int accno = result.getDataInt("account");
	if(accno < 1)
		return false;

	// Getting all player properties
	player->setGUID((int)result.getDataInt("id"));

	player->accountNumber = result.getDataInt("account");
	player->setSex((playersex_t)result.getDataInt("sex"));

	player->guildId   = result.getDataInt("guildid");
	player->guildRank = result.getDataString("guildrank");
	player->guildNick = result.getDataString("guildnick");

	player->setDirection((Direction)result.getDataInt("direction"));
	player->experience = result.getDataLong("experience");
	player->level = result.getDataInt("level");
	player->capacity = result.getDataInt("cap");
	player->maxDepotLimit = result.getDataInt("maxdepotitems");
	player->lastLoginSaved = result.getDataInt("lastlogin");
	#ifdef __SKULLSYSTEM__
	long redSkullSeconds = result.getDataInt("redskulltime") - std::time(NULL);
	if(redSkullSeconds > 0){
		//ensure that we round up the number of ticks
		player->redSkullTicks = (redSkullSeconds + 2)*1000;
		if(result.getDataInt("redskull") == 1){
			player->skull = SKULL_RED;
		}
	}
	#endif

	player->setVocation(result.getDataInt("vocation"));
	player->accessLevel = result.getDataInt("access");
	player->updateBaseSpeed();
	
	player->mana = result.getDataInt("mana");
	player->manaMax = result.getDataInt("manamax");
	player->manaSpent = result.getDataInt("manaspent");
	player->magLevel = result.getDataInt("maglevel");

	player->health = result.getDataInt("health");
	if(player->health <= 0)
		player->health = 100;

	player->healthMax = result.getDataInt("healthmax");
	if(player->healthMax <= 0)
		player->healthMax = 100;

	if(player->isOnline()){
		player->addDefaultRegeneration(result.getDataInt("food") * 1000);
	}

	if(result.getDataInt("soul") > 0)
		player->soul = result.getDataInt("soul");
	if(result.getDataInt("soulmax") > 0)
		player->soulMax = result.getDataInt("soulmax");

	player->defaultOutfit.lookType = result.getDataInt("looktype");
	player->defaultOutfit.lookHead = result.getDataInt("lookhead");
	player->defaultOutfit.lookBody = result.getDataInt("lookbody");
	player->defaultOutfit.lookLegs = result.getDataInt("looklegs");
	player->defaultOutfit.lookFeet = result.getDataInt("lookfeet");
	player->defaultOutfit.lookAddons = result.getDataInt("lookaddons");
	player->currentOutfit = player->defaultOutfit;

	boost::char_separator<char> sep(";");

	//SPAWN
	std::string pos = result.getDataString("pos");
	tokenizer tokens(pos, sep);

	tokenizer::iterator spawnit = tokens.begin();
	player->loginPosition.x = atoi(spawnit->c_str()); spawnit++;
	player->loginPosition.y = atoi(spawnit->c_str()); spawnit++;
	player->loginPosition.z = atoi(spawnit->c_str());

	//MASTERSPAWN
	std::string masterpos = result.getDataString("masterpos");
	tokenizer mastertokens(masterpos, sep);

	tokenizer::iterator mspawnit = mastertokens.begin();
	player->masterPos.x = atoi(mspawnit->c_str()); mspawnit++;
	player->masterPos.y = atoi(mspawnit->c_str()); mspawnit++;
	player->masterPos.z = atoi(mspawnit->c_str());

	//get password
	query << "SELECT * FROM accounts WHERE accno='" << accno << "'";
	if(!mysql->storeQuery(query, result))
		return false;

	if(result.getDataInt("accno") != accno)
		return false;

	player->password = result.getDataString("password");


	if(player->guildId){
		query << "SELECT guildname FROM guilds WHERE guildid='" << player->guildId << "'";
		if(mysql->storeQuery(query, result)){
			player->guildName = result.getDataString("guildname");
		}
	}

	// we need to find out our skills
	query << "SELECT * FROM skills WHERE player='" << player->getGUID() << "'";
	if(mysql->storeQuery(query, result)){
		//now iterate over the skills
		for(uint32_t i = 0; i < result.getNumRows(); ++i){
			int skillid = result.getDataInt("id",i);
			player->skills[skillid][SKILL_LEVEL] = result.getDataInt("skill",i);
			player->skills[skillid][SKILL_TRIES] = result.getDataInt("tries",i);
		}
	}

	//load the items
	OTSERV_HASH_MAP<int,std::pair<Item*,int> > itemmap;
	
	query << "SELECT * FROM items WHERE player='" << player->getGUID() << "' ORDER BY sid ASC";
	if(mysql->storeQuery(query, result) && (result.getNumRows() > 0)){		
		for(uint32_t i=0; i < result.getNumRows(); ++i){
			int type = result.getDataInt("type",i);
			int count = result.getDataInt("number",i);
			Item* myItem = Item::CreateItem(type, count);
			if(result.getDataInt("actionid",i) >= 100)
				myItem->setActionId(result.getDataInt("actionid",i));
			if(result.getDataInt("duration",i) > 0)
				myItem->setDuration(result.getDataInt("duration",i));
			if(result.getDataInt("charges",i) > 0)
				myItem->setItemCharge(result.getDataInt("charges",i));
			if((ItemDecayState_t)result.getDataInt("decaystate",i) != DECAYING_FALSE)
				myItem->setDecaying(DECAYING_PENDING);

			myItem->setText(result.getDataString("text",i));
			myItem->setSpecialDescription(result.getDataString("specialdesc",i));
			std::pair<Item*, int> myPair(myItem, result.getDataInt("pid",i));
			itemmap[result.getDataInt("sid",i)] = myPair;
			if(int slotid = result.getDataInt("slot",i)){
				if(slotid > 0 && slotid <= 10){
					player->__internalAddThing(slotid, myItem);
				}
				else{
					if(Container* container = myItem->getContainer()){
						if(Depot* depot = container->getDepot()){
							player->addDepot(depot, slotid - 100);
						}
						else{
							std::cout << "Error loading depot "<< slotid << " for player " <<
								player->getGUID() << std::endl;
							delete myItem;
						}
					}
					else{
						std::cout << "Error loading depot "<< slotid << " for player " <<
							player->getGUID() << std::endl;
						delete myItem;
					}
				}
			}
		}
	}

	OTSERV_HASH_MAP<int,std::pair<Item*,int> >::iterator it;
	
	for(int i = (int)itemmap.size(); i > 0; --i){
		it = itemmap.find(i);
		if(it == itemmap.end())
			continue;

		if(int p=(*it).second.second){
			if(Container* c = itemmap[p].first->getContainer()) {
				c->__internalAddThing((*it).second.first);
			}
		}
	}

	player->updateInventoryWeigth();
	player->updateItemsLight(true);
	player->setSkillsPercents();

	//load storage map
	query << "SELECT * FROM playerstorage WHERE player='" << player->getGUID() << "'";

	if(mysql->storeQuery(query,result)){
		for(uint32_t i=0; i < result.getNumRows(); ++i){
			unsigned long key = result.getDataInt("key",i);
			long value = result.getDataInt("value",i);
			player->addStorageValue(key,value);
		}
	}

	if(!loadVIP(player)){
		return false;
	}

	#ifdef __XID_SEPERATE_ADDONS__
	query << "SELECT * FROM addons WHERE player='" << player->getGUID() << "'";
	if(mysql->storeQuery(query, result)){
		for(int i=0; i < result.getNumRows(); ++i){
			uint32_t lookType, addon = 0;
			lookType  = result.getDataInt("outfit",i);
			addon = result.getDataInt("addon",i);            

			player->addAddon(lookType, addon);
		}            
	}
	#endif

	#ifdef __XID_LEARN_SPELLS__
	query << "SELECT spell FROM spells WHERE player='" << player->getGUID() << "'";
	if(mysql->storeQuery(query, result)){
		for(int i = 0; i < result.getNumRows(); ++i){
			player->learnSpell(result.getDataString("spell", i));
		}
	}
	else{
		query.reset();
	}
	#endif

	#ifdef __JD_DEATH_LIST__
	query << "SELECT * FROM deathlist WHERE player='" << player->getGUID() << "'";
	if(mysql->storeQuery(query, result)){
        for(int i=0; i < result.getNumRows(); ++i){
            Death death;
            death.level  = result.getDataInt("level",i);
            death.time   = result.getDataInt("date",i);            
            death.killer = result.getDataString("killer",i);            
			player->deathList.push_back(death);            
		}            
	}
	#endif

	#ifdef __XID_BLESS_SYSTEM__
	query << "SELECT * FROM blessings WHERE player='" << player->getGUID() << "'";
	if(mysql->storeQuery(query, result)){
		for(int i=0; i < result.getNumRows(); ++i){
			player->addBlessing(result.getDataInt("id",i));
		}            
	}
	#endif

	return true;
}

bool IOPlayerSQL::savePlayer(Player* player)
{
	player->preSave();

	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}


	//check if the player have to be saved or not
	query << "SELECT save FROM players WHERE id='" << player->getGUID() << "'";
	if(!mysql->storeQuery(query,result) || (result.getNumRows() != 1) )
		return false;

	if(result.getDataInt("save") != 1) // If save var is not 1 don't save the player info
		return true;

	DBTransaction trans(mysql);
	if(!trans.start())
		return false;

	//First, an UPDATE query to write the player itself
	query << "UPDATE `players` SET ";
	query << "`level` = " << player->level << ", ";
	query << "`vocation` = " << (int)player->getVocationId() << ", ";
	query << "`health` = " << player->health << ", ";
	query << "`healthmax` = " << player->healthMax << ", ";
	
	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition){
		query << "`food` = " << condition->getTicks() / 1000 << ", ";
	}
	else{
		query << "`food` = " << 0 << ", ";
	}
	
	query << "`soul` = " << player->soul << ", ";
	query << "`soulmax` = " << player->soulMax << ", ";
	query << "`direction` = " << (int)player->getDirection() << ", ";
	query << "`experience` = " << player->experience << ", ";
	query << "`lookbody` = " << (int)player->defaultOutfit.lookBody << ", ";
	query << "`lookfeet` = " << (int)player->defaultOutfit.lookFeet << ", ";
	query << "`lookhead` = " << (int)player->defaultOutfit.lookHead << ", ";
	query << "`looklegs` = " << (int)player->defaultOutfit.lookLegs << ", ";
	query << "`looktype` = " << (int)player->defaultOutfit.lookType << ", ";
	query << "`lookaddons` = " << (int)player->defaultOutfit.lookAddons << ", ";
	query << "`maglevel` = " << player->magLevel << ", ";
	query << "`mana` = " << player->mana << ", ";
	query << "`manamax` = " << player->manaMax << ", ";
	query << "`manaspent` = " << player->manaSpent << ", ";
	query << "`masterpos` = '" << player->masterPos.x<<";"<< player->masterPos.y<<";"<< player->masterPos.z << "', ";
	query << "`pos` = '" << player->getLoginPosition().x<<";"<< player->getLoginPosition().y<<";"<< player->getLoginPosition().z << "', ";
	query << "`speed` = " << player->baseSpeed << ", ";
	query << "`cap` = " << player->getCapacity() << ", ";
	query << "`sex` = " << player->sex << ", ";
	query << "`lastlogin` = " << player->lastlogin << ", ";
	query << "`lastip` = " << player->lastip << " ";

	#ifdef __SKULLSYSTEM__
	long redSkullTime = 0;
	if(player->redSkullTicks > 0){
		redSkullTime = std::time(NULL) + player->redSkullTicks/1000;
	}

	query << ", `redskulltime` = " << redSkullTime << ", ";
	long redSkull = 0;
	if(player->skull == SKULL_RED){
		redSkull = 1;
	}

	query << "`redskull` = " << redSkull << " ";
	#endif

	query << " WHERE `id` = "<< player->getGUID()
	#ifndef __USE_SQLITE__
	<<" LIMIT 1";
    #else
    ;
    #endif

	if(!mysql->executeQuery(query))
		return false;

	// Saving Skills
	for(int i = 0; i <= 6; i++){
		query << "UPDATE `skills` SET `skill` = " << player->skills[i][SKILL_LEVEL] <<", `tries` = "<< player->skills[i][SKILL_TRIES] << " WHERE `player` = " << player->getGUID() << " AND  `id` = " << i;

		if(!mysql->executeQuery(query))
			return false;
	}

	//now item saving
	query << "DELETE FROM items WHERE player='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query))
		return false;

	DBSplitInsert query_insert(mysql);
	query_insert.setQuery("INSERT INTO `items` (`player` , `slot` , `sid` , `pid` , `type` , `number` , `actionid` , `duration` , `charges` , `decaystate` , `text` , `specialdesc` ) VALUES ");
	
	int runningID = 0;

	typedef std::pair<Container*, int> containerStackPair;
	std::list<containerStackPair> stack;
	Item* item = NULL;
	Container* container = NULL;
	Container* topcontainer = NULL;

	int parentid = 0;
	std::stringstream streamitems;
	
	for(int slotid = 1; slotid <= 10; ++slotid){
		if(!player->inventory[slotid])
			continue;

		item = player->inventory[slotid];
		++runningID;

		streamitems << "(" << player->getGUID() <<"," << slotid << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," << (int)item->getActionId()<<"," <<
			(int)item->getDuration()<<"," << (int)item->getItemCharge()<<"," << (int)item->getDecaying()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"')";
		
		if(!query_insert.addRow(streamitems.str()))
			return false;
		
		streamitems.str("");
        
		topcontainer = item->getContainer();
		if(topcontainer) {
			stack.push_back(containerStackPair(topcontainer, runningID));
		}
	}

	while(stack.size() > 0) {
		
		containerStackPair csPair = stack.front();
		container = csPair.first;
		parentid = csPair.second;
		stack.pop_front();

		for(uint32_t i = 0; i < container->size(); i++){
			++runningID;
			Item* item = container->getItem(i);
			Container* container = item->getContainer();
			if(container){
				stack.push_back(containerStackPair(container, runningID));
			}

			streamitems << "(" << player->getGUID() <<"," << 0 /*slotid*/ << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," << (int)item->getActionId()<<"," <<
				(int)item->getDuration()<<"," << (int)item->getItemCharge()<<"," << (int)item->getDecaying()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"')";
		
			if(!query_insert.addRow(streamitems.str()))
				return false;
			
			streamitems.str("");	
		}
	}

	parentid = 0;
	//save depot items
	for(DepotMap::reverse_iterator dit = player->depots.rbegin(); dit !=player->depots.rend() ;++dit){
		item = dit->second;
		++runningID;

			
		streamitems << "(" << player->getGUID() <<"," << dit->first + 100 << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," << (int)item->getActionId()<<"," <<
			(int)item->getDuration()<<"," << (int)item->getItemCharge()<<"," << (int)item->getDecaying()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"')";

		if(!query_insert.addRow(streamitems.str()))
			return false;
		
		streamitems.str("");

		topcontainer = item->getContainer();
		if(topcontainer){
			stack.push_back(containerStackPair(topcontainer, runningID));
		}
	}

	while(stack.size() > 0) {

		containerStackPair csPair = stack.front();
		container = csPair.first;
		parentid = csPair.second;
		stack.pop_front();

		for(uint32_t i = 0; i < container->size(); i++){
			++runningID;
			Item* item = container->getItem(i);
			Container* container = item->getContainer();
			if(container) {
				stack.push_back(containerStackPair(container, runningID));
			}

			streamitems << "(" << player->getGUID() <<"," << /*slotid*/ 0 << ","<< runningID <<","<< parentid <<"," << item->getID()<<","<< (int)item->getItemCountOrSubtype() << "," << (int)item->getActionId()<<"," <<
			(int)item->getDuration()<<"," << (int)item->getItemCharge()<<"," << (int)item->getDecaying()<<",'"<< Database::escapeString(item->getText()) <<"','" << Database::escapeString(item->getSpecialDescription()) <<"')";

			if(!query_insert.addRow(streamitems.str()))
				return false;

            streamitems.str("");
		}
	}
	if(!query_insert.executeQuery())
		return false;

	//save storage map
	query.reset();
	query << "DELETE FROM playerstorage WHERE player='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query))
		return false;

	query_insert.setQuery("INSERT INTO `playerstorage` (`player` , `key` , `value` ) VALUES ");
	player->genReservedStorageRange();
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++){
		streamitems << "(" << player->getGUID() <<","<< cit->first <<","<< cit->second<<")";
		
		if(!query_insert.addRow(streamitems.str()))
			return false;
		
		streamitems.str("");
	}
	if(!query_insert.executeQuery())
		return false;
	
	if(!saveVIP(player)){
		return false;
	}

	#ifdef __XID_SEPERATE_ADDONS__
	query.reset();
	query << "DELETE FROM `addons` WHERE player='" << player->getGUID() << "'";

	if(!mysql->executeQuery(query))
		return false;    

	streamitems.str("");
	query_insert.setQuery("INSERT INTO `addons` (`player`, `outfit`, `addon`) VALUES ");
	for(KnowAddonMap::const_iterator it = player->getKnowAddonBegin(); it != player->getKnowAddonEnd(); it++){
		streamitems << "(" << player->getGUID() << ", " << it->first << ", " << it->second << ")";
		
		if(!query_insert.addRow(streamitems.str()))
			return false;

		streamitems.str("");
	}    

	if(!query_insert.executeQuery())
		return false;
    #endif
	
    #ifdef __XID_LEARN_SPELLS__
	query.reset();
	query << "DELETE FROM `spells` WHERE player='"<< player->getGUID() << "'";

	if(!mysql->executeQuery(query))
		return false;

	streamitems.str("");
	query_insert.setQuery("INSERT INTO `spells` (`player` , `spell` ) VALUES ");
	for(StringVector::iterator it = player->learnedSpells.begin(); it != player->learnedSpells.end(); it++){
		streamitems << "(" << player->getGUID() << ", '"<< *it <<"')";

		if(!query_insert.addRow(streamitems.str()))
			return false;
		
		streamitems.str("");
	}
	
	if(!query_insert.executeQuery())
		return false;
	#endif

	#ifdef __JD_DEATH_LIST__
	query.reset();
	query << "DELETE FROM `deathlist` WHERE player='" << player->getGUID() << "'";

	if(!mysql->executeQuery(query))
		return false;    

	streamitems.str("");
	query_insert.setQuery("INSERT INTO `deathlist` (`player`, `killer`, `level`, `date`) VALUES ");
	for(std::list<Death>::iterator it = player->deathList.begin(); it != player->deathList.end(); it++){
		streamitems << "(" << player->getGUID() << ", '" << (*it).killer << "', " << (*it).level << ", " << (*it).time << ")";
		
		if(!query_insert.addRow(streamitems.str()))
			return false;

		streamitems.str("");
	}    

	if(!query_insert.executeQuery())
		return false;
    #endif

	#ifdef __XID_BLESS_SYSTEM__
	query.reset();
	query << "DELETE FROM `blessings` WHERE player='" << player->getGUID() << "'";

	if(!mysql->executeQuery(query))
		return false;    

	streamitems.str("");
	query_insert.setQuery("INSERT INTO `blessings` (`player`, `id`) VALUES ");
	for(int i=0; i<6; i++){
		if(player->getBlessing(i)){
			streamitems << "(" << player->getGUID() << ", " << i << ")";
		
			if(!query_insert.addRow(streamitems.str()))
				return false;

			streamitems.str("");
		}
	}

	if(!query_insert.executeQuery())
		return false;
    #endif
    
	//End the transaction
	return trans.success();
}

std::string IOPlayerSQL::getItems(Item* i, int &startid, int slot, int player,int parentid)
{
	++startid;
	std::stringstream ss;

	ss << "(" << player <<"," << slot << ","<< startid <<","<< parentid <<"," << i->getID()<<","<< (int)i->getItemCountOrSubtype() << "," << (int)i->getActionId()<<"," <<
		(int)i->getDuration()<<"," << (int)i->getItemCharge()<<"," << (int)i->getDecaying()<<",'"<< Database::escapeString(i->getText()) <<"','" << Database::escapeString(i->getSpecialDescription()) <<"'),";

	if(Container* c = i->getContainer()){
		int pid = startid;
		for(uint32_t i = 0; i < c->size(); i++){
			Item* item = c->getItem(i);
			ss << getItems(item, startid, 0, player, pid);
		}
	}

	return ss.str();
}

bool IOPlayerSQL::loadVIP(Player* player)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;

	query << "SELECT player FROM viplist WHERE account='" << player->getAccount() << "'";		
	if(mysql->storeQuery(query,result)){
		for(uint32_t i = 0; i < result.getNumRows(); ++i){
			player->vip[i] = result.getDataString("player",i);
		}
	}
	else{
		query.reset();
	}
	
	return true;
}

bool IOPlayerSQL::saveVIP(Player* player) 
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;

	query << "DELETE FROM `viplist` WHERE account='" << player->getAccount() << "'";
	if(!mysql->executeQuery(query))
		return false;    

	DBSplitInsert query_insert(mysql);
	std::stringstream ss;
	query_insert.setQuery("INSERT INTO `viplist` (`account`, `player`) VALUES ");
	for(int i = 0; i < MAX_VIPS; i++){
		if(!player->vip[i].empty()){
			ss << "(" << player->getAccount() <<",'"<< player->vip[i] <<"')";
			
			if(!query_insert.addRow(ss.str())){
				return false;
			}
		
			ss.str("");
		}
	}    

	if(!query_insert.executeQuery())
		return false;
	
	return true;
}

bool IOPlayerSQL::playerExists(std::string name)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}

	query << "SELECT name FROM players WHERE name='" << Database::escapeString(name) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	return true;
}

bool IOPlayerSQL::reportMessage(const std::string& message) 
{
	Database* mysql = Database::instance();
	DBSplitInsert query_insert(mysql);
	
	time_t currentTime = std::time(NULL);
	std::string reportDate = ctime(&currentTime);

	std::stringstream ss;	
	query_insert.setQuery("INSERT INTO `reports` (`id`, `date`, `report`) VALUES");
	ss << "(" << 0 << ", '" << Database::escapeString(reportDate) << "', '" << Database::escapeString(message) << "')";
	
	if(!query_insert.addRow(ss.str())){
		return false;
	}

	if(!query_insert.executeQuery())
		return false;
	
	return true;
}

#ifdef __XID_ACCOUNT_MANAGER__
bool IOPlayerSQL::createPlayer(Player* player)
{
	Database* mysql = Database::instance();
	if(!mysql->connect()){
		return false;
	}
	
	DBQuery query;
	query << "INSERT INTO `players` (`id`, `name`, `account`) VALUES ('" << 0 << "', '" << player->getName() << "', '" << player->getAccount() << "')";
	
	if(!mysql->executeQuery(query)){
		return false;
	}
	
	getPlayerGuid(player);

	return true;
}

bool IOPlayerSQL::getPlayerGuid(Player* player)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}
	
	query << "SELECT * FROM players WHERE name='" << Database::escapeString(player->getName()) << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	player->setGUID((int)result.getDataInt("id"));	

	return true;
}
#endif
