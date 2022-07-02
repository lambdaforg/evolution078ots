local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_FIRE)
setCombatParam(combat, COMBAT_PARAM_CREATEITEM, 1487)

local area = createCombatArea( {
 {0, 0, 1, 0, 0},
 {0, 1, 1, 1, 0},
 {1, 1, 3, 1, 1},
 {0, 1, 1, 1, 0},
 {0, 0, 1, 0, 0}
 } )
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
