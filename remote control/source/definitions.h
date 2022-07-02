//////////////////////////////////////////////////////////////////////
// OTAdmin - OpenTibia
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

#ifndef __OTADMIN_DEFINITIONS_H__
#define __OTADMIN_DEFINITIONS_H__

#include "stdint.h"
#define NETWORKMESSAGE_MAXSIZE 16768

#if defined WIN32 || defined __WINDOWS__

#include "windows.h"

#define EWOULDBLOCK WSAEWOULDBLOCK

#ifndef __GNUC__
#pragma comment( lib, "Ws2_32.lib" )
typedef unsigned long uint32_t;
typedef signed long int32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif

inline void OTSYS_SLEEP(uint32_t t){
	Sleep(t);
}


#else

#include <time.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef closesocket
#define closesocket close
#endif

#ifndef SOCKADDR
#define SOCKADDR sockaddr
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

inline void OTSYS_SLEEP(int t)
{
	timespec tv;
	tv.tv_sec  = t / 1000;
	tv.tv_nsec = (t % 1000)*1000000;
	nanosleep(&tv, NULL);
}

#endif

#endif

