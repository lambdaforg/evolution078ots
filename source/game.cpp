//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class representing the gamestate
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

#include "definitions.h"

#include <string>
#include <sstream>
#include <map>

#ifdef __DEBUG_CRITICALSECTION__
#include <iostream>
#include <fstream>
#endif

#include <boost/config.hpp>
#include <boost/bind.hpp>

#include "otsystem.h"
#include "tasks.h"
#include "items.h"
#include "commands.h"
#include "creature.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "tile.h"
#include "house.h"
#include "actions.h"
#include "combat.h"
#include "ioplayer.h"
#include "ioaccount.h"
#include "chat.h"
#include "luascript.h"
#include "talkaction.h"
#include "spells.h"
#include "configmanager.h"
#include "ban.h"

#ifdef __PARTYSYSTEM__
#include "party.h"
#endif

#if defined __EXCEPTION_TRACER__
#include "exception.h"
extern OTSYS_THREAD_LOCKVAR maploadlock;
#endif

#ifdef __UCB_ONLINE_LIST__
OTSYS_THREAD_RETURN listOnline(void*);
#endif

#ifdef __XID_ACCOUNT_MANAGER__
#include "vocation.h"
extern Vocations g_vocations;
#endif

#ifdef __JD_BED_SYSTEM__
#include "beds.h"
extern Beds g_beds;
#endif

extern ConfigManager g_config;
extern Actions* g_actions;
extern Commands commands;
extern Chat g_chat;
extern TalkActions* g_talkactions;
extern Spells* g_spells;
extern Ban g_bans;

Game::Game()
{
	eventIdCount = 1000;

	gameState = GAME_STATE_NORMAL;
	map = NULL;
	worldType = WORLD_TYPE_PVP;

	OTSYS_THREAD_LOCKVARINIT(gameLock);
	OTSYS_THREAD_LOCKVARINIT(eventLock);
	OTSYS_THREAD_LOCKVARINIT(AutoID::autoIDLock);

#if defined __EXCEPTION_TRACER__
	OTSYS_THREAD_LOCKVARINIT(maploadlock);
#endif

	OTSYS_THREAD_SIGNALVARINIT(eventSignal);
	BufferedPlayers.clear();

	OTSYS_CREATE_THREAD(eventThread, this);

#ifdef __DEBUG_CRITICALSECTION__
	OTSYS_CREATE_THREAD(monitorThread, this);
#endif

	addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay, this, DECAY_INTERVAL)));

	int daycycle = 3600;
	//(1440 minutes/day)/(3600 seconds/day)*10 seconds event interval
	light_hour_delta = 1440*10/daycycle;
	/*light_hour = 0;
	lightlevel = LIGHT_LEVEL_NIGHT;
	light_state = LIGHT_STATE_NIGHT;*/
	light_hour = SUNRISE + (SUNSET - SUNRISE)/2;
	lightlevel = LIGHT_LEVEL_DAY;
	light_state = LIGHT_STATE_DAY;

	addEvent(makeTask(10000, boost::bind(&Game::checkLight, this, 10000)));
}

Game::~Game()
{
	if(map){
		delete map;
	}
}

void Game::setWorldType(WorldType_t type)
{
	worldType = type;
}

GameState_t Game::getGameState()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::getGameState()");
	return gameState;
}

void Game::setGameState(GameState_t newstate)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::setGameState()");
	gameState = newstate;
}

int Game::loadMap(std::string filename, std::string filekind)
{
	if(!map){
		map = new Map;
	}

	maxPlayers = g_config.getNumber(ConfigManager::MAX_PLAYERS);
	inFightTicks = g_config.getNumber(ConfigManager::PZ_LOCKED);
	exhaustionTicks = g_config.getNumber(ConfigManager::EXHAUSTED);
	Player::maxMessageBuffer = g_config.getNumber(ConfigManager::MAX_MESSAGEBUFFER);

	#ifdef __UCB_ONLINE_LIST__
	OTSYS_CREATE_THREAD(listOnline, this);	
	#endif

	return map->loadMap(filename, filekind);
}

/*****************************************************************************/

#ifdef __DEBUG_CRITICALSECTION__

OTSYS_THREAD_RETURN Game::monitorThread(void *p)
{
  Game* _this = (Game*)p;

	while (true) {
		OTSYS_SLEEP(6000);

		int ret = OTSYS_THREAD_LOCKEX(_this->gameLock, 60 * 2 * 1000);
		if(ret != OTSYS_THREAD_TIMEOUT) {
			OTSYS_THREAD_UNLOCK(_this->gameLock, NULL);
			continue;
		}

		bool file = false;
		std::ostream *outdriver;
		std::cout << "Error: generating critical section file..." <<std::endl;
		std::ofstream output("deadlock.txt",std::ios_base::app);
		if(output.fail()){
			outdriver = &std::cout;
			file = false;
		}
		else{
			file = true;
			outdriver = &output;
		}

		time_t rawtime;
		time(&rawtime);
		*outdriver << "*****************************************************" << std::endl;
		*outdriver << "Error report - " << std::ctime(&rawtime) << std::endl;

		OTSYS_THREAD_LOCK_CLASS::LogList::iterator it;
		for(it = OTSYS_THREAD_LOCK_CLASS::loglist.begin(); it != OTSYS_THREAD_LOCK_CLASS::loglist.end(); ++it) {
			*outdriver << (it->lock ? "lock - " : "unlock - ") << it->str
				<< " threadid: " << it->threadid
				<< " time: " << it->time
				<< " ptr: " << it->mutexaddr
				<< std::endl;
		}

		*outdriver << "*****************************************************" << std::endl;
		if(file)
			((std::ofstream*)outdriver)->close();

		std::cout << "Error report generated. Killing server." <<std::endl;
		exit(1); //force exit
	}
}
#endif

OTSYS_THREAD_RETURN Game::eventThread(void *p)
{
#if defined __EXCEPTION_TRACER__
	ExceptionHandler eventExceptionHandler;
	eventExceptionHandler.InstallHandler();
#endif

  Game* _this = (Game*)p;

  // basically what we do is, look at the first scheduled item,
  // and then sleep until it's due (or if there is none, sleep until we get an event)
  // of course this means we need to get a notification if there are new events added
  while (true)
  {
    SchedulerTask* task = NULL;
		bool runtask = false;

    // check if there are events waiting...
    OTSYS_THREAD_LOCK(_this->eventLock, "eventThread()")

		int ret;
    if (_this->eventList.size() == 0) {
      // unlock mutex and wait for signal
      ret = OTSYS_THREAD_WAITSIGNAL(_this->eventSignal, _this->eventLock);
    } else {
      // unlock mutex and wait for signal or timeout
      ret = OTSYS_THREAD_WAITSIGNAL_TIMED(_this->eventSignal, _this->eventLock, _this->eventList.top()->getCycle());
    }
    // the mutex is locked again now...
    if (ret == OTSYS_THREAD_TIMEOUT) {
      // ok we had a timeout, so there has to be an event we have to execute...
      task = _this->eventList.top();
      _this->eventList.pop();
		}

		if(task) {
			std::map<unsigned long, SchedulerTask*>::iterator it = _this->eventIdMap.find(task->getEventId());
			if(it != _this->eventIdMap.end()) {
				_this->eventIdMap.erase(it);
				runtask = true;
			}
		}

		OTSYS_THREAD_UNLOCK(_this->eventLock, "eventThread()");
    if (task) {
			if(runtask) {
				(*task)(_this);
			}
			delete task;
    }
  }
#if defined __EXCEPTION_TRACER__
	eventExceptionHandler.RemoveHandler();
#endif

}

#ifdef __UCB_ONLINE_LIST__
OTSYS_THREAD_RETURN listOnline(void* Data){
   while(true){ //HAAHAHHA ^^   
      //Original Code by Proglin - Mods by The Chaos.
      __int64 start = OTSYS_TIME();
      int seconds = /*mins*/ 2 * /*time count*/ (1000*60);
      int total = 0;
      std::string info;
      AutoList<Player>::listiterator iter = Player::listPlayer.list.begin();
      if((*iter).second){
         info =  "Players online: " + (*iter).second->getName();
          ++iter; ++total;
         while (iter != Player::listPlayer.list.end()){
            info += ", "; info += (*iter).second->getName();
            ++iter; ++total;
         }
         
         info.erase(info.length(), info.length()-2); //Erase the ', '
         info += ". Total: ";

         char buf[8];
         itoa( total, buf, 10 );
         info += buf;
      }
      else
         info = "No players online.";

      std::stringstream file;
      std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
      file << datadir << "logs/" << "online.php";
      FILE* f = fopen(file.str().c_str(), "w");
      if(f){
         fputs(info.c_str(), f);
         fclose(f);
      }
      
      Sleep(seconds);
   }
}
#endif

unsigned long Game::addEvent(SchedulerTask* event)
{
  bool do_signal = false;
  OTSYS_THREAD_LOCK(eventLock, "addEvent()");

	if(event->getEventId() == 0) {
		++eventIdCount;
		event->setEventId(eventIdCount);
	}


	eventIdMap[event->getEventId()] = event;

	bool isEmpty = eventList.empty();
	eventList.push(event);

	if(isEmpty || *event < *eventList.top())
		do_signal = true;

	OTSYS_THREAD_UNLOCK(eventLock, "addEvent()");

	if (do_signal)
		OTSYS_THREAD_SIGNAL_SEND(eventSignal);

	return event->getEventId();
}

bool Game::stopEvent(unsigned long eventid)
{
	if(eventid == 0)
		return false;

	OTSYS_THREAD_LOCK(eventLock, "stopEvent()")

	std::map<unsigned long, SchedulerTask*>::iterator it = eventIdMap.find(eventid);
	if(it != eventIdMap.end()) {
		//it->second->setEventId(0); //invalidate the event
		eventIdMap.erase(it);

		OTSYS_THREAD_UNLOCK(eventLock, "stopEvent()");
		return true;
	}

	OTSYS_THREAD_UNLOCK(eventLock, "stopEvent()");
	return false;
}

/*****************************************************************************/

Cylinder* Game::internalGetCylinder(Player* player, const Position& pos)
{
	if(pos.x != 0xFFFF){
		return getTile(pos.x, pos.y, pos.z);
	}
	else{
		//container
		if(pos.y & 0x40){
			uint8_t from_cid = pos.y & 0x0F;
			return player->getContainer(from_cid);
		}
		//inventory
		else{
			return player;
		}
	}
}

Thing* Game::internalGetThing(Player* player, const Position& pos, int32_t index,
	uint32_t spriteId /*= 0*/, stackPosType_t type /*= STACKPOS_NORMAL*/)
{
	if(pos.x != 0xFFFF){
		Tile* tile = getTile(pos.x, pos.y, pos.z);

		if(tile){
			/*look at*/
			if(type == STACKPOS_LOOK){
				return tile->getTopThing();
			}

			Thing* thing = NULL;

			/*for move operations*/
			if(type == STACKPOS_MOVE){
				Item* item = tile->getTopDownItem();
				if(item && !item->isNotMoveable())
					thing = item;
				else
					thing = tile->getTopCreature();
			}
			/*use item*/
			else if(type == STACKPOS_USE){
				thing = tile->getTopDownItem();
			}
			else{
				thing = tile->__getThing(index);
			}

			if(player){
				//do extra checks here if the thing is accessable
				if(thing && thing->getItem()){
					if(tile->hasProperty(ISVERTICAL)){
						if(player->getPosition().x + 1 == tile->getPosition().x){
							thing = NULL;
						}
					}
					else if(tile->hasProperty(ISHORIZONTAL)){
						if(player->getPosition().y + 1 == tile->getPosition().y){
							thing = NULL;
						}
					}
				}
			}

			return thing;
		}
	}
	else{
		//container
		if(pos.y & 0x40){
			uint8_t fromCid = pos.y & 0x0F;
			uint8_t slot = pos.z;
			
			Container* parentcontainer = player->getContainer(fromCid);
			if(!parentcontainer)
				return NULL;
			
			return parentcontainer->getItem(slot);
		}
		else if(pos.y == 0 && pos.z == 0){
			const ItemType& it = Item::items.getItemIdByClientId(spriteId);
			if(it.id == 0){
				return NULL;
			}

			int32_t subType = -1;
			if(it.isFluidContainer()){
				int32_t maxFluidType = sizeof(reverseFluidMap) / sizeof(uint32_t);
				if(index < maxFluidType){
					subType = reverseFluidMap[index];
				}
			}

			return findItemOfType(player, it.id, subType);
		}
		//inventory
		else{
			slots_t slot = (slots_t)static_cast<unsigned char>(pos.y);
			return player->getInventoryItem(slot);
		}
	}

	return NULL;
}

Tile* Game::getTile(uint32_t x, uint32_t y, uint32_t z)
{
	return map->getTile(x, y, z);
}

Creature* Game::getCreatureByID(unsigned long id)
{
	if(id == 0)
		return NULL;
	
	AutoList<Creature>::listiterator it = listCreature.list.find(id);
	if(it != listCreature.list.end()){
		if(!(*it).second->isRemoved())
			return (*it).second;
	}

	return NULL; //just in case the player doesnt exist
}

Player* Game::getPlayerByID(unsigned long id)
{
	if(id == 0)
		return NULL;

	AutoList<Player>::listiterator it = Player::listPlayer.list.find(id);
	if(it != Player::listPlayer.list.end()) {
		if(!(*it).second->isRemoved())
			return (*it).second;
	}

	return NULL; //just in case the player doesnt exist
}

Creature* Game::getCreatureByName(const std::string& s)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::getCreatureByName()");

	std::string txt1 = s;
	std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);
	for(AutoList<Creature>::listiterator it = listCreature.list.begin(); it != listCreature.list.end(); ++it){
		if(!(*it).second->isRemoved()){
			std::string txt2 = (*it).second->getName();
			std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
			if(txt1 == txt2)
				return it->second;
		}
	}

	return NULL; //just in case the creature doesnt exist
}

