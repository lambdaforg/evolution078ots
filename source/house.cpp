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

#include <sstream>
#include <algorithm>

#include "house.h"
#include "ioplayer.h"
#include "game.h"
#include "town.h"
#include "configmanager.h"
#include "tools.h"

extern ConfigManager g_config;
extern Game g_game;

House::House(uint32_t _houseid) :
transfer_container(ITEM_LOCKER1)
{
	isLoaded = false;
	houseName = "OTServ headquarter (Flat 1, Area 42)";
	houseOwner = "";
	posEntry.x = 0;
	posEntry.y = 0;
	posEntry.z = 0;
	paidUntil = 0;
	houseid = _houseid;
	rentWarnings = 0;
	rent = 0;
	townid = 0;
	transferItem = NULL;
}

House::~House()
{
	//
}

void House::addTile(HouseTile* tile)
{
	tile->setFlag(TILESTATE_PROTECTIONZONE);
	houseTiles.push_back(tile);
}

void House::setHouseOwner(std::string name)
{
	if(isLoaded && houseOwner == name)
		return;
	
	isLoaded = true;

	if(houseOwner != ""){
		//send items to depot
		transferToDepot();

		//...TODO...
		//TODO: remove players from beds

		//clean access lists
		houseOwner = "";
		setAccessList(SUBOWNER_LIST, "");
		setAccessList(GUEST_LIST, "");
		/*
		guestList.parseList("");
		subOwnerList.parseList("");
		*/
		HouseDoorList::iterator it;
		for(it = doorList.begin(); it != doorList.end(); ++it){
			(*it)->setAccessList("");
		}

		//reset paid date
		paidUntil = 0;
		rentWarnings = 0;
	}
		
	std::stringstream houseDescription;
	houseDescription << "It belongs to house '" << houseName << "'. " << std::endl;

	if(name != "" && IOPlayer::instance()->playerExists(name)){
		houseOwner = name;
		houseDescription << name;
	}
	else{
		houseDescription << "Nobody";
	}
	houseDescription << " owns this house." << std::endl;

	#ifdef __PB_BUY_HOUSE__
	if(getHouseOwner() == ""){
		int price = 0;
		for(HouseTileList::iterator it = getHouseTileBegin(); it != getHouseTileEnd(); it++){
			price += g_config.getNumber(ConfigManager::HOUSE_PRICE);
		}
		
		houseDescription << " This house costs " << price << " gold." << std::endl;
	}
	#endif
	
	HouseDoorList::iterator it;
	for(it = doorList.begin(); it != doorList.end(); ++it){
		(*it)->setSpecialDescription(houseDescription.str());
	}
}

AccessHouseLevel_t House::getHouseAccessLevel(const Player* player)
{
	if(player->getAccessLevel() >= ACCESS_HOUSE)
		return HOUSE_OWNER;
	
	if(player->getName() == houseOwner)
		return HOUSE_OWNER;
	
	if(subOwnerList.isInList(player))
		return HOUSE_SUBOWNER;
	
	if(guestList.isInList(player))
		return HOUSE_GUEST;
	
	return HOUSE_NO_INVITED;
}

bool House::kickPlayer(Player* player, const std::string& name)
{
	Player* kickingPlayer = g_game.getPlayerByName(name);
	if(kickingPlayer){
		HouseTile* houseTile = dynamic_cast<HouseTile*>(kickingPlayer->getTile());
		
		if(houseTile && houseTile->getHouse() == this){
			if(getHouseAccessLevel(player) >= getHouseAccessLevel(kickingPlayer) && player->getAccessLevel() >= kickingPlayer->getAccessLevel()){
				if(g_game.internalTeleport(kickingPlayer, getEntryPosition()) == RET_NOERROR){
					g_game.addMagicEffect(getEntryPosition(), NM_ME_ENERGY_AREA);
				}
				return true;
			}
		}
	}
	return false;
}

