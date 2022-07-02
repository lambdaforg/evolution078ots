local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_POISONDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_POISONAREA)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, 0, -90, 0, -300)

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
