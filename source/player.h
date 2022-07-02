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

#ifndef __OTSERV_PLAYER_H__
#define __OTSERV_PLAYER_H__

#include "definitions.h"
#include "creature.h"
#include "container.h"
#include "depot.h"
#include "cylinder.h"
#include "outfit.h"
#include "enums.h"
#include "vocation.h"
#include "protocol79.h"
#include "guilds.h"

#ifdef __PARTYSYSTEM__
#include "party.h"
class Party;
#endif

#include <vector>
#include <ctime>
#include <algorithm>

class House;
class Weapon;
class Protocol79;

enum skillsid_t {
	SKILL_LEVEL=0,
	SKILL_TRIES=1,
	SKILL_PERCENT=2
};

enum playerinfo_t {
	PLAYERINFO_LEVEL,
	PLAYERINFO_LEVELPERCENT,
	PLAYERINFO_HEALTH,
	PLAYERINFO_MAXHEALTH,
	PLAYERINFO_MANA,
	PLAYERINFO_MAXMANA,
	PLAYERINFO_MANAPERCENT,
	PLAYERINFO_MAGICLEVEL,
	PLAYERINFO_MAGICLEVELPERCENT,
	PLAYERINFO_SOUL,
};

enum freeslot_t {
	SLOT_TYPE_NONE,
	SLOT_TYPE_INVENTORY,
	SLOT_TYPE_CONTAINER
};

enum chaseMode_t {
	CHASEMODE_STANDSTILL,
	CHASEMODE_FOLLOW,
};

enum fightMode_t {
	FIGHTMODE_ATTACK,
	FIGHTMODE_BALANCED,
	FIGHTMODE_DEFENSE
};

enum tradestate_t {
	TRADE_NONE,
	TRADE_INITIATED,
	TRADE_ACCEPT,
	TRADE_ACKNOWLEDGE,
	TRADE_TRANSFER
};

struct walkToItem_t {
	int walkToType;
	Position fromPos, toPos;
	uint8_t fromStackPos, toStackPos;
	uint16_t fromSpriteId, toSpriteId;
	uint32_t index;
};

typedef std::pair<uint32_t, Container*> containervector_pair;
typedef std::vector<containervector_pair> ContainerVector;
typedef std::map<uint32_t, Depot*> DepotMap;
typedef std::map<uint32_t, int32_t> StorageMap;
typedef std::map<std::string, uint32_t> MuteCountMap;
#ifdef __XID_SEPERATE_ADDONS__
typedef std::map<uint32_t, uint32_t> KnowAddonMap;
#endif

const int MAX_VIPS = 50;

//////////////////////////////////////////////////////////////////////
// Defines a player...

class Player : public Creature, public Cylinder
{
public:
	Player(const std::string& name, Protocol79* p);
	virtual ~Player();
	
	virtual Player* getPlayer() {return this;};
	virtual const Player* getPlayer() const {return this;};

	static MuteCountMap muteCountMap;
	static int32_t maxMessageBuffer;

	virtual const std::string& getName() const {return name;};
	virtual const std::string& getNameDescription() const {return name;};
	virtual std::string getDescription(int32_t lookDistance) const;

	void setGUID(uint32_t _guid) {guid = _guid;};
	uint32_t getGUID() const { return guid;};
	virtual uint32_t idRange(){ return 0x10000000;}
	static AutoList<Player> listPlayer;
	void removeList();
	void addList();
	void kickPlayer() {client->logout();}
	
	uint32_t getGuildId() const {return guildId;};
	const std::string& getGuildName() const {return guildName;};
	const std::string& getGuildRank() const {return guildRank;};
	const std::string& getGuildNick() const {return guildNick;};
	
	void setGuildRank(const std::string& rank) {guildRank = rank;};
	void setGuildNick(const std::string& nick) {guildNick = nick;};
	
	void setFlags(uint32_t flags){};
	
	bool isOnline() {return (client != NULL);};
	uint32_t getIP() const;

	void addContainer(uint32_t containerid, Container* container);
	#ifdef __YUR_CLEAN_MAP__
	ContainerVector::const_iterator closeContainer(unsigned char containerid);
	#else
	void closeContainer(unsigned char containerid);
	#endif
	int32_t getContainerID(const Container* container) const;
	Container* getContainer(uint32_t cid);

