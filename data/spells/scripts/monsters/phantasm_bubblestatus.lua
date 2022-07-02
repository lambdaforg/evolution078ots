local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICAL)
setCombatParam(combat, COMBAT_PARAM_EFFECT, 12)
setCombatFormula(combat, COMBAT_FORMULA_LEVELMAGIC, 0, 0, 0, 0)

local arr = {
{0, 0, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 3, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 0, 0}
}

local area = createCombatArea(arr)
setCombatArea(combat, area)

local condition = createConditionObject(CONDITION_ENERGY)
setConditionParam(condition, CONDITION_PARAM_DELAYED, 2)
addDamageCondition(condition, 1, 3000, -0)
addDamageCondition(condition, 10, 3000, -20)

setCombatCondition(combat, condition)

setCombatCondition(combat, condition)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end
