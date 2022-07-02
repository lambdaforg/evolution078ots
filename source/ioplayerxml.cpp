//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Player Loader/Saver implemented with XML
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

#include "ioplayer.h"
#include "ioplayerxml.h"
#include "ioaccount.h"
#include "item.h"
#include "player.h"
#include "tools.h"
#include "configmanager.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/threads.h>

xmlMutexPtr xmlmutex;
typedef std::vector<std::string> StringVector;
extern ConfigManager g_config;

IOPlayerXML::IOPlayerXML()
{
	if(xmlmutex == NULL){
		xmlmutex = xmlNewMutex();
	}
}

bool IOPlayerXML::loadPlayer(Player* player, std::string name)
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::string filename = datadir + "players/" + name + ".xml";
	toLowerCaseString(filename); //all players are saved as lowercase

	xmlMutexLock(xmlmutex);
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if(doc){
		bool isLoaded = true;
		xmlNodePtr root, p;

		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"player") != 0){
			std::cout << "Error while loading " << name << std::endl;
		}

		int intValue;
		std::string strValue;

		int account = 0;
		if(readXMLInteger(root, "account", intValue)){
			account = intValue;
		}
		else{
		  xmlFreeDoc(doc);
		  xmlMutexUnlock(xmlmutex);
		  return false;
		}

		//need to unlock and relock in order to load xml account (both share the same mutex)
		xmlMutexUnlock(xmlmutex);
		Account a = IOAccount::instance()->loadAccount(account);
		xmlMutexLock(xmlmutex);

		player->password = a.password;
		if(a.accnumber == 0 || a.accnumber != (unsigned long)account){
		  xmlFreeDoc(doc);
		  xmlMutexUnlock(xmlmutex);
		  return false;
		}
		
		player->accountNumber = account;
		if(readXMLInteger(root, "sex", intValue)){
			player->setSex((playersex_t)intValue);
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "lookdir", intValue)){
			player->setDirection((Direction)intValue);
		}
		else
			isLoaded = false;

		if(readXMLString(root, "exp", strValue)){
			player->experience = _atoi64(strValue.c_str());
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "level", intValue)){
			player->level = intValue;
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "maglevel", intValue)){
			player->magLevel = intValue;
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "soul", intValue)){
			player->soul = intValue;
			player->soulMax = player->vocation->getSoulMax();
		}

		if(readXMLInteger(root, "voc", intValue)){
			player->setVocation(intValue);
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "access", intValue)){
			player->accessLevel = intValue;
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "cap", intValue)){
			player->capacity = intValue;
		}
		else
			isLoaded = false;

		if(readXMLInteger(root, "maxdepotitems", intValue)){
			player->maxDepotLimit = intValue;
		}
		else
			isLoaded = false;

		player->updateBaseSpeed();

		if(readXMLInteger(root, "lastlogin", intValue)){
			player->lastLoginSaved = intValue;
		}
		else
			player->lastLoginSaved = 0;

		p = root->children;

		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"mana") == 0){

				if(readXMLInteger(p, "now", intValue)){
					player->mana = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "max", intValue)){
					player->manaMax = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "spent", intValue)){
					player->manaSpent = intValue;
				}
				else
					isLoaded = false;

			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"health") == 0){

				if(readXMLInteger(p, "now", intValue)){
					player->health = intValue;

					if(player->health <= 0)
						player->health = 100;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "max", intValue)){
					player->healthMax = intValue;
				}
				else
					isLoaded = false;
					
				if(readXMLInteger(p, "food", intValue)){
					if(player->isOnline()){
						player->addDefaultRegeneration(intValue * 1000);
					}
				}
				else
					isLoaded = false;
			}
			#ifdef __SKULLSYSTEM__
			else if(xmlStrcmp(p->name, (const xmlChar*)"skull") == 0){
				if(readXMLInteger(p, "redskulltime", intValue)){
					long redSkullSeconds = intValue - std::time(NULL);
					if(redSkullSeconds > 0){
						//ensure that we round up the number of ticks
						player->redSkullTicks = (redSkullSeconds + 2)*1000;
						if(readXMLInteger(p, "redskull", intValue)){
							if(intValue == 1)
								player->skull = SKULL_RED;
							else
								player->skull = Skulls_t(0);
						}
						else
							isLoaded = false;
					}
				}
				else
					isLoaded = false;
			}
			#endif
			else if(xmlStrcmp(p->name, (const xmlChar*)"look") == 0){

				if(readXMLInteger(p, "type", intValue)){
					player->defaultOutfit.lookType = intValue;
					player->currentOutfit.lookType = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "head", intValue)){
					player->defaultOutfit.lookHead = intValue;
					player->currentOutfit.lookHead = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "body", intValue)){
					player->defaultOutfit.lookBody = intValue;
					player->currentOutfit.lookBody = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "legs", intValue)){
					player->defaultOutfit.lookLegs = intValue;
					player->currentOutfit.lookLegs = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "feet", intValue)){
					player->defaultOutfit.lookFeet = intValue;
					player->currentOutfit.lookFeet = intValue;
				}
				else
					isLoaded = false;
				
				if(readXMLInteger(p, "addons", intValue)){
					player->defaultOutfit.lookAddons = intValue;
					player->currentOutfit.lookAddons = intValue;
				}
				else
					isLoaded = false;
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"spawn") == 0){

				if(readXMLInteger(p, "x", intValue)){
					player->loginPosition.x = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "y", intValue)){
					player->loginPosition.y = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "z", intValue)){
					player->loginPosition.z = intValue;
				}
				else
					isLoaded = false;
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"temple") == 0){

				if(readXMLInteger(p, "x", intValue)){
					player->masterPos.x = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "y", intValue)){
					player->masterPos.y = intValue;
				}
				else
					isLoaded = false;

				if(readXMLInteger(p, "z", intValue)){
					player->masterPos.z = intValue;
				}
				else
					isLoaded = false;
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"skills") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					int s_id = 0;
					int s_lvl = 0;
					int s_tries = 0;
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"skill") == 0){

						if(readXMLInteger(tmpNode, "skillid", intValue)){
							s_id = intValue;
						}
						else
							isLoaded = false;

						if(readXMLInteger(tmpNode, "level", intValue)){
							s_lvl = intValue;
						}
						else
							isLoaded = false;

						if(readXMLInteger(tmpNode, "tries", intValue)){
							s_tries = intValue;
						}
						else
							isLoaded = false;

						player->skills[s_id][SKILL_LEVEL]=s_lvl;
						player->skills[s_id][SKILL_TRIES]=s_tries;
					}

					tmpNode = tmpNode->next;
				}
			}
			#ifdef __XID_SEPERATE_ADDONS__
			else if(xmlStrcmp(p->name, (const xmlChar*)"addons") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"addon") == 0){
						uint32_t lookType, addon = 0;

						if(readXMLInteger(tmpNode, "outfit", intValue)){
							lookType = intValue;
						}

						if(readXMLInteger(tmpNode, "type", intValue)){
							addon = intValue;
						}

						player->addAddon(lookType, addon);
					}

					tmpNode = tmpNode->next;
				}
			}
			#endif
			#ifdef __JD_DEATH_LIST__
			else if (xmlStrcmp(p->name, (const xmlChar*)"deaths") == 0){
				xmlNodePtr tmpNode = p->children;
                while (tmpNode){
                    if(xmlStrcmp(tmpNode->name, (const xmlChar*)"death") == 0){
						Death death;

						if(readXMLString(tmpNode, "name", strValue)){
                            death.killer = strValue;
                        }
                        if(readXMLInteger(tmpNode, "level", intValue)){
                            death.level = intValue;
                        }
                        if(readXMLInteger(tmpNode, "time", intValue)){
                            death.time = (time_t)intValue;
                        }				

						player->deathList.push_back(death);
					}
					
					tmpNode = tmpNode->next;
				}
			}
			#endif
			#ifdef __XID_LEARN_SPELLS__
			else if(xmlStrcmp(p->name, (const xmlChar*)"spells") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"spell") == 0){
				        if(readXMLString(tmpNode, "words", strValue)){
					        player->learnSpell(strValue);
				        }
					}
					
					tmpNode = tmpNode->next;
				}
			}
			#endif
			#ifdef __XID_BLESS_SYSTEM__
			else if(xmlStrcmp(p->name, (const xmlChar*)"blessings") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"blessing") == 0){
				        if(readXMLInteger(tmpNode, "id", intValue)){
					        player->addBlessing(intValue);
				        }
					}
					
					tmpNode = tmpNode->next;
				}
			}
			#endif
			else if(xmlStrcmp(p->name, (const xmlChar*)"inventory") == 0){
				xmlNodePtr slotNode = p->children;
				while(slotNode){
					if(xmlStrcmp(slotNode->name, (const xmlChar*)"slot") == 0){
						int32_t itemId = 0;
						int32_t slotId = 0;

						if(readXMLInteger(slotNode, "slotid", intValue)){
							slotId = intValue;
						}
						else{
							isLoaded = false;
						}

						xmlNodePtr itemNode = slotNode->children;
						while(itemNode){
							if(xmlStrcmp(itemNode->name, (const xmlChar*)"item") == 0){
								if(readXMLInteger(itemNode, "id", intValue)){
									itemId = intValue;

									Item* item = Item::CreateItem(itemId);
									if(item){
										item->unserialize(itemNode);
										player->__internalAddThing(slotId, item);
									}
								}
								else{
									isLoaded = false;
								}
							}

							itemNode = itemNode->next;
						}
					}

					slotNode = slotNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"depots") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"depot") == 0){
						int32_t depotId = 0;
						int32_t itemId = 0;

						if(readXMLInteger(tmpNode, "depotid", intValue)){
							depotId = intValue;
						}
						else{
							isLoaded = false;
						}

						xmlNodePtr itemNode = tmpNode->children;
						while(itemNode){
							if(xmlStrcmp(itemNode->name, (const xmlChar*)"item") == 0){
								if(readXMLInteger(itemNode, "id", intValue)){
									itemId = intValue;

									if(itemId != 0){
										Depot* myDepot = new Depot(itemId);
										myDepot->useThing2();
										myDepot->unserialize(itemNode);
										myDepot->setDepotId(depotId);

										player->addDepot(myDepot, depotId);
									}

									break;
								}
								else{
									isLoaded = false;
								}
							}

							itemNode = itemNode->next;
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"storage") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"data") == 0){
						unsigned long key = 0;
						long value = 0;

						if(readXMLInteger(tmpNode, "key", intValue)){
							key = intValue;
						}

						if(readXMLInteger(tmpNode, "value", intValue)){
							value = intValue;
						}

						player->addStorageValue(key, value);
					}

					tmpNode = tmpNode->next;
				}
			}

			p = p->next;
		}

		player->updateInventoryWeigth();
		player->updateItemsLight(true);
		player->setSkillsPercents();

		xmlFreeDoc(doc);
		xmlMutexUnlock(xmlmutex);

		if(!loadVIP(player))
			return false;

		return isLoaded;
	}

	xmlMutexUnlock(xmlmutex);
	return false;
}

