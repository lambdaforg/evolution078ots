TRUE = 1
FALSE = 0

LUA_ERROR = -1
LUA_NO_ERROR = 0

NORTH = 0
EAST = 1
SOUTH = 2
WEST = 3
SOUTHWEST = 4
SOUTHEAST = 5
NORTHWEST = 6
NORTHEAST = 7

COMBAT_FORMULA_UNDEFINED = 0
COMBAT_FORMULA_LEVELMAGIC = 1
COMBAT_FORMULA_SKILL = 2

CONDITION_PARAM_OWNER = 1
CONDITION_PARAM_TICKS = 2
CONDITION_PARAM_OUTFIT = 3
CONDITION_PARAM_HEALTHGAIN = 4
CONDITION_PARAM_HEALTHTICKS = 5
CONDITION_PARAM_MANAGAIN = 6
CONDITION_PARAM_MANATICKS = 7
CONDITION_PARAM_DELAYED = 8
CONDITION_PARAM_SPEED = 9
CONDITION_PARAM_LIGHT_LEVEL = 10
CONDITION_PARAM_LIGHT_COLOR = 11
CONDITION_PARAM_SOULGAIN = 12
CONDITION_PARAM_SOULTICKS = 13
CONDITION_PARAM_MINVALUE = 14
CONDITION_PARAM_MAXVALUE = 15
CONDITION_PARAM_STARTVALUE = 16
CONDITION_PARAM_TICKINTERVAL = 17

COMBAT_PARAM_TYPE = 1
COMBAT_PARAM_EFFECT = 2
COMBAT_PARAM_DISTANCEEFFECT = 3
COMBAT_PARAM_BLOCKSHIELD = 4
COMBAT_PARAM_BLOCKARMOR = 5
COMBAT_PARAM_TARGETCASTERORTOPMOST = 6
COMBAT_PARAM_CREATEITEM = 7
COMBAT_PARAM_AGGRESSIVE = 8
COMBAT_PARAM_DISPEL = 9

CALLBACK_PARAM_LEVELMAGICVALUE = 1
CALLBACK_PARAM_SKILLVALUE = 2
CALLBACK_PARAM_TARGETTILE = 3
CALLBACK_PARAM_TARGETCREATURE = 4

COMBAT_NONE = 0
COMBAT_PHYSICALDAMAGE = 1
COMBAT_ENERGYDAMAGE = 2
COMBAT_POISONDAMAGE = 4 
COMBAT_FIREDAMAGE = 8
COMBAT_UNDEFINEDDAMAGE = 16
COMBAT_LIFEDRAIN = 32
COMBAT_MANADRAIN = 64
COMBAT_HEALING = 128
COMBAT_DROWNDAMAGE = 256

CONDITION_NONE = 0
CONDITION_POISON = 1
CONDITION_FIRE = 2
CONDITION_ENERGY = 4
CONDITION_LIFEDRAIN = 8
CONDITION_HASTE = 16
CONDITION_PARALYZE = 32
CONDITION_OUTFIT = 64
CONDITION_INVISIBLE = 128
CONDITION_LIGHT = 256
CONDITION_MANASHIELD = 512
CONDITION_INFIGHT = 1024
CONDITION_DRUNK = 2048
CONDITION_EXHAUSTED = 4096
CONDITION_FOOD = 8192
CONDITION_REGENERATION = 8192
CONDITION_SOUL = 16384
CONDITION_DROWN = 32768
CONDITION_MUTED = 65536

CONST_SLOT_HEAD = 1
CONST_SLOT_NECKLACE = 2
CONST_SLOT_BACKPACK = 3
CONST_SLOT_ARMOR = 4
CONST_SLOT_RIGHT = 5
CONST_SLOT_LEFT = 6
CONST_SLOT_LEGS = 7
CONST_SLOT_FEET = 8
CONST_SLOT_RING = 9
CONST_SLOT_AMMO = 10

