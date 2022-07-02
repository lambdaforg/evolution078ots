local combat = createCombatObject()

local fieldItems = {1487, 1488, 1489, 1490, 1491, 1492, 1493, 1494, 1495, 1496, 1500, 1501, 1502, 1503, 1504, 5061, 5062, 5063, 5064, 5065, 5066, 5067}

function onTargetTile(cid, pos)
	local posEx = {x=pos.x, y=pos.y, z=pos.z, stackpos=254}
	item = getThingfromPos(posEx)

	if item.itemid > 0 then
		if isInArray(fieldItems, item.itemid) == 1 then
			doRemoveItem(item.uid,1)
		end
	end

	doSendMagicEffect(pos,2)
end

setCombatCallback(combat, CALLBACK_PARAM_TARGETTILE, "onTargetTile")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