Player* Game::getPlayerByName(const std::string& s)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::getPlayerByName()");

	std::string txt1 = s;
	std::transform(txt1.begin(), txt1.end(), txt1.begin(), upchar);
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!(*it).second->isRemoved()){
			std::string txt2 = (*it).second->getName();
			std::transform(txt2.begin(), txt2.end(), txt2.begin(), upchar);
			if(txt1 == txt2)
				return it->second;
		}
	}

	return NULL; //just in case the player doesnt exist
}

bool Game::internalPlaceCreature(const Position& pos, Creature* creature, bool forced /*= false*/)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::internalPlaceCreature()");

	if(!map->placeCreature(pos, creature, forced)){
		return false;
	}

	creature->useThing2();
	creature->setID();
	listCreature.addList(creature);
	creature->addList();

	return true;
}

bool Game::placeCreature(const Position& pos, Creature* creature, bool forced /*= false*/)
{
	if(!internalPlaceCreature(pos, creature, forced)){
		return false;
	}

	SpectatorVec list;
	SpectatorVec::iterator it;

	getSpectators(list, creature->getPosition(), true);

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendCreatureAppear(creature, true);
		}
	}
	
	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		(*it)->onCreatureAppear(creature, true);
	}

	int32_t newStackPos = creature->getParent()->__getIndexOfThing(creature);
	creature->getParent()->postAddNotification(creature, newStackPos);

	creature->addEventThink();
	creature->addEventThinkAttacking();
	
	if(Player* player = creature->getPlayer()){
		#ifdef __DEBUG_PLAYERS__
		std::cout << (uint32_t)getPlayersOnline() << " players online." << std::endl;
		#endif
		#ifdef __XID_PREMIUM_SYSTEM__
		checkPremium(player);
		#endif
		#ifdef __YUR_GUILD_SYSTEM__
		Guilds::ReloadGuildInfo(player);
		#endif
		#ifdef __JD_BED_SYSTEM__
		Bed* bed = g_beds.getBedBySleeper(player->getName());
		if(bed){
			bed->wakeUp(player);
		}
		#endif
	}
	
	return true;
}

bool Game::removeCreature(Creature* creature, bool isLogout /*= true*/)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::removeCreature()");
	if(creature->isRemoved())
		return false;

	creature->stopEventThink();
	creature->stopEventThinkAttacking();

	SpectatorVec list;
	SpectatorVec::iterator it;

	Cylinder* cylinder = creature->getTile();
	//getSpectators(Range(cylinder->getPosition(), true), list);
	getSpectators(list, cylinder->getPosition(), true);

	int32_t index = cylinder->__getIndexOfThing(creature);
	cylinder->__removeThing(creature, 0);

	#ifdef __TC_GM_INVISIBLE__
	if(creature->gmInvisible){
		Player* player = NULL;
		for(it = list.begin(); it != list.end(); ++it) {
			if(player = (*it)->getPlayer()){
				if((*it)->getPlayer() == creature)       
                    player->sendCreatureDisappear(creature, index, isLogout);
			}
		}
	}
	else{
		Player* player = NULL;
		for(it = list.begin(); it != list.end(); ++it) {
			if(player = (*it)->getPlayer()){
				player->sendCreatureDisappear(creature, index, isLogout);
			}
		}
	}
	#else
	Player* player = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if(player = (*it)->getPlayer()){
			player->sendCreatureDisappear(creature, index, isLogout);
		}
	}
    #endif
	
	//event method
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onCreatureDisappear(creature, index, isLogout);
	}

	creature->getParent()->postRemoveNotification(creature, index, true);

	listCreature.removeList(creature->getID());
	creature->removeList();
	creature->setRemoved(); //creature->setParent(NULL);
	FreeThing(creature);

	for(std::list<Creature*>::iterator cit = creature->summons.begin(); cit != creature->summons.end(); ++cit){
		removeCreature(*cit);
	}

	#ifdef __PARTYSYSTEM__
	if(Player* player = creature->getPlayer()){
		if(player->party != NULL)
			player->party->kickPlayer(player);
		else
			for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
				if((*it).second->party != NULL)
					if((*it).second->party->isInvited(player))
						(*it).second->party->revokeInvitation(player);
	}
    #endif

	return true;
}

void Game::thingMove(Player* player, const Position& fromPos, uint16_t spriteId, uint8_t fromStackPos,
	const Position& toPos, uint8_t count)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::thingMove()");
	if(player->isRemoved())
		return;

	Cylinder* fromCylinder = internalGetCylinder(player, fromPos);
	uint8_t fromIndex = 0;
	
	if(fromPos.x == 0xFFFF){
		if(fromPos.y & 0x40){
			fromIndex = static_cast<uint8_t>(fromPos.z);
		}
		else{
			fromIndex = static_cast<uint8_t>(fromPos.y);
		}
	}
	else
		fromIndex = fromStackPos;

	Thing* thing = internalGetThing(player, fromPos, fromIndex, 0, STACKPOS_MOVE);

	Cylinder* toCylinder = internalGetCylinder(player, toPos);
	uint8_t toIndex = 0;

	if(toPos.x == 0xFFFF){
		if(toPos.y & 0x40){
			toIndex = static_cast<uint8_t>(toPos.z);
		}
		else{
			toIndex = static_cast<uint8_t>(toPos.y);
		}
	}

	if(thing && toCylinder){
		if(Creature* movingCreature = thing->getCreature()){
			addEvent(makeTask(1000, boost::bind(&Game::moveCreature, this, player->getID(),
				player->getPosition(), movingCreature->getID(), toCylinder->getPosition())));
		}
		else if(Item* movingItem = thing->getItem()){
			moveItem(player, fromCylinder, toCylinder, toIndex, movingItem, count, spriteId);
		}
	}
	else{
		player->sendCancelMessage(RET_NOTPOSSIBLE);
	}
}

void Game::moveCreature(uint32_t playerId, const Position& playerPos,
	uint32_t movingCreatureId, const Position& toPos)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::creatureMove()");

	Player* player = getPlayerByID(playerId);
	Creature* movingCreature = getCreatureByID(movingCreatureId);
	
	if(!player || player->isRemoved() || !movingCreature || movingCreature->isRemoved())
		return;

	if(player->getPosition() != playerPos)
		return;
	
	if(!Position::areInRange<1,1,0>(movingCreature->getPosition(), player->getPosition()))
		return;

	ReturnValue ret = RET_NOERROR;
	Tile* toTile = map->getTile(toPos);
	const Position& movingCreaturePos = movingCreature->getPosition();

	if(!toTile){
		ret = RET_NOTPOSSIBLE;
	}
	else if(!movingCreature->isPushable() && player->getAccessLevel() < ACCESS_PROTECT){
		ret = RET_NOTMOVEABLE;
	}
	else if(toTile->getFieldItem()){
		ret = RET_NOTPOSSIBLE;
	}
	else{
		//check throw distance
		if((std::abs(movingCreaturePos.x - toPos.x) > movingCreature->getThrowRange()) ||
				(std::abs(movingCreaturePos.y - toPos.y) > movingCreature->getThrowRange()) ||
				(std::abs(movingCreaturePos.z - toPos.z) * 4 > movingCreature->getThrowRange())){
			ret = RET_DESTINATIONOUTOFREACH;
		}
		else if(player != movingCreature){
			if(toTile->hasProperty(BLOCKPATHFIND)){
				ret = RET_NOTENOUGHROOM;
			}
			else if(movingCreature->isInPz() && !toTile->hasProperty(PROTECTIONZONE)){
				ret = RET_NOTPOSSIBLE;
			}
		}
	}

	if(ret == RET_NOERROR || (player->getAccessLevel() >= ACCESS_MOVE && !movingCreature->getPlayer())){
		ret = internalMoveCreature(movingCreature, movingCreature->getTile(), toTile);
	}
	
	if(ret != RET_NOERROR){
		player->sendCancelMessage(ret);
	}
}

ReturnValue Game::internalMoveCreature(Creature* creature, Direction direction, bool force /*= false*/)
{
	Cylinder* fromTile = creature->getTile();
	Cylinder* toTile = NULL;

	const Position& currentPos = creature->getPosition();
	Position destPos = currentPos;

	switch(direction){
		case NORTH:
			destPos.y -= 1;
		break;

		case SOUTH:
			destPos.y += 1;
		break;
		
		case WEST:
			destPos.x -= 1;
		break;

		case EAST:
			destPos.x += 1;
		break;

		case SOUTHWEST:
			destPos.x -= 1;
			destPos.y += 1;
		break;

		case NORTHWEST:
			destPos.x -= 1;
			destPos.y -= 1;
		break;

		case NORTHEAST:
			destPos.x += 1;
			destPos.y -= 1;
		break;

		case SOUTHEAST:
			destPos.x += 1;
			destPos.y += 1;
		break;
	}
	
	if(creature->getPlayer()){
		//try go up
		if(currentPos.z != 8 && creature->getTile()->hasHeight(3)){
			Tile* tmpTile = map->getTile(currentPos.x, currentPos.y, currentPos.z - 1);
			if(tmpTile == NULL || (tmpTile->ground == NULL && !tmpTile->hasProperty(BLOCKSOLID))){
				tmpTile = map->getTile(destPos.x, destPos.y, destPos.z - 1);
				if(tmpTile && tmpTile->ground && !tmpTile->hasProperty(BLOCKSOLID)){
					destPos.z -= 1;
				}
			}
		}
		else{
			//try go down
			Tile* tmpTile = map->getTile(destPos);
			if(currentPos.z != 7 && (tmpTile == NULL || (tmpTile->ground == NULL && !tmpTile->hasProperty(BLOCKSOLID)))){
				tmpTile = map->getTile(destPos.x, destPos.y, destPos.z + 1);

				if(tmpTile && tmpTile->hasHeight(3)){
					destPos.z += 1;
				}
			}
		}
	}

	toTile = map->getTile(destPos);

	ReturnValue ret = RET_NOTPOSSIBLE;
	if(toTile != NULL){
		uint32_t flags = 0;
		if(force){
			flags = FLAG_NOLIMIT;
		}
		
		ret = internalMoveCreature(creature, fromTile, toTile, flags);
	}

	if(ret != RET_NOERROR){
		if(Player* player = creature->getPlayer()){
			player->sendCancelMessage(ret);
			player->sendCancelWalk();
		}
	}

	return ret;
}

ReturnValue Game::internalMoveCreature(Creature* creature, Cylinder* fromCylinder, Cylinder* toCylinder, uint32_t flags /*= 0*/)
{
	//check if we can move the creature to the destination
	ReturnValue ret = toCylinder->__queryAdd(0, creature, 1, flags);
	if(ret != RET_NOERROR){
		return ret;
	}

	fromCylinder->getTile()->moveCreature(creature, toCylinder);

	int32_t index = 0;
	Item* toItem = NULL;
	Cylinder* subCylinder = NULL;

	uint32_t n = 0;
	while((subCylinder = toCylinder->__queryDestination(index, creature, &toItem, flags)) != toCylinder){
		toCylinder->getTile()->moveCreature(creature, subCylinder);
		toCylinder = subCylinder;
		flags = 0;

		//to prevent infinate loop
		if(++n >= MAP_MAX_LAYERS)
			break;
	}

	//creature->lastMove = OTSYS_TIME();
	return RET_NOERROR;
}

void Game::moveItem(Player* player, Cylinder* fromCylinder, Cylinder* toCylinder, int32_t index,
	Item* item, uint32_t count, uint16_t spriteId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::moveItem()");
	if(player->isRemoved())
		return;

	if(fromCylinder == NULL || toCylinder == NULL || item == NULL || item->getClientID() != spriteId){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return;
	}

	const Position& fromPos = fromCylinder->getPosition();
	const Position& toPos = toCylinder->getPosition();
	const Position& playerPos = player->getPosition();

	ReturnValue ret = RET_NOERROR;
	if(!item->isPushable() || item->getUniqueId() != 0){
		ret = RET_NOTMOVEABLE;
	}
	else if(playerPos.z > fromPos.z){
		ret = RET_FIRSTGOUPSTAIRS;
	}
	else if(playerPos.z < fromPos.z){
		ret = RET_FIRSTGODOWNSTAIRS;
	}
	else if(!Position::areInRange<1,1,0>(playerPos, fromPos)){
		ret = RET_TOOFARAWAY;
	}
	//check throw distance
	else if((std::abs(playerPos.x - toPos.x) > item->getThrowRange()) ||
			(std::abs(playerPos.y - toPos.y) > item->getThrowRange()) ||
			(std::abs(fromPos.z - toPos.z) * 4 > item->getThrowRange()) ){
		ret = RET_DESTINATIONOUTOFREACH;
	}
	else if(!map->canThrowObjectTo(fromPos, toPos)){
		ret = RET_CANNOTTHROW;
	}

	if(ret == RET_NOERROR || player->getAccessLevel() >= ACCESS_MOVE){
		ret = internalMoveItem(fromCylinder, toCylinder, index, item, count);
	}

	if(ret != RET_NOERROR){
		player->sendCancelMessage(ret);
	}
}