	ContainerVector::const_iterator getContainers() const { return containerVec.begin();}
	ContainerVector::const_iterator getEndContainer() const { return containerVec.end();}
	
	void addStorageValue(const uint32_t key, const int32_t value);
	bool getStorageValue(const uint32_t key, int32_t& value) const;
	void genReservedStorageRange();
	
	inline StorageMap::const_iterator getStorageIteratorBegin() const {return storageMap.begin();}
	inline StorageMap::const_iterator getStorageIteratorEnd() const {return storageMap.end();}

	#ifdef __XID_SEPERATE_ADDONS__
	KnowAddonMap::const_iterator getKnowAddonBegin() const {return knowAddonMap.begin();}
	KnowAddonMap::const_iterator getKnowAddonEnd() const {return knowAddonMap.end();}
	
	void addAddon(uint32_t lookType, uint32_t addon);
	uint32_t getOutfitAddon(uint32_t lookType);
	#endif
	
	int32_t getAccount() const {return accountNumber;}
	int32_t getLevel() const {return level;}
	int32_t getMagicLevel() const {return magLevel;}
	int32_t getAccessLevel() const {return accessLevel;}

	void setVocation(uint32_t vocId);
	uint32_t getVocationId() const;

	void setSkillsPercents();

	playersex_t getSex() const {return sex;}	
	void setSex(playersex_t);
	int getPlayerInfo(playerinfo_t playerinfo) const;	
	int64_t getExperience() const {return experience;}
	uint32_t getManaSpent() const {return manaSpent;}

	time_t getLastLoginSaved() const { return lastLoginSaved; };
	const Position& getLoginPosition() { return loginPosition; };
	const Position& getTemplePosition() { return masterPos; };
	
	void setTown(uint32_t _town) {town = _town;}
	uint32_t getTown() const { return town; };

	virtual bool isPushable() const;
	virtual int getThrowRange() const {return 1;};

	bool isMuted(uint32_t& muteTime);
	void addMessageBuffer();
	void removeMessageBuffer();

	double getCapacity() const {
		if(getAccessLevel() < ACCESS_PROTECT) {
			return capacity;
		}
		else
			return 0.00;
	}
	
	double getFreeCapacity() const {
		if(accessLevel < ACCESS_PROTECT) {
			return std::max(0.00, capacity - inventoryWeight);
		}
		else
			return 0.00;
	}
	
	Item* getInventoryItem(slots_t slot) const;

	bool isItemAbilityEnabled(slots_t slot) const {return inventoryAbilities[slot];}
	void setItemAbility(slots_t slot, bool enabled) {inventoryAbilities[slot] = enabled;}

	int32_t getVarSkill(skills_t skill) const {return varSkills[skill];}
	void setVarSkill(skills_t skill, int32_t modifier) {varSkills[skill] += modifier;}
	void setConditionSuppressions(uint32_t conditions, bool remove);

	Depot* getDepot(uint32_t depotId, bool autoCreateDepot);
	bool addDepot(Depot* depot, uint32_t depotId);
	
	virtual bool canSee(const Position& pos) const;
	virtual bool canSeeCreature(const Creature* creature) const;
	
	virtual RaceType_t getRace() const {return RACE_BLOOD;}

	//safe-trade functions
	void setTradeState(tradestate_t state) {tradeState = state;};
	tradestate_t getTradeState() {return tradeState;};
	Item* getTradeItem() {return tradeItem;};
	
	//V.I.P. functions
	std::string vip[MAX_VIPS];
	void sendVIPLogin(std::string vipname);
	void sendVIPLogout(std::string vipname);

	//follow functions
	virtual bool setFollowCreature(Creature* creature);

	//follow events
	virtual void onFollowCreature(const Creature* creature);

	//walk events
	virtual void onWalk();
	virtual void onWalk(Direction& dir);
	virtual void onWalkAborted();

	void setChaseMode(chaseMode_t mode);
	void setFightMode(fightMode_t mode);

	//combat functions
	virtual bool setAttackedCreature(Creature* creature);
	bool isImmune(CombatType_t type) const;
	bool isImmune(ConditionType_t type) const;
	bool hasShield() const;
	virtual bool isAttackable() const;
	
