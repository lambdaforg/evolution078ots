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

#include "creature.h"
#include "game.h"
#include "otsystem.h"
#include "player.h"
#include "npc.h"
#include "monster.h"
#include "container.h"
#include "condition.h"
#include "combat.h"

#include <string>
#include <sstream>
#include <algorithm>
#include "configmanager.h"

#include <vector>

OTSYS_THREAD_LOCKVAR AutoID::autoIDLock;
unsigned long AutoID::count = 1000;
AutoID::list_type AutoID::list;

extern Game g_game;
extern ConfigManager g_config;

Creature::Creature() :
  isInternalRemoved(false)
{
	direction  = NORTH;
	master = NULL;
	summon = false;

	health     = 1000;
	healthMax  = 1000;
	mana = 0;
	manaMax = 0;
	
	lastMove = 0;
	lastStepCost = 1;
	baseSpeed = 220;
	varSpeed = 0;

	masterRadius = -1;
	masterPos.x = 0;
	masterPos.y = 0;
	masterPos.z = 0;

	attackStrength = 0;
	defenseStrength = 0;

	followCreature = NULL;
	eventWalk = 0;
	internalUpdateFollow = false;

	eventCheck = 0;
	eventCheckAttacking = 0;
	attackedCreature = NULL;
	lastHitCreature = 0;
	blockCount = 0;
	blockTicks = 0;
	
	#ifdef __TC_GM_INVISIBLE__
	gmInvisible = false;
	#endif	
}

Creature::~Creature()
{
	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(NULL);
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();
	}

	summons.clear();

	for(ConditionList::iterator it = conditions.begin(); it != conditions.end(); ++it){
		(*it)->endCondition(this, REASON_ABORT);
		delete *it;
	}

	conditions.clear();

	attackedCreature = NULL;
}

void Creature::setRemoved()
{
	isInternalRemoved = true;
}

bool Creature::canSee(const Position& pos) const
{
	const Position& myPos = getPosition();

	if(myPos.z <= 7){
		//we are on ground level or above (7 -> 0)
		//view is from 7 -> 0
		if(pos.z > 7){
			return false;
		}
	}
	else if(myPos.z >= 8){
		//we are underground (8 -> 15)
		//view is +/- 2 from the floor we stand on
		if(std::abs(myPos.z - pos.z) > 2){
			return false;
		}
	}

	int offsetz = myPos.z - pos.z;

	if ((pos.x >= myPos.x - 8 + offsetz) && (pos.x <= myPos.x + 9 + offsetz) &&
		(pos.y >= myPos.y - 6 + offsetz) && (pos.y <= myPos.y + 7 + offsetz))
		return true;

	return false;
}

bool Creature::canSeeCreature(const Creature* creature) const
{
	if(!canSeeInvisibility() && creature->isInvisible()){
		return false;
	}

	return true;
}

void Creature::addEventThink()
{
	if(eventCheck == 0){
		eventCheck = g_game.addEvent(makeTask(1000, boost::bind(&Game::checkCreature, &g_game, getID(), 1000)));
		//onStartThink();
	}
}

void Creature::addEventThinkAttacking()
{
	if(eventCheckAttacking == 0){
		int attackSpeed = 500;
		if(getPlayer()){
			attackSpeed = getPlayer()->vocation->getAttackSpeed();
		}
		
		eventCheckAttacking = g_game.addEvent(makeTask(attackSpeed, boost::bind(&Game::checkCreatureAttacking, &g_game, getID(), attackSpeed)));
	}
}

void Creature::stopEventThink()
{
	if(eventCheck != 0){
		g_game.stopEvent(eventCheck);
		eventCheck = 0;
	}
}

void Creature::stopEventThinkAttacking()
{
	if(eventCheckAttacking != 0){
		g_game.stopEvent(eventCheckAttacking);
		eventCheckAttacking = 0;
	}
}

void Creature::onThink(uint32_t interval)
{
	if(eventCheck != 0){
		eventCheck = 0;
		addEventThink();
	}
}

void Creature::onThinkAttacking(uint32_t interval)
{
	if(followCreature && getMaster() != followCreature && !canSeeCreature(followCreature)){
		onCreatureDisappear(followCreature, false);
	}

	if(attackedCreature && getMaster() != attackedCreature && !canSeeCreature(attackedCreature)){
		onCreatureDisappear(attackedCreature, false);
	}

	if(internalUpdateFollow && followCreature){
		internalUpdateFollow = false;
		setFollowCreature(followCreature);
	}

	blockTicks += interval;

	if(blockTicks >= 1000){
		blockCount = std::min((uint32_t)blockCount + 1, (uint32_t)2);
		blockTicks = 0;
	}

	onAttacking(interval);

	if(eventCheckAttacking != 0){
		eventCheckAttacking = 0;
		addEventThinkAttacking();
	}
}