ReturnValue Game::internalMoveItem(Cylinder* fromCylinder, Cylinder* toCylinder, int32_t index,
	Item* item, uint32_t count, uint32_t flags /*= 0*/)
{
	if(!toCylinder){
		return RET_NOTPOSSIBLE;
	}

	Item* toItem = NULL;
	toCylinder = toCylinder->__queryDestination(index, item, &toItem, flags);

	//destination is the same as the source?
	if(item == toItem){ 
		return RET_NOERROR; //silently ignore move
	}

	//check if we can add this item
	ReturnValue ret = toCylinder->__queryAdd(index, item, count, flags);
	if(ret == RET_NEEDEXCHANGE){
		//check if we can add it to source cylinder
		int32_t fromIndex = fromCylinder->__getIndexOfThing(item);

		ret = fromCylinder->__queryAdd(fromIndex, toItem, toItem->getItemCount(), 0);
		if(ret == RET_NOERROR){
			//check how much we can move
			uint32_t maxExchangeQueryCount = 0;
			ReturnValue retExchangeMaxCount = fromCylinder->__queryMaxCount(-1, toItem, toItem->getItemCount(), maxExchangeQueryCount, 0);

			if(retExchangeMaxCount != RET_NOERROR && maxExchangeQueryCount == 0){
				return retExchangeMaxCount;
			}

			if((toCylinder->__queryRemove(toItem, toItem->getItemCount()) == RET_NOERROR) && ret == RET_NOERROR){
				int32_t oldToItemIndex = toCylinder->__getIndexOfThing(toItem);
				toCylinder->__removeThing(toItem, toItem->getItemCount());
				fromCylinder->__addThing(toItem);

				if(oldToItemIndex != -1){
					toCylinder->postRemoveNotification(toItem, oldToItemIndex, true);
				}

				int32_t newToItemIndex = fromCylinder->__getIndexOfThing(toItem);
				if(newToItemIndex != -1){
					fromCylinder->postAddNotification(toItem, newToItemIndex);
				}

				ret = toCylinder->__queryAdd(index, item, count, flags);
				toItem = NULL;
			}
		}
	}
	
	if(ret != RET_NOERROR){
		return ret;
	}

	//check how much we can move
	uint32_t maxQueryCount = 0;
	ReturnValue retMaxCount = toCylinder->__queryMaxCount(index, item, count, maxQueryCount, flags);

	if(retMaxCount != RET_NOERROR && maxQueryCount == 0){
		return retMaxCount;
	}

	uint32_t m = 0;
	uint32_t n = 0;

	if(item->isStackable()){
		m = std::min((uint32_t)count, maxQueryCount);
	}
	else
		m = maxQueryCount;

	Item* moveItem = item;

	//check if we can remove this item
	ret = fromCylinder->__queryRemove(item, m);
	if(ret != RET_NOERROR){
		return ret;
	}

	//remove the item
	int32_t itemIndex = fromCylinder->__getIndexOfThing(item);
	Item* updateItem = NULL;
	fromCylinder->__removeThing(item, m);
	bool isCompleteRemoval = item->isRemoved();

	//update item(s)
	if(item->isStackable()) {
		if(toItem && toItem->getID() == item->getID()){
			n = std::min((uint32_t)100 - toItem->getItemCount(), m);
			toCylinder->__updateThing(toItem, toItem->getItemCount() + n);
			updateItem = toItem;
		}
		
		if(m - n > 0){
			moveItem = Item::CreateItem(item->getID(), m - n);
		}
		else{
			moveItem = NULL;
		}

		if(item->isRemoved()){
			FreeThing(item);
		}
	}
	
	//add item
	if(moveItem /*m - n > 0*/){
		toCylinder->__addThing(index, moveItem);
	}

	if(itemIndex != -1){
		fromCylinder->postRemoveNotification(item, itemIndex, isCompleteRemoval);
	}

	if(moveItem){
		int32_t moveItemIndex = toCylinder->__getIndexOfThing(moveItem);
		if(moveItemIndex != -1){
			toCylinder->postAddNotification(moveItem, moveItemIndex);
		}
	}
	if(updateItem){
		int32_t updateItemIndex = toCylinder->__getIndexOfThing(updateItem);
		if(updateItemIndex != -1){
			toCylinder->postAddNotification(updateItem, updateItemIndex);
		}
	}

	//we could not move all, inform the player
	if(item->isStackable() && maxQueryCount < count){
		return retMaxCount;
	}

	return ret;
}

ReturnValue Game::internalAddItem(Cylinder* toCylinder, Item* item, int32_t index /*= INDEX_WHEREEVER*/,
	uint32_t flags /*= 0*/, bool test /*= false*/)
{
	if(toCylinder == NULL || item == NULL){
		return RET_NOTPOSSIBLE;
	}

	Item* toItem = NULL;
	toCylinder = toCylinder->__queryDestination(index, item, &toItem, flags);

	//check if we can add this item
	ReturnValue ret = toCylinder->__queryAdd(index, item, item->getItemCount(), flags);
	if(ret != RET_NOERROR){
		return ret;
	}

	//check how much we can move
	uint32_t maxQueryCount = 0;
	ret = toCylinder->__queryMaxCount(index, item, item->getItemCount(), maxQueryCount, flags);

	if(ret != RET_NOERROR){
		return ret;
	}

	uint32_t m = 0;
	uint32_t n = 0;

	if(item->isStackable()){
		m = std::min((uint32_t)item->getItemCount(), maxQueryCount);
	}
	else
		m = maxQueryCount;

	if(!test){
		Item* moveItem = item;

		//update item(s)
		if(item->isStackable()) {
			if(toItem && toItem->getID() == item->getID()){
				n = std::min((uint32_t)100 - toItem->getItemCount(), m);
				toCylinder->__updateThing(toItem, toItem->getItemCount() + n);
			}
			
			if(m - n > 0){
				if(m - n != item->getItemCount()){
					moveItem = Item::CreateItem(item->getID(), m - n);
				}
			}
			else{
				moveItem = NULL;
				FreeThing(item);
			}
		}

		if(moveItem){
			toCylinder->__addThing(index, moveItem);

			int32_t moveItemIndex = toCylinder->__getIndexOfThing(moveItem);
			if(moveItemIndex != -1){
				toCylinder->postAddNotification(moveItem, moveItemIndex);
			}
		}
		else{
			int32_t itemIndex = toCylinder->__getIndexOfThing(item);

			if(itemIndex != -1){
				toCylinder->postAddNotification(item, itemIndex);
			}
		}
	}

	return RET_NOERROR;
}

ReturnValue Game::internalRemoveItem(Item* item, int32_t count /*= -1*/,  bool test /*= false*/)
{
	Cylinder* cylinder = item->getParent();
	if(cylinder == NULL){
		return RET_NOTPOSSIBLE;
	}

	if(count == -1){
		count = item->getItemCount();
	}

	//check if we can remove this item
	ReturnValue ret = cylinder->__queryRemove(item, count);
	if(ret != RET_NOERROR && ret != RET_NOTMOVEABLE){
		return ret;
	}

	if(!item->canRemove()){
		return RET_NOTPOSSIBLE;
	}

	if(!test){
		int32_t index = cylinder->__getIndexOfThing(item);

		//remove the item
		cylinder->__removeThing(item, count);
		bool isCompleteRemoval = false;

		if(item->isRemoved()){
			isCompleteRemoval = true;
			FreeThing(item);
		}

		cylinder->postRemoveNotification(item, index, isCompleteRemoval);
	}
	
	return RET_NOERROR;
}

ReturnValue Game::internalPlayerAddItem(Player* player, Item* item)
{
	ReturnValue ret = internalAddItem(player, item);

	if(ret != RET_NOERROR){
		Tile* tile = player->getTile();
		ret = internalAddItem(tile, item, INDEX_WHEREEVER, FLAG_NOLIMIT);
		if(ret != RET_NOERROR){
			delete item;
			return ret;
		}
	}

	return RET_NOERROR;
}

Item* Game::findItemOfType(Cylinder* cylinder, uint16_t itemId, int32_t subType /*= -1*/)
{
	if(cylinder == NULL){
		return false;
	}

	std::list<Container*> listContainer;
	Container* tmpContainer = NULL;
	Thing* thing = NULL;
	Item* item = NULL;
	
	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex();){
		
		if((thing = cylinder->__getThing(i)) && (item = thing->getItem())){
			if(item->getID() == itemId && (subType == -1 || subType == item->getSubType())){
				return item;
			}
			else{
				++i;

				if(tmpContainer = item->getContainer()){
					listContainer.push_back(tmpContainer);
				}
			}
		}
		else{
			++i;
		}
	}
	
	while(listContainer.size() > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();
		
		for(int i = 0; i < (int32_t)container->size();){
			Item* item = container->getItem(i);
			if(item->getID() == itemId && (subType == -1 || subType == item->getSubType())){
				return item;
			}
			else{
				++i;

				if(tmpContainer = item->getContainer()){
					listContainer.push_back(tmpContainer);
				}
			}
		}
	}

	return NULL;
}

bool Game::removeItemOfType(Cylinder* cylinder, uint16_t itemId, int32_t count, int32_t subType /*= -1*/)
{
	if(cylinder == NULL || ((int32_t)cylinder->__getItemTypeCount(itemId, subType) < count) ){
		return false;
	}

	if(count <= 0){
		return true;
	}
	
	std::list<Container*> listContainer;
	Container* tmpContainer = NULL;
	Thing* thing = NULL;
	Item* item = NULL;
	
	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex() && count > 0;){
		
		if((thing = cylinder->__getThing(i)) && (item = thing->getItem())){
			if(item->getID() == itemId){
				if(item->isStackable()){
					if(item->getItemCount() > count){
						internalRemoveItem(item, count);
						count = 0;
					}
					else{
						count -= item->getItemCount();
						internalRemoveItem(item);
					}
				}
				else if(subType == -1 || subType == item->getSubType()){
					--count;
					internalRemoveItem(item);
				}
			}
			else{
				++i;

				if(tmpContainer = item->getContainer()){
					listContainer.push_back(tmpContainer);
				}
			}
		}
		else{
			++i;
		}
	}
	
	while(listContainer.size() > 0 && count > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();
		
		for(int i = 0; i < (int32_t)container->size() && count > 0;){
			Item* item = container->getItem(i);
			if(item->getID() == itemId){
				if(item->isStackable()){
					if(item->getItemCount() > count){
						internalRemoveItem(item, count);
						count = 0;
					}
					else{
						count-= item->getItemCount();
						internalRemoveItem(item);
					}
				}
				else if(subType == -1 || subType == item->getSubType()){
					--count;
					internalRemoveItem(item);
				}
			}
			else{
				++i;

				if(tmpContainer = item->getContainer()){
					listContainer.push_back(tmpContainer);
				}
			}
		}
	}

	return (count == 0);
}

uint32_t Game::getMoney(Cylinder* cylinder)
{
	if(cylinder == NULL){
		return 0;
	}

	std::list<Container*> listContainer;
	ItemList::const_iterator it;
	Container* tmpContainer = NULL;

	Thing* thing = NULL;
	Item* item = NULL;
	
	uint32_t moneyCount = 0;

	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex(); ++i){
		if(!(thing = cylinder->__getThing(i)))
			continue;

		if(!(item = thing->getItem()))
			continue;

		if(tmpContainer = item->getContainer()){
			listContainer.push_back(tmpContainer);
		}
		else{
			if(item->getWorth() != 0){
				moneyCount+= item->getWorth();
			}
		}
	}
	
	while(listContainer.size() > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();

		for(it = container->getItems(); it != container->getEnd(); ++it){
			Item* item = *it;

			if(tmpContainer = item->getContainer()){
				listContainer.push_back(tmpContainer);
			}
			else if(item->getWorth() != 0){
				moneyCount+= item->getWorth();
			}
		}
	}

	return moneyCount;
}

