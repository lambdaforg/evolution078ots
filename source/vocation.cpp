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

#include "definitions.h"
#include "vocation.h"
#include <iostream>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <cmath>

#include "tools.h"

bool Vocations::loadFromXml(const std::string& datadir)
{
	std::string filename = datadir + "vocations.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"vocations") != 0){
			xmlFreeDoc(doc);
			return false;
		}
		
		p = root->children;
		
		while(p){
			std::string str;
			int intVal;
			float floatVal;
			if(xmlStrcmp(p->name, (const xmlChar*)"vocation") == 0){
				Vocation* voc = new Vocation();
				uint32_t voc_id;
				xmlNodePtr skillNode;
				if(readXMLInteger(p, "id", intVal)){
					voc_id = intVal;
					if(readXMLString(p, "name", str)){
						voc->name = str;
					}
					if(readXMLString(p, "description", str)){
						voc->description = str;
					}
					if(readXMLInteger(p, "gaincap", intVal)){
						voc->gainCap = intVal;
					}
					if(readXMLInteger(p, "gainhp", intVal)){
						voc->gainHP = intVal;
					}
					if(readXMLInteger(p, "gainmana", intVal)){
						voc->gainMana = intVal;
					}
					if(readXMLInteger(p, "gainhpticks", intVal)){
						voc->gainHealthTicks = intVal;
					}
					if(readXMLInteger(p, "gainhpamount", intVal)){
						voc->gainHealthAmount = intVal;
					}
					if(readXMLInteger(p, "gainmanaticks", intVal)){
						voc->gainManaTicks = intVal;
					}
					if(readXMLInteger(p, "gainmanaamount", intVal)){
						voc->gainManaAmount = intVal;
					}
					if(readXMLInteger(p, "gainsoulticks", intVal)){
						voc->gainSoulTicks = intVal;
					}
					if(readXMLInteger(p, "gainsoulamount", intVal)){
						voc->gainSoulAmount = intVal;
					}
					if(readXMLInteger(p, "maxsoul", intVal)){
						if(intVal > 255){
							intVal = 255;
						}
						
						voc->soulMax = intVal;
					}
					if(readXMLFloat(p, "manamultiplier", floatVal)){
						voc->manaMultiplier = floatVal;
					}
					if(readXMLInteger(p, "attackspeed", intVal)){
						voc->attackSpeed = intVal;
					}
					if(readXMLInteger(p, "prevoc", intVal)){
						voc->preVocation = intVal;
					}
					if(readXMLInteger(p, "basespeed", intVal)){
						voc->baseSpeed = intVal;
					}
					
					skillNode = p->children;
					while(skillNode){
						if(xmlStrcmp(skillNode->name, (const xmlChar*)"skill") == 0){
							uint32_t skill_id;
							if(readXMLInteger(skillNode, "id", intVal)){
								skill_id = intVal;
								if(skill_id < SKILL_FIRST || skill_id > SKILL_LAST){
									std::cout << "No valid skill id. " << skill_id << std::endl;
								}
								else{
									if(readXMLFloat(skillNode, "multiplier", floatVal)){
										voc->skillMultiplier[skill_id] = floatVal;
									}
								}
							}
							else{
								std::cout << "Missing skill id." << std::endl;
							}
						}
						else if(xmlStrcmp(skillNode->name, (const xmlChar*)"diepercent") == 0){
							if(readXMLInteger(skillNode, "experience", intVal)){
								voc->lostExperience = intVal;
							}
							if(readXMLInteger(skillNode, "magic", intVal)){
								voc->lostMagic = intVal;
							}
							if(readXMLInteger(skillNode, "skill", intVal)){
								voc->lostSkill = intVal;
							}
							if(readXMLInteger(skillNode, "equipment", intVal)){
								voc->lostEquipment = intVal;
							}
							if(readXMLInteger(skillNode, "container", intVal)){
								voc->lostContainer = intVal;
							}
						}
						#ifdef __XID_CVS_MODS__
						else if(xmlStrcmp(skillNode->name, (const xmlChar*)"formula") == 0){
							if(readXMLFloat(skillNode, "damage", floatVal)){
								voc->damageMultiplier = floatVal;
							}
							if(readXMLFloat(skillNode, "defense", floatVal)){
								voc->defenseMultiplier = floatVal;
							}
							if(readXMLFloat(skillNode, "armor", floatVal)){
								voc->armorMultiplier = floatVal;
							}
						}
						#endif
						
						skillNode = skillNode->next;
					}
					
					vocationsMap[voc_id] = voc;
					
				}
				else{
					std::cout << "Missing vocation id." << std::endl;
				}
			}
			p = p->next;
		}
		xmlFreeDoc(doc);
	}
	return true;
}