void Creature::onAttacking(uint32_t interval)
{
	if(attackedCreature){
		onAttacked();
		attackedCreature->onAttacked();

		if(g_game.canThrowObjectTo(getPosition(), attackedCreature->getPosition())){
			doAttacking(interval);
		}
	}
}

void Creature::onWalk()
{
	if(getSleepTicks() <= 0){
		Direction dir;
		if(getNextStep(dir)){
			ReturnValue ret = g_game.internalMoveCreature(this, dir);

			if(ret != RET_NOERROR){
				internalUpdateFollow = true;
			}
		}
	}

	if(eventWalk != 0){
		eventWalk = 0;
		addEventWalk();
	}
}

void Creature::onWalk(Direction& dir)
{
	if(hasCondition(CONDITION_DRUNK)){
		uint32_t r = random_range(0, 16);

		if(r <= 4){
			switch(r){
				case 0: dir = NORTH; break;
				case 1: dir = WEST;  break;
				case 3: dir = SOUTH; break;
				case 4: dir = EAST;  break;

				default:
					break;
			}

			g_game.internalCreatureSay(this, SPEAK_SAY, "Hicks!");
		}
	}
}

bool Creature::getNextStep(Direction& dir)
{
	if(!listWalkDir.empty()){
		Position pos = getPosition();
		dir = listWalkDir.front();
		listWalkDir.pop_front();
		onWalk(dir);

		return true;
	}

	return false;
}

bool Creature::startAutoWalk(std::list<Direction>& listDir)
{
	listWalkDir = listDir;
	addEventWalk();
	return true;
}

void Creature::addEventWalk()
{
	if(eventWalk == 0){
		int64_t ticks = getEventStepTicks();
		eventWalk = g_game.addEvent(makeTask(ticks, std::bind2nd(std::mem_fun(&Game::checkWalk), getID())));
	}
}

void Creature::stopEventWalk()
{
	if(eventWalk != 0){
		g_game.stopEvent(eventWalk);
		eventWalk = 0;

		if(!listWalkDir.empty()){
			listWalkDir.clear();
			onWalkAborted();
		}
	}
}

void Creature::validateWalkPath()
{
	if(!internalUpdateFollow && followCreature){
		if(listWalkDir.empty() || !g_game.isPathValid(this, listWalkDir, followCreature->getPosition())){
			internalUpdateFollow = true;
		}
	}
}

void Creature::onAddTileItem(const Position& pos, const Item* item)
{
	validateWalkPath();
}

void Creature::onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem)
{
	validateWalkPath();
}

void Creature::onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	validateWalkPath();
}

void Creature::onUpdateTile(const Position& pos)
{
	validateWalkPath();
}

void Creature::onCreatureAppear(const Creature* creature, bool isLogin)
{
	validateWalkPath();
}

void Creature::onCreatureDisappear(const Creature* creature, bool isLogout)
{
	if(attackedCreature == creature){
		setAttackedCreature(NULL);
		onAttackedCreatureDissapear(isLogout);
	}

	if(followCreature == creature){
		setFollowCreature(NULL);
		onFollowCreatureDissapear(isLogout);
	}
}

void Creature::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	validateWalkPath();

	onCreatureDisappear(creature, true);
}

void Creature::onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos,
	uint32_t oldStackPos, bool teleport)
{
	if(creature == this){
		lastMove = OTSYS_TIME();
		
		lastStepCost = 1;

		if(!teleport){
			if(oldPos.z != newPos.z){
				//floor change extra cost
				lastStepCost = 2;
			}
			else if(std::abs(newPos.x - oldPos.x) >=1 && std::abs(newPos.y - oldPos.y) >= 1){
				//diagonal extra cost
				lastStepCost = 2;
			}
		}
	}

	if(followCreature == creature || (creature == this && followCreature)){
		if(newPos.z != oldPos.z || !canSee(followCreature->getPosition())){
			onCreatureDisappear(followCreature, false);
		}
		
		validateWalkPath();
	}

	if(attackedCreature == creature || (creature == this && attackedCreature)){
		if(newPos.z != oldPos.z || !canSee(attackedCreature->getPosition())){
			onCreatureDisappear(attackedCreature, false);
		}
		else if(attackedCreature->isInPz() || isInPz()){
			onCreatureDisappear(attackedCreature, false);
		}
	}
}