CONST_ME_DRAWBLOOD = 0
CONST_ME_LOSEENERGY = 1
CONST_ME_POFF = 2
CONST_ME_BLOCKHIT = 3
CONST_ME_EXPLOSIONAREA = 4
CONST_ME_EXPLOSIONHIT = 5
CONST_ME_FIREAREA = 6
CONST_ME_YELLOW_RINGS = 7
CONST_ME_GREEN_RINGS = 8
CONST_ME_HITAREA = 9
CONST_ME_ENERGYAREA = 10
CONST_ME_ENERGYHIT = 11
CONST_ME_MAGIC_BLUE = 12
CONST_ME_MAGIC_RED = 13
CONST_ME_MAGIC_GREEN = 14
CONST_ME_HITBYFIRE = 15
CONST_ME_HITBYPOISON = 16
CONST_ME_MORTAREA = 17
CONST_ME_SOUND_BLUE = 18
CONST_ME_SOUND_RED = 19
CONST_ME_POISONAREA = 20
CONST_ME_SOUND_YELLOW = 21
CONST_ME_SOUND_PURPLE = 22
CONST_ME_SOUND_BLUE = 23
CONST_ME_SOUND_WHITE = 24
CONST_ME_BUBBLES = 25
CONST_ME_CRAPS = 26
CONST_ME_NONE = 255


CONST_ANI_SPEAR = 0
CONST_ANI_BOLT = 1
CONST_ANI_ARROW = 2
CONST_ANI_FIRE = 3
CONST_ANI_ENERGY = 4
CONST_ANI_POISONARROW = 5
CONST_ANI_BURSTARROW = 6
CONST_ANI_THROWINGSTAR = 7
CONST_ANI_THROWINGKNIFE = 8
CONST_ANI_SMALLSTONE = 9
CONST_ANI_SUDDENDEATH = 10
CONST_ANI_LARGEROCK = 11
CONST_ANI_SNOWBALL = 12
CONST_ANI_POWERBOLT = 13
CONST_ANI_POISON = 14
CONST_ANI_INFERNALBOLT = 15
CONST_ANI_NONE = 255

TALKTYPE_SAY  = 1
TALKTYPE_WHISPER = 2
TALKTYPE_YELL = 3
TALKTYPE_PRIVATE = 4
TALKTYPE_CHANNEL_Y = 5
TALKTYPE_BROADCAST = 9
TALKTYPE_CHANNEL_R1 = 10
TALKTYPE_PRIVATE_RED = 11
TALKTYPE_CHANNEL_O = 12
TALKTYPE_CHANNEL_R2 = 14
TALKTYPE_ORANGE_1 = 16
TALKTYPE_ORANGE_2 = 17

MESSAGE_STATUS_WARNING = 18
MESSAGE_EVENT_ADVANCE = 19
MESSAGE_EVENT_DEFAULT = 20
MESSAGE_STATUS_DEFAULT = 21
MESSAGE_INFO_DESCR = 22
MESSAGE_STATUS_SMALL = 23
MESSAGE_STATUS_CONSOLE_BLUE = 24
MESSAGE_STATUS_CONSOLE_RED  = 25

TEXTCOLOR_BLUE        = 5
TEXTCOLOR_LIGHTBLUE   = 35
TEXTCOLOR_LIGHTGREEN  = 30
TEXTCOLOR_LIGHTGREY   = 172
TEXTCOLOR_RED         = 180
TEXTCOLOR_ORANGE      = 198
TEXTCOLOR_WHITE_EXP   = 215
TEXTCOLOR_NONE        = 255

