//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Pvp Arena by Nathaniel Fries
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
#ifdef __NFS_PVP_ARENA__
#ifndef __PVP_ARENA_H__
#define __PVP_ARENA_H__


#include "tile.h"
#include "position.h"
#include <list>

enum PvPArenaFlags_t
{
     ARENA_FLAG_NONE = 0,
     ARENA_FLAG_NOSUMMONS = 2,
     ARENA_FLAG_MULTICOMBAT = 4,
     ARENA_FLAG_NO_FIELDS = 8,
};
     

class PvPArena
{
    public:
       PvPArena();
       ~PvPArena();
       
       int flags;
       
       Position exitPos;
       
       virtual bool canAdd(const Thing*);
       std::list<Creature*> arenaMembers;
       
       static bool loadArenas();
       
       void removeCreature(Creature* cr,  bool death = false);
       void addCreature(Creature* cr, bool isLogin = false);
};

#endif //__PVP_ARENA_H__
#endif //__PVP_ARENA__
