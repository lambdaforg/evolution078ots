#ifdef __PARTYSYSTEM__
#include "party.h"
#include "configmanager.h"

extern ConfigManager g_config;

Party* Party::createParty(Player* _leader, Player* _firstInvite)
{
    //std::cout << _leader->getName() << " creates a party and invites player " << _firstInvite->getName() << "." << std::endl;
    if (_firstInvite->party != NULL)
    {
        std::stringstream info;
        info << _firstInvite->getName() << " is already in a party.";
        _leader->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
        return NULL;
    }
    
    Party* party = new Party();
    
    _leader->party = party;
    party->leader = _leader;
    
    party->invitePlayer(_firstInvite);
    _leader->sendCreatureShield(_leader);
    _leader->sendCreatureSkull(_leader);
    return party;
}

void Party::invitePlayer(Player* p)
{
    //std::cout << "Player " << p->getName() << " is invited to " << leader->getName() << "'s party." << std::endl;
    if (p->party != NULL)
    {
        std::stringstream info;
        info << p->getName() << " is already in a party.";
        leader->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
        return;
    }
    std::stringstream info;
    info << p->getName() << " has been invited.";
    leader->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());

    info.str("");
    info << leader->getName() << " invites you to " << (leader->getSex() == PLAYERSEX_MALE? "his" : "her") << " party.";
    p->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
    
    invitedplayers.push_back(p);
    
    p->sendCreatureShield(leader);
    p->sendCreatureShield(p);
    leader->sendCreatureShield(p);
}

void Party::acceptInvitation(Player* p)
{
    //std::cout << p->getName() << " accepts " << leader->getName() << "'s invitation." << std::endl;
    std::vector<Player*>::iterator invited = std::find(invitedplayers.begin(), invitedplayers.end(), p);
    if(invited != invitedplayers.end())
        invitedplayers.erase(invited);
    else
        //Player has not been invited.
        return;
    
    members.push_back(p);
    p->party = this;
    
    std::stringstream info;
    info << p->getName() << " has joined the party.";
    
    for(int i = 0; i < members.size(); i++)
    {
        members[i]->sendCreatureSkull(p);
        members[i]->sendCreatureShield(p);
        
        p->sendCreatureSkull(members[i]);
        p->sendCreatureShield(members[i]);
        
        //Give info message
        members[i]->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
    }

    leader->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
    leader->sendCreatureSkull(p);
    leader->sendCreatureShield(p);
    
    info.str("");
    info <<  "You have joined "<< leader->getName() << "'s party.";

    p->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
    p->sendCreatureShield(leader);
    p->sendCreatureSkull(leader);
}

void Party::passLeadership(Player* newLeader)
{
    Player* oldLeader = this->leader;
    
    //std::cout << oldLeader->getName() << " passes leadership to " << newLeader->getName() << std::endl;
    if(newLeader->party != this)
        return;
    
    //First do the actual transfer
    std::vector<Player*>::iterator q = std::find(members.begin(), members.end(), newLeader);
    members.erase(q);
    
    leader = newLeader;
    members.insert(members.begin(), oldLeader);
    
    std::stringstream info;
    info << newLeader->getName() << " is now the leader of the party.";
    for(unsigned int i = 0; i < members.size(); i++)
    {
        members[i]->sendCreatureShield(oldLeader);
        members[i]->sendCreatureShield(newLeader);
        members[i]->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
    }
    
    newLeader->sendTextMessage(MSG_INFO_DESCR, "You are now the leader of the party.");
    newLeader->sendCreatureShield(newLeader);
    newLeader->sendCreatureShield(oldLeader);
    
    //Cancel the invites.
    for(unsigned int i = 0; i < invitedplayers.size(); i++)
    {
        oldLeader->sendCreatureShield(invitedplayers[i]);
        invitedplayers[i]->sendCreatureShield(oldLeader);
    }
    invitedplayers.clear();
}