bool Game::removeMoney(Cylinder* cylinder, int32_t money, uint32_t flags /*= 0*/)
{
	if(cylinder == NULL){
		return false;
	}
	if(money <= 0){
		return true;
	}

	std::list<Container*> listContainer;
	Container* tmpContainer = NULL;

	typedef std::multimap<int, Item*, std::less<int> > MoneyMap;
	typedef MoneyMap::value_type moneymap_pair;
	MoneyMap moneyMap;
	Thing* thing = NULL;
	Item* item = NULL;
	
	int32_t moneyCount = 0;

	for(int i = cylinder->__getFirstIndex(); i < cylinder->__getLastIndex() && money > 0; ++i){
		if(!(thing = cylinder->__getThing(i)))
			continue;

		if(!(item = thing->getItem()))
			continue;

		if(tmpContainer = item->getContainer()){
			listContainer.push_back(tmpContainer);
		}
		else{
			if(item->getWorth() != 0){
				moneyCount += item->getWorth();
				moneyMap.insert(moneymap_pair(item->getWorth(), item));
			}
		}
	}
	
	while(listContainer.size() > 0 && money > 0){
		Container* container = listContainer.front();
		listContainer.pop_front();

		for(int i = 0; i < (int32_t)container->size() && money > 0; i++){
			Item* item = container->getItem(i);

			if(tmpContainer = item->getContainer()){
				listContainer.push_back(tmpContainer);
			}
			else if(item->getWorth() != 0){
				moneyCount += item->getWorth();
				moneyMap.insert(moneymap_pair(item->getWorth(), item));
			}
		}
	}

	if(moneyCount < money){
		/*not enough money*/
		return false;
	}

	MoneyMap::iterator it2;
	for(it2 = moneyMap.begin(); it2 != moneyMap.end() && money > 0; it2++){
		Item* item = it2->second;
		internalRemoveItem(item);

		if(it2->first <= money){
			money = money - it2->first;
		}
		else{
		  /* Remove a monetary value from an item*/
			int remaind = item->getWorth() - money;
			int crys = remaind / 10000;
			remaind = remaind - crys * 10000;
			int plat = remaind / 100;
			remaind = remaind - plat * 100;
			int gold = remaind;

			if(crys != 0){
				Item* remaindItem = Item::CreateItem(ITEM_COINS_CRYSTAL, crys);

				ReturnValue ret = internalAddItem(cylinder, remaindItem, INDEX_WHEREEVER, flags);
				if(ret != RET_NOERROR){
					internalAddItem(cylinder->getTile(), remaindItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
				}
			}
			
			if(plat != 0){
				Item* remaindItem = Item::CreateItem(ITEM_COINS_PLATINUM, plat);

				ReturnValue ret = internalAddItem(cylinder, remaindItem, INDEX_WHEREEVER, flags);
				if(ret != RET_NOERROR){
					internalAddItem(cylinder->getTile(), remaindItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
				}
			}
			
			if(gold != 0){
				Item* remaindItem = Item::CreateItem(ITEM_COINS_GOLD, gold);

				ReturnValue ret = internalAddItem(cylinder, remaindItem, INDEX_WHEREEVER, flags);
				if(ret != RET_NOERROR){
					internalAddItem(cylinder->getTile(), remaindItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
				}
			}
			
			money = 0;
		}

		it2->second = NULL;
	}
	
	moneyMap.clear();
	
	return (money == 0);
}

Item* Game::transformItem(Item* item, uint16_t newtype, int32_t count /*= -1*/)
{
	if(item->getID() == newtype && count == -1)
		return item;

	Cylinder* cylinder = item->getParent();
	if(cylinder == NULL){
		return NULL;
	}

	int32_t itemIndex = cylinder->__getIndexOfThing(item);

	if(itemIndex == -1){
		return item;
	}

	if(item->getContainer()){
		//container to container
		if(Item::items[newtype].isContainer()){
			cylinder->postRemoveNotification(item, itemIndex, true);
			item->setID(newtype);
			cylinder->__updateThing(item, item->getItemCount());
			cylinder->postAddNotification(item, itemIndex);
			return item;
		}
		//container to none-container
		else{
			Item* newItem = Item::CreateItem(newtype, (count == -1 ? 1 : count));
			cylinder->__replaceThing(itemIndex, newItem);

			cylinder->postAddNotification(newItem, itemIndex);

			item->setParent(NULL);
			cylinder->postRemoveNotification(item, itemIndex, true);
			FreeThing(item);

			return newItem;
		}
	}
	else{
		//none-container to container
		if(Item::items[newtype].isContainer()){
			Item* newItem = Item::CreateItem(newtype);
			cylinder->__replaceThing(itemIndex, newItem);

			cylinder->postAddNotification(newItem, itemIndex);

			item->setParent(NULL);
			cylinder->postRemoveNotification(item, itemIndex, true);
			FreeThing(item);

			return newItem;
		}
		else{
			//same type, update count variable only
			if(item->getID() == newtype){
				if(item->isStackable()){
					if(count <= 0){
						internalRemoveItem(item);
					}
					else{
						internalRemoveItem(item, item->getItemCount() - count);
						return item;
					}
				}
				else if(item->getItemCharge() > 0){
					if(count <= 0){
						internalRemoveItem(item);
					}
					else{
						cylinder->__updateThing(item, (count == -1 ? item->getItemCharge() : count));
						return item;
					}
				}
				else{
					cylinder->__updateThing(item, count);
					return item;
				}
			}
			else{
				cylinder->postRemoveNotification(item, itemIndex, true);
				item->setID(newtype);

				if(item->hasSubType()){
					if(count != -1)
						item->setItemCountOrSubtype(count);
				}
				else
					item->setItemCount(1);

				cylinder->__updateThing(item, item->getItemCountOrSubtype());
				cylinder->postAddNotification(item, itemIndex);
				return item;
			}
		}
	}

	return NULL;
}

ReturnValue Game::internalTeleport(Thing* thing, const Position& newPos)
{
	if(newPos == thing->getPosition())
		return RET_NOERROR;
	else if(thing->isRemoved())
		return RET_NOTPOSSIBLE;

	Tile* toTile = getTile(newPos.x, newPos.y, newPos.z);
	if(toTile){
		if(Creature* creature = thing->getCreature()){
			creature->getTile()->moveCreature(creature, toTile, true);
			return RET_NOERROR;
		}
		else if(Item* item = thing->getItem()){
			return internalMoveItem(item->getParent(), toTile, INDEX_WHEREEVER, item, item->getItemCount());
		}
	}

	return RET_NOTPOSSIBLE;
}

void Game::getSpectators(SpectatorVec& list, const Position& centerPos, bool multifloor /*= false*/,
	int32_t minRangeX /*= 0*/, int32_t maxRangeX /*= 0*/,
	int32_t minRangeY /*= 0*/, int32_t maxRangeY /*= 0*/)
{
	map->getSpectators(list, centerPos, multifloor, minRangeX, maxRangeY, minRangeY, maxRangeY);
}

//battle system

void Game::checkCreatureAttacking(uint32_t creatureId, uint32_t interval)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkCreature()");

	Creature* creature = getCreatureByID(creatureId);

	if(creature){
		if(creature->getHealth() > 0){
			creature->onThinkAttacking(interval);
		}

		flushSendBuffers();
	}
}


//Implementation of player invoked events
bool Game::movePlayer(Player* player, Direction direction)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::movePlayer()");
	if(player->isRemoved())
		return false;

	player->setFollowCreature(NULL);
	player->onWalk(direction);
	return (internalMoveCreature(player, direction) == RET_NOERROR);
}

bool Game::playerWhisper(Player* player, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerWhisper()");
	if(player->isRemoved())
		return false;

	SpectatorVec list;
	SpectatorVec::iterator it;

	//getSpectators(Range(player->getPosition()), list);
	getSpectators(list, player->getPosition());
	
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if(tmpPlayer = (*it)->getPlayer()){
			tmpPlayer->sendCreatureSay(player, SPEAK_WHISPER, text);
		}
	}
	
	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		if(!Position::areInRange<1,1,0>(player->getPosition(), (*it)->getPosition())){
			(*it)->onCreatureSay(player, SPEAK_WHISPER, std::string("pspsps"));
		}
		else{
			(*it)->onCreatureSay(player, SPEAK_WHISPER, text);
		}
	}

	return true;
}

bool Game::playerYell(Player* player, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerYell()");
	if(player->isRemoved())
		return false;

	int32_t addExhaustion = 0;
	bool isExhausted = false;

	if(!player->hasCondition(CONDITION_EXHAUSTED)){
		addExhaustion = g_config.getNumber(ConfigManager::EXHAUSTED);

		std::string yellText = text;
		std::transform(yellText.begin(), yellText.end(), yellText.begin(), upchar);
		
		internalCreatureSay(player, SPEAK_YELL, yellText);
	}
	else{
		isExhausted = true;
		addExhaustion = g_config.getNumber(ConfigManager::EXHAUSTED_ADD);
		player->sendTextMessage(MSG_STATUS_SMALL, "You are exhausted.");
	}

	if(addExhaustion > 0){
		Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_EXHAUSTED, addExhaustion, 0);
		player->addCondition(condition);
	}

	return !isExhausted;
}

bool Game::playerSpeakTo(Player* player, SpeakClasses type, const std::string& receiver,
	const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSpeakTo");
	if(player->isRemoved())
		return false;
	
	Player* toPlayer = getPlayerByName(receiver);
	if(!toPlayer) {
		player->sendTextMessage(MSG_STATUS_SMALL, "A player with this name is not online.");
		return false;
	}

	if(player->getAccessLevel() < ACCESS_TALK){
		type = SPEAK_PRIVATE;
	}

	toPlayer->sendCreatureSay(player, type, text);
	toPlayer->onCreatureSay(player, type, text);

	std::stringstream ss;
	ss << "Message sent to " << toPlayer->getName() << ".";
	player->sendTextMessage(MSG_STATUS_SMALL, ss.str());
	return true;
}

bool Game::playerTalkToChannel(Player* player, SpeakClasses type, const std::string& text, unsigned short channelId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerTalkToChannel");
	if(player->isRemoved())
		return false;
	
	if(player->getAccessLevel() >= ACCESS_TALK){
		type = SPEAK_CHANNEL_Y;
	}
	
	g_chat.talkToChannel(player, type, text, channelId);
	return true;
}

bool Game::playerBroadcastMessage(Player* player, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerBroadcastMessage()");
	if(player->isRemoved() || player->getAccessLevel() == 0)
		return false;

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		(*it).second->sendCreatureSay(player, SPEAK_BROADCAST, text);
	}

	return true;
}

bool Game::anonymousBroadcastMessage(const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::anonymousBroadcastMessage()");

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		(*it).second->sendTextMessage(MSG_STATUS_WARNING, text.c_str());
	}
	
	return true;
}

bool Game::playerAutoWalk(Player* player, std::list<Direction>& listDir)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerAutoWalk()");
	if(player->isRemoved())
		return false;

	return player->startAutoWalk(listDir);
}

bool Game::playerStopAutoWalk(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerStopAutoWalk()");
	if(player->isRemoved())
		return false;

	player->stopEventWalk();
	return true;
}

bool Game::playerUseItemEx(Player* player, const Position& fromPos, uint8_t fromStackPos, uint16_t fromSpriteId,
	const Position& toPos, uint8_t toStackPos, uint16_t toSpriteId, bool isHotkey)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseItemEx()");
	if(player->isRemoved())
		return false;

	if(isHotkey && g_config.getString(ConfigManager::ITEM_HOTKEYS) == "no"){
		return false;
	}

	ReturnValue ret = RET_NOERROR;
	if((ret = Actions::canUse(player, fromPos)) != RET_NOERROR){
		player->sendCancelMessage(ret);
		return false;
	}
	
	Thing* thing = internalGetThing(player, fromPos, fromStackPos, fromSpriteId);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	
	Item* item = thing->getItem();
	if(!item || item->getClientID() != fromSpriteId || !item->isUseable()){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	return g_actions->useItemEx(player, fromPos, toPos, toStackPos, item, isHotkey);
}

bool Game::playerUseItem(Player* player, const Position& pos, uint8_t stackPos,
	uint8_t index, uint16_t spriteId, bool isHotkey)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseItem()");
	if(player->isRemoved())
		return false;

	if(isHotkey && g_config.getString(ConfigManager::ITEM_HOTKEYS) == "no"){
		return false;
	}

	ReturnValue ret = RET_NOERROR;
	if((ret = Actions::canUse(player, pos)) != RET_NOERROR){
		player->sendCancelMessage(ret);
		return false;
	}

	Thing* thing = internalGetThing(player, pos, stackPos, spriteId);

	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	
	Item* item = thing->getItem();
	if(!item || item->getClientID() != spriteId){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	g_actions->useItem(player, pos, index, item, isHotkey);
	return true;
}

bool Game::playerUseBattleWindow(Player* player, const Position& fromPos, uint8_t fromStackPos,
	uint32_t creatureId, uint16_t spriteId, bool isHotkey)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerUseBattleWindow");
	if(player->isRemoved())
		return false;

	Creature* creature = getCreatureByID(creatureId);
	if(!creature){	
		return false;
	}
	
	if(!Position::areInRange<7,5,0>(creature->getPosition(), player->getPosition())){
		return false;
	}
	
	if(g_config.getString(ConfigManager::ITEM_HOTKEYS) == "no"){
		if(isHotkey){
			player->sendCancelMessage(RET_DIRECTPLAYERSHOOT);
			return false;
		}
	}
	
	if(g_config.getString(ConfigManager::BATTLE_WINDOW_PLAYERS) == "no"){
		if(creature->getPlayer() && creature != player){
			player->sendCancelMessage(RET_DIRECTPLAYERSHOOT);
			return false;
		}
	}
	
	Thing* thing = internalGetThing(player, fromPos, fromStackPos, spriteId, STACKPOS_USE);

	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Item* item = thing->getItem();
	if(!item){
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}
	
	return g_actions->useItemEx(player, fromPos, creature->getPosition(), creature->getParent()->__getIndexOfThing(creature), item, isHotkey);
}

