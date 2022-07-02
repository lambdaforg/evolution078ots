local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_SOUND_RED)

local arr = {
{1, 1, 1},
{0, 1, 0},
{0, 3, 0},
}

local area = createCombatArea(arr)

setCombatArea(combat, area)

local condition = createConditionObject(CONDITION_DRUNK)
setConditionParam(condition, CONDITION_PARAM_TICKS, 20000)
setCombatCondition(combat, condition)

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end