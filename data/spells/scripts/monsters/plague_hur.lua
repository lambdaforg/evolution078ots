local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_POISONDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_YELLOW_RINGS)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, 0, -40, 0, -120)


local arr = {
{1, 1, 1},
{1, 1, 1},
{0, 1, 0},
{0, 1, 0},
{0, 3, 0},
}

local condition = createConditionObject(CONDITION_DRUNK)
setConditionParam(condition, CONDITION_PARAM_TICKS, 20000)
setCombatCondition(combat, condition)

local area = createCombatArea(arr)

setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end