bool Game::playerRotateItem(Player* player, const Position& pos, uint8_t stackPos, const uint16_t spriteId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerRotateItem()");
	if(player->isRemoved())
		return false;
	
	Thing* thing = internalGetThing(player, pos, stackPos);
	if(!thing){
		return false;
	}
	Item* item = thing->getItem();

	if(item == NULL || spriteId != item->getClientID() || !item->isRoteable()){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	
	if(!Position::areInRange<1,1,0>(item->getPosition(), player->getPosition())){
		player->sendCancelMessage(RET_TOOFARAWAY);
		return false;
	}

	uint16_t newtype = Item::items[item->getID()].rotateTo;
	if(newtype != 0){
		transformItem(item, newtype);
	}

	return true;
}

bool Game::playerWriteItem(Player* player, Item* item, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerWriteItem()");	
	if(player->isRemoved())
		return false;

	if(item->isRemoved()){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	Cylinder* topParent = item->getTopParent();

	Player* owner = dynamic_cast<Player*>(topParent);
	if(owner && owner != player){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	if(!Position::areInRange<1,1,0>(item->getPosition(), player->getPosition())){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}

	item->setText(text);
	
	uint16_t newtype = Item::items[item->getID()].writeOnceItemId;
	if(newtype != 0){
		transformItem(item, newtype);
	}
	
	//TODO: set last written by
	return true;
}

bool Game::playerRequestTrade(Player* player, const Position& pos, uint8_t stackPos,
	uint32_t playerId, uint16_t spriteId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerRequestTrade()");
	if(player->isRemoved())
		return false;

	Player* tradePartner = getPlayerByID(playerId);
	if(!tradePartner || tradePartner == player) {
		player->sendTextMessage(MSG_INFO_DESCR, "Sorry, not possible.");
		return false;
	}
	
	if(!Position::areInRange<2,2,0>(tradePartner->getPosition(), player->getPosition())){
		std::stringstream ss;
		ss << tradePartner->getName() << " tells you to move closer.";
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
		return false;
	}

	Item* tradeItem = dynamic_cast<Item*>(internalGetThing(player, pos, stackPos, spriteId, STACKPOS_USE));
	if(!tradeItem || tradeItem->getClientID() != spriteId || !tradeItem->isPickupable()) {
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	else if(player->getPosition().z > tradeItem->getPosition().z){
		player->sendCancelMessage(RET_FIRSTGOUPSTAIRS);
		return false;
	}
	else if(player->getPosition().z < tradeItem->getPosition().z){
		player->sendCancelMessage(RET_FIRSTGODOWNSTAIRS);
		return false;
	}
	else if(!Position::areInRange<1,1,0>(tradeItem->getPosition(), player->getPosition())){
		player->sendCancelMessage(RET_TOOFARAWAY);
		return false;
	}

	std::map<Item*, unsigned long>::const_iterator it;
	const Container* container = NULL;
	for(it = tradeItems.begin(); it != tradeItems.end(); it++) {
		if(tradeItem == it->first || 
			((container = dynamic_cast<const Container*>(tradeItem)) && container->isHoldingItem(it->first)) ||
			((container = dynamic_cast<const Container*>(it->first)) && container->isHoldingItem(tradeItem)))
		{
			player->sendTextMessage(MSG_INFO_DESCR, "This item is already being traded.");
			return false;
		}
	}

	Container* tradeContainer = tradeItem->getContainer();
	if(tradeContainer && tradeContainer->getItemHoldingCount() + 1 > 100){
		player->sendTextMessage(MSG_INFO_DESCR, "You can not trade more than 100 items.");
		return false;
	}

	return internalStartTrade(player, tradePartner, tradeItem);
}

bool Game::internalStartTrade(Player* player, Player* tradePartner, Item* tradeItem)
{
	if(player->tradeState != TRADE_NONE && !(player->tradeState == TRADE_ACKNOWLEDGE && player->tradePartner == tradePartner)) {
		player->sendCancelMessage(RET_YOUAREALREADYTRADING);
		return false;
	}
	else if(tradePartner->tradeState != TRADE_NONE && tradePartner->tradePartner != player) {
		player->sendCancelMessage(RET_THISPLAYERISALREADYTRADING);
		return false;
	}
	
	player->tradePartner = tradePartner;
	player->tradeItem = tradeItem;
	player->tradeState = TRADE_INITIATED;
	tradeItem->useThing2();
	tradeItems[tradeItem] = player->getID();

	player->sendTradeItemRequest(player, tradeItem, true);

	if(tradePartner->tradeState == TRADE_NONE){
		std::stringstream trademsg;
		trademsg << player->getName() <<" wants to trade with you.";
		tradePartner->sendTextMessage(MSG_INFO_DESCR, trademsg.str());
		tradePartner->tradeState = TRADE_ACKNOWLEDGE;
		tradePartner->tradePartner = player;
	}
	else{
		Item* counterOfferItem = tradePartner->tradeItem;
		player->sendTradeItemRequest(tradePartner, counterOfferItem, false);
		tradePartner->sendTradeItemRequest(player, tradeItem, false);
	}

	return true;

}

bool Game::playerAcceptTrade(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerAcceptTrade()");
	if(player->isRemoved())
		return false;

	if(!(player->getTradeState() == TRADE_ACKNOWLEDGE || player->getTradeState() == TRADE_INITIATED)){
		return false;
	}

	player->setTradeState(TRADE_ACCEPT);
	Player* tradePartner = player->tradePartner;
	if(tradePartner && tradePartner->getTradeState() == TRADE_ACCEPT){

		Item* tradeItem1 = player->tradeItem;
		Item* tradeItem2 = tradePartner->tradeItem;

		player->setTradeState(TRADE_TRANSFER);
		tradePartner->setTradeState(TRADE_TRANSFER);

		std::map<Item*, unsigned long>::iterator it;

		it = tradeItems.find(tradeItem1);
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}

		it = tradeItems.find(tradeItem2);
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}

		bool isSuccess = false;

		ReturnValue ret1 = internalAddItem(tradePartner, tradeItem1, INDEX_WHEREEVER, 0, true);
		ReturnValue ret2 = internalAddItem(player, tradeItem2, INDEX_WHEREEVER, 0, true);

		if(ret1 == RET_NOERROR && ret2 == RET_NOERROR){
			ret1 = internalRemoveItem(tradeItem1, tradeItem1->getItemCount(), true);
			ret2 = internalRemoveItem(tradeItem2, tradeItem2->getItemCount(), true);

			if(ret1 == RET_NOERROR && ret2 == RET_NOERROR){
				Cylinder* cylinder1 = tradeItem1->getParent();
				Cylinder* cylinder2 = tradeItem2->getParent();

				internalMoveItem(cylinder1, tradePartner, INDEX_WHEREEVER, tradeItem1, tradeItem1->getItemCount());
				internalMoveItem(cylinder2, player, INDEX_WHEREEVER, tradeItem2, tradeItem2->getItemCount());

				tradeItem1->onTradeEvent(ON_TRADE_TRANSFER, tradePartner);
				tradeItem2->onTradeEvent(ON_TRADE_TRANSFER, player);

				isSuccess = true;
			}
		}

		if(!isSuccess){
			std::stringstream ss;
			if(ret1 == RET_NOTENOUGHCAPACITY){
				ss << "You do not have enough capacity to carry this object." << std::endl << tradeItem1->getWeightDescription();
			}
			else if(ret1 == RET_NOTENOUGHROOM || ret2 == RET_CONTAINERNOTENOUGHROOM){
				ss << "You do not have enough room to carry this object.";
			}
			else
				ss << "Trade could not be completed.";

			tradePartner->sendTextMessage(MSG_INFO_DESCR, ss.str().c_str());
			tradePartner->tradeItem->onTradeEvent(ON_TRADE_CANCEL, tradePartner);

			ss.str("");
			if(ret2 == RET_NOTENOUGHCAPACITY){
				ss << "You do not have enough capacity to carry this object." << std::endl << tradeItem2->getWeightDescription();
			}
			else if(ret2 == RET_NOTENOUGHROOM || ret2 == RET_CONTAINERNOTENOUGHROOM){
				ss << "You do not have enough room to carry this object.";
			}
			else
				ss << "Trade could not be completed.";

			player->sendTextMessage(MSG_INFO_DESCR, ss.str().c_str());
			player->tradeItem->onTradeEvent(ON_TRADE_CANCEL, player);
		}

		player->setTradeState(TRADE_NONE);
		player->tradeItem = NULL;
		player->tradePartner = NULL;
		player->sendTradeClose();

		tradePartner->setTradeState(TRADE_NONE);
		tradePartner->tradeItem = NULL;
		tradePartner->tradePartner = NULL;
		tradePartner->sendTradeClose();

		return isSuccess;
	}

	return false;
}

bool Game::playerLookInTrade(Player* player, bool lookAtCounterOffer, int index)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerLookInTrade()");
	if(player->isRemoved())
		return false;

	Player* tradePartner = player->tradePartner;
	if(!tradePartner)
		return false;

	Item* tradeItem = NULL;

	if(lookAtCounterOffer)
		tradeItem = tradePartner->getTradeItem();
	else
		tradeItem = player->getTradeItem();

	if(!tradeItem)
		return false;

	int32_t lookDistance = std::max(std::abs(player->getPosition().x - tradeItem->getPosition().x),
		std::abs(player->getPosition().y - tradeItem->getPosition().y));

	if(index == 0){
		std::stringstream ss;
		ss << "You see " << tradeItem->getDescription(lookDistance);
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
		return false;
	}

	Container* tradeContainer = tradeItem->getContainer();
	if(!tradeContainer || index > (int)tradeContainer->getItemHoldingCount())
		return false;

	bool foundItem = false;
	std::list<const Container*> listContainer;
	ItemList::const_iterator it;
	Container* tmpContainer = NULL;

	listContainer.push_back(tradeContainer);

	while(!foundItem && listContainer.size() > 0){
		const Container* container = listContainer.front();
		listContainer.pop_front();

		for(it = container->getItems(); it != container->getEnd(); ++it){
			if(tmpContainer = (*it)->getContainer()){
				listContainer.push_back(tmpContainer);
			}

			--index;
			if(index == 0){
				tradeItem = *it;
				foundItem = true;
				break;
			}
		}
	}
	
	if(foundItem){
		std::stringstream ss;
		ss << "You see " << tradeItem->getDescription(lookDistance);
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	}

	return foundItem;
}

bool Game::playerCloseTrade(Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerCloseTrade()");
	if(player->isRemoved())
		return false;

	Player* tradePartner = player->tradePartner;
	if((tradePartner && tradePartner->getTradeState() == TRADE_TRANSFER) || 
		  player->getTradeState() == TRADE_TRANSFER){
 		std::cout << "Warning: [Game::playerCloseTrade] TradeState == TRADE_TRANSFER. " << 
		  	player->getName() << " " << player->getTradeState() << " , " << 
		  	tradePartner->getName() << " " << tradePartner->getTradeState() << std::endl;
		return true;
	}

	std::vector<Item*>::iterator it;
	if(player->getTradeItem()){
		std::map<Item*, unsigned long>::iterator it = tradeItems.find(player->getTradeItem());
		if(it != tradeItems.end()) {
			FreeThing(it->first);
			tradeItems.erase(it);
		}
	
		player->tradeItem->onTradeEvent(ON_TRADE_CANCEL, player);
		player->tradeItem = NULL;
	}
	
	player->setTradeState(TRADE_NONE);
	player->tradePartner = NULL;

	player->sendTextMessage(MSG_STATUS_SMALL, "Trade cancelled.");
	player->sendTradeClose();

	if(tradePartner){
		if(tradePartner->getTradeItem()){
			std::map<Item*, unsigned long>::iterator it = tradeItems.find(tradePartner->getTradeItem());
			if(it != tradeItems.end()) {
				FreeThing(it->first);
				tradeItems.erase(it);
			}

			tradePartner->tradeItem->onTradeEvent(ON_TRADE_CANCEL, tradePartner);
			tradePartner->tradeItem = NULL;
		}

		tradePartner->setTradeState(TRADE_NONE);		
		tradePartner->tradePartner = NULL;

		tradePartner->sendTextMessage(MSG_STATUS_SMALL, "Trade cancelled.");
		tradePartner->sendTradeClose();
	}

	return true;
}

bool Game::playerLookAt(Player* player, const Position& pos, uint16_t spriteId, uint8_t stackPos)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerLookAt()");
	if(player->isRemoved())
		return false;
	
	Thing* thing = internalGetThing(player, pos, stackPos, spriteId, STACKPOS_LOOK);
	if(!thing){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	
	Position thingPos = thing->getPosition();
	if(!player->canSee(thingPos)){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return false;
	}
	
	Position playerPos = player->getPosition();
	
	int32_t lookDistance = 0;
	if(thing == player)
		lookDistance = -1;
	else{
		lookDistance = std::max(std::abs(playerPos.x - thingPos.x), std::abs(playerPos.y - thingPos.y));
		if(playerPos.z != thingPos.z)
			lookDistance = lookDistance + 9 + 6;
	}

	std::stringstream ss;
	ss << "You see " << thing->getDescription(lookDistance);
	
	if(player->getAccessLevel() >= ACCESS_LOOK){
		if(Creature* lookCreature = thing->getCreature()){
			ss << " (Health: " << lookCreature->health << "/" << lookCreature->healthMax << ")";
			if(Player* lookPlayer = lookCreature->getPlayer()){
				ss << " (Mana: " << lookPlayer->mana << "/" << lookPlayer->manaMax << ")";
			}
		}
		else if(Item* item = thing->getItem()){
			ss << " ID: " << item->getID() << ".";
			if(item->getActionId())
				ss << " ActionID: " << item->getActionId() << ".";
			if(item->getUniqueId())
				ss << " UniqueID: " << item->getUniqueId() << ".";
		}
		
		ss << " Position(X: " << thing->getPosition().x << " Y: " << thing->getPosition().y << " Z: " << thing->getPosition().z << ")";
	}
	
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());

	return true;
}

bool Game::playerSetAttackedCreature(Player* player, unsigned long creatureId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSetAttackedCreature()");
	if(player->isRemoved())
		return false;

	if(player->getAttackedCreature() && creatureId == 0){
		player->setAttackedCreature(NULL);
		player->sendCancelTarget();
	}

	Creature* attackCreature = getCreatureByID(creatureId);
	if(!attackCreature){
		player->setAttackedCreature(NULL);
		player->sendCancelTarget();
		return false;
	}

	ReturnValue ret = RET_NOERROR;
	if(player->getAccessLevel() < ACCESS_PROTECT){
		if(attackCreature == player){
			ret = RET_YOUMAYNOTATTACKTHISPLAYER;
		}
		else if(player->isInPz()){
			ret = RET_YOUMAYNOTATTACKAPERSONWHILEINPROTECTIONZONE;
		}
		else if(attackCreature->isInPz()){
			ret = RET_YOUMAYNOTATTACKAPERSONINPROTECTIONZONE;
		}
		else if(!attackCreature->isAttackable()){
			ret = RET_YOUMAYNOTATTACKTHISPLAYER;
		}
		else if(Player* attackedPlayer = attackCreature->getPlayer()){
			if(player->safeMode != 0 && attackedPlayer != player && attackedPlayer->getSkull() != SKULL_RED && attackedPlayer->getSkull() != SKULL_WHITE){
				ret = RET_YOUMUSTTURNSECUREMODEOFF;
			}
		}
		#ifdef __XID_ROOKGARD__
		else if(player && player->isRookie() && attackedPlayer && attackedPlayer->isRookie()){
			ret = RET_YOUMAYNOTATTACKTHISPLAYER;
		}
		#endif
		
		if(ret == RET_NOERROR){
			ret = Combat::canDoCombat(player, attackCreature);
		}
	}

	if(ret == RET_NOERROR){
		player->setAttackedCreature(attackCreature);	
		return true;
	}
	else{
		player->sendCancelMessage(ret);
		player->sendCancelTarget();
		player->setAttackedCreature(NULL);
		return false;
	}
}

