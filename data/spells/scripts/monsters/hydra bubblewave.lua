local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, 1)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, 0, -40, 0, -120)

local arr = {
{1, 1, 1, 1, 1, 1 ,1},
{1, 1, 1, 1, 1, 1 ,1},
{1, 1, 1, 1, 1, 1 ,1},
{0, 1, 1, 1, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 0, 0},
{0, 0, 0, 1, 0, 0, 0},
{0, 0, 0, 3, 0, 0, 0},
}

local area = createCombatArea(arr)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	doCombat(cid, combat, var)
end
