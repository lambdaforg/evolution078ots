//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// various definitions needed by most files
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


#ifndef __OTSERV_DEFINITIONS_H__
#define __OTSERV_DEFINITIONS_H__

#include "exception.h"
#include "configmanager.h"

extern ConfigManager g_config;

#ifdef XML_GCC_FREE
	#define xmlFreeOTSERV(s)	free(s)
#else
	#define xmlFreeOTSERV(s)	xmlFree(s)
#endif

#ifdef __DEBUG_EXCEPTION_REPORT__
	#define DEBUG_REPORT int *a = NULL; *a = 1;
#else
	#ifdef __EXCEPTION_TRACER__
		#define DEBUG_REPORT ExceptionHandler::dumpStack();
	#else
		#define DEBUG_REPORT
	#endif
#endif

#ifdef __USE_SQLITE__
    #define __SPLIT_QUERIES__
#endif

#if defined __USE_MYSQL__ || __USE_SQLITE__
	#define USE_SQL_ENGINE
#endif

#if defined(__USE_MYSQL__) && !defined(__USE_SQLITE__)
	#define USE_MYSQL_ONLY
#endif

#ifdef __XID_PREMIUM_SYSTEM__
	#define FREE_PREMIUM g_config.getString(ConfigManager::FREE_PREMIUM)
#endif

#ifdef __XID_PROTECTION_SYSTEM__
	#define PROTECTION_LIMIT g_config.getNumber(ConfigManager::PROTECTION_LIMIT)
#endif

#ifdef __XID_CONFIG_CAP__
	#define CAP_SYSTEM g_config.getString(ConfigManager::CAP_SYSTEM)
#else
	#define CAP_SYSTEM "yes"
#endif

#ifdef __PARTYSYSTEM__
	#ifndef __SKULLSYSTEM__
		#undef __PARTYSYSTEM__
	#endif
#endif

#define ACCESS_ENTER g_config.getNumber(ConfigManager::ACCESS_ENTER)
#define ACCESS_PROTECT g_config.getNumber(ConfigManager::ACCESS_PROTECT)
#define ACCESS_HOUSE g_config.getNumber(ConfigManager::ACCESS_HOUSE)
#define ACCESS_TALK g_config.getNumber(ConfigManager::ACCESS_TALK)
#define ACCESS_MOVE g_config.getNumber(ConfigManager::ACCESS_MOVE)
#define ACCESS_LOOK g_config.getNumber(ConfigManager::ACCESS_LOOK)

#define GUILD_SYSTEM g_config.getString(ConfigManager::GUILD_SYSTEM)

#define CRITICAL_CHANCE g_config.getCriticalString(1)
#define CRITICAL_DAMAGE g_config.getCriticalString(2)

#define ipText(a) (unsigned int)a[0] << "." << (unsigned int)a[1] << "." << (unsigned int)a[2] << "." << (unsigned int)a[3]

#if defined __WINDOWS__ || defined WIN32

#ifndef __FUNCTION__
	#define	__FUNCTION__ __func__
#endif

#define OTSYS_THREAD_RETURN  void
#define EWOULDBLOCK WSAEWOULDBLOCK

#ifdef __GNUC__
	#include <ext/hash_map>
	#include <ext/hash_set>
	#define OTSERV_HASH_MAP __gnu_cxx::hash_map
	#define OTSERV_HASH_SET __gnu_cxx::hash_set

#else
	typedef unsigned long long uint64_t;

	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#include <hash_map>
	#include <hash_set>
	#include <limits>
	#include <time.h>
	
	#define OTSERV_HASH_MAP stdext::hash_map
	#define OTSERV_HASH_SET stdext::hash_set

	#include <cstring>
	inline int strcasecmp(const char *s1, const char *s2)
	{
		return ::_stricmp(s1, s2);
	}

	typedef __int64 int64_t;
	typedef unsigned long uint32_t;
	typedef signed long int32_t;
	typedef unsigned short uint16_t;
	typedef signed short int16_t;
	typedef unsigned char uint8_t;

	#pragma warning(disable:4786) // msvc too long debug names in stl
	#pragma warning(disable:4250) // 'class1' : inherits 'class2::member' via dominance

#endif

//*nix systems
#else
	#define OTSYS_THREAD_RETURN void*

	#include <stdint.h>
	#include <string.h>
	#include <ext/hash_map>
	#include <ext/hash_set>

	#define OTSERV_HASH_MAP __gnu_cxx::hash_map
	#define OTSERV_HASH_SET __gnu_cxx::hash_set
	typedef int64_t int64_t;

#endif

#endif
