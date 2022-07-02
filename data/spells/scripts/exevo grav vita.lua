area = {
{0, 1, 0},
{2, 0, 3},
{0, 4, 0}
}

    attackType = ATTACK_NONE
    needDirection = true
    areaEffect = NM_ME_MAGIC_ENERGIE
    animationEffect = NM_ANI_NONE

    hitEffect = NM_ME_NONE
    damageEffect = NM_ME_MAGIC_ENERGIE
    animationColor = GREEN
    offensive = false
    drawblood = false
minDmg = 0
maxDmg = 0

WildGrowthObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)
SubWildGrowthObject1 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, minDmg, maxDmg)
SubWildGrowthObject2 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

return doAreaGroundMagic(cid, centerpos, needDirection, areaEffect, area, WildGrowthObject:ordered(),
	0, 1, SubWildGrowthObject1:ordered(),
	5000, 5, SubWildGrowthObject2:ordered(),
	2, 60000, 1499, 1)

end