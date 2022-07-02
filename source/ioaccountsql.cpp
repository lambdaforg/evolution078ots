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

#include "ioaccountsql.h"
#include <algorithm>
#include <functional>
#include <sstream>

#include "database.h"
#include <iostream>

#include "configmanager.h"

extern ConfigManager g_config;

IOAccountSQL::IOAccountSQL()
{
	//
}

Account IOAccountSQL::loadAccount(unsigned long accno)
{
	Account acc;

	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;

	if(!mysql->connect()){
		return acc;
	}

	query << "SELECT * FROM accounts WHERE accno='" << accno << "'";
	if(!mysql->storeQuery(query, result))
		return acc;

	acc.accnumber = result.getDataInt("accno");
	acc.password = result.getDataString("password");
	acc.premDays = result.getDataInt("premDays");
	#ifdef __XID_PREMIUM_SYSTEM__
	acc.premEnd = result.getDataInt("premEnd");
	#endif

	query << "SELECT name FROM players where account='" << accno << "'";
	if(!mysql->storeQuery(query, result))
		return acc;

	for(uint32_t i = 0; i < result.getNumRows(); ++i){
		std::string ss = result.getDataString("name", i);
		acc.charList.push_back(ss.c_str());
	}

	acc.charList.sort();
	return acc;
}


bool IOAccountSQL::getPassword(unsigned long accno, const std::string &name, std::string &password)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}

	query << "SELECT password FROM accounts WHERE accno='" << accno << "'";
	if(!mysql->storeQuery(query, result))
		return false;

	std::string acc_password = result.getDataString("password");

	query << "SELECT name FROM players where account='" << accno << "'";
	if(!mysql->storeQuery(query, result))
		return false;

	for(uint32_t i = 0; i < result.getNumRows(); ++i)
	{
		std::string ss = result.getDataString("name", i);
		if(ss == name){
			password = acc_password;
			return true;
		}
	}
	return false;
}

bool IOAccountSQL::accountExists(unsigned long accno)
{
	Database* mysql = Database::instance();
	DBQuery query;
	DBResult result;
	
	if(!mysql->connect()){
		return false;
	}

	query << "SELECT accno FROM accounts WHERE accno='" << accno << "'";
	if(!mysql->storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	return true;
}

bool IOAccountSQL::saveAccount(Account account, unsigned long premiumTicks)
{
    Database* db = Database::instance();
    DBQuery query;

	if(!db->connect()){
		return false;
	}

	int days;                    
	time_t timeNow = std::time(NULL);    
	if(timeNow < premiumTicks){
		days = premiumTicks - timeNow; 
		days = (days / 86400);
	}
	else
		days = 0;

    DBTransaction trans(db);
    if(!trans.start())
        return false;

    query << "UPDATE `accounts` SET ";
	query << "`accno`='" << account.accnumber << "', ";
	query << "`password`='" << account.password << "', ";
    query << "`premDays`='" << days << "', ";
    query << "`premEnd`='" << premiumTicks << "' ";

	query << " WHERE `accno` = "<< account.accnumber
	#ifndef __USE_SQLITE__
	<<" LIMIT 1";
    #else
    ;
    #endif

    if(!db->executeQuery(query))
        return false;

    return trans.success();
}

#ifdef __XID_ACCOUNT_MANAGER__
bool IOAccountSQL::createAccount(Account account)
{
	Database* mysql = Database::instance();
	if(!mysql->connect()){
		return false;
	}
	
	DBQuery query;
	query << "INSERT INTO `accounts` (`id`, `accno`, `password`) VALUES ('" << 0 << "', '" << account.accnumber << "', '" << account.password << "')";
	
	if(!mysql->executeQuery(query)){
		return false;
	}

	return true;
}
#endif