void Creature::onCreatureChangeVisible(const Creature* creature, bool visible)
{
	/*
	if(!visible && !canSeeInvisibility() && getMaster() != creature){
		if(followCreature == creature){
			onCreatureDisappear(followCreature, false);
		}

		if(attackedCreature == creature){
			onCreatureDisappear(attackedCreature, false);
		}
	}
	*/
}

void Creature::die()
{
	Creature* lastHitCreature = NULL;
	Creature* mostDamageCreature = NULL;

	if(getKillers(&lastHitCreature, &mostDamageCreature)){
		if(lastHitCreature){
			lastHitCreature->onKilledCreature(this);
		}

		if(mostDamageCreature && mostDamageCreature != lastHitCreature){
			mostDamageCreature->onKilledCreature(this);
		}
	}

	for(DamageMap::iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		if(Creature* attacker = g_game.getCreatureByID((*it).first)){
			attacker->onAttackedCreatureKilled(this);
		}
	}

	if(getMaster()){
		getMaster()->removeSummon(this);
	}
}

bool Creature::getKillers(Creature** _lastHitCreature, Creature** _mostDamageCreature)
{
	*_lastHitCreature = g_game.getCreatureByID(lastHitCreature);

	int32_t mostDamage = 0;
	damageBlock_t db;
	for(DamageMap::iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		db = it->second;

		if((db.total > mostDamage && (OTSYS_TIME() - db.ticks <= g_game.getInFightTicks()))){
			if(*_mostDamageCreature = g_game.getCreatureByID((*it).first)){
				mostDamage = db.total;
			}
		}
	}

	return (*_lastHitCreature || *_mostDamageCreature);
}

bool Creature::hasBeenAttacked(uint32_t attackerId)
{
	DamageMap::iterator it = damageMap.find(attackerId);
	if(it != damageMap.end()){
		return (OTSYS_TIME() - it->second.ticks <= g_game.getInFightTicks());
	}

	return false;
}

Item* Creature::getCorpse()
{
	Item* corpse = Item::CreateItem(getLookCorpse());

	/*
	if(corpse){
		if(Container* corpseContainer = corpse->getContainer()){
			dropLoot(corpseContainer);
		}
	}
	*/

	return corpse;
}

void Creature::changeHealth(int32_t healthChange)
{
	if(healthChange > 0){
		health += std::min(healthChange, healthMax - health);
	}
	else{
		health = std::max((int32_t)0, health + healthChange);
	}

	g_game.addCreatureHealth(this);
}

void Creature::changeMana(int32_t manaChange)
{
	if(manaChange > 0){
		mana += std::min(manaChange, manaMax - mana);
	}
	else{
		mana = std::max((int32_t)0, mana + manaChange);
	}
}

void Creature::drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage)
{
	changeHealth(-damage);

	if(attacker){
		attacker->onAttackedCreatureDrainHealth(this, damage);
	}
}

void Creature::drainMana(Creature* attacker, int32_t manaLoss)
{
	onAttacked();
	changeMana(-manaLoss);
}

BlockType_t Creature::blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
	bool checkDefense /* = false */, bool checkArmor /* = false */)
{
	BlockType_t blockType = BLOCK_NONE;

	if(blockType == BLOCK_NONE && isImmune(combatType)){
		damage = 0;
		blockType = BLOCK_IMMUNITY;
	}

	if((checkDefense || checkArmor) && blockCount > 0){
		--blockCount;

		#ifdef __XID_CVS_MODS__
		Player* attackPlayer = attacker->getPlayer();
		Player* attackedPlayer = getPlayer();
		double newDamage = damage;
		
		if(attackPlayer){
			newDamage *= attackPlayer->vocation->getDamageMultiplier();
			
			damage = (int32_t)newDamage;
			
			for(int i = SLOT_FIRST; i < SLOT_LAST; i++){
				Item* item = NULL;
				if(attackPlayer->inventory[i]){
					item = attackPlayer->inventory[i];
					int increasePhysicalPercent = Item::items[item->getID()].increasePhysicalPercent;
					
    				if(item && item->getSlotPosition() << i && increasePhysicalPercent != 0){
     	   				newDamage += (damage * increasePhysicalPercent)/100;
					}
				}
			}
		}
		
		damage = (int32_t)newDamage;
		#endif

		if(blockType == BLOCK_NONE && checkDefense){
			double defense = getDefense();
			defense = defense + (defense * defenseStrength) / 100;
			
			#ifdef __XID_CVS_MODS__
			if(attackedPlayer){
				defense *= attackedPlayer->vocation->getDefenseMultiplier();
			}
			#endif

			damage = damage - (int32_t)defense;
			if(damage <= 0){
				damage = 0;
				blockType = BLOCK_DEFENSE;
			}
		}
		
		if(blockType == BLOCK_NONE && checkArmor){
			double armor = getArmor();

			#ifdef __XID_CVS_MODS__
			if(attackedPlayer){
				armor *= attackedPlayer->vocation->getArmorMultiplier();
			}
			#endif

			damage -= (int32_t)armor;
			if(damage <= 0){
				damage = 0;
				blockType = BLOCK_ARMOR;
			}
		}
	}

	if(attacker){
		attacker->onAttackedCreature(this);
		attacker->onAttackedCreatureBlockHit(this, blockType);
	}

	onAttacked();

	return blockType;
}

