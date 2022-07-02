local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_BLOCKARMOR, 1)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_SPEAR)
setCombatFormula(combat, COMBAT_FORMULA_SKILL, 0, 0, 0, 0)

function onUseWeapon(cid, var)
	return doCombat(cid, combat, var)
end