RETURNVALUE_NOERROR = 1
RETURNVALUE_NOTPOSSIBLE = 2
RETURNVALUE_NOTENOUGHROOM = 3
RETURNVALUE_PLAYERISPZLOCKED = 4
RETURNVALUE_PLAYERISNOTINVITED = 5
RETURNVALUE_CANNOTTHROW = 6
RETURNVALUE_THEREISNOWAY = 7
RETURNVALUE_DESTINATIONOUTOFREACH = 8
RETURNVALUE_CREATUREBLOCK = 9
RETURNVALUE_NOTMOVEABLE = 10
RETURNVALUE_DROPTWOHANDEDITEM = 11
RETURNVALUE_BOTHHANDSNEEDTOBEFREE = 12
RETURNVALUE_CANONLYUSEONEWEAPON = 13
RETURNVALUE_NEEDEXCHANGE = 14
RETURNVALUE_CANNOTBEDRESSED = 15
RETURNVALUE_PUTTHISOBJECTINYOURHAND = 16
RETURNVALUE_PUTTHISOBJECTINBOTHHANDS = 17
RETURNVALUE_TOOFARAWAY = 18
RETURNVALUE_FIRSTGODOWNSTAIRS = 19
RETURNVALUE_FIRSTGOUPSTAIRS = 20
RETURNVALUE_CONTAINERNOTENOUGHROOM = 21
RETURNVALUE_NOTENOUGHCAPACITY = 22
RETURNVALUE_CANNOTPICKUP = 23
RETURNVALUE_THISISIMPOSSIBLE = 24
RETURNVALUE_DEPOTISFULL = 25
RETURNVALUE_CREATUREDOESNOTEXIST = 26
RETURNVALUE_CANNOTUSETHISOBJECT = 27
RETURNVALUE_PLAYERWITHTHISNAMEISNOTONLINE = 28
RETURNVALUE_NOTREQUIREDLEVELTOUSERUNE = 29
RETURNVALUE_YOUAREALREADYTRADING = 30
RETURNVALUE_THISPLAYERISALREADYTRADING = 31
RETURNVALUE_YOUMAYNOTLOGOUTDURINGAFIGHT = 32
RETURNVALUE_DIRECTPLAYERSHOOT = 33
RETURNVALUE_NOTENOUGHLEVEL = 34
RETURNVALUE_NOTENOUGHMAGICLEVEL = 35
RETURNVALUE_NOTENOUGHMANA = 36
RETURNVALUE_NOTENOUGHSOUL = 37
RETURNVALUE_YOUAREEXHAUSTED = 38
RETURNVALUE_PLAYERISNOTREACHABLE = 39
RETURNVALUE_CANONLYUSETHISRUNEONCREATURES = 40
RETURNVALUE_ACTIONNOTPERMITTEDINPROTECTIONZONE = 41
RETURNVALUE_YOUMAYNOTATTACKTHISPLAYER = 42
RETURNVALUE_YOUMAYNOTATTACKAPERSONINPROTECTIONZONE = 43
RETURNVALUE_YOUMAYNOTATTACKAPERSONWHILEINPROTECTIONZONE = 44

ITEM_GOLD_COIN = 2148
ITEM_PLATINUM_COIN = 2152
ITEM_CRYSTAL_COIN = 2160


function doPlayerGiveItem(cid, itemid, count, charges)
	local hasCharges = (isItemRune(itemid) == TRUE or isItemFluidContainer(itemid) == TRUE)
	if(hasCharges and charges == nil) then
		charges = 1
	end
	
	while count > 0 do
    	local tempcount = 1
    	
    	if(hasCharges) then
    		tempcount = charges
    	end
    	if(isItemStackable(itemid) == TRUE) then
    		tempcount = math.min (100, count)
   		end
    	
       	local ret = doPlayerAddItem(cid, itemid, tempcount)
       	if(ret == LUA_ERROR) then
        	ret = doCreateItem(itemid, tempcount, getPlayerPosition(cid))
        end
        
        if(ret ~= LUA_ERROR) then
        	if(hasCharges) then
        		count = count-1
        	else
        		count = count-tempcount
        	end
        else
        	return LUA_ERROR
        end
	end
    return LUA_NO_ERROR
end


function doPlayerTakeItem(cid, itemid, count)
	if(getPlayerItemCount(cid,itemid) >= count) then
		
		while count > 0 do
			local tempcount = 0
    		if(isItemStackable(itemid) == TRUE) then
    			tempcount = math.min (100, count)
    		else
    			tempcount = 1
    		end
        	local ret = doPlayerRemoveItem(cid, itemid, tempcount)
        	
            if(ret ~= LUA_ERROR) then
            	count = count-tempcount
            else
            	return LUA_ERROR
            end
		end
		
		if(count == 0) then
			return LUA_NO_ERROR
		end
		
	else
		return LUA_ERROR
	end