Vocation* Vocations::getVocation(uint32_t vocId)
{
	VocationsMap::iterator it = vocationsMap.find(vocId);
	if(it != vocationsMap.end()){
		return it->second;
	}
	else{
		std::cout << "Warning: [Vocations::getVocation] Vocation " << vocId << " not found." << std::endl;
		return &def_voc;
	}
}

int32_t Vocations::getVocationId(const std::string& name)
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it){
		if(strcasecmp(it->second->name.c_str(), name.c_str()) == 0){
			return it->first;
		}
	}

	return -1;
}

uint32_t Vocation::skillBase[SKILL_LAST + 1] = { 50, 50, 50, 50, 30, 100, 20 };

Vocation::Vocation()
{
	name = "none";
	gainHealthTicks = 6;
	gainHealthAmount = 1;
	gainManaTicks = 6;
	gainManaAmount = 1;
	gainSoulTicks = 15;
	gainSoulAmount = 1;
	gainCap = 5;
	gainMana = 5;
	gainHP = 5;
	soulMax = 100;
	manaMultiplier = 4;
	attackSpeed = 1500;
	preVocation = 0;

	lostExperience = 7;
	lostMagic = 7;
	lostSkill = 7;
	lostEquipment = 7;
	lostContainer = 100;
	
	#ifdef __XID_CVS_MODS__
	damageMultiplier = 1.2;
    defenseMultiplier = 1.1;
   	armorMultiplier = 1.1;
   	#endif
}

Vocation::~Vocation()
{
	cacheMana.clear();
	for(int i = SKILL_FIRST; i < SKILL_LAST; ++i){
		cacheSkill[i].clear();
	}
}

uint32_t Vocation::getReqSkillTries(int skill, int level)
{
	if(skill < SKILL_FIRST || skill > SKILL_LAST){
		return 0;
	}
	
	cacheMap& skillMap = cacheSkill[skill];
	cacheMap::iterator it = skillMap.find(level);
	if(it != cacheSkill[skill].end()){
		return it->second;
	}
	uint32_t tries = (unsigned int)(skillBase[skill] * pow((float)skillMultiplier[skill], (float)(level - 11)));
	skillMap[level] = tries;
	return tries;
}

uint32_t Vocation::getReqMana(int magLevel)
{
	if(magLevel < 0){
		magLevel = 1;
	}
	
	cacheMap::iterator it = cacheMana.find(magLevel);
	if(it != cacheMana.end()){
		return it->second;
	}
	uint32_t reqMana = (unsigned int)(400*pow(manaMultiplier, magLevel-1));
	if (reqMana % 20 < 10)
    	reqMana = reqMana - (reqMana % 20);
	else
    	reqMana = reqMana - (reqMana % 20) + 20;

	cacheMana[magLevel] = reqMana;

	return reqMana;
}

int32_t Vocation::getHealthForLv(int32_t level)
{
	int32_t newHealth = 185;
	int32_t newLevel = level - 8;
	
	if(newLevel <= 8){
		newHealth = (level*5) + 150;
	}
	else{
		newHealth += newLevel * gainHP;
	}

	return newHealth;
}

int32_t Vocation::getManaForLv(int32_t level)
{
	int32_t newMana = 35;
	int32_t newLevel = level - 8;
	
	if(newLevel <= 8){
		newMana = (level*5);
	}
	else{
		newMana += newLevel * gainMana;
	}

	return newMana;
}

int32_t Vocation::getMagicLevelForManaSpent(int32_t manaSpent)
{
	uint32_t magicLevel = 0;
	while(getReqMana(magicLevel) < manaSpent){
		magicLevel++;
	}

	return magicLevel;
}

double Vocation::getCapacityForLv(int32_t level)
{
	int32_t newCapacity = 370;
	int32_t newLevel = level - 8;
	
	if(newLevel <= 8){
		newCapacity = (level*10) + 300;
	}
	else{
		newCapacity += newLevel * gainCap;
	}

	return newCapacity;
}

uint32_t Vocation::getDiePercent(int32_t lostType)
{
	switch(lostType){
		case 1:	return lostExperience;
		case 2: return lostMagic;
		case 3: return lostSkill;
		case 4: return lostEquipment;
		case 5: return lostContainer;
	}
	
	return 0;
}

void Vocation::debugVocation()
{
	/*std::cout << "name: " << name << std::endl;
	std::cout << "gain cap: " << gainCap << " hp: " << gainHP << " mana: " << gainMana << std::endl;
	std::cout << "gain time: Healht(" << gainHealthTicks << " ticks, +" << gainHealthAmount << "). Mana(" << 
		gainManaTicks << " ticks, +" << gainManaAmount << ")" << std::endl;
	std::cout << "mana multiplier: " << manaMultiplier << std::endl;
	for(int i = SKILL_FIRST; i < SKILL_LAST; ++i){
		std::cout << "Skill id: " << i << " multiplier: " << skillMultipliers[i] << std::endl;
	}*/
}
