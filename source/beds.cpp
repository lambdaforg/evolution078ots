#ifdef __JD_BED_SYSTEM__

#include "beds.h"
#include "game.h"
#include "ioplayer.h"
#include "vocation.h"

#include <cstdlib>



Beds g_beds;
extern Game g_game;
extern Vocations g_vocations;

Bed::Bed(uint16_t _type):
Item(_type)
{
	g_beds.addBed(this);
	sleeper = "";
	startsleep = 0;
	setSpecialDescription("Nobody is sleeping there.");
}

Bed::~Bed()
{
	//
}

bool Bed::canUse(const Player* player)
{
	if(!getTile() || !dynamic_cast<HouseTile*>(getTile()) || sleeper != ""){
		return false;
	}
	
	return true;
}

void Bed::useBed(Player* player)
{
	if(!player || player->isRemoved()){
		return;
	}
	
	if(!canUse(player)){
		player->sendCancelMessage(RET_NOTPOSSIBLE);
		return;
	}

	if(!player->isPremium()){
		player->sendCancelMessage(RET_YOUDONOTHAVEAPREMIUMACCOUNT);
		return;
	}
	
	if(player->hasCondition(CONDITION_INFIGHT)){
		player->sendCancelMessage(RET_YOUMAYNOTLOGOUTDURINGAFIGHT);
		return;
	}
	
	player->sendCreatureMove(player, getPosition(), player->getPosition(), player->getTile()->__getIndexOfThing(player), false);
	
	std::string name = player->getName();
	player->kickPlayer();
	setSleeper(name);
	startSleep();
}

void Bed::setSleeper(const std::string& name)
{
	if(name == sleeper){
		return;
	}
	
	if(name != ""){
		//Change to occupied bed
		g_game.transformItem(this, getOccupiedId());
		
		if(Item* part = getSecondPart()){
			g_game.transformItem(part, getOccupiedId(part));
		}
		
		if(IOPlayer::instance()->playerExists(name)){
			setSpecialDescription(name + " is sleeping there.");
		}
		else{
			//setSpecialDescription("Somebody is sleeping there.");
			//Player doesn't exist any more. Probably deleted. 
			setSleeper("");
			return;
		}
	}
	else{
		//Change to empty bed
		g_game.transformItem(this, getEmptyId());
		
		if(Item* part = getSecondPart()) {
			g_game.transformItem(part, getEmptyId(part));
		}
		
		setSpecialDescription("Nobody is sleeping there.");
	}
	
	sleeper = name;
}

uint32_t Bed::getOccupiedId(const Item* item) const
{
	uint32_t itemid = getID();
	if(item) itemid = item->getID();
	
	uint32_t ret = itemid;
	switch(itemid){
		case 1754:
			ret = 1762;
			break;
		case 1755:
			ret = 1763;
			break;
		case 1756:
			ret = 1768;
			break;
		case 1757:
			ret = 1769;
			break;
		case 1758:
			ret = 1766;
			break;
		case 1759:
			ret = 1767;
			break;
		case 1760:
			ret = 1764;
			break;
		case 1761:
			ret = 1765;
			break;
	}
	
	return ret;
}

uint32_t Bed::getEmptyId(const Item* item) const
{
	uint32_t itemid = getID();
	if(item) itemid = item->getID();
	
	uint32_t ret = itemid;
	switch(itemid){
		case 1762:
			ret = 1754;
			break;
		case 1763:
			ret = 1755;
			break;
		case 1768:
			ret = 1756;
			break;
		case 1769:
			ret = 1757;
			break;
		case 1766:
			ret = 1758;
			break;
		case 1767:
			ret = 1759;
			break;
		case 1764:
			ret = 1760;
			break;
		case 1765:
			ret = 1761;
			break;
	}
	
	return ret;
}

