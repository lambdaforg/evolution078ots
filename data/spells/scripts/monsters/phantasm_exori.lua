local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_YELLOW_RINGS)
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_POISONDAMAGE)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, -2.3, -184, -3, -240)
local condition = createConditionObject(CONDITION_POISON)
setConditionParam(condition, CONDITION_PARAM_DELAYED, 1)
addDamageCondition(condition, 1, 3000, -0)
addDamageCondition(condition, 2, 3000, -26)
addDamageCondition(condition, 3, 3000, -25)
addDamageCondition(condition, 3, 3000, -24)
addDamageCondition(condition, 4, 3000, -23)
addDamageCondition(condition, 4, 3000, -22)
addDamageCondition(condition, 6, 3000, -21)
addDamageCondition(condition, 6, 3000, -20)
addDamageCondition(condition, 8, 3000, -19)
addDamageCondition(condition, 10, 3000, -18)
addDamageCondition(condition, 14, 3000, -17)
addDamageCondition(condition, 17, 3000, -16)
addDamageCondition(condition, 19, 3000, -15)
addDamageCondition(condition, 21, 3000, -14)
addDamageCondition(condition, 24, 3000, -13)
addDamageCondition(condition, 28, 3000, -12)
addDamageCondition(condition, 32, 3000, -11)
addDamageCondition(condition, 38, 3000, -10)
addDamageCondition(condition, 42, 3000, -9)
addDamageCondition(condition, 47, 3000, -8)
addDamageCondition(condition, 51, 3000, -7)
addDamageCondition(condition, 55, 3000, -6)
addDamageCondition(condition, 59, 3000, -5)
addDamageCondition(condition, 61, 3000, -4)
addDamageCondition(condition, 64, 3000, -3)
addDamageCondition(condition, 77, 3000, -2)
addDamageCondition(condition, 80, 3000, -1)
setCombatCondition(combat, condition)

local arr = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 3, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
}

local area = createCombatArea(arr)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