void House::setAccessList(uint32_t listId, const std::string& textlist)
{
	if(listId == GUEST_LIST){
		guestList.parseList(textlist);
	}
	else if(listId == SUBOWNER_LIST){
		subOwnerList.parseList(textlist);
	}
	else{
		Door* door = getDoorByNumber(listId);
		if(door){
			door->setAccessList(textlist);
		}
		else{
		}
		//We dont have kick anyone
		return;
	}
	
	//kick uninvited players
	typedef std::list<Player*> KickPlayerList;
	KickPlayerList kickList;
	HouseTileList::iterator it;
	for(it = houseTiles.begin(); it != houseTiles.end(); ++it){
		HouseTile* hTile = *it;
		if(hTile->creatures.size() > 0){
			CreatureVector::iterator cit;
			for(cit = hTile->creatures.begin(); cit != hTile->creatures.end(); ++cit){
				Player* player = (*cit)->getPlayer();
				if(player && isInvited(player) == false){
					kickList.push_back(player);
				}
			}
		}
	}

	KickPlayerList::iterator itkick;
	for(itkick = kickList.begin(); itkick != kickList.end(); ++itkick){
		if(g_game.internalTeleport(*itkick, getEntryPosition()) == RET_NOERROR){
			g_game.addMagicEffect(getEntryPosition(), NM_ME_ENERGY_AREA);
		}
	}
}

bool House::transferToDepot()
{
	if(townid == 0 || houseOwner == ""){
		return false;
	}

	if(!IOPlayer::instance()->playerExists(houseOwner)){
		return false;
	}

	Player* player = g_game.getPlayerByName(houseOwner);
	
	if(!player){
		player = new Player(houseOwner, NULL);

		if(!IOPlayer::instance()->loadPlayer(player, houseOwner)){
			delete player;
			return false;
		}
	}

	Depot* depot = player->getDepot(townid, true);

	std::list<Item*> moveItemList;
	Container* tmpContainer = NULL;
	Item* item = NULL;

	for(HouseTileList::iterator it = houseTiles.begin(); it != houseTiles.end(); ++it){
		for(uint32_t i = 0; i < (*it)->getThingCount(); ++i){
			item = (*it)->__getThing(i)->getItem();

			if(!item)
				continue;

			if(item->isPickupable()){
				moveItemList.push_back(item);
			}
			else if(tmpContainer = item->getContainer()){
				for(ItemList::const_iterator it = tmpContainer->getItems(); it != tmpContainer->getEnd(); ++it){
					moveItemList.push_back(*it);
				}
			}
		}
	}

	for(std::list<Item*>::iterator it = moveItemList.begin(); it != moveItemList.end(); ++it){
		g_game.internalMoveItem((*it)->getParent(), depot, INDEX_WHEREEVER, (*it), (*it)->getItemCount(), FLAG_NOLIMIT);
	}

	if(!player->isOnline()){
		IOPlayer::instance()->savePlayer(player);
		delete player;
	}

	return true;
}

bool House::getAccessList(uint32_t listId, std::string& list) const
{
	if(listId == GUEST_LIST){
		guestList.getList(list);
		return true;
	}
	else if(listId == SUBOWNER_LIST){
		subOwnerList.getList(list);
		return true;
	}
	else{
		Door* door = getDoorByNumber(listId);
		if(door){
			return door->getAccessList(list);
		}
		else{
			return false;
		}
	}
	return false;
}

bool House::isInvited(const Player* player)
{
	if(getHouseAccessLevel(player) != HOUSE_NO_INVITED){
		return true;
	}
	else{
		return false;
	}
}

void House::addDoor(Door* door)
{
	door->useThing2();
	doorList.push_back(door);
	door->setHouse(this);
}

Door* House::getDoorByNumber(uint32_t doorId)
{
	HouseDoorList::iterator it;
	for(it = doorList.begin(); it != doorList.end(); ++it){
		if((*it)->getDoorId() == doorId){
			return *it;
		}
	}
	return NULL;
}

Door* House::getDoorByNumber(uint32_t doorId) const
{
	HouseDoorList::const_iterator it;
	for(it = doorList.begin(); it != doorList.end(); ++it){
		if((*it)->getDoorId() == doorId){
			return *it;
		}
	}
	return NULL;
}

Door* House::getDoorByPosition(const Position& pos)
{
	for(HouseDoorList::iterator it = doorList.begin(); it != doorList.end(); ++it){
		if((*it)->getPosition() == pos){
			return *it;
		}
	}

	return NULL;
}

bool House::canEditAccessList(uint32_t listId, const Player* player)
{
	switch(getHouseAccessLevel(player)){
	case HOUSE_OWNER:
		return true;
		break;
	case HOUSE_SUBOWNER:
		/*subowners can edit guest access list*/
		if(listId == GUEST_LIST){
			return true;
		}
		else /*subowner/door list*/{
			return false;
		}
		break;
	default:
		return false;	
	}
}