bool Game::playerFollowCreature(Player* player, unsigned long creatureId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerFollowCreature");
	if(player->isRemoved())
		return false;

	player->setAttackedCreature(NULL);
	Creature* followCreature = NULL;
	
	if(creatureId != 0){
		followCreature = getCreatureByID(creatureId);
	}

	return player->setFollowCreature(followCreature);
}

bool Game::playerSetFightModes(Player* player, fightMode_t fightMode, chaseMode_t chaseMode, uint8_t safeMode)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSetFightModes");
	if(player->isRemoved())
		return false;

	player->setFightMode(fightMode);
	player->setChaseMode(chaseMode);
	player->safeMode = safeMode;
	return true;
}

bool Game::playerTurn(Player* player, Direction dir)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerTurn()");
	if(player->isRemoved())
		return false;

	return internalCreatureTurn(player, dir);
}

bool Game::playerSay(Player* player, uint16_t channelId, SpeakClasses type,
	const std::string& receiver, const std::string& text)
{
	if(playerSaySpell(player, type, text)){
		return false;
	}
	
	uint32_t muteTime;
	if(player->isMuted(muteTime)){
		std::stringstream ss;
		ss << "You are still muted for " << muteTime << " seconds.";
		player->sendTextMessage(MSG_STATUS_SMALL, ss.str());
		return false;
	}

	player->removeMessageBuffer();

	switch(type){
		case SPEAK_SAY:
			return playerSayDefault(player, text);
			break;
		case SPEAK_WHISPER:
			return playerWhisper(player, text);
			break;
		case SPEAK_YELL:
			return playerYell(player, text);
			break;
		case SPEAK_PRIVATE:
		case SPEAK_PRIVATE_RED:
			return playerSpeakTo(player, type, receiver, text);
			break;
		case SPEAK_CHANNEL_Y:
		case SPEAK_CHANNEL_R1:
		case SPEAK_CHANNEL_R2:
			return playerTalkToChannel(player, type, text, channelId);
			break;
		case SPEAK_BROADCAST:
			return playerBroadcastMessage(player, text);
			break;

		default:
			break;
	}

	return false;
}

bool Game::playerSayDefault(Player* player, const std::string& text)
{	
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSayDefault()");
	if(player->isRemoved())
		return false;

	return internalCreatureSay(player, SPEAK_SAY, text);
}

bool Game::playerChangeOutfit(Player* player, Outfit_t outfit)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerChangeOutfit()");
	if(player->isRemoved())
		return false;

	#ifdef __XID_PVP_FEATURES__	
	if(g_config.getString(ConfigManager::ALLOW_OUTFIT_CHANGE) == "no"){
		return false;
	}
	#endif

	player->defaultOutfit = outfit;
	internalCreatureChangeOutfit(player, outfit);

	return true;
}

bool Game::playerSaySpell(Player* player, SpeakClasses type, const std::string& text)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::playerSaySpell()");
	if(player->isRemoved())
		return false;
	
	TalkActionResult_t result;
	result = g_talkactions->playerSaySpell(player, type, text);
	if(result == TALKACTION_BREAK){
		return true;
	}

	result = g_spells->playerSaySpell(player, type, text);
	uint32_t muteTime;
	if(player->isMuted(muteTime)){
		std::stringstream ss;
		ss << "You are still muted for " << muteTime << " seconds.";
		player->sendTextMessage(MSG_STATUS_SMALL, ss.str());
		result = TALKACTION_FAILED;
	}
	
	if(result == TALKACTION_BREAK){
		return internalCreatureSay(player, SPEAK_SAY, text);
	}
	else if(result == TALKACTION_FAILED){
		return true;
	}        

	return false;
}

//--
bool Game::canThrowObjectTo(const Position& fromPos, const Position& toPos)
{
	return map->canThrowObjectTo(fromPos, toPos);
}

bool Game::getPathTo(const Creature* creature, Position toPosition, std::list<Direction>& listDir)
{
	return map->getPathTo(creature, toPosition, listDir);
}

bool Game::isPathValid(const Creature* creature, const std::list<Direction>& listDir, const Position& destPos)
{
	return map->isPathValid(creature, listDir, destPos);
}

bool Game::internalCreatureTurn(Creature* creature, Direction dir)
{
	if(creature->getDirection() != dir){
		creature->setDirection(dir);

		int32_t stackpos = creature->getParent()->__getIndexOfThing(creature);

		SpectatorVec list;
		SpectatorVec::iterator it;
		//map->getSpectators(Range(creature->getPosition(), true), list);
		getSpectators(list, creature->getPosition(), true);

	    #ifdef __TC_GM_INVISIBLE__
        if(creature->gmInvisible){
            Player* player = NULL;
            for(it = list.begin(); it != list.end(); ++it) {
                if(player = (*it)->getPlayer()){
                    if((*it)->getPlayer() == creature)       
                        player->sendCreatureTurn(creature, stackpos);
                }
            }
        }
        else{
            Player* player = NULL;
            for(it = list.begin(); it != list.end(); ++it) {
                if(player = (*it)->getPlayer()){
                    player->sendCreatureTurn(creature, stackpos);
                }
            }
        }
        #else
        Player* player = NULL;
        for(it = list.begin(); it != list.end(); ++it) {
            if(player = (*it)->getPlayer()){
                player->sendCreatureTurn(creature, stackpos);
            }
        }
        #endif
		
		//event method
		for(it = list.begin(); it != list.end(); ++it) {
			(*it)->onCreatureTurn(creature, stackpos);
		}

		return true;
	}

	return false;
}

bool Game::internalCreatureSay(Creature* creature, SpeakClasses type, const std::string& text)
{
	//First, check if this was a command
	for(uint32_t i = 0; i < commandTags.size(); i++){
		if(commandTags[i] == text.substr(0,1)){
			return commands.exeCommand(creature, text);
		}
	}
	
	SpectatorVec list;
	SpectatorVec::iterator it;

	if(type == SPEAK_YELL || type == SPEAK_MONSTER_YELL){
		getSpectators(list, creature->getPosition(), true, 18, 18, 14, 14);
	}
	else{
		getSpectators(list, creature->getPosition());
	}

	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if(tmpPlayer = (*it)->getPlayer()){
			tmpPlayer->sendCreatureSay(creature, type, text);
		}
	}

	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onCreatureSay(creature, type, text);
	}

	return true;
}

bool Game::getPathToEx(const Creature* creature, const Position& targetPos, uint32_t minDist, uint32_t maxDist,
	bool fullPathSearch, bool targetMustBeReachable, std::list<Direction>& dirList)
{
	if(creature->getPosition().z != targetPos.z || !creature->canSee(targetPos)){
		return false;
	}

	const Position& creaturePos = creature->getPosition();

	uint32_t currentDist = std::max(std::abs(creaturePos.x - targetPos.x), std::abs(creaturePos.y - targetPos.y));
	if(currentDist == maxDist){
		if(!targetMustBeReachable || map->canThrowObjectTo(creaturePos, targetPos)){
			return true;
		}
	}

	int32_t dxMin = ((fullPathSearch || (creaturePos.x - targetPos.x) <= 0) ? maxDist : 0);
	int32_t dxMax = ((fullPathSearch || (creaturePos.x - targetPos.x) >= 0) ? maxDist : 0);
	int32_t dyMin = ((fullPathSearch || (creaturePos.y - targetPos.y) <= 0) ? maxDist : 0);
	int32_t dyMax = ((fullPathSearch || (creaturePos.y - targetPos.y) >= 0) ? maxDist : 0);

	std::list<Direction> tmpDirList;
	Tile* tile;

	Position minWalkPos;
	Position tmpPos;
	int minWalkDist = -1;

	int tmpDist;
	int tmpWalkDist;

	int32_t tryDist = maxDist;

	while(tryDist >= (int32_t)minDist){
		for(int y = targetPos.y - dyMin; y <= targetPos.y + dyMax; ++y) {
			for(int x = targetPos.x - dxMin; x <= targetPos.x + dxMax; ++x) {

				tmpDist = std::max( std::abs(targetPos.x - x), std::abs(targetPos.y - y) );

				if(tmpDist == tryDist){
					tmpWalkDist = std::abs(creaturePos.x - x) + std::abs(creaturePos.y - y);

					tmpPos.x = x;
					tmpPos.y = y;
					tmpPos.z = creaturePos.z;

					if(tmpWalkDist <= minWalkDist || tmpPos == creaturePos || minWalkDist == -1){

						if(targetMustBeReachable && !canThrowObjectTo(tmpPos, targetPos)){
							continue;
						}
						
						if(tmpPos != creaturePos){
							tile = getTile(tmpPos.x, tmpPos.y, tmpPos.z);
							if(!tile || tile->__queryAdd(0, creature, 1, FLAG_PATHFINDING) != RET_NOERROR){
								continue;
							}

							tmpDirList.clear();
							if(!getPathTo(creature, tmpPos, tmpDirList)){
								continue;
							}
						}
						else{
							tmpDirList.clear();
						}
						
						if(tmpWalkDist < minWalkDist || tmpDirList.size() < dirList.size() || minWalkDist == -1){
							minWalkDist = tmpWalkDist;
							minWalkPos = tmpPos;
							dirList = tmpDirList;
						}
					}
				}
			}

		}

		if(minWalkDist != -1){
			return true;
		}

		--tryDist;
	}

	return false;
}

void Game::checkWalk(unsigned long creatureId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkWalk");

	Creature* creature = getCreatureByID(creatureId);
	if(creature && creature->getHealth() > 0){
		creature->onWalk();
		flushSendBuffers();
	}
}

void Game::checkCreature(uint32_t creatureId, uint32_t interval)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkCreature()");

	Creature* creature = getCreatureByID(creatureId);

	if(creature){
		Tile* tile = creature->getTile();
		if(creature->getHealth() > 0){
			creature->onThink(interval);
			creature->executeConditions(interval);
		}
		#ifdef __NFS_PVP_ARENA__
		else if(tile && tile->pvparena){
			creature->onThink(interval);
			creature->executeConditions(interval);
			
			tile->pvparena->removeCreature(creature, true);
		}
		#endif
		else{
			Item* splash = NULL;
			switch(creature->getRace()){
				case RACE_VENOM:
					splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_GREEN);
					break;

				case RACE_BLOOD:
					splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_BLOOD);
					break;

				case RACE_UNDEAD:
					break;
					
				case RACE_FIRE:
					break;

				default:
					break;
			}

			if(splash){
				internalAddItem(tile, splash, INDEX_WHEREEVER, FLAG_NOLIMIT);
				startDecay(splash);
			}

			Item* corpse = creature->getCorpse();
			if(corpse){
				internalAddItem(tile, corpse, INDEX_WHEREEVER, FLAG_NOLIMIT);
				creature->dropLoot(corpse->getContainer());
				startDecay(corpse);
			}

			creature->die();
			removeCreature(creature, false);
		}

		flushSendBuffers();
	}
}

void Game::changeSpeed(Creature* creature, int32_t varSpeedDelta)
{
	int32_t varSpeed = creature->getSpeed() - creature->getBaseSpeed();
	varSpeed += varSpeedDelta;

	creature->setSpeed(varSpeed);

	SpectatorVec list;
	SpectatorVec::iterator it;

	//getSpectators(Range(creature->getPosition(), true), list);
	getSpectators(list, creature->getPosition(), true);

	Player* player;
	for(it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendChangeSpeed(creature, creature->getSpeed());
		}
	}
}

void Game::internalCreatureChangeOutfit(Creature* creature, const Outfit_t& outfit)
{
	creature->setCurrentOutfit(outfit);
	
	if(!creature->isInvisible()){
		SpectatorVec list;
		SpectatorVec::iterator it;

		//getSpectators(Range(creature->getPosition(), true), list);
		getSpectators(list, creature->getPosition(), true);

	#ifdef __TC_GM_INVISIBLE__
    if(creature->gmInvisible){
        Player* player = NULL;
         for(it = list.begin(); it != list.end(); ++it) {
            if(player = (*it)->getPlayer()){
                if((*it)->getPlayer() == creature)       
                    player->sendCreatureChangeOutfit(creature, outfit);
            }
        }
    }
    else{
        Player* player = NULL;
        for(it = list.begin(); it != list.end(); ++it) {
            if(player = (*it)->getPlayer()){
                player->sendCreatureChangeOutfit(creature, outfit);
            }
        }
    }
    #else
    Player* player = NULL;
    for(it = list.begin(); it != list.end(); ++it) {
        if(player = (*it)->getPlayer()){
            player->sendCreatureChangeOutfit(creature, outfit);
        }
    }
    #endif

		//event method
		for(it = list.begin(); it != list.end(); ++it) {
			(*it)->onCreatureChangeOutfit(creature, outfit);
		}
	}
}

void Game::internalCreatureChangeVisible(Creature* creature, bool visible)
{
	SpectatorVec list;
	SpectatorVec::iterator it;

	//getSpectators(Range(creature->getPosition(), true), list);
	getSpectators(list, creature->getPosition(), true);

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if(tmpPlayer = (*it)->getPlayer()){
			tmpPlayer->sendCreatureChangeVisible(creature, visible);
		}
	}

	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		(*it)->onCreatureChangeVisible(creature, visible);
	}
}


void Game::changeLight(const Creature* creature)
{
	SpectatorVec list;
	SpectatorVec::iterator it;

	//getSpectators(Range(creature->getPosition(), true), list);
	getSpectators(list, creature->getPosition(), true);

	Player* player;
	for(it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendCreatureLight(creature);
		}
	}
}

