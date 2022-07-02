local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_RED)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_MANADRAIN)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -0.5, -30, -1.0, 0)

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
