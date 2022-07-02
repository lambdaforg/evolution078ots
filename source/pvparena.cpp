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

#include "pvparena.h"
#include "player.h"
#include "configmanager.h"
extern ConfigManager g_config;

#include "game.h"
extern Game g_game;

#include "tools.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

PvPArena::PvPArena()
{
    exitPos = Position(0, 0, 7);
    flags = 0;
    arenaMembers.clear();
}

PvPArena::~PvPArena()
{
    arenaMembers.clear();
}

bool PvPArena::canAdd(const Thing* thing)
{
    if(!thing)
         return false;
    
    if(const Creature* creature = thing->getCreature())
    {
         if(!creature->isAttackable())
              return true;
         if(flags & ARENA_FLAG_NOSUMMONS && creature->getMonster() != NULL)
              return false;
         else if(!(flags & ARENA_FLAG_MULTICOMBAT) && arenaMembers.size() > 2 && creature->getPlayer())
         {
              std::list<Creature*>::iterator cit = std::find(arenaMembers.begin(), arenaMembers.end(), creature);
              if(cit == arenaMembers.end())
                  return false;
         }
    }
    else if(const Item* item = thing->getItem())
    {
         if(flags & ARENA_FLAG_NO_FIELDS && item->isMagicField())
              return false;
    }
    return true;
}


bool PvPArena::loadArenas()
{
     std::string filename = g_config.getString(ConfigManager::DATA_DIRECTORY) + "pvparenas.xml";
     xmlDocPtr doc = xmlParseFile(filename.c_str());
     if(!doc)
         return false;
     
     xmlNodePtr root, arenanode, tilenode;
     root = xmlDocGetRootElement(doc);
	 if(xmlStrcmp(root->name,(const xmlChar*)"pvparenas") != 0){
		xmlFreeDoc(doc);
		return false;
	 }
	 
	 arenanode = root->children;
	 while (arenanode){
         if(xmlStrcmp(arenanode->name,(const xmlChar*)"pvparena") == 0)
         {
             PvPArena* newArena = new PvPArena();
             int x, y, z;
             if(readXMLInteger(arenanode,"exitx",x) && 
                readXMLInteger(arenanode,"exity",y) && 
                readXMLInteger(arenanode,"exitz",z))
              {
                  newArena->exitPos = Position(x, y, z);
              }
              else
              {
                  puts("ERROR: Missing/incomplete exit pos for pvparena! Skipping...");
                  delete newArena;
                  arenanode = arenanode->next;
                  continue;
              }
              std::string strValue;
              int intValue;
              if(readXMLString(arenanode,"allowsummons",strValue) && strValue == "no")
                  newArena->flags |= ARENA_FLAG_NOSUMMONS;
              if(readXMLString(arenanode,"multi-combat",strValue) && strValue == "yes")
                  newArena->flags |= ARENA_FLAG_MULTICOMBAT;
              if(readXMLString(arenanode,"allowfields",strValue) && strValue == "no")
                  newArena->flags |= ARENA_FLAG_NO_FIELDS;
              tilenode = arenanode->children;
              while(tilenode)
              {
                  if(xmlStrcmp(tilenode->name,(const xmlChar*)"tiles") == 0)
                  {
                       int tox, toy, toz;
                       int fromx, fromy, fromz;
                       if(readXMLInteger(tilenode,"tox",tox) &&
                          readXMLInteger(tilenode,"toy",toy) &&
                          readXMLInteger(tilenode,"toz",toz) &&
                          readXMLInteger(tilenode,"fromx",fromx) &&
                          readXMLInteger(tilenode,"fromy",fromy) &&
                          readXMLInteger(tilenode,"fromz",fromz))
                        {
                            if(tox < fromx)
                               std::swap(tox, fromx);
                            if(toy < fromy)
                               std::swap(toy, fromy);
                            if(toz < fromz)
                               std::swap(toz, fromz);
                            for(int dx = fromx; dx <= tox; dx++)
                            {
                               for(int dy = fromy; dy <= toy; dy++)
                               {
                                   for(int dz = fromz; dz <= toz; dz++)
                                   {
                                       if(Tile* t = g_game.getTile(dx, dy, dz))
                                       {
                                            t->pvparena = newArena;
                                            t->setFlag(TILESTATE_PVP);
                                            t->setFlag(TILESTATE_NOSKULLS);
                                       }
                                   }
                               }
                            }
                        }
                        else
                            puts("ERROR: incomplete tile range! Skipping...");
                  }
                  tilenode = tilenode->next;
              }
         }
         arenanode = arenanode->next;
     }
     xmlFreeDoc(doc);
     return true;
	 
}

void PvPArena::removeCreature(Creature* cr, bool death)
{
     if(!cr)
        return;
     if(!cr->isAttackable())
        return;
     
     std::list<Creature*>::iterator cit = std::find(arenaMembers.begin(), arenaMembers.end(), cr);
     if(cit != arenaMembers.end())
         arenaMembers.erase(cit);
     if(death)
     {
         if(Player* pr = cr->getPlayer())
         {
              if(g_game.internalTeleport(cr, exitPos) == RET_NOERROR)
                   g_game.addMagicEffect(exitPos, NM_ME_ENERGY_AREA);
              pr->changeHealth(pr->getMaxHealth());
         }
         else
         {
              g_game.removeCreature(cr);
         }
     }
}

void PvPArena::addCreature(Creature* cr, bool isLogin)
{
     if(!cr)
          return;
     if(!cr->isAttackable())
          return;
     // cant login on a pvp arena
     if(isLogin && cr->getPlayer())
     {
          if(g_game.internalTeleport(cr, cr->getMasterPos()) == RET_NOERROR)
          {
              g_game.addMagicEffect(cr->getMasterPos(), NM_ME_ENERGY_AREA);
              return;
          }
     }
     arenaMembers.push_back(cr);
}


#endif