Item* Bed::getSecondPart() const
{
	
	Direction dir = NORTH;
	switch(getID()){
		case 1756:
		case 1760:
		case 1764:
		case 1768:
			dir = WEST;
			break;
	}
	
	Item* ret = NULL;
	Position pos = getPosition();
	
	if(dir == NORTH){
		pos.y++;
	}
	else{
		pos.x++;
	}
	
	if(Tile* tile = g_game.getTile(pos.x, pos.y, pos.z)){
		for(uint32_t i = tile->__getFirstIndex(); i <= tile->__getLastIndex(); i++){
			Thing* thing = tile->__getThing(i);
			if(thing){
				if(Item* item = thing->getItem()){
					if(getEmptyId(item) != item->getID() || getOccupiedId(item) != item->getID()){
						ret = item;
						break;
					}
				}
			}
		}
	}
	
	return ret;
}


uint32_t Bed::getGainedHealth(const Player* player, uint32_t &usedFood) const
{
	if(!player) return 0;
	
	uint32_t ret = 0;	
	uint32_t food = 0;
	
	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition){
		food = condition->getTicks() / 1000;
	}
	
	uint64_t sleepTime = std::time(NULL) - getStartSleep();
	sleepTime = std::min((uint64_t)food, sleepTime);
	
	Vocation* voc = g_vocations.getVocation(player->getVocationId());
	if(voc) {
		uint32_t ticks = voc->getHealthGainTicks();
		uint32_t amount = voc->getHealthGainAmount();
		
		ret = (sleepTime/ticks)*amount;
		ret /= 2;
	}
	
	uint32_t healthdiff = player->getMaxHealth() - player->getHealth();
	usedFood = std::min((uint64_t)food, sleepTime);
	
	return std::min(healthdiff, ret);
}

uint32_t Bed::getGainedMana(const Player* player, uint32_t &usedFood) const
{
	if(!player) return 0;
	
	uint32_t ret = 0;	
	uint32_t food = 0;
	
	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition){
		food = condition->getTicks() / 1000;
	}
	
	uint64_t sleepTime = std::time(NULL) - getStartSleep();
	sleepTime = std::min((uint64_t)food, sleepTime);
	
	Vocation* voc = g_vocations.getVocation(player->getVocationId());
	if(voc) {
		uint32_t ticks = voc->getManaGainTicks();
		uint32_t amount = voc->getManaGainAmount();
		
		ret = (sleepTime/ticks)*amount;
		ret /= 2;
	}
	
	uint32_t manadiff = player->getMaxMana() - player->getMana();	
	usedFood = std::min((uint64_t)food, sleepTime);
	
	return std::min(manadiff, ret);
	
}

void Bed::wakeUp(Player* player)
{
	if(!player || player->getName() != sleeper){
		return;
	}
	
	uint32_t usedFood = 0;
	int mana, health;
	
	player->changeHealth(health = getGainedHealth(player, usedFood));
	player->changeMana(mana = getGainedMana(player, usedFood));
	
	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition){
		uint32_t food = condition->getTicks() / 1000;
		condition->setTicks(std::max((uint32_t)0, (uint32_t)(food-usedFood)) * 1000);
	}
	
	setSleeper("");
}

Beds::Beds()
{
	beds.clear();
}

bool Beds::loadBeds()
{
	return IOBeds::instance()->loadBeds("");
}

bool Beds::saveBeds()
{
	return IOBeds::instance()->saveBeds("");
}

Bed* Beds::getBedFromPosition(const Position &pos)
{
	Bed* ret = NULL;
	if(Tile* tile = g_game.getTile(pos.x, pos.y, pos.z)) {
		for(uint32_t i = tile->__getFirstIndex(); i <= tile->__getLastIndex(); i++) {
			Thing* thing = tile->__getThing(i);
			if(thing) {
				if(Item* item = thing->getItem()) {
					if(Bed* bed = item->getBed()) {
						ret = bed;
						break;
					}
				}
			}
		}
	}
	
	return ret;
}

Bed* Beds::getBedBySleeper(const std::string& name)
{
	BedList::iterator it;
	for(it = beds.begin(); it != beds.end(); it++) {
		Bed* bed = (*it);
		if(bed->getSleeper() == name) {
			return bed;
		}
	}
	
	return NULL;
}


