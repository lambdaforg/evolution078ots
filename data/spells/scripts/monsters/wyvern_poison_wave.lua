local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_POISONDAMAGE)
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_POISONAREA)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, 0, 0, 0, 0)

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

local condition = createConditionObject(CONDITION_POISON)
setConditionParam(condition, CONDITION_PARAM_DELAYED, 1)
addDamageCondition(condition, 1, 3000, -0)
addDamageCondition(condition, 1, 3000, -17)
addDamageCondition(condition, 1, 3000, -16)
addDamageCondition(condition, 1, 3000, -15)
addDamageCondition(condition, 1, 3000, -14)
addDamageCondition(condition, 2, 3000, -13)
addDamageCondition(condition, 2, 3000, -12)
addDamageCondition(condition, 2, 3000, -11)
addDamageCondition(condition, 2, 3000, -10)
addDamageCondition(condition, 3, 3000, -9)
addDamageCondition(condition, 3, 3000, -8)
addDamageCondition(condition, 3, 3000, -7)
addDamageCondition(condition, 4, 3000, -6)
addDamageCondition(condition, 4, 3000, -5)
addDamageCondition(condition, 5, 3000, -4)
addDamageCondition(condition, 5, 3000, -3)
addDamageCondition(condition, 6, 3000, -2)
addDamageCondition(condition, 10, 3000, -1)
setCombatCondition(combat, condition)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end