bool Creature::setAttackedCreature(Creature* creature)
{
	if(creature){
		const Position& creaturePos = creature->getPosition();
		if(creaturePos.z != getPosition().z || !canSee(creaturePos)){
			attackedCreature = NULL;
			return false;
		}
	}

	attackedCreature = creature;

	if(attackedCreature){
		onAttackedCreature(attackedCreature);
		attackedCreature->onAttacked();
	}

	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(creature);
	}

	return true;
}

void Creature::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	fpp.fullPathSearch = false;
	fpp.needReachable = true;
	fpp.targetDistance = 1;

	if(followCreature != creature || !g_game.canThrowObjectTo(getPosition(), creature->getPosition())){
		fpp.fullPathSearch = true;
	}
}

bool Creature::setFollowCreature(Creature* creature)
{
	if(creature){
		const Position& creaturePos = creature->getPosition();
		if(creaturePos.z != getPosition().z || !canSee(creaturePos)){
			followCreature = NULL;
			return false;
		}
	}

	if(creature){
		FindPathParams fpp;
		getPathSearchParams(creature, fpp);

		listWalkDir.clear();
		//uint32_t maxDistance = getFollowDistance();
		//bool fullPathSearch = getFullPathSearch();
		//bool reachable = getFollowReachable();
		if(!g_game.getPathToEx(this, creature->getPosition(), 1, fpp.targetDistance, fpp.fullPathSearch, fpp.needReachable, listWalkDir)){
			followCreature = NULL;
			return false;
		}

		startAutoWalk(listWalkDir);
	}

	followCreature = creature;
	onFollowCreature(creature);
	return true;
}

double Creature::getDamageRatio(Creature* attacker) const
{
	int32_t totalDamage = 0;
	int32_t attackerDamage = 0;

	damageBlock_t db;
	for(DamageMap::const_iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		db = it->second;

		totalDamage += db.total;

		if(it->first == attacker->getID()){
			attackerDamage += db.total;
		}
	}

	return ((double)attackerDamage / totalDamage);
}

int64_t Creature::getGainedExperience(Creature* attacker) const
{
	int64_t lostExperience = getLostExperience();
	
	#ifdef __XID_EXPERIENCE_STAGES__
	if(Player* player = attacker->getPlayer()){
		return (int64_t)std::floor(getDamageRatio(attacker) * lostExperience * g_game.getExperienceStage(player->getLevel()));
	}
	else{
		return (int64_t)std::floor(getDamageRatio(attacker) * lostExperience * g_config.getNumber(ConfigManager::RATE_EXPERIENCE));
	}
	#else
	return (int64_t)std::floor(getDamageRatio(attacker) * lostExperience * g_config.getNumber(ConfigManager::RATE_EXPERIENCE));
	#endif
}

bool Creature::addDamagePoints(Creature* attacker, int32_t damagePoints)
{
	if(damagePoints > 0){
		uint32_t attackerId = (attacker ? attacker->getID() : 0);
		//damageMap[attackerId] += damagePoints;

		DamageMap::iterator it = damageMap.find(attackerId);

		if(it == damageMap.end()){
			damageBlock_t db;
			db.ticks = OTSYS_TIME();
			db.total = damagePoints;
			damageMap[attackerId] = db;
		}
		else{
			it->second.total += damagePoints;
			it->second.ticks = OTSYS_TIME();
		}

		lastHitCreature = attackerId;
	}

	return true;
}

