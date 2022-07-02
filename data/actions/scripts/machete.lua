function onUse(cid, item, frompos, item2, topos)
	if item2.itemid == 2782 then
		doTransformItem(item2.uid,2737)
		doDecayItem(item2.uid)
	elseif item2.itemid == 3985 then
		doTransformItem(item2.uid,5463)
		doDecayItem(item2.uid)
	else 
		return 0
	end

	return 1
end