	virtual void changeHealth(int32_t healthChange);
	virtual void changeMana(int32_t manaChange);
	void changeSoul(int32_t soulChange);

	bool isPzLocked() const { return pzLocked; }
	virtual BlockType_t blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
		bool checkDefense = false, bool checkArmor = false);
	virtual void doAttacking(uint32_t interval);
	int32_t getWeaponDamage();

	int getSkill(skills_t skilltype, skillsid_t skillinfo) const;
	uint32_t getSkillPoint() const {return skillPoint;}

	virtual void drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage);
	virtual void drainMana(Creature* attacker, int32_t manaLoss);	
	void addManaSpent(uint32_t amount);
	void addSkillAdvance(skills_t skill, uint32_t count);

	virtual int32_t getArmor() const;
	virtual int32_t getDefense() const;

	void addExhaustionTicks();
	void addHealExhaustionTicks();
	void addInFightTicks();
	void addDefaultRegeneration(uint32_t addTicks);

	virtual void die();
	virtual Item* getCorpse();
	virtual int64_t getGainedExperience(Creature* attacker) const;

	//combat event functions
	virtual void onAddCondition(ConditionType_t type);
	virtual void onAddCombatCondition(ConditionType_t type);
	virtual void onEndCondition(ConditionType_t type);
	virtual void onCombatRemoveCondition(const Creature* attacker, Condition* condition);
	virtual void onAttackedCreature(Creature* target);
	virtual void onAttacked();
	virtual void onAttackedCreatureDrainHealth(Creature* target, int32_t points);
	virtual void onKilledCreature(Creature* target);
	virtual void onGainExperience(int64_t gainExperience);
	virtual void onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType);

	virtual void getCreatureLight(LightInfo& light) const;

	bool setFollowItem(const Position& fromPos);
	void setFollowItem(int walkToType, const Position& fromPos, uint8_t fromStackPos, int16_t fromSpriteId,
		const Position& toPos, uint8_t toStackPos, uint16_t toSpriteId, uint8_t index);

	#ifdef __SKULLSYSTEM__
	Skulls_t getSkull() const;
	Skulls_t getSkullClient(const Player* player) const;
	bool hasAttacked(const Player* attacked) const;
	void addAttacked(const Player* attacked);
	void clearAttacked();
	void addUnjustifiedDead(const Player* attacked);
	void setSkull(Skulls_t newSkull) {skull = newSkull;}
	void sendCreatureSkull(const Creature* creature, Skulls_t newSkull = SKULL_NONE) const
		{client->sendCreatureSkull(creature, newSkull);}
	void checkRedSkullTicks(long ticks);
	#endif

	#ifdef __XID_PREMIUM_SYSTEM__
	bool isPremium() const{ 
		return premiumTicks > 0 || FREE_PREMIUM == "yes";
	}
	unsigned long premiumTicks;
	#endif

	#ifdef __YUR_GUILD_SYSTEM__
	void setGuildInfo(gstat_t gstat, unsigned long gid, std::string gname, std::string grank, std::string nick);
	#endif

	#ifdef __TR_ANTI_AFK__
	void checkAfk(int thinkTics);
	void notAfk();
	#endif

	#ifdef __JD_DEATH_LIST__
	void addDeath(const std::string& killer, int level, time_t time);
	#endif    

	#ifdef __XID_LEARN_SPELLS__
	void learnSpell(const std::string& words);
	bool knowsSpell(const std::string& words) const;
	#endif

	#ifdef __XID_ROOKGARD__
	bool isRookie() const;
	#endif

	#ifdef __XID_ACCOUNT_MANAGER__
	bool isAccountManager; 
	unsigned long realAccount;
	std::string realName;
	
	bool yesorno[20];
	#endif
	
	#ifdef __XID_PROTECTION_SYSTEM__
	bool isProtected(Player* player) const{ 
		return (level < PROTECTION_LIMIT) || player->getLevel() < PROTECTION_LIMIT;
	}
	#endif
	
	#ifdef __XID_BLESS_SYSTEM__
	void addBlessing(int32_t blessId){ blessMap[blessId] = true;};
	bool getBlessing(int32_t blessId) const;
	void getBlessPercent(int64_t lostValue);
	#endif

	const OutfitListType& getPlayerOutfits();
	bool canWear(uint32_t _looktype);
	void addOutfit(uint32_t _looktype, uint32_t _premium);
	bool remOutfit(uint32_t _looktype, uint32_t _premium);
	
	//tile
	//send methods
	void sendAddTileItem(const Position& pos, const Item* item)
		{client->sendAddTileItem(pos, item);}
	void sendUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* olditem, const Item* newitem)
		{client->sendUpdateTileItem(pos, stackpos, newitem);}
	void sendRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item)
		{client->sendRemoveTileItem(pos, stackpos);}
	void sendUpdateTile(const Position& pos)
		{client->UpdateTile(pos);}

	void sendCreatureAppear(const Creature* creature, bool isLogin)
		{client->sendAddCreature(creature, isLogin);}
	void sendCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
		{client->sendRemoveCreature(creature, creature->getPosition(), stackpos, isLogout);}
	void sendCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos, uint32_t oldStackPos, bool teleport)
		{client->sendMoveCreature(creature, newPos, oldPos, oldStackPos, teleport);}

	void sendCreatureTurn(const Creature* creature, uint32_t stackpos)
		{client->sendCreatureTurn(creature, stackpos);}
	void sendCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text)
		{client->sendCreatureSay(creature, type, text);}
	void sendCreatureSquare(const Creature* creature, SquareColor_t color)
		{client->sendCreatureSquare(creature, color);}
	void sendCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit)
		{client->sendCreatureOutfit(creature, outfit);}
	void sendCreatureChangeVisible(const Creature* creature, bool visible)
		{
			if(visible)
				client->sendCreatureOutfit(creature, creature->getCurrentOutfit());
			else
				client->sendCreatureInvisible(creature);
		}
	void sendCreatureLight(const Creature* creature)
		{client->sendCreatureLight(creature);}

	//container
	void sendAddContainerItem(const Container* container, const Item* item);
	void sendUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem);
	void sendRemoveContainerItem(const Container* container, uint8_t slot, const Item* item);

	//inventory
	void sendAddInventoryItem(slots_t slot, const Item* item)
		{client->sendAddInventoryItem(slot, item);}
	void sendUpdateInventoryItem(slots_t slot, const Item* oldItem, const Item* newItem)
		{client->sendUpdateInventoryItem(slot, newItem);}
	void sendRemoveInventoryItem(slots_t slot, const Item* item)
		{client->sendRemoveInventoryItem(slot);}

	//event methods
	virtual void onAddTileItem(const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem);
	virtual void onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	virtual void onUpdateTile(const Position& pos);
	
	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos,
		uint32_t oldStackPos, bool teleport);

	virtual void onAttackedCreatureDissapear(bool isLogout);
	virtual void onFollowCreatureDissapear(bool isLogout);

	//container
	void onAddContainerItem(const Container* container, const Item* item);
	void onUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem);
	void onRemoveContainerItem(const Container* container, uint8_t slot, const Item* item);
	
	void onCloseContainer(const Container* container);
	void onSendContainer(const Container* container);
	void autoCloseContainers(const Container* container);

	//inventory
	void onAddInventoryItem(slots_t slot, Item* item);
	void onUpdateInventoryItem(slots_t slot, Item* oldItem, Item* newItem);
	void onRemoveInventoryItem(slots_t slot, Item* item);

	//other send messages
	void sendAnimatedText(const Position& pos, unsigned char color, std::string text) const
		{client->sendAnimatedText(pos,color,text);}
	void sendCancel(const char* msg) const
		{client->sendCancel(msg);}
	void sendCancelMessage(ReturnValue message) const;
	void sendCancelTarget() const
		{client->sendCancelTarget();}
	void sendCancelWalk() const
		{client->sendCancelWalk();}
	void sendChangeSpeed(const Creature* creature, uint32_t newSpeed) const 
		{client->sendChangeSpeed(creature, newSpeed);}
	void sendCreatureHealth(const Creature* creature) const
		{client->sendCreatureHealth(creature);}
	void sendDistanceShoot(const Position& from, const Position& to, unsigned char type) const
		{client->sendDistanceShoot(from, to,type);}
	void sendHouseWindow(House* _house, uint32_t _listid) const;
	void sendClosePrivate(uint16_t channelId) const
		{client->sendClosePrivate(channelId);}
	void sendIcons() const; 
	void sendMagicEffect(const Position& pos, unsigned char type) const
		{client->sendMagicEffect(pos,type);}
	void sendPing();
	void sendStats();
	void sendSkills() const
		{client->sendSkills();}
	void sendTextMessage(MessageClasses mclass, const std::string& message) const
		{client->sendTextMessage(mclass, message);}
	void sendTextWindow(Item* item,const uint16_t maxlen, const bool canWrite) const
		{client->sendTextWindow(item,maxlen,canWrite);}
	void sendTextWindow(uint32_t itemid, const std::string& text) const
		{client->sendTextWindow(itemid,text);}
	void sendToChannel(Creature* creature, SpeakClasses type, const std::string& text, uint16_t channelId) const
		{client->sendToChannel(creature, type, text, channelId);}
	void sendTradeItemRequest(const Player* player, const Item* item, bool ack) const
		{client->sendTradeItemRequest(player, item, ack);}
	void sendTradeClose() const 
		{client->sendCloseTrade();}
	void sendWorldLight(LightInfo& lightInfo)
		{client->sendWorldLight(lightInfo);}
	
	void receivePing() {if(npings > 0) npings--;}
	void flushMsg() {client->flushOutputBuffer();}

	virtual void onThink(uint32_t interval);
	virtual void onThinkAttacking(uint32_t interval);
	virtual void onAttacking(uint32_t interval);

	virtual void postAddNotification(Thing* thing, int32_t index, cylinderlink_t link = LINK_OWNER);
	virtual void postRemoveNotification(Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);
	virtual uint32_t __getItemTypeCount(uint16_t itemId, int32_t subType = -1, bool itemCount = true) const;
	
	void setLastAction(uint64_t time) {lastAction = time;}
	int64_t getLastAction() const {return lastAction;}

	//items
	Item* inventory[11]; //equipement of the player
	ContainerVector containerVec;
	void preSave();

	//depots	
	DepotMap depots;
	uint32_t maxDepotLimit;
	
	uint8_t safeMode;
	uint32_t lastip;

	Vocation* vocation;