HouseTransferItem* House::getTransferItem()
{
	if(transferItem != NULL)
		return NULL;
	
	transfer_container.setParent(NULL);
	transferItem =  HouseTransferItem::createHouseTransferItem(this);
	transfer_container.__addThing(transferItem);
	return transferItem;
}

void House::resetTransferItem()
{
	if(transferItem){
		Item* tmpItem = transferItem;
		transferItem = NULL;
		transfer_container.setParent(NULL);

		transfer_container.__removeThing(tmpItem, tmpItem->getItemCount());
		g_game.FreeThing(tmpItem);
	}
}

HouseTransferItem* HouseTransferItem::createHouseTransferItem(House* house)
{
	HouseTransferItem* transferItem = new HouseTransferItem(house);
	transferItem->useThing2();
	transferItem->setID(ITEM_DOCUMENT_RO);
	transferItem->setItemCountOrSubtype(1);
	std::stringstream stream;
	stream << " It is a house transfer document for '" << house->getName() << "'.";
	transferItem->setSpecialDescription(stream.str());
	return transferItem;
}

bool HouseTransferItem::onTradeEvent(TradeEvents_t event, Player* owner)
{
	House* house;
	switch(event){
		case ON_TRADE_TRANSFER:
		{
			house = getHouse();
			if(house){
				house->executeTransfer(this, owner);
			}

			g_game.internalRemoveItem(this, 1);
			break;
		}

		case ON_TRADE_CANCEL:
		{
			house = getHouse();
			if(house){
				house->resetTransferItem();
			}
			
			break;
		}

		default:
			break;
	}

	return true;
}

bool House::executeTransfer(HouseTransferItem* item, Player* newOwner)
{
	if(transferItem != item){
		return false;
	}

	setHouseOwner(newOwner->getName());
	transferItem = NULL;
	return true;
}

AccessList::AccessList()
{
	//
}

AccessList::~AccessList()
{
	//
}

bool AccessList::parseList(const std::string& _list)
{
	playerList.clear();
	guildList.clear();
	expressionList.clear();
	regExList.clear();
	list = _list;
	
	if(_list == "")
		return true;
	
	std::stringstream listStream(_list);
	std::string line;
	while(getline(listStream, line)){
		//trim left
		trim_left(line, " ");
		trim_left(line, "\t");

		//trim right
		trim_right(line, " ");
		trim_right(line, "\t");
		
		std::transform(line.begin(), line.end(), line.begin(), tolower);

		if(line.substr(0,1) == "#")
			continue;

		if(line.length() > 100)
			continue;
		
		if(line.find("@") != std::string::npos){
			std::string::size_type pos = line.find("@");
			addGuild(line.substr(pos + 1), "");
		}
		else if(line.find("!") != std::string::npos || line.find("*") != std::string::npos || line.find("?") != std::string::npos){
			addExpression(line);
		}
		else{
			addPlayer(line);
		}
	}
	return true;
}

bool AccessList::addPlayer(std::string& name)
{
	std::string dbName = name;
	if(IOPlayer::instance()->playerExists(dbName)){
		if(std::find(playerList.begin(), playerList.end(), dbName) == playerList.end()){
			playerList.push_back(dbName);
			return true;
		}
	}
	return false;
}

bool AccessList::addGuild(const std::string& guildName, const std::string& rank)
{
	return false;
}

bool AccessList::addExpression(const std::string& expression)
{
	ExpressionList::iterator it;
	for(it = expressionList.begin(); it != expressionList.end(); ++it){
		if((*it) == expression){
			return false;
		}
	}
	
	std::string outExp;
	std::string metachars = ".[{}()\\+|^$";

	for(std::string::const_iterator it = expression.begin(); it != expression.end(); ++it){
		if(metachars.find(*it) != std::string::npos){
			outExp += "\\";
		}

		outExp += (*it);
	}

	replaceString(outExp, "*", ".*");
	replaceString(outExp, "?", ".?");

	try{
		if(outExp.length() > 0){
			expressionList.push_back(outExp);

			if(outExp.substr(0,1) == "!"){
				if(outExp.length() > 1){
					//push 'NOT' expressions upfront so they are checked first
					regExList.push_front(std::make_pair(boost::regex(outExp.substr(1)), false));
				}
			}
			else{
				regExList.push_back(std::make_pair(boost::regex(outExp), true));
			}
		}
	}
	catch(...){
		//
	}

	return true;
}