bool IOPlayerXML::savePlayer(Player* player)
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::string filename = datadir + "players/" + player->getName() + ".xml";
	toLowerCaseString(filename); //store all player files in lowercase

	std::stringstream sb;

	xmlMutexLock(xmlmutex);
	xmlNodePtr nn, sn, pn, root;

	xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"player", NULL);
	root = doc->children;

	player->preSave();

	sb << player->getName();
	xmlSetProp(root, (const xmlChar*)"name", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->accountNumber;
	xmlSetProp(root, (const xmlChar*)"account", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->sex;
	xmlSetProp(root, (const xmlChar*)"sex", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << (int)player->getDirection();
	xmlSetProp(root, (const xmlChar*)"lookdir", (const xmlChar*)sb.str().c_str());

	sb.str("");
	char buf[128];
	_i64toa(player->experience, buf, 10);
	sb << buf;
	xmlSetProp(root, (const xmlChar*)"exp", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << (int)player->getVocationId();
	xmlSetProp(root, (const xmlChar*)"voc", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->level;
	xmlSetProp(root, (const xmlChar*)"level", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->accessLevel;
	xmlSetProp(root, (const xmlChar*)"access", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->getCapacity();
	xmlSetProp(root, (const xmlChar*)"cap", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->magLevel;
	xmlSetProp(root, (const xmlChar*)"maglevel", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->soul;
	xmlSetProp(root, (const xmlChar*)"soul", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->maxDepotLimit;
	xmlSetProp(root, (const xmlChar*)"maxdepotitems", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->lastlogin;
	xmlSetProp(root, (const xmlChar*)"lastlogin", (const xmlChar*)sb.str().c_str());

	pn = xmlNewNode(NULL,(const xmlChar*)"spawn");

	sb.str("");
	sb << player->getLoginPosition().x;
	xmlSetProp(pn, (const xmlChar*)"x", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->getLoginPosition().y;
	xmlSetProp(pn,  (const xmlChar*)"y", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->getLoginPosition().z;
	xmlSetProp(pn,  (const xmlChar*)"z", (const xmlChar*)sb.str().c_str());

	xmlAddChild(root, pn);

	pn = xmlNewNode(NULL,(const xmlChar*)"temple");

	sb.str("");
	sb << player->masterPos.x;
	xmlSetProp(pn, (const xmlChar*)"x", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->masterPos.y;
	xmlSetProp(pn, (const xmlChar*)"y", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->masterPos.z;
	xmlSetProp(pn, (const xmlChar*)"z", (const xmlChar*)sb.str().c_str());
	xmlAddChild(root, pn);

	pn = xmlNewNode(NULL,(const xmlChar*)"health");

	sb.str("");
	sb << player->health;
	xmlSetProp(pn, (const xmlChar*)"now", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << player->healthMax;
	xmlSetProp(pn, (const xmlChar*)"max", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition){
		sb << condition->getTicks() / 1000;
	}
	else{
		sb << 0;
	}
	xmlSetProp(pn, (const xmlChar*)"food", (const xmlChar*)sb.str().c_str());
	
	xmlAddChild(root, pn);

	pn = xmlNewNode(NULL,(const xmlChar*)"mana");

	sb.str("");
	sb << player->mana;
	xmlSetProp(pn, (const xmlChar*)"now", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->manaMax;
	xmlSetProp(pn, (const xmlChar*)"max", (const xmlChar*)sb.str().c_str());

	sb.str("");
	sb << player->manaSpent;
	xmlSetProp(pn, (const xmlChar*)"spent", (const xmlChar*)sb.str().c_str());
	xmlAddChild(root, pn);

	#ifdef __SKULLSYSTEM__
	pn = xmlNewNode(NULL,(const xmlChar*)"skull");
	long redSkullTime = 0;
	if(player->redSkullTicks > 0){
		redSkullTime = std::time(NULL) + player->redSkullTicks/1000;
	}

	sb.str("");
	sb << redSkullTime;
    xmlSetProp(pn, (const xmlChar*)"redskulltime", (const xmlChar*)sb.str().c_str());
	long redSkull = 0;
	if(player->skull == SKULL_RED){
		redSkull = 1;
	}
	
	sb.str("");
	sb << redSkull;
    xmlSetProp(pn, (const xmlChar*)"redskull", (const xmlChar*)sb.str().c_str());
    
	xmlAddChild(root, pn);
	#endif

	//upconversion of uchar(uint8_t) to get value not character into the stream
	pn = xmlNewNode(NULL,(const xmlChar*)"look");

	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookType;
	xmlSetProp(pn, (const xmlChar*)"type", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookHead;
	xmlSetProp(pn, (const xmlChar*)"head", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookBody;
	xmlSetProp(pn, (const xmlChar*)"body", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookLegs;
	xmlSetProp(pn, (const xmlChar*)"legs", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookFeet;
	xmlSetProp(pn, (const xmlChar*)"feet", (const xmlChar*)sb.str().c_str());
	
	sb.str("");
	sb << (int16_t)player->defaultOutfit.lookAddons;
	xmlSetProp(pn, (const xmlChar*)"addons", (const xmlChar*)sb.str().c_str());

	xmlAddChild(root, pn);

	sn = xmlNewNode(NULL,(const xmlChar*)"skills");
	for(int i = 0; i <= 6; i++){
	  pn = xmlNewNode(NULL,(const xmlChar*)"skill");

		sb.str("");
		sb << i;
		xmlSetProp(pn, (const xmlChar*)"skillid", (const xmlChar*)sb.str().c_str());
		
		sb.str("");
		sb << player->skills[i][SKILL_LEVEL];
		xmlSetProp(pn, (const xmlChar*)"level",   (const xmlChar*)sb.str().c_str());
		
		sb.str("");
		sb << player->skills[i][SKILL_TRIES];
		xmlSetProp(pn, (const xmlChar*)"tries",   (const xmlChar*)sb.str().c_str());

		xmlAddChild(sn, pn);
	}

	xmlAddChild(root, sn);

	#ifdef __XID_SEPERATE_ADDONS__
    sn = xmlNewNode(NULL,(const xmlChar*)"addons");
	for(KnowAddonMap::const_iterator it = player->getKnowAddonBegin(); it != player->getKnowAddonEnd(); it++){
		pn = xmlNewNode(NULL,(const xmlChar*)"addon");
		
		sb.str("");		
		sb << it->first;
        xmlSetProp(pn, (const xmlChar*) "outfit", (const xmlChar*)sb.str().c_str());
        
        sb.str("");
		sb << it->second;
        xmlSetProp(pn, (const xmlChar*) "type", (const xmlChar*)sb.str().c_str());
        
		xmlAddChild(sn, pn);
	}
	
	xmlAddChild(root, sn);
	#endif

	#ifdef __JD_DEATH_LIST__
    sn = xmlNewNode(NULL,(const xmlChar*)"deaths");
	for (std::list<Death>::iterator it = player->deathList.begin(); it != player->deathList.end(); it++){
		pn = xmlNewNode(NULL,(const xmlChar*)"death");
		
		sb.str("");		
		sb << (*it).killer;
        xmlSetProp(pn, (const xmlChar*) "name", (const xmlChar*)sb.str().c_str());
        
        sb.str("");
		sb << (*it).level;
        xmlSetProp(pn, (const xmlChar*) "level", (const xmlChar*)sb.str().c_str());
        
        sb.str("");
		sb << long((*it).time);
        xmlSetProp(pn, (const xmlChar*) "time", (const xmlChar*)sb.str().c_str());
        
		xmlAddChild(sn, pn);
	}
	
	xmlAddChild(root, sn);
	#endif

	#ifdef __XID_LEARN_SPELLS__
	sn = xmlNewNode(NULL,(const xmlChar*)"spells"); 
	for(StringVector::iterator it = player->learnedSpells.begin(); it != player->learnedSpells.end(); ++it){
		pn = xmlNewNode(NULL,(const xmlChar*)"spell");
		
		sb.str("");
		sb << *it;
        xmlSetProp(pn, (const xmlChar*) "words", (const xmlChar*)sb.str().c_str());
        
		xmlAddChild(sn, pn);
	}
	
	xmlAddChild(root, sn);
	#endif

	#ifdef __XID_BLESS_SYSTEM__
	sn = xmlNewNode(NULL,(const xmlChar*)"blessings"); 
	for(int i=0; i<6; i++){
		if(player->getBlessing(i)){
			pn = xmlNewNode(NULL,(const xmlChar*)"blessing");
		
			sb.str("");
			sb << i;
        	xmlSetProp(pn, (const xmlChar*) "id", (const xmlChar*)sb.str().c_str());
        
			xmlAddChild(sn, pn);
		}
	}
	
	xmlAddChild(root, sn);
	#endif

	sn = xmlNewNode(NULL,(const xmlChar*)"inventory");
	for(int i = 1; i <= 10; i++){
		if(player->inventory[i]){
			pn = xmlNewNode(NULL,(const xmlChar*)"slot");

			sb.str("");
			sb << i;
			xmlSetProp(pn, (const xmlChar*)"slotid", (const xmlChar*)sb.str().c_str());

			nn = player->inventory[i]->serialize();

			xmlAddChild(pn, nn);
			xmlAddChild(sn, pn);
		}
	}

	xmlAddChild(root, sn);

	sn = xmlNewNode(NULL,(const xmlChar*)"depots");

	for(DepotMap::reverse_iterator it = player->depots.rbegin(); it !=player->depots.rend()  ;++it){
		pn = xmlNewNode(NULL,(const xmlChar*)"depot");

		sb.str("");
		sb << it->first;
		xmlSetProp(pn, (const xmlChar*)"depotid", (const xmlChar*)sb.str().c_str());

		nn = (it->second)->serialize();
		xmlAddChild(pn, nn);
		xmlAddChild(sn, pn);
	}

	xmlAddChild(root, sn);

	sn = xmlNewNode(NULL,(const xmlChar*)"storage");
	player->genReservedStorageRange();
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd();cit++){
		pn = xmlNewNode(NULL,(const xmlChar*)"data");
		sb.str("");
		sb << cit->first;
		xmlSetProp(pn, (const xmlChar*)"key", (const xmlChar*)sb.str().c_str());

		sb.str("");
		sb << cit->second;
		xmlSetProp(pn, (const xmlChar*)"value", (const xmlChar*)sb.str().c_str());

		xmlAddChild(sn, pn);
	}
	xmlAddChild(root, sn);

	//Save the character
	//if(xmlSaveFile(filename.c_str(), doc)){
	bool result = xmlSaveFormatFileEnc(filename.c_str(), doc, "UTF-8", 1);
	
	if(!saveVIP(player)){
		result = false;
	}
	
	xmlFreeDoc(doc);
	xmlMutexUnlock(xmlmutex);
	return result;
}

bool IOPlayerXML::loadVIP(Player* player)
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::stringstream filename;
	filename << datadir << "vip/" << player->accountNumber << ".xml";

	xmlDocPtr doc;
	xmlMutexLock(xmlmutex);
	doc = xmlParseFile(filename.str().c_str());

	if(!doc){
		xmlMutexUnlock(xmlmutex);
		return false;
	}

	xmlNodePtr root, vipNode;
	root = xmlDocGetRootElement(doc);
	int i = 0;

	if(xmlStrcmp(root->name,(const xmlChar*) "vips")){
		return false;
	}

	vipNode = root->children;
	while(vipNode){
		if(strcmp((char*)vipNode->name, "vip") == 0){
			player->vip[i++] = (const char*)xmlGetProp(vipNode, (const xmlChar *)"name");	  
		}
                     
		vipNode = vipNode->next;
	}

	xmlFreeDoc(doc);  
	xmlMutexUnlock(xmlmutex);  
	return true;
}

bool IOPlayerXML::saveVIP(Player* player) 
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::stringstream filename;
	filename << datadir << "vip/" << player->accountNumber << ".xml";
	   
	xmlDocPtr doc;        
	xmlMutexLock(xmlmutex);    
	xmlNodePtr root, vipNode;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"vips", NULL);
	root = doc->children;

	for(int i = 0; i < MAX_VIPS; i++){
		if(!player->vip[i].empty()){
			vipNode = xmlNewNode(NULL,(const xmlChar*)"vip");
			xmlSetProp(vipNode, (const xmlChar*) "name", (const xmlChar*)player->vip[i].c_str()); 
			xmlAddChild(root, vipNode);
		}    
	}
	
	bool result = xmlSaveFormatFileEnc(filename.str().c_str(), doc, "UTF-8", 1);
	xmlFreeDoc(doc);
	xmlMutexUnlock(xmlmutex);
	
	return result;
}

bool IOPlayerXML::playerExists(std::string name)
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::string filename = datadir + "players/" + name + ".xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);

	return fileExists(filename.c_str());
}

bool IOPlayerXML::reportMessage(const std::string& message) 
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::stringstream filename;
	
	time_t currentTime = std::time(NULL);
	std::string date = ctime(&currentTime);
	
	filename << datadir << "logs/" << date << ".xml";
	   
	xmlDocPtr doc;        
	xmlMutexLock(xmlmutex);    
	xmlNodePtr root, vipNode;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"report", NULL);
	root = doc->children;

	vipNode = xmlNewNode(NULL,(const xmlChar*)"report");
	xmlSetProp(vipNode, (const xmlChar*) "name", (const xmlChar*)message.c_str()); 
	xmlAddChild(root, vipNode);
	
	bool result = xmlSaveFormatFileEnc(filename.str().c_str(), doc, "UTF-8", 1);
	xmlFreeDoc(doc);
	xmlMutexUnlock(xmlmutex);
	
	return result;
}