IOBeds* IOBeds::_instance = NULL;

IOBeds* IOBeds::instance()
{
	if(!_instance) {
		//Only SQL so far...
		_instance = new IOBedsXML();
	}
	
	return _instance;
}



IOBedsXML::IOBedsXML()
{
	//
}

bool IOBedsXML::loadBeds(const std::string &identifier)
{
	std::cout << ":: Loading beds.xml... ";
	std::string filename = g_config.getString(ConfigManager::DATA_DIRECTORY);
	filename += "beds.xml";
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	
	if(doc){
		xmlNodePtr root, bednode, innerbednode;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"beds") != 0){
			xmlFreeDoc(doc);			
			return false;
		}
		  
		bednode = root->children;
		int intValue = 0;
		std::string strValue;
		  
		while(bednode){
			if(xmlStrcmp(bednode->name,(const xmlChar*)"bed") == 0){
				std::string _sleeper;
				time_t _startsleep = std::time(NULL);
				Position bedPos;
				
				if(readXMLInteger(bednode, "startsleep", intValue)){
					_startsleep = intValue;
				}
				
				if(readXMLInteger(bednode, "x", intValue)){
					bedPos.x = intValue;
				}
				
				if(readXMLInteger(bednode, "y", intValue)){
					bedPos.y = intValue;
				}
				
				if(readXMLInteger(bednode, "z", intValue)){
					bedPos.z = intValue;
				}
				
				if(readXMLString(bednode, "sleeper", strValue)){
					_sleeper = strValue;
				}
		
				Bed* thisBed = g_beds.getBedFromPosition(bedPos);
				if(thisBed){
					thisBed->setSleeper(_sleeper);
					thisBed->setStartSleep(_startsleep);
				}
				else{
					std::cout << "[Warning] No bed at position " << bedPos.x << ", " << bedPos.y << ", " << bedPos.z << "!" << std::endl;
				}
			}
			
			bednode = bednode->next;
		}

		xmlFreeDoc(doc);
	}

	std::cout << "[done]" << std::endl;
	
	return false;
}

bool IOBedsXML::saveBeds(const std::string &identifier)
{
	std::string filename = g_config.getString(ConfigManager::DATA_DIRECTORY);
	filename += "beds.xml";
	
	std::stringstream sb;
	xmlNodePtr bednode, root;
	xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
     
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"beds", NULL);
	root = doc->children;
 
	BedList* bedList = g_beds.getBedList();
	BedList::iterator it;

	for(it = bedList->begin(); it != bedList->end(); it++){
		Bed* bed = (*it);
		if(bed->getSleeper() == "")
			continue;
			
		bednode = xmlNewNode(NULL,(const xmlChar*)"bed");

		sb.str("");
		sb << bed->getSleeper();
		xmlSetProp(bednode, (const xmlChar*)"sleeper", (const xmlChar*)sb.str().c_str());         
		
		sb.str("");
		sb << bed->getStartSleep();
		xmlSetProp(bednode, (const xmlChar*)"sleepstart", (const xmlChar*)sb.str().c_str());
		
		sb.str("");
		sb << bed->getPosition().x;
		xmlSetProp(bednode, (const xmlChar*)"x", (const xmlChar*)sb.str().c_str());
		
		sb.str("");
		sb << bed->getPosition().y;
		xmlSetProp(bednode, (const xmlChar*)"y", (const xmlChar*)sb.str().c_str());
		
		sb.str("");
		sb << bed->getPosition().z;
		xmlSetProp(bednode, (const xmlChar*)"z", (const xmlChar*)sb.str().c_str());
		
		sb.str("");
		xmlAddChild(root, bednode);
	}
     
	if(xmlSaveFile(filename.c_str(), doc)){
		xmlFreeDoc(doc);
		return true;
	}
	else{
		xmlFreeDoc(doc);
	}
     
	return false;
}

#endif
