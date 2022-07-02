#ifdef __JD_BED_SYSTEM__

#ifndef __BEDS_H__
#define __BEDS_H__


class Item;
#include "item.h"
#include "house.h"
#include "player.h"
#include "tile.h"

typedef std::list<Bed*> BedList;

class Bed : public Item {
public:
	
	Bed(uint16_t _type);
	virtual ~Bed();
	
	virtual Bed* getBed() {return this;};
	virtual const Bed* getBed() const {return this;};
	
	bool canUse(const Player* player);
	void useBed(Player *player);
	
	void startSleep() { startsleep = std::time(NULL); }
	void setStartSleep(uint64_t newStart) { startsleep = newStart; }
	uint64_t getStartSleep() const { return startsleep; }
	
	uint32_t getOccupiedId(const Item* item = NULL) const;
	uint32_t getEmptyId(const Item* item = NULL) const;
	Item* getSecondPart() const;
	
	uint32_t getGainedHealth(const Player* player, uint32_t &usedFood) const;
	uint32_t getGainedMana(const Player* player, uint32_t &usedFood) const;
	
	void setSleeper(const std::string& name);
	std::string getSleeper() { return sleeper; };
	
	void wakeUp(Player* player);
	
	//overrides
	virtual bool canRemove() const {return false;}
	
private:
	std::string sleeper;
	uint64_t startsleep;
};

class Beds {
public:
	Beds();
	virtual ~Beds() {};
	
	bool loadBeds();
	bool saveBeds();
	
	void addBed(Bed* newBed) {
		beds.push_back(newBed);
	};
	
	BedList* getBedList(){ return &beds; };
	
	Bed* getBedFromPosition(const Position &pos);
	Bed* getBedBySleeper(const std::string& sleeper);
	
private:
	BedList beds;
};

class IOBeds {
protected:
	IOBeds() {};
	virtual ~IOBeds() {};
	
public:
	
	static IOBeds* instance();
	
	virtual bool loadBeds(const std::string &identifier) {};
	virtual bool saveBeds(const std::string &identifier) {};
	
protected:
	static IOBeds* _instance;
};

class IOBedsXML : public IOBeds {
public:
	IOBedsXML();
	virtual ~IOBedsXML() {};
	
	virtual bool loadBeds(const std::string &identifier);
	virtual bool saveBeds(const std::string &identifier);
};

#endif

#endif
