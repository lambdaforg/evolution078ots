#ifdef __PARTYSYSTEM__
#ifndef __PARTY_H__
#define __PARTY_H__

#include <sstream>
#include <vector>

#include "player.h"
#include "const79.h"

class Party
{
public:
    /* Party* createParty(Player* leader, Player* firstInvitedPlayer)
     * Creates a new party and returns it.
     * First parameter is the player creating the party
     * Second parameter is the player who is the first "invitation"
     */
    static Party* createParty(Player* _leader, Player* _firstInvite);
    
    /* Player* getLeader()
     * returns the leader of the party
     */
    Player* getLeader() {return leader;};
    
    /* bool isInvited(Player* p)
     * returns true if the player is invited into the party.
     */
    bool isInvited(Player* p);
    
    /* void acceptInvitation(Player* p)
     * Player p accepts an invitation.
     * First parameter is the player that accepts the invitation
     */
    void acceptInvitation(Player* p);
    /* void invitePlayer(Player*, bool)
     * Invites a player to the current party
     * first parameter is the player to invite
     */
    void invitePlayer(Player* p);
    /* void revokeInvitation(Player* revokingPlayer)
     * revokes the invitation, displaying appropiate messages
     * first parameter player that revokes.
     */
    void revokeInvitation(Player* p);
    /* void kickPlayer(Player* p)
     * Kicks the selected player out of the party,
     * if it's the leader, leadership is transferred
     * to the first person in the membors vector.
     * first parameter is player to kick
     */    
    void kickPlayer(Player* p);
    
    /* void passLeadership(Player* p)
     * Passes the leadership of the party to player p.
     * First parameter, new leader
     */
    void passLeadership(Player* newLeader);
    /* void disband()
     * Disbands the party.
     */
    void disband();

private:
    std::vector<Player*> members;
    std::vector<Player*> invitedplayers;
    
    Player* leader;
};

#endif
#endif