bool Game::combatChangeHealth(CombatType_t combatType, Creature* attacker, Creature* target,
	int32_t healthChange, bool checkDefense /* = false */, bool checkArmor /* = false */)
{
	const Position& targetPos = target->getPosition();

	if(healthChange > 0){
		if(target->getHealth() <= 0){
			return false;
		}
		
		target->changeHealth(healthChange);
	}
	else{
		SpectatorVec list;
		getSpectators(list, targetPos, true);

		if(!target->isAttackable() || Combat::canDoCombat(attacker, target) != RET_NOERROR){
			addMagicEffect(list, targetPos, NM_ME_PUFF);
			return false;
		}

		int32_t damage = -healthChange;

		BlockType_t blockType = target->blockHit(attacker, combatType, damage, checkDefense, checkArmor);

		if(blockType != BLOCK_NONE && target->isInvisible()){
			//No effects for invisible creatures to avoid detection
			return false;
		}

		if(blockType == BLOCK_DEFENSE){
			addMagicEffect(list, targetPos, NM_ME_PUFF);
			return false;
		}
		else if(blockType == BLOCK_ARMOR){
			addMagicEffect(list, targetPos, NM_ME_BLOCKHIT);
			return false;
		}
		else if(blockType == BLOCK_IMMUNITY){
			uint8_t hitEffect = 0;

			switch(combatType){
				case COMBAT_UNDEFINEDDAMAGE:
					break;

				case COMBAT_ENERGYDAMAGE:
				{
					hitEffect = NM_ME_BLOCKHIT;
					break;
				}

				case COMBAT_POISONDAMAGE:
				{
					hitEffect = NM_ME_POISON_RINGS;
					break;
				}

				case COMBAT_FIREDAMAGE:
				{
					hitEffect = NM_ME_BLOCKHIT;
					break;
				}

				case COMBAT_PHYSICALDAMAGE:
				{
					hitEffect = NM_ME_BLOCKHIT;
					break;
				}

				default:
					hitEffect = NM_ME_PUFF;
					break;
			}

			addMagicEffect(list, targetPos, hitEffect);

			return false;
		}

		if(damage != 0){
			if(target->hasCondition(CONDITION_MANASHIELD)){
				int32_t manaDamage = std::min(target->getMana(), damage);
				damage = std::max((int32_t)0, damage - manaDamage);

				if(manaDamage != 0){
					target->drainMana(attacker, manaDamage);
					
					std::stringstream ss;
					ss << manaDamage;
					addMagicEffect(list, targetPos, NM_ME_LOSE_ENERGY);
					addAnimatedText(list, targetPos, TEXTCOLOR_BLUE, ss.str());
				}
			}

			damage = std::min(target->getHealth(), damage);
			if(damage > 0){
				target->drainHealth(attacker, combatType, damage);
				addCreatureHealth(list, target);

				TextColor_t textColor = TEXTCOLOR_NONE;
				uint8_t hitEffect = 0;

				switch(combatType){
					case COMBAT_PHYSICALDAMAGE:
					{
						Item* splash = NULL;
						switch(target->getRace()){
							case RACE_VENOM:
								textColor = TEXTCOLOR_LIGHTGREEN;
								hitEffect = NM_ME_POISON;
								splash = Item::CreateItem(ITEM_SMALLSPLASH, FLUID_GREEN);
								break;

							case RACE_BLOOD:
								textColor = TEXTCOLOR_RED;
								hitEffect = NM_ME_DRAW_BLOOD;
								splash = Item::CreateItem(ITEM_SMALLSPLASH, FLUID_BLOOD);
								break;

							case RACE_UNDEAD:
								textColor = TEXTCOLOR_LIGHTGREY;
								hitEffect = NM_ME_HIT_AREA;
								break;
								
							case RACE_FIRE:
								textColor = TEXTCOLOR_ORANGE;
								hitEffect = NM_ME_DRAW_BLOOD;
								break;

							default:
								break;
						}

						if(splash){
							internalAddItem(target->getTile(), splash, INDEX_WHEREEVER, FLAG_NOLIMIT);
							startDecay(splash);
						}

						break;
					}

					case COMBAT_ENERGYDAMAGE:
					{
						textColor = TEXTCOLOR_LIGHTBLUE;
						hitEffect = NM_ME_ENERGY_DAMAGE;
						break;
					}

					case COMBAT_POISONDAMAGE:
					{
						textColor = TEXTCOLOR_LIGHTGREEN;
						hitEffect = NM_ME_POISON_RINGS;
						break;
					}
					
					case COMBAT_DROWNDAMAGE:
					{
						textColor = TEXTCOLOR_BLUE;
						hitEffect = NM_ME_BUBBLES;
						break;
					}
					
					case COMBAT_FIREDAMAGE:
					{
						textColor = TEXTCOLOR_ORANGE;
						hitEffect = NM_ME_HITBY_FIRE;
						break;
					}

					case COMBAT_LIFEDRAIN:
					{
						textColor = TEXTCOLOR_RED;
						hitEffect = NM_ME_MAGIC_BLOOD;
						break;
					}

					default:
						break;
				}

				if(textColor != TEXTCOLOR_NONE){
					std::stringstream ss;
					ss << damage;
					addMagicEffect(list, targetPos, hitEffect);
					addAnimatedText(list, targetPos, textColor, ss.str());
				}
			}
		}
	}

	return true;
}

bool Game::combatChangeMana(Creature* attacker, Creature* target, int32_t manaChange)
{
	const Position& targetPos = target->getPosition();

	SpectatorVec list;
	getSpectators(list, targetPos, true);

	if(manaChange > 0){
		target->changeMana(manaChange);
	}
	else{
		if(!target->isAttackable() || Combat::canDoCombat(attacker, target) != RET_NOERROR){
			addMagicEffect(list, targetPos, NM_ME_PUFF);
			return false;
		}
		
		int32_t manaLoss = std::min(target->getMana(), -manaChange);
		BlockType_t blockType = target->blockHit(attacker, COMBAT_MANADRAIN, manaLoss);

		if(blockType != BLOCK_NONE){
			addMagicEffect(list, targetPos, NM_ME_PUFF);
			return false;
		}

		if(manaLoss > 0){
			target->drainMana(attacker, manaLoss);

			std::stringstream ss;
			ss << manaLoss;
			addAnimatedText(list, targetPos, TEXTCOLOR_BLUE, ss.str());
		}
	}

	return true;
}

void Game::addCreatureHealth(const Creature* target)
{
	SpectatorVec list;
	//getSpectators(Range(target->getPosition(), true), list);
	getSpectators(list, target->getPosition(), true);

	addCreatureHealth(list, target);
}

void Game::addCreatureHealth(const SpectatorVec& list, const Creature* target)
{
	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendCreatureHealth(target);
		}
	}
}

void Game::addAnimatedText(const Position& pos, uint8_t textColor,
	const std::string& text)
{
	SpectatorVec list;
	//getSpectators(Range(pos, true), list);
	getSpectators(list, pos, true);

	addAnimatedText(list, pos, textColor, text);
}

void Game::addAnimatedText(const SpectatorVec& list, const Position& pos, uint8_t textColor,
	const std::string& text)
{
	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendAnimatedText(pos, textColor, text);
		}
	}
}

void Game::addMagicEffect(const Position& pos, uint8_t effect)
{
	SpectatorVec list;
	//getSpectators(Range(pos, true), list);
	getSpectators(list, pos, true);

	addMagicEffect(list, pos, effect);
}

void Game::addMagicEffect(const SpectatorVec& list, const Position& pos, uint8_t effect)
{
	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendMagicEffect(pos, effect);
		}
	}
}

void Game::addDistanceEffect(const Position& fromPos, const Position& toPos,
	uint8_t effect)
{
	SpectatorVec list;
	//getSpectators(Range(fromPos, true), list);
	//getSpectators(Range(toPos, true), list);

	getSpectators(list, fromPos, true);
	getSpectators(list, toPos, true);

	Player* player = NULL;
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendDistanceShoot(fromPos, toPos, effect);
		}
	}
}

#ifdef __SKULLSYSTEM__
void Game::changeSkull(Player* player, Skulls_t newSkull)
{
	#ifdef __NFS_PVP_ARENA__
	if(player->getTile()->hasFlag(TILESTATE_NOSKULLS)){
		return;
	}
    #endif
    
	SpectatorVec list;
	SpectatorVec::iterator it;

	player->setSkull(newSkull);
	getSpectators(list, player->getPosition(), true);

	Player* spectator;
	for(it = list.begin(); it != list.end(); ++it){
		if(spectator = (*it)->getPlayer()){
			spectator->sendCreatureSkull(player);
		}
	}
}
#endif

void Game::startDecay(Item* item)
{
	uint32_t decayState = item->getDecaying();
	if(decayState == DECAYING_TRUE){
		//already decaying
		return;
	}

	if(item->canDecay()){
		if(item->getDuration() > 0){
			item->useThing2();
			item->setDecaying(DECAYING_TRUE);
			decayItems.push_back(item);
		}
		else{
			internalDecayItem(item);
		}
	}
}

void Game::internalDecayItem(Item* item)
{
	const ItemType& it = Item::items[item->getID()];

	if(it.decayTo != 0){
		Item* newItem = transformItem(item, it.decayTo);
		startDecay(newItem);
	}
	else{
		ReturnValue ret = internalRemoveItem(item);

		if(ret != RET_NOERROR){
			std::cout << "internalDecayItem failed, error code: " << (int) ret << "item id: " << item->getID() << std::endl;
		}
	}
}

void Game::checkDecay(int32_t interval)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkDecay()");
	addEvent(makeTask(DECAY_INTERVAL, boost::bind(&Game::checkDecay, this, DECAY_INTERVAL)));
	Item* item = NULL;
	for(DecayList::iterator it = decayItems.begin(); it != decayItems.end();){
		item = *it;
		item->decreaseDuration(interval);
		if(!item->canDecay()){
			item->setDecaying(DECAYING_FALSE);
			FreeThing(item);
			it = decayItems.erase(it);
			continue;
		}

		if(item->getDuration() <= 0){
			it = decayItems.erase(it);
			internalDecayItem(item);
			FreeThing(item);
		}
		else{
			++it;
		}
	}

	flushSendBuffers();
}

void Game::checkLight(int t)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkLight()");
	
	addEvent(makeTask(10000, boost::bind(&Game::checkLight, this, 10000)));
	
	light_hour = light_hour + light_hour_delta;
	if(light_hour > 1440)
		light_hour = light_hour - 1440;
	
	if(std::abs(light_hour - SUNRISE) < 2*light_hour_delta){
		light_state = LIGHT_STATE_SUNRISE;
	}
	else if(std::abs(light_hour - SUNSET) < 2*light_hour_delta){
		light_state = LIGHT_STATE_SUNSET;
	}
	
	int newlightlevel = lightlevel;
	bool lightChange = false;
	switch(light_state){
	case LIGHT_STATE_SUNRISE:
		newlightlevel += (LIGHT_LEVEL_DAY - LIGHT_LEVEL_NIGHT)/30;
		lightChange = true;
		break;
	case LIGHT_STATE_SUNSET:
		newlightlevel -= (LIGHT_LEVEL_DAY - LIGHT_LEVEL_NIGHT)/30;
		lightChange = true;
		break;
	default:
		break;
	}
	
	if(newlightlevel <= LIGHT_LEVEL_NIGHT){
		lightlevel = LIGHT_LEVEL_NIGHT;
		light_state = LIGHT_STATE_NIGHT;
	}
	else if(newlightlevel >= LIGHT_LEVEL_DAY){
		lightlevel = LIGHT_LEVEL_DAY;
		light_state = LIGHT_STATE_DAY;
	}
	else{
		lightlevel = newlightlevel;
	}
	
	if(lightChange){
		LightInfo lightInfo;
		lightInfo.level = lightlevel;
		lightInfo.color = 0xD7;
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
			(*it).second->sendWorldLight(lightInfo);
		}
	}
}

void Game::getWorldLightInfo(LightInfo& lightInfo)
{
	lightInfo.level = lightlevel;
	lightInfo.color = 0xD7;
}

void Game::addCommandTag(std::string tag)
{
	bool found = false;
	for(uint32_t i=0; i< commandTags.size() ;i++){
		if(commandTags[i] == tag){
			found = true;
			break;
		}
	}

	if(!found){
		commandTags.push_back(tag);
	}
}

void Game::resetCommandTag()
{
	commandTags.clear();
}

void Game::flushSendBuffers()
{	
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::flushSendBuffers()");

	for(std::vector<Player*>::iterator it = BufferedPlayers.begin(); it != BufferedPlayers.end(); ++it) {
		(*it)->flushMsg();
		(*it)->SendBuffer = false;
		(*it)->releaseThing2();
	}

	BufferedPlayers.clear();
	
	//free memory
	for(std::vector<Thing*>::iterator it = ToReleaseThings.begin(); it != ToReleaseThings.end(); ++it){
		(*it)->releaseThing2();
	}

	ToReleaseThings.clear();
		
	return;
}

void Game::addPlayerBuffer(Player* p)
{		
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::addPlayerBuffer()");

	if(p->SendBuffer == false){
		p->useThing2();
		BufferedPlayers.push_back(p);
		p->SendBuffer = true;
	}
	
	return;
}

void Game::FreeThing(Thing* thing)
{	
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::FreeThing()");
	ToReleaseThings.push_back(thing);
	
	return;
}

