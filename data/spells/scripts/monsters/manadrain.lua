local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_MANADRAIN)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_NONE)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