bool AccessList::isInList(const Player* player)
{
	RegExList::iterator it;
	std::string name = player->getName();
	boost::cmatch what;

	std::transform(name.begin(), name.end(), name.begin(), tolower);
	for(it = regExList.begin(); it != regExList.end(); ++it){
		if(boost::regex_match(name.c_str(), what, it->first)){
			if(it->second){
				return true;
			}
			else{
				return false;
			}
		}
	}

	PlayerList::iterator playerIt = std::find(playerList.begin(), playerList.end(), name);
	if(playerIt != playerList.end())
		return true;

	GuildList::iterator guildIt = guildList.find(player->getGuildId());
	if(guildIt != guildList.end())
		return true;

	return false;
}
	
void AccessList::getList(std::string& _list) const
{
	_list = list;
}

Door::Door(uint16_t _type):
Item(_type)
{
	house = NULL;
	accessList = NULL;
	doorId = 0;
}

Door::~Door()
{
	if(accessList)
		delete accessList;
}

bool Door::unserialize(xmlNodePtr nodeItem)
{
	bool ret = Item::unserialize(nodeItem);

	int intValue;
	if(readXMLInteger(nodeItem, "doorId", intValue)){
		setDoorId(doorId);
	}

	/*
	char* nodeValue;
	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "doorId");
	if(nodeValue){
		setDoorId(atoi(nodeValue));
		xmlFreeOTSERV(nodeValue);
	}
	*/
	
	return ret;
}

xmlNodePtr Door::serialize()
{
	//dont call Item::serialize()
	xmlNodePtr xmlptr = xmlNewNode(NULL,(const xmlChar*)"item");

	std::stringstream ss;
	ss.str(""); //empty the stringstream
	ss << getID();
	xmlSetProp(xmlptr, (const xmlChar*)"id", (const xmlChar*)ss.str().c_str());

	/*
	ss.str("");
	ss << (int) doorId;
	xmlSetProp(xmlptr, (const xmlChar*)"doorId", (const xmlChar*)ss.str().c_str());
	*/

	return xmlptr;
}

bool Door::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if(ATTR_HOUSEDOORID == attr){
		unsigned char _doorId = 0;
		if(!propStream.GET_UCHAR(_doorId)){
			return false;
		}

		setDoorId(_doorId);
		return true;
	}
	else
		return Item::readAttr(attr, propStream);
}

bool Door::serializeAttr(PropWriteStream& propWriteStream)
{
	//dont call Item::serializeAttr(propWriteStream);

	/*
	if(house){
		unsigned char _doorId = getDoorId();
		propWriteStream.ADD_UCHAR(ATTR_HOUSEDOORID);
		propWriteStream.ADD_UCHAR(_doorId);
	}
	*/

	return true;
}

void Door::setHouse(House* _house)
{
	if(house != NULL){
		return;
	}
	house = _house;
	accessList = new AccessList();
}

bool Door::canUse(const Player* player)
{
	if(!house){
		return true;
	}
	
	if(house->getHouseAccessLevel(player) >= HOUSE_SUBOWNER)
		return true;
	
	return accessList->isInList(player);
}
	
void Door::setAccessList(const std::string& textlist)
{
	if(!house){
		return;
	}
	accessList->parseList(textlist);
}

bool Door::getAccessList(std::string& list) const
{
	if(!house){
		return false;
	}
	accessList->getList(list);
	return true;
}

Houses::Houses()
{
	std::string strRentPeriod = g_config.getString(ConfigManager::HOUSE_RENT_PERIOD);

	rentPeriod = RENTPERIOD_MONTHLY;

	if(strRentPeriod == "yearly"){
		rentPeriod = RENTPERIOD_YEARLY;
	}
	else if(strRentPeriod == "weekly"){
		rentPeriod = RENTPERIOD_WEEKLY;
	}
	else if(strRentPeriod == "daily"){
		rentPeriod = RENTPERIOD_DAILY;
	}
}

Houses::~Houses()
{
	//
}

House* Houses::getHouseByPlayerName(std::string playerName)
{
	for(HouseMap::iterator it = houseMap.begin(); it != houseMap.end(); ++it){
		House* house = it->second;
		if(house->getHouseOwner() == playerName){
			return house;
		}
	}
	return NULL;
}