void Creature::onAddCondition(ConditionType_t type)
{
	if(type == CONDITION_PARALYZE && hasCondition(CONDITION_HASTE)){
		removeCondition(CONDITION_HASTE);
	}
	else if(type == CONDITION_HASTE && hasCondition(CONDITION_PARALYZE)){
		removeCondition(CONDITION_PARALYZE);
	}
}

void Creature::onAddCombatCondition(ConditionType_t type)
{
	//
}

void Creature::onEndCondition(ConditionType_t type)
{
	//
}

void Creature::onTickCondition(ConditionType_t type, bool& bRemove)
{
	if(const MagicField* field = getTile()->getFieldItem()){
		switch(type){
			case CONDITION_FIRE: bRemove = (field->getCombatType() != COMBAT_FIREDAMAGE); break;
			case CONDITION_ENERGY: bRemove = (field->getCombatType() != COMBAT_ENERGYDAMAGE); break;
			case CONDITION_POISON: bRemove = (field->getCombatType() != COMBAT_POISONDAMAGE); break;
			case CONDITION_DROWN: bRemove = (field->getCombatType() != COMBAT_DROWNDAMAGE); break;
			default: 
				break;
		}
	}
}

void Creature::onCombatRemoveCondition(const Creature* attacker, Condition* condition)
{
	removeCondition(condition);
}

void Creature::onAttackedCreature(Creature* target)
{
	//
}

void Creature::onAttacked()
{
	//
}

void Creature::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	target->addDamagePoints(this, points);
}

void Creature::onAttackedCreatureKilled(Creature* target)
{
	if(target != this){
		int64_t gainedExperience = target->getGainedExperience(this);
		onGainExperience(gainedExperience);
	}
}

void Creature::onKilledCreature(Creature* target)
{
	if(getMaster()){
		getMaster()->onKilledCreature(target);
	}
	
	#ifdef __JD_DEATH_LIST__
	if(target->getPlayer()){
		target->getPlayer()->addDeath(getName(), target->getPlayer()->getLevel(), std::time(NULL));
	}
	#endif
}

void Creature::onGainExperience(int64_t gainExperience)
{
	if(gainExperience > 0){
		if(getMaster()){
			gainExperience = gainExperience / 2;
			getMaster()->onGainExperience(gainExperience);
		}

		std::stringstream strExp;
		strExp << gainExperience;

		g_game.addAnimatedText(getPosition(), TEXTCOLOR_WHITE_EXP, strExp.str());
	}
}

void Creature::onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType)
{
	//
}

void Creature::addSummon(Creature* creature)
{
	creature->setMaster(this);
	creature->useThing2();
	summons.push_back(creature);
}

void Creature::removeSummon(const Creature* creature)
{
	std::list<Creature*>::iterator cit = std::find(summons.begin(), summons.end(), creature);
	if(cit != summons.end()){
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();
		summons.erase(cit);
	}
}

bool Creature::addCondition(Condition* condition)
{
	if(condition == NULL){
		return false;
	}

	Condition* prevCond = getCondition(condition->getType(), condition->getId());

	if(prevCond){
		prevCond->addCondition(this, condition);
		delete condition;
		return true;
	}

	if(condition->startCondition(this)){
		conditions.push_back(condition);
		onAddCondition(condition->getType());
		return true;
	}

	delete condition;
	return false;
}

bool Creature::addCombatCondition(Condition* condition)
{
	if(!addCondition(condition)){
		return false; 
	} 

	onAddCombatCondition(condition->getType());
	return true;
}

void Creature::removeCondition(ConditionType_t type)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getType() == type){
			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, REASON_ABORT);
			delete condition;

			onEndCondition(type);
		}
		else{
			++it;
		}
	}
}

void Creature::removeCondition(ConditionType_t type, ConditionId_t id)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getType() == type && (*it)->getId() == id){
			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, REASON_ABORT);
			delete condition;

			onEndCondition(type);
		}
		else{
			++it;
		}
	}
}

void Creature::removeCondition(const Creature* attacker, ConditionType_t type)
{
	ConditionList tmpList = conditions;

	for(ConditionList::iterator it = tmpList.begin(); it != tmpList.end(); ++it){
		if((*it)->getType() == type){
			onCombatRemoveCondition(attacker, *it);
		}
	}
}

void Creature::removeCondition(Condition* condition)
{
	ConditionList::iterator it = std::find(conditions.begin(), conditions.end(), condition);

	if(it != conditions.end()){
		Condition* condition = *it;
		it = conditions.erase(it);

		condition->endCondition(this, REASON_ABORT);
		onEndCondition(condition->getType());
		delete condition;
	}
}