end


function doPlayerAddMoney(cid, amount)
	local crystals = math.floor(amount/10000)
	amount = amount - crystals*10000
	local platinum = math.floor(amount/100)
	amount = amount - platinum*100
	local gold = amount
	local ret = 0
	if(crystals > 0) then
		ret = doPlayerGiveItem(cid, ITEM_CRYSTAL_COIN, crystals)
		if(ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	if(platinum > 0) then
		ret = doPlayerGiveItem(cid, ITEM_PLATINUM_COIN, platinum)
		if(ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	if(gold > 0) then
		ret = doPlayerGiveItem(cid, ITEM_GOLD_COIN, gold)
		if(ret ~= LUA_NO_ERROR) then
			return LUA_ERROR
		end
	end
	return LUA_NO_ERROR
end


function doPlayerBuyItem(cid, itemid, count, cost, charges)
    if(doPlayerRemoveMoney(cid, cost) == TRUE) then
    	return doPlayerGiveItem(cid, itemid, count, charges)
    else
        return LUA_ERROR
    end
end


function doPlayerSellItem(cid, itemid, count, cost)
	
	if(doPlayerTakeItem(cid, itemid, count) == LUA_NO_ERROR) then
		if(doPlayerAddMoney(cid, cost) ~= LUA_NO_ERROR) then
			error('Could not add money to ' .. getPlayerName(cid) .. '(' .. cost .. 'gp)')
		end
		return LUA_NO_ERROR
	else
		return LUA_ERROR
	end
	
end

DIRECTION_NORTH = 0
DIRECTION_EAST = 1
DIRECTION_SOUTH = 2
DIRECTION_WEST = 3
DIRECTION_SOUTHWEST = 4
DIRECTION_SOUTHEAST = 5
DIRECTION_NORTHWEST = 6
DIRECTION_NORTHEAST = 7

DOORSTATE_OPEN = 1
DOORSTATE_CLOSED = 2

DoorHandler = {
	doors = {
		north = {
			{ 1212, 1214 },
			{ 1213, 1214 },
			{ 1221, 1222 },
			{ 1225, 1226 },
			{ 1229, 1230 },
			{ 1234, 1236 },
			{ 1235, 1236 },
			{ 1239, 1240 },
			{ 1243, 1244 },
			{ 1247, 1248 },
			{ 1252, 1254 },
			{ 1253, 1254 },
			{ 1257, 1258 },
			{ 1261, 1262 },
			{ 3535, 3537 },
			{ 3536, 3537 },
			{ 3538, 3539 },
			{ 3540, 3541 },
			{ 3542, 3543 },
			{ 5084, 5085 },
			{ 5098, 5100 },
			{ 5099, 5100 },
			{ 5101, 5102 },
			{ 5103, 5104 },
			{ 5116, 5118 },
			{ 5117, 5118 },
			{ 5119, 5120 },
			{ 5121, 5122 },
			{ 5123, 5124 },
			{ 5134, 5136 },
			{ 5135, 5136 },
			{ 5137, 5139 },
			{ 5138, 5139 },
			{ 5278, 5280 },
			{ 5279, 5280 },
			{ 5286, 5287 },
			{ 5290, 5291 },
			{ 5294, 5295 },
			{ 5517, 5518 },
			{ 5748, 5749 }
		},
		
		west = {
			{ 1209, 1211 },
			{ 1210, 1211 },
			{ 1219, 1220 },
			{ 1223, 1224 },
			{ 1227, 1228 },
			{ 1231, 1233 },
			{ 1232, 1233 },
			{ 1237, 1238 },
			{ 1241, 1242 },
			{ 1245, 1246 },
			{ 1249, 1251 },
			{ 1250, 1251 },
			{ 1255, 1256 },
			{ 1259, 1260 },
			{ 3544, 3546 },
			{ 3545, 3546 },
			{ 3547, 3548 },
			{ 3549, 3550 },
			{ 3551, 3552 },
			{ 5082, 5083 },
			{ 5107, 5109 },
			{ 5108, 5109 },
			{ 5110, 5111 },
			{ 5112, 5113 },
			{ 5114, 5115 },
			{ 5125, 5127 },
			{ 5126, 5127 },
			{ 5128, 5129 },
			{ 5130, 5131 },
			{ 5132, 5133 },
			{ 5140, 5142 },
			{ 5141, 5142 },
			{ 5143, 5145 },
			{ 5144, 5145 },
			{ 5281, 5283 },
			{ 5282, 5283 },
			{ 5284, 5285 },
			{ 5288, 5289 },
			{ 5292, 5293 },
			{ 5515, 5516 },
			{ 5748, 5749 }
		}
	}
}

function DoorHandler:new(o)
	o = o or {}   -- create object if user does not provide one
	setmetatable(o, self)
	self.__index = self
	return o
end

function DoorHandler:searchArray(arr, itemid)
	local ret = nil
	local i = 1
	while true do
		if(arr[i] == nil) then
			break
		end
		
		local tmp = arr[i]
		
		if(tmp[1] == itemid or tmp[2] == itemid) then
			ret = tmp
			break
		end
		
		i = i+1
	end
	
	return ret
end

function DoorHandler:getDoor(item)
	
	local retDoor = {
		dir = 0,
		id = itemid,
		toid = 0,
		state = 0,
		minlvl = 0,
		keyid = 0
	}
	
	local ret = self:searchArray(self.doors.north, item.itemid)
	if(ret == nil) then
		ret = self:searchArray(self.doors.west, item.itemid)
		if(ret ~= nil) then
			retDoor.dir = DIRECTION_WEST
		end
	else
		retDoor.dir = DIRECTION_NORTH
	end
	
	if(ret == nil) then
		return nil
	end
	
	if(ret[1] == item.itemid) then
		retDoor.state = DOORSTATE_CLOSED
		retDoor.toid = ret[2]
	else
		retDoor.state = DOORSTATE_OPEN
		retDoor.toid = ret[1]
	end
	
	if(item.actionid >= 1000 and item.actionid < 2000) then
		retDoor.minlvl = item.actionid-1000
	elseif(item.actionid > 0) then
		retDoor.keyid = item.actionid
	end
	
	return retDoor
	
end

function DoorHandler:getCreatureFromPos(pos)
	local ret = 0
	
	local tmp = { x=pos.x, y=pos.y, z=pos.z, stackpos=253}
	local creature = getThingfromPos(tmp)
	
	if(creature ~= nil and creature ~= 0 and creature.itemid > 0) then
		ret = creature.uid
	end
	
	return ret
	
end

function DoorHandler:movePlayerTo(cid, toPos)
	
	local p = getPlayerPosition(cid)
	
	local dist = math.max(math.abs(p.x-toPos.x), math.abs(p.y-toPos.y))
	
	if(dist > 1 or p.z ~= toPos.z) then
		local ret = doTeleportThing(cid, toPos)
		return (ret == LUA_NO_ERROR)
	end
	
	if(dist <= 0) then
		return true
	end
	
	local dir = -1
	if(p.y < toPos.y) then --SouthXXX
		if(p.x < toPos.x) then -- SouthEast
			dir = DIRECTION_SOUTHEAST
		elseif(p.x > toPos.x) then -- SouthWest
			dir = DIRECTION_SOUTHWEST
		else --South
			dir = DIRECTION_SOUTH
		end
	elseif(p.y > toPos.y) then --NorthXXX
		if(p.x < toPos.x) then -- NorthEast
			dir = DIRECTION_NORTHEAST
		elseif(p.x > toPos.x) then -- NorthWest
			dir = DIRECTION_NORTHWEST
		else --North
			dir = DIRECTION_NORTH
		end
	else -- West/East
		if(p.x < toPos.x) then -- East
			dir = DIRECTION_EAST
		elseif(p.x > toPos.x) then -- West
			dir = DIRECTION_WEST
		else --None
			dir = -1
		end
	end
	
	if(dir == -1) then
		return false
	end
	
	local ret = doMoveCreature(cid, dir)
	
	
	return (ret == RETURNVALUE_NOERROR)
	
end

function DoorHandler:moveCreatures(thisDoor, doorpos)
	
	local lastcid = 0
	local cid = self:getCreatureFromPos(doorpos)
	
	while (cid ~= 0 and cid ~= lastcid) do
		lastcid = cid
		
		if(thisDoor.dir == DIRECTION_NORTH) then
			local ret = doMoveCreature(cid, DIRECTION_SOUTH)
			if(ret ~= RETURNVALUE_NOERROR) then
				ret = doMoveCreature(cid, DIRECTION_NORTH)
				if(ret ~= RETURNVALUE_NOERROR) then
					local destPos = { x = doorpos.x, y = doorpos.y+1, z = doorpos.z }
					ret = doTeleportThing(cid, destPos)
					if(ret ~= LUA_NO_ERROR) then
						destPos = { x = doorpos.x, y = doorpos.y-1, z = doorpos.z }
						doTeleportThing(cid, destPos)
					end
				end
			end
		elseif(thisDoor.dir == DIRECTION_WEST) then
			local ret = doMoveCreature(cid, DIRECTION_EAST)
			if(ret ~= RETURNVALUE_NOERROR) then
				ret = doMoveCreature(cid, DIRECTION_WEST)
				if(ret ~= RETURNVALUE_NOERROR) then
					local destPos = { x = doorpos.x+1, y = doorpos.y, z = doorpos.z }
					ret = doTeleportThing(cid, destPos)
					if(ret ~= LUA_NO_ERROR) then
						destPos = { x = doorpos.x-1, y = doorpos.y, z = doorpos.z }
						doTeleportThing(cid, destPos)
					end
				end
			end
		end
		
		cid = self:getCreatureFromPos(doorpos)
	end
	
end

function DoorHandler:useDoor(item, pos, userid, key, playerGenerated)
	local thisDoor = self:getDoor(item)
	
	if(thisDoor == nil) then
		return 0
	end
	
	
	if(key ~= nil and key.actionid ~= thisDoor.keyid) then
		doPlayerSendTextMessage(userid,22,"The key does not match.")
		return 1
	end
	
	
	if(thisDoor.keyid > 0 and key == nil) then
		doPlayerSendTextMessage(userid,22,"The door is locked.")
		return 1
	end
	
	if(thisDoor.state == DOORSTATE_OPEN and thisDoor.minlvl == 0) then
		self:moveCreatures(thisDoor, pos)
		doTransformItem(item.uid, thisDoor.toid)
	elseif(thisDoor.state == DOORSTATE_CLOSED) then
		if(thisDoor.minlvl > 0) then
			local lvl = getPlayerLevel(userid)
			if(lvl < thisDoor.minlvl) then
				doPlayerSendTextMessage(userid,22,"You need level " .. thisDoor.minlvl .. " to pass this door.")
				return 1
			else
				doTransformItem(item.uid, thisDoor.toid)
				self:movePlayerTo(userid, pos)
			end
		else
			doTransformItem(item.uid, thisDoor.toid)
		end
	elseif(thisDoor.state == DOORSTATE_OPEN and thisDoor.minlvl > 0 and not playerGenerated) then
		doTransformItem(item.uid, thisDoor.toid)
	end
	
	return 1
end

if(doorHandler == nil) then
	doorHandler = DoorHandler:new(nil)
end

function isPlayerOnPos(pos)
	local pos2 = pos
	pos2.stackpos = 253
	local thing = getThingfromPos(pos2)
	if(thing ~= nil and thing.itemid > 0) then
		if(isPlayer(thing.uid)) then
			return true
		end
	end
	return false
end

function comparePos(pos1, pos2)
	return (pos1.x == pos2.x and pos1.y == pos2.y and pos1.z == pos2.z)
end