bool Houses::loadHousesXML(std::string filename)
{
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		xmlNodePtr root, houseNode;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"houses") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		int intValue;
		std::string strValue;

		houseNode = root->children;
		while(houseNode){
			if(xmlStrcmp(houseNode->name,(const xmlChar*)"house") == 0){
				int _houseid = 0;
				Position entryPos;

				if(!readXMLInteger(houseNode, "houseid", _houseid)){
					std::cout << "Error: [Houses::loadHousesXML] Unknown house, id = " << _houseid << std::endl;
					return false;
				}

				House* house = Houses::getInstance().getHouse(_houseid);
				if(!house){
					return false;
				}

				if(readXMLString(houseNode, "name", strValue)){
					house->setName(strValue);
				}

				if(readXMLInteger(houseNode, "entryx", intValue)){
					entryPos.x = intValue;
				}

				if(readXMLInteger(houseNode, "entryy", intValue)){
					entryPos.y = intValue;
				}

				if(readXMLInteger(houseNode, "entryz", intValue)){
					entryPos.z = intValue;
				}
				
				house->setEntryPos(entryPos);
				
				if(readXMLInteger(houseNode, "rent", intValue)){
					house->setRent(intValue);
				}

				if(readXMLInteger(houseNode, "townid", intValue)){
					house->setTownId(intValue);
				}
				
				house->setHouseOwner("");
			}

			houseNode = houseNode->next;
		}

		return true;
	}

	return false;
}

bool Houses::payHouses(bool mail)
{
	uint32_t currentTime = std::time(NULL);

	for(HouseMap::iterator it = houseMap.begin(); it != houseMap.end(); ++it){
		House* house = it->second;

		if(house->getHouseOwner() != "" && house->getPaidUntil() < currentTime &&
			house->getRent() != 0){
			std::string owner = house->getHouseOwner();
			Town* town = Towns::getInstance().getTown(house->getTownId());
			if(!town){
				continue;
			}
			
			if(!IOPlayer::instance()->playerExists(owner)){
				//player doesnt exist, remove it as house owner?
				house->setHouseOwner("");
				continue;
			}

			Player* player = g_game.getPlayerByName(owner);
			if(!player){
				player = new Player(owner, NULL);

				if(!IOPlayer::instance()->loadPlayer(player, owner)){
					delete player;
					continue;
				}
			}

			Depot* depot = player->getDepot(town->getTownID(), true);
			if(depot){
				//get money from depot
				if(g_game.removeMoney(depot, house->getRent(), FLAG_NOLIMIT)){
					
					uint32_t paidUntil = currentTime;
					switch(rentPeriod){
					case RENTPERIOD_DAILY:
						paidUntil += 24 * 60 * 60;
						break;
					case RENTPERIOD_WEEKLY:
						paidUntil += 24 * 60 * 60 * 7;
						break;
					case RENTPERIOD_MONTHLY:
						paidUntil += 24 * 60 * 60 * 30;
						break;
					case RENTPERIOD_YEARLY:
						paidUntil += 24 * 60 * 60 * 365;
						break;
					}

					house->setPaidUntil(paidUntil);
				}
				else{
					if(house->getPayRentWarnings() >= 7){
						house->setHouseOwner("");
					}
					else{
						int daysLeft = 7 - house->getPayRentWarnings();

						Item* letter = Item::CreateItem(ITEM_LETTER_STAMPED);
						std::string period = "";

						switch(rentPeriod){
							case RENTPERIOD_DAILY:
								period = "daily";
							break;

							case RENTPERIOD_WEEKLY:
								period = "weekly";
							break;

							case RENTPERIOD_MONTHLY:
								period = "monthly";
							break;

							case RENTPERIOD_YEARLY:
								period = "yearly";
							break;
						}

						std::stringstream warningText;
						warningText << "Warning! \n" <<
							"The " << period << " rent of " << house->getRent() << " gold for your house \""
							<< house->getName() << "\" is payable. Have it available within " << daysLeft <<
							" days, or you will lose this house.";
						
						if(mail == true){	
							letter->setText(warningText.str());
							g_game.internalAddItem(depot, letter, INDEX_WHEREEVER, FLAG_NOLIMIT);
							house->setPayRentWarnings(house->getPayRentWarnings() + 1);
						}
						else{
							delete letter;
						}
					}
				}
			}

			if(!player->isOnline()){
				IOPlayer::instance()->savePlayer(player);
				delete player;
			}
		}
	}

	return true;
}
