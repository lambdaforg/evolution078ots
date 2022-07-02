//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the Account Loader/Saver
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

#include "ioaccountxml.h"
#include "tools.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <fstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/threads.h>

#include "configmanager.h"

extern xmlMutexPtr xmlmutex;
extern ConfigManager g_config;

IOAccountXML::IOAccountXML()
{
	if(xmlmutex == NULL){
		xmlmutex = xmlNewMutex();
	}
}

Account IOAccountXML::loadAccount(unsigned long accno)
{
	Account acc;

	std::stringstream accsstr;
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	accsstr << datadir + "accounts/" << accno << ".xml";
	std::string filename = accsstr.str();
	xmlMutexLock(xmlmutex);

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"account") != 0){
			xmlFreeDoc(doc);			
			xmlMutexUnlock(xmlmutex);
			return acc;
		}

		p = root->children;

		std::string strValue;
		int intValue;

		if(readXMLString(root, "pass", strValue)){
			acc.password = strValue;
		}

		if(readXMLInteger(root, "premDays", intValue)){
			acc.premDays = intValue;
		}

		#ifdef __XID_PREMIUM_SYSTEM__
		if(readXMLInteger(root, "premEnd", intValue)){
			acc.premEnd = intValue;
		}
		#endif

		// now load in characters.
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"characters") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(readXMLString(tmpNode, "name", strValue)){
						if(xmlStrcmp(tmpNode->name, (const xmlChar*)"character") == 0){
							acc.charList.push_back(strValue);
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);

		// Organize the char list.
		acc.charList.sort();
		acc.accnumber = accno;
	}	

	xmlMutexUnlock(xmlmutex);
	return acc;
}

bool IOAccountXML::getPassword(unsigned long accno, const std::string& name, std::string& password)
{
	std::string acc_password;
	
	std::stringstream accsstr;
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	accsstr << datadir + "accounts/" << accno << ".xml";
	std::string filename = accsstr.str();
	
	xmlMutexLock(xmlmutex);
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"account") != 0){
			xmlFreeDoc(doc);			
			xmlMutexUnlock(xmlmutex);
			return false;
		}

		p = root->children;
		
		std::string strValue;
		if(readXMLString(root, "pass", strValue)){
			acc_password = strValue;
		}

		// now load in characters.
		while(p){
			if(xmlStrcmp(p->name, (const xmlChar*)"characters") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(readXMLString(tmpNode, "name", strValue)){
						if(xmlStrcmp(tmpNode->name, (const xmlChar*)"character") == 0){
							if(strValue == name){
								password = acc_password;
								xmlFreeDoc(doc);
								xmlMutexUnlock(xmlmutex);
								return true;
							}
						}
					}
					tmpNode = tmpNode->next;
				}
			}
			p = p->next;
		}
		
		xmlFreeDoc(doc);
	}

	xmlMutexUnlock(xmlmutex);
	return false;
}

bool IOAccountXML::accountExists(unsigned long accno)
{
	std::string datadir = g_config.getString(ConfigManager::DATA_DIRECTORY);
	std::string filename = datadir + "accounts/" + str(accno) + ".xml";
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);

	return fileExists(filename.c_str());
}

bool IOAccountXML::saveAccount(Account account, unsigned long premiumTicks)
{
	int days;                    
	time_t timeNow = std::time(NULL);    
	if(timeNow < premiumTicks){
		days = premiumTicks - timeNow; 
		days = (days / 86400);
	}
	else
		days = 0;

	std::stringstream dir;
	dir << "data/accounts/" << account.accnumber << ".xml";
	std::ofstream accFile(dir.str().c_str());
	
	dir.str("");				
	std::list<std::string>::iterator it;
	for(it = account.charList.begin(); it != account.charList.end(); it++){
		dir << "<character name=\"" << (*it) << "\" />";
	}
	
	accFile << "<?xml version=\"1.0\"?><account pass=\"" << account.password << "\" premDays=\"" << days << "\" premEnd=\"" << premiumTicks << "\"><characters>" << 
		dir.str().c_str() << "</characters></account>" << std::endl;
	
	return true;
}