protected:
	void checkTradeState(const Item* item);
	bool hasCapacity(const Item* item, uint32_t count) const;

	//combat help functions
	//bool getCombatItem(Item** tool, const Weapon** weapon);
	Item* getWeapon() const;

	int getSkillId(const std::string& skillName);
	std::string getSkillName(int skillid);
	void addExperience(int64_t exp);

	bool NeedUpdateStats();
	void updateInventoryWeigth();

	//cylinder implementations
	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
		uint32_t flags) const;
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
		uint32_t flags) const;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count) const;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
		uint32_t& flags);

	virtual void __addThing(Thing* thing);
	virtual void __addThing(int32_t index, Thing* thing);

	virtual void __updateThing(Thing* thing, uint32_t count);
	virtual void __replaceThing(uint32_t index, Thing* thing);

	virtual bool __removeThing(Thing* thing, uint32_t count);

	virtual int32_t __getIndexOfThing(const Thing* thing) const;
	virtual int32_t __getFirstIndex() const;
	virtual int32_t __getLastIndex() const;
	virtual Thing* __getThing(uint32_t index) const;

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

protected:
	Protocol79* client;

	int32_t level;
	int32_t magLevel;
	int32_t accessLevel;
	int64_t experience;
	uint32_t damageImmunities;
	uint32_t conditionImmunities;
	uint32_t conditionSuppressions;
	uint32_t condition;
	int32_t manaSpent;
	Vocation_t vocation_id;
	playersex_t sex;
	int32_t soul, soulMax;
	int32_t attackPower;

	double inventoryWeight;
	double capacity;
	
	bool SendBuffer;
	uint32_t internal_ping;
	uint32_t npings;
	int64_t lastAction;

	walkToItem_t walkToItem;

	uint32_t MessageBufferTicks;
	int32_t MessageBufferCount;

	bool pzLocked;
	int32_t bloodHitCount;
	int32_t shieldBlockCount;
	uint32_t skillPoint;
	uint32_t attackTicks;
	
	chaseMode_t chaseMode;
	fightMode_t fightMode;

	//account variables
	int accountNumber;
	std::string password;
	time_t lastlogin;
	time_t lastLoginSaved;
	Position loginPosition;
	
	//inventory variables
	bool inventoryAbilities[11];
	//Abilities ExtraAbilities;
	
	//player advances variables
	uint32_t skills[SKILL_LAST + 1][3];

	//extra skill modifiers
	int32_t varSkills[SKILL_LAST + 1];
	
	//reminder: 0 = None, 1 = Sorcerer, 2 = Druid, 3 = Paladin, 4 = Knight
	static const int CapGain[5];
	static const int ManaGain[5];
	static const int HPGain[5];
	static const int gainManaVector[5][2];
	static const int gainHealthVector[5][2];
	
	unsigned char level_percent;
	unsigned char maglevel_percent;

	//trade variables
	Player* tradePartner;
	tradestate_t tradeState;
	Item* tradeItem;

	struct SentStats{
		int32_t health;
		int32_t healthMax;
		int64_t experience;
		int32_t level;
		double freeCapacity;
		int32_t mana;
		int32_t manaMax;
		int32_t manaSpent;
		int32_t magLevel;
	};
	
	SentStats lastSentStats;

	std::string name;	
	std::string nameDescription;
	uint32_t guid;
	
	uint32_t town;
	
	//guild variables
	uint32_t guildId;
	std::string guildName;
	std::string guildRank;
	std::string guildNick;
	uint32_t guildLevel;
	
	StorageMap storageMap;
	LightInfo itemsLight;
	
	OutfitList m_playerOutfits;
	
	#ifdef __SKULLSYSTEM__
	int64_t redSkullTicks;
	Skulls_t skull;
	typedef std::set<uint32_t> AttackedSet;
	AttackedSet attackedSet;
	#endif

	#ifdef __XID_SEPERATE_ADDONS__
	KnowAddonMap knowAddonMap;
	#endif

	#ifdef __YUR_GUILD_SYSTEM__
	gstat_t guildStatus;
	#endif

	#ifdef __TR_ANTI_AFK__
	int idleTime;
	bool warned;
	#endif

	#ifdef __JD_DEATH_LIST__
	typedef std::list<Death> DeathList;
	DeathList deathList;
	#endif	

	#ifdef __XID_LEARN_SPELLS__
	typedef std::vector<std::string> StringVector;
	StringVector learnedSpells;
	#endif
	
	#ifdef __XID_BLESS_SYSTEM__
	bool blessMap[5];
	#endif
	
	void updateItemsLight(bool internal = false);
	void updateBaseSpeed(){ 
		if(getAccessLevel() < ACCESS_PROTECT){
			baseSpeed = vocation->getBaseSpeed() + (2* (level - 1));
		}
		else{
			baseSpeed = 900;
		};
	}

	virtual int64_t getLostExperience(){
		int64_t lostExperience = (int)std::floor(((double)experience * vocation->getDiePercent(1) / 100));
		#ifdef __XID_BLESS_SYSTEM__
		getBlessPercent(lostExperience);
		#endif

		return lostExperience;
	}
	
	virtual void dropLoot(Container* corpse);
	virtual uint32_t getDamageImmunities() const { return damageImmunities; }
	virtual uint32_t getConditionImmunities() const { return conditionImmunities; }
	virtual uint32_t getConditionSuppressions() const { return conditionSuppressions; }
	virtual uint16_t getLookCorpse() const;
	virtual uint32_t getAttackSpeed();

	friend OTSYS_THREAD_RETURN ConnectionHandler(void *dat);
	
	friend class Game;
	friend class LuaScriptInterface;
	friend class Commands;
	friend class Map;
	friend class IOPlayerXML;
	friend class IOPlayerSQL;
#ifdef __PARTYSYSTEM__
public:
	Party* party;
	void sendCreatureShield(Creature* c);
	shields_t getShieldClient(Player* player);
#endif
};

#endif