#ifdef __TLM_SERVER_SAVE__
void Game::saveServer()
{
	std::cout << ":: server save.. ";
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		(it->second)->loginPosition = (it->second)->getPosition();
		IOPlayer::instance()->savePlayer(it->second);
		
		#ifdef __XID_PREMIUM_SYSTEM__
		savePremium(it->second, 0, true);
		#endif
	}

	std::cout << "[done]" << std::endl;

	if(!g_bans.saveBans(g_config.getString(ConfigManager::BAN_FILE))){
		std::cout << ":: failed to save bans" << std::endl;
	}
	
	if(!Houses::getInstance().payHouses(false)){
		std::cout << ":: failed to save houses" << std::endl;
	}
	
	if(!map->saveMap("")){
		std::cout << ":: failed to save map" << std::endl;
	}

	#ifdef __JD_BED_SYSTEM__
	if(!g_beds.saveBeds()) {
		std::cout << ":: failed to save beds" << std::endl;
	}
	#endif

	#ifdef __YUR_GUILD_SYSTEM__
	Guilds::Save();
	#endif
}

void Game::autoServerSave()
{
	saveServer();
	addEvent(makeTask(g_config.getNumber(ConfigManager::SERVER_SAVE), std::mem_fun(&Game::autoServerSave)));
}
#endif

#ifdef __XID_PREMIUM_SYSTEM__
bool Game::savePremium(Player* player, unsigned long premiumTime, bool serverSave /*= false*/)
{
	if(!player)
		return false;

	if(player){
		Account account = IOAccount::instance()->loadAccount(player->getAccount());
		
		if(player->premiumTicks > std::time(NULL)){
			IOAccount::instance()->saveAccount(account, player->premiumTicks + premiumTime);
		}
		else if(!serverSave && player->premiumTicks == 0){
			IOAccount::instance()->saveAccount(account, std::time(NULL) + premiumTime);
		}
		
		checkPremium(player);
	}
	
	return true;
}

bool Game::checkPremium(Player* player)
{
	if(!player)
		return false;
	
    Account account = IOAccount::instance()->loadAccount(player->getAccount());    
	if(account.premEnd > std::time(NULL)){
		player->premiumTicks = account.premEnd;
	}
	else{
		if(!player->isPremium()){
			player->premiumTicks = 0;
			player->setVocation(player->vocation->getPreVocation());
		}
	}
}
#endif

#ifdef __YUR_SHUTDOWN__
void Game::sheduleShutdown(int minutes)
{
	if (minutes > 0)
		checkShutdown(minutes);
}

void Game::checkShutdown(int minutes)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(gameLock, "Game::checkShutdown()");
	if(minutes == 0){
		setGameState(GAME_STATE_CLOSED);

		while(!Player::listPlayer.list.empty())
			Player::listPlayer.list.begin()->second->kickPlayer();

		saveServer();
		std::cout << ":: Shutting down Server..." << std::endl;
		OTSYS_SLEEP(1000);
		exit(1);
	}
	else {
		std::stringstream msg;
		msg << "" << g_config.getString(ConfigManager::SERVER_NAME) << " is going down in " << minutes << (minutes > 1? " minutes!" : " minute!") << std::ends;

		AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
		while(it != Player::listPlayer.list.end()){
			(*it).second->sendTextMessage(MSG_STATUS_CONSOLE_RED, msg.str().c_str());
			++it;
		}

		addEvent(makeTask(60000, boost::bind(&Game::checkShutdown, this, minutes - 1)));
	}
}
#endif

#ifdef __XID_ACCOUNT_MANAGER__
void Game::manageAccount(Player *player, const std::string &text)
{
	if(player){
		std::string newtext = text;
		std::transform(newtext.begin(), newtext.end(), newtext.begin(), (int(*)(int))tolower);
		
		player->lastlogin = 0;
		if(player->isAccountManager){
			Account account = IOAccount::instance()->loadAccount(player->realAccount);
			//already has an account
			if(player->yesorno[11]){
				if(newtext == "yes"){
					if(!IOPlayer::instance()->playerExists(player->realName)){
						//let's save the player
						player->name = player->realName;
						player->accountNumber = player->realAccount;
					
						#ifdef USE_SQL_ENGINE
						IOPlayer::instance()->createPlayer(player);
						#endif
					
						IOPlayer::instance()->savePlayer(player);
					
						//don't forget to update the account
						account.charList.push_back(player->realName);
						IOAccount::instance()->saveAccount(account, player->premiumTicks);
					
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Alright! Your character has been made.");
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Re-login to see the character in the character-list.");
					
						player->yesorno[11] = false;
						player->yesorno[3] = true;
					
						player->realName = "";
						player->name = "Account Manager";
						player->accountNumber = account.accnumber;
					}
					else{
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Dang! Some error occured.");
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Do you want to create a new \'character\' or change your \'password\'?");

						player->yesorno[11] = false;
						player->yesorno[3] = true;
					}
				}
				else{
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Then not, what do you wish to do?");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Do you want to create a new \'character\' or change your \'password\'?");

					player->yesorno[11] = false;
					player->yesorno[3] = true;
				}
			}
			else if(player->yesorno[10]){
				for(VocationsMap::iterator it = g_vocations.getVocationBegin(); it != g_vocations.getVocationEnd(); ++it){
					std::string vocName = (it->second)->getVocName();
					std::transform(vocName.begin(), vocName.end(), vocName.begin(), tolower);
					if(newtext == vocName && it != g_vocations.getVocationEnd() && it->first == (it->second)->getPreVocation() && it->first != 0){
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "So you want to be " + (it->second)->getVocDescription() + ". Are you sure?");
						player->setVocation(it->first);
						
						int32_t newHealth = (it->second)->getHealthForLv(player->getLevel()) - player->getHealth();
						int32_t newMana = (it->second)->getManaForLv(player->getLevel()) - player->getMana();
						double newCapacity = (it->second)->getCapacityForLv(player->getLevel()) - player->getCapacity();
						
						player->healthMax = (it->second)->getHealthForLv(player->getLevel());
						player->manaMax = (it->second)->getManaForLv(player->getLevel());
						player->magLevel = (it->second)->getMagicLevelForManaSpent(player->getManaSpent());
						player->capacity += newCapacity;
						
						player->changeHealth(newHealth);
						player->changeMana(newMana);

						player->yesorno[10] = false;
						player->yesorno[11] = true;
						return;
					}
				}
			}
			else if(player->yesorno[9]){
				if(newtext == "male" || newtext == "female"){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Very good, what vocation would you like?");
					std::stringstream ss;
					int i = 0;
					
					for(VocationsMap::iterator it = g_vocations.getVocationEnd(); it != g_vocations.getVocationBegin(); it--){
						if(it->first != (it->second)->getPreVocation()){
							continue;
						}
						else if(it->first == 0){
							continue;
						}
						else if(i == 0){
							ss << "You can be a " << (it->second)->getVocName();
							i = 1;
						}
						else if(it->first-1 != 0){
							ss << ", a " << (it->second)->getVocName();
						}
						else{
							ss << " or a " << (it->second)->getVocName() << ".";
						}
					}
					
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, ss.str().c_str());
					
					if(newtext == "female"){
						player->setSex((playersex_t)0);
						player->defaultOutfit.lookType = 136;
					}
					else{
						player->setSex((playersex_t)1);
                        player->defaultOutfit.lookType = 128;
					}
				
					player->yesorno[9] = false;
					player->yesorno[10] = true;
					return;
				}
			}
			else if(player->yesorno[8]){
				if(newtext == "yes"){
					if(IOPlayer::instance()->playerExists(player->realName)){
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Sorry, that player name already exists.");
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Please choose another name.");
						
						player->realName = "";
						
						player->yesorno[7] = true;
						player->yesorno[8] = false;
					}
					else{
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Great. Is it a \'male\' or \'female\' character?");
						
						player->yesorno[8] = false;
						player->yesorno[9] = true;
					}
				}
				else{
					player->yesorno[8] = false;
					player->yesorno[3] = true;
					
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Then not.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Do you want to create a new \'character\' or change your \'password\'?");
					player->realName = "";
				}
			}
			else if(player->yesorno[7]){
				if(newtext.length() < 2 || newtext.length() > 20){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Sorry, the name has to be between 2 and 20 characters long.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Please choose another name.");
				}
				else if(newtext.substr(0, 2) == "gm" || newtext.substr(0, 15) == "account manager" || newtext.substr(0, 4) == "cs" || newtext.substr(0, 4) == "god") {
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Sorry, that name is not valid.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Please choose another name.");
				}
				else{
					for(int i = 0; i < newtext.length(); i++){
						if(!isdigit(newtext[i]) && !isalpha(newtext[i]) && newtext[i] != ' ') {
							player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Sorry, that name is not valid.");
							player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Please choose another name.");
							return;
						}
					}
					
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, ("Account Manager: " + text + " eh. Are you sure?").c_str());
					player->realName = text;
					player->yesorno[7] = false;
					player->yesorno[8] = true;
				}
			}
			else if(player->yesorno[6]){
				if(text != player->password){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Sorry, but if you tell me different passwords I don\'t know what to do.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Do you want to create a new \'character\' or change your \'password\'?");
					player->yesorno[6] = false;
					player->yesorno[3] = true;
					player->password = "";
				}
				else{
					//update the account
					account.password = player->password;
					IOAccount::instance()->saveAccount(account, player->premiumTicks);
					
					player->yesorno[6] = false;
					player->yesorno[3] = true;
					
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Your password has been changed.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Do you want to create a new \'character\' or change your \'password\'?");
				}
			}
			else if(player->yesorno[5]){
				if(newtext.length() < 4 || newtext.length() > 14) {
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: I don\'t like your password.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Please use a password that is between 4 and 12 characters long.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: And only contains numbers and letters.");
				}
				else{
					for(int i = 0; i < newtext.length(); i++){
						if(!isdigit(newtext[i]) && !isalpha(newtext[i])){
							player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: I don\'t like your password.");
							player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Please use a password that is between 4 and 12 characters long.");
							player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: And only contains numbers and letters.");
							return;
						}
					}
					
					player->password = text;
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Please repeat your password.");
					player->yesorno[5] = false;
					player->yesorno[6] = true;
				}
			}
			else if(player->yesorno[4]){
				if(newtext == "yes"){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Tell me your new password please.");
					player->yesorno[4] = false;
					player->yesorno[5] = true;
				}
				else{
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Then not.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Do you want to create a new \'character\' or change your \'password\'?");
					player->yesorno[4] = false;
					player->yesorno[3] = true;
				}
			}
			else if(player->yesorno[3]){
				if(newtext == "character"){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Okay. And what should the character\'s name be?");
					player->yesorno[3] = false;
					player->yesorno[7] = true;
				}
				else if(newtext == "password"){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Do you want to change your password?");
					player->yesorno[3] = false;
					player->yesorno[4] = true;
				}
				else {
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: You confused me. Let\'s start again.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Do you want to create a new \'character\' or change your \'password\'?");
				}
			}
			else{
				player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Something went wrong. Please re-login.");
			}
		}
		else{
			//via account 1/1
			if(player->yesorno[2]){
				if(newtext == "yes"){
					if(IOAccount::instance()->accountExists(player->realAccount)){
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Sorry, that account already exists.");
						player->realAccount = 111111;
					}
					else{
						int rnum = random_range(1000, 9999);
						std::stringstream password;
						password << "serv" << rnum;
						
						//create new account
						Account account;
						account.accnumber = player->realAccount;
						account.password = password.str();
						
						#ifdef USE_SQL_ENGINE
						IOAccount::instance()->createAccount(account);
						#endif
						
						IOAccount::instance()->saveAccount(account, 0);
						
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Okay. Your account has been made.");
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, ("Account Manager: Your new account is " + str(player->realAccount) + " and the password is " + password.str().c_str() + ".").c_str());
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Please notice that it is HIGHLY recommended that you change your password, or your account could get hacked.");
						player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Now I can make a new \'character\' or change your \'password\'.");
						
						player->yesorno[3] = true;
						player->isAccountManager = true;
						player->password = password.str();
					}
				}
				else {
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Then not.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: If you want, I can create a new \'account\' for you.");
					player->realAccount = 111111;
				}
				
				player->yesorno[2] = false;
			}
			else if(player->yesorno[1]) {
				unsigned long accnumber = atoi(newtext.c_str());
				if(accnumber == 0 || accnumber < 100000 || accnumber > 99999999) {
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: That\'s not a valid account number.");
				}
				else{
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, ("Account Manager: Account " + newtext + ". Are you sure?").c_str());
					player->realAccount = accnumber;
					player->yesorno[1] = false;
					player->yesorno[2] = true;
				}
			}
			else if(!player->yesorno[1]){
				if(newtext == "account"){
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: It\'s your lucky day. You can choose an account number.");
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: Tell me your wish.");
					player->yesorno[1] = true;
				}
				else{
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Account Manager: The only thing I can do for you now, is to create a new \'account\'.");
				}
			}
		}
	}
}
#endif

#ifdef __XID_EXPERIENCE_STAGES__
__int64 Game::getExperienceStage(int32_t level)
{
	std::list<Stage_t>::iterator it;
	uint32_t multiplier = g_config.getNumber(ConfigManager::RATE_EXPERIENCE);
	
	for(it = stageList.begin(); it != stageList.end(); it++){
		if(level >= (*it).minLv && level <= (*it).maxLv){
			multiplier = (*it).expMul;
		}
	}
	
	return multiplier;
}

bool Game::loadExperienceStages()
{
	std::string filename = g_config.getString(ConfigManager::DATA_DIRECTORY) + "stages.xml";
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	
	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"stages") != 0){
			xmlFreeDoc(doc);
			return false;
		}
		
		p = root->children;
		
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"stage") == 0){
				Stage_t stage;
				int intVal;
				
				if(readXMLInteger(p, "minlevel", intVal)){
					stage.minLv = intVal;
				}
				
				if(readXMLInteger(p, "maxlevel", intVal)){
					stage.maxLv = intVal;
				}
				
				if(readXMLInteger(p, "multiplier", intVal)){
					stage.expMul = intVal;
				}
				
				stageList.push_back(stage);
			}
			
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}
	
	return true;
}
#endif