void Party::revokeInvitation(Player* p)
{
    //std::cout << p->getName() << " invitation is rejected by " << leader->getName() << "." << std::endl;
    std::vector<Player*>::iterator invited = std::find(invitedplayers.begin(), invitedplayers.end(), p);
    if(invited != invitedplayers.end())
        invitedplayers.erase(invited);
    else
        //Player has not been invited.
        return;
    
    std::stringstream info;
    info << "Invitation for " << p->getName() << " has been revoked.";
    leader->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
    
    info.str("");
    info << leader->getName() << " has revoked " << (leader->getSex() == PLAYERSEX_MALE? "his":"her") << " invitation.";
    p->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
    
    p->sendCreatureShield(leader);
    leader->sendCreatureShield(p);
    
    //If only the leader remains in the party, disband it.
    if(members.size() == 0 && invitedplayers.size() == 0)
        disband();
}

void Party::kickPlayer(Player* p)
{
    //std::cout << "Player " << p->getName() << " is kicked from " << leader->getName() << "'s party." << std::endl;
    if(members.size() > 0)
    {
        if(p == leader)
            // So it was the leader... transfer the 
            // leadership to the first person that joined.
            passLeadership(members.front());
        
        std::vector<Player*>::iterator q = std::find(members.begin(), members.end(), p);
        if(q != members.end())
            members.erase(q);
        else
            //Player is not a member of the party
            return;
    }
    else if(p == leader)
    {
        disband();
        return;
    }
    p->party = NULL;
    
    std::stringstream info;
    info << p->getName() << " has left the party.";
    
    for(unsigned int i = 0; i < members.size(); i++)
    {
        members[i]->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
        
        members[i]->sendCreatureShield(p);
        members[i]->sendCreatureSkull(p);
        
        p->sendCreatureShield(members[i]);
        p->sendCreatureSkull(members[i]);
    }
    leader->sendTextMessage(MSG_INFO_DESCR, info.str().c_str());
    leader->sendCreatureShield(p);
    leader->sendCreatureSkull(p);
    if(leader != p)
    {
        p->sendCreatureShield(leader);
        p->sendCreatureSkull(leader);
    }
    
    p->sendCreatureSkull(p);
    p->sendCreatureShield(p);
    p->sendTextMessage(MSG_INFO_DESCR, "You have left the party.");
    
    //If only one person remains, disband the party.
    if(members.size() == 0 && invitedplayers.size() == 0)
        disband();
}

void Party::disband()
{
    //std::cout << leader->getName() << "'s party is disbanded." << std::endl;
    //Temporary storage
    std::vector<Player*> tmp_members = members;
    std::vector<Player*> tmp_invited = invitedplayers;
    Player* tmp_leader = leader;
    
    //Clear the invitedplayers
    invitedplayers.clear();
    //Reset the leader
    leader->party = NULL;
    leader = NULL;
    
    //Clear the member vector
    while(!members.empty())
    {
        members.back()->party = NULL;
        members.pop_back();
    }
    
    //Display the shields
    for(unsigned int i = 0; i < tmp_members.size(); i++)
    {
        
        for(unsigned int j = 0; j < tmp_members.size(); j++)
        {
            tmp_members[i]->sendCreatureShield(tmp_members[j]);
            tmp_members[i]->sendCreatureSkull(tmp_members[j]);
        }
        tmp_members[i]->sendCreatureShield(tmp_leader);
        tmp_members[i]->sendCreatureSkull(tmp_leader);
        
        tmp_leader->sendCreatureShield(tmp_members[i]);
        tmp_leader->sendCreatureSkull(tmp_members[i]);
        
        tmp_members[i]->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
    }
    //Display the invited shields
    for(unsigned int i = 0; i < tmp_invited.size(); i++)
    {
        tmp_invited[i]->sendCreatureShield(tmp_leader);
        tmp_leader->sendCreatureShield(tmp_invited[i]);
    }
    //Display shield for leader
    tmp_leader->sendCreatureShield(tmp_leader);
    tmp_leader->sendCreatureSkull(tmp_leader);
    tmp_leader->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
    delete this;
}

bool Party::isInvited(Player* p)
{
    std::vector<Player*>::iterator invited = std::find(invitedplayers.begin(), invitedplayers.end(), p);
    if(invited == invitedplayers.end())
        return false;
    return true;
}
#endif
