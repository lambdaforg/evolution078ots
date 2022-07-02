local text = ""
local nutrition = 0
local effect = 50

function onUse(cid, item, frompos, item2, topos)
	if item.itemid == 2684 or item.itemid == 2362 or item.itemid == 2691 then
		nutrition = 8
		text = "Crunch."
	elseif item.itemid == 2695 then
		nutrition = 6
		text = "Gulp."
	elseif item.itemid == 2674 then
		nutrition = 6
		text = "Yum."
	elseif item.itemid == 2787 then
		nutrition = 9
		text = "Munch."
	elseif item.itemid == 2690 then
		nutrition = 3
		text = "Crunch."
	elseif item.itemid == 2666 then
		nutrition = 15
		text = "Munch."
	elseif item.itemid == 2667 then
		nutrition = 12
		text = "Munch."
	elseif item.itemid == 2668 then
		nutrition = 10
		text = "Mmmm."
	elseif item.itemid == 2689 then
		nutrition = 10
		text = "Crunch."
	elseif item.itemid == 2669 then
		nutrition = 17
		text = "Munch."
	elseif item.itemid == 2670 then
		nutrition = 4
		text = "Gulp."
	elseif item.itemid == 2671 then
		nutrition = 30
		text = "Chomp."
	elseif item.itemid == 2672 then
		nutrition = 60
		text = "Chomp."
	elseif item.itemid == 2673 then
		nutrition = 5
		text = "Yum."
	elseif item.itemid == 2675 then
		nutrition = 13
		text = "Yum."
	elseif item.itemid == 2677 or item.itemid == 2679 then
		nutrition = 1
		text = "Yum."
	elseif item.itemid == 2678 then
		nutrition = 18
		text = "Slurp."
	elseif item.itemid == 2680 then
		nutrition = 2
		text = "Yum."
	elseif item.itemid == 2687 then
		nutrition = 2
		text = "Crunch."
	elseif item.itemid == 2681 then
		nutrition = 9
		text = "Yum."
	elseif item.itemid == 2686 then
		nutrition = 9
		text = "Crunch."
	elseif item.itemid == 2696 then
		nutrition = 9
		text = "Smack."
	elseif item.itemid == 2682 then
		nutrition = 20
		text = "Yum."
	elseif item.itemid == 2683 then
		nutrition = 17
		text = "Munch."
	elseif item.itemid == 2685 then
		nutrition = 6
		text = "Munch."
	elseif item.itemid == 2688 or item.itemid == 2793 then
		nutrition = 9
		text = "Munch."
	elseif item.itemid == 2788 then
		nutrition = 4
		text = "Munch."
	elseif item.itemid == 2789 then
		nutrition = 22
		text = "Munch."
	elseif item.itemid == 2790 or item.itemid == 2791 then
		nutrition = 30
		text = "Munch."
	elseif item.itemid == 2792 then
		nutrition = 6
		text = "Munch."
	elseif item.itemid == 2794 then
		nutrition = 3
		text = "Munch."
	elseif item.itemid == 2795 then
		nutrition = 36
		text = "Munch."
	elseif item.itemid == 2796 then
		nutrition = 5
		text = "Munch."
	elseif item.itemid == 6574 then
		nutrition = 4
		text = "Mmmm."
	elseif item.itemid == 6394 then
		nutrition = 4
		text = "Mmmm."
	elseif item.itemid == 6569 then
		nutrition = 1
		text = "Mmmm."
		effect = 27
	end

	if (getPlayerFood(cid) + nutrition > 400) then
		doPlayerSendCancel(cid,"You are full.")
	else
		doPlayerFeed(cid, nutrition * 4)
		doPlayerSay(cid, text, 16)
		doRemoveItem(item.uid, 1)
		if effect < 31 then
			doSendMagicEffect(getPlayerPosition(cid), 27)
		end
	end
	return 1
end