Condition* Creature::getCondition(ConditionType_t type, ConditionId_t id) const
{
	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type && (*it)->getId() == id){
			return *it;
		}
	}

	return NULL;
}

void Creature::executeConditions(int32_t newticks)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		//(*it)->executeCondition(this, newticks);
		//if((*it)->getTicks() <= 0){

		if(!(*it)->executeCondition(this, newticks)){
			ConditionType_t type = (*it)->getType();

			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, REASON_ENDTICKS);
			delete condition;

			onEndCondition(type);
		}
		else{
			++it;
		}
	}
}

bool Creature::hasCondition(ConditionType_t type) const
{
	if(isSuppress(type)){
		return false;
	}

	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type){
			return true;
		}
	}

	return false;
}

bool Creature::isImmune(CombatType_t type) const
{
	return ((getDamageImmunities() & (uint32_t)type) == (uint32_t)type);
}

bool Creature::isImmune(ConditionType_t type) const
{
	return ((getConditionImmunities() & (uint32_t)type) == (uint32_t)type);
}

bool Creature::isSuppress(ConditionType_t type) const
{
	return ((getConditionSuppressions() & (uint32_t)type) == (uint32_t)type);
}

std::string Creature::getDescription(int32_t lookDistance) const
{
	std::string str = "a creature";
	return str;
}

void Creature::updateLookDirection(const Creature* creature /*= NULL*/)
{
	Direction newDir = getDirection();
	if(!creature){
		creature = attackedCreature;
	}
	
	if(creature){
		const Position& pos = getPosition();
		const Position& creaturePos = creature->getPosition();
		int32_t dx = creaturePos.x - pos.x;
		int32_t dy = creaturePos.y - pos.y;

		if(std::abs(dx) > std::abs(dy)){
			//look EAST/WEST
			if(dx < 0){
				newDir = WEST;
			}
			else{
				newDir = EAST;
			}
		}
		else if(std::abs(dx) < std::abs(dy)){
			//look NORTH/SOUTH
			if(dy < 0){
				newDir = NORTH;
			}
			else{
				newDir = SOUTH;
			}
		}
		else{
			if(dx < 0 && dy < 0){
				if(getDirection() == SOUTH){
					newDir = WEST;
				}
				else if(getDirection() == EAST){
					newDir = NORTH;
				}
			}
			else if(dx < 0 && dy > 0){
				if(getDirection() == NORTH){
					newDir = WEST;
				}
				else if(getDirection() == EAST){
					newDir = SOUTH;
				}
			}
			else if(dx > 0 && dy < 0){
				if(getDirection() == SOUTH){
					newDir = EAST;
				}
				else if(getDirection() == WEST){
					newDir = NORTH;
				}
			}
			else{
				if(getDirection() == NORTH){
					newDir = EAST;
				}
				else if(getDirection() == WEST){
					newDir = SOUTH;
				}
			}
		}
	}

	g_game.internalCreatureTurn(this, newDir);
}

int Creature::getStepDuration() const
{
	OTSYS_THREAD_LOCK_CLASS lockClass(g_game.gameLock, "Creature::getStepDuration()");

	int32_t duration = 0;

	if(isRemoved()){
		return duration;
	}

	const Position& tilePos = getPosition();
	Tile* tile = g_game.getTile(tilePos.x, tilePos.y, tilePos.z);
	if(tile && tile->ground){
		uint32_t groundId = tile->ground->getID();
		uint16_t stepSpeed = Item::items[groundId].speed;

		if(stepSpeed != 0){
			duration =  (1000 * stepSpeed) / getSpeed();
		}
	}

	return duration * lastStepCost;
};

int64_t Creature::getSleepTicks() const
{
	int64_t delay = 0;
	int stepDuration = getStepDuration();
	
	if(lastMove != 0) {
		delay = (((int64_t)(lastMove)) + ((int64_t)(stepDuration))) - ((int64_t)(OTSYS_TIME()));
	}
	
	return delay;
}

int64_t Creature::getEventStepTicks() const
{
	int64_t ret = getSleepTicks();

	if(ret <= 0){
		ret = getStepDuration();
	}

	return ret;
}

void Creature::getCreatureLight(LightInfo& light) const
{
	light = internalLight;
}

void Creature::setNormalCreatureLight()
{
	internalLight.level = 0;
	internalLight.color = 0;
}
