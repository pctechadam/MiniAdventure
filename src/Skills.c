#include <pebble.h>
#include "AutoSkillConstants.h"
#include "CombatantClass.h"
#include "Logging.h"
#include "MiniAdventure.h"
#include "ResourceStory.h"
#include "Skills.h"

char *GetSkillName(Skill *skill)
{
    if(!skill)
        return NULL;
    
    return skill->name;
}

char *GetSkillDescription(Skill *skill)
{
    if(!skill)
        return NULL;
    
    return skill->description;
}

uint16_t GetSkillSpeed(Skill *skill)
{
    return skill->speed;
}

int GetSkillCooldown(Skill *skill)
{
    return skill->cooldown;
}

static int ComputeSkillPotency(Skill *skill, BattleActorWrapper *attacker, BattleActorWrapper *defender)
{
    int attackPower = 1;
    int defensePower = 1;
    
    // Compute immunity first. If the defender is immune, return 0
    if(defender && skill->damageType & defender->battlerWrapper->battler.immune)
        return 0;
    
    if(skill->damageType & PHYSICAL)
    {
        attackPower = CombatantClass_GetStrength(&attacker->battlerWrapper->battler.combatantClass, attacker->actor.level);
        if(defender)
            defensePower = CombatantClass_GetDefense(&defender->battlerWrapper->battler.combatantClass, defender->actor.level);
    }
    else if(skill->damageType & MAGIC)
    {
        attackPower = CombatantClass_GetMagic(&attacker->battlerWrapper->battler.combatantClass, attacker->actor.level);
        if(defender)
            defensePower = CombatantClass_GetMagicDefense(&defender->battlerWrapper->battler.combatantClass, defender->actor.level);
    }
    
    if(defensePower == 0)
        defensePower = 1;
    
    int baseDamage = (skill->potency + (attackPower * skill->potency)) / 10;
    
    int damage = (baseDamage * (100 - defensePower)) / 100;
    
    if(damage <= 0 && skill->potency > 0)
        damage = 1;
    
    if(defender && skill->damageType & defender->battlerWrapper->battler.vulnerable)
        damage *= 2;

    if(defender && skill->damageType & defender->battlerWrapper->battler.resistant)
        damage /= 2;
    
    if(defender && skill->damageType & defender->battlerWrapper->battler.absorb)
        damage = -damage;
    
    return damage;
}

void DealDamage(int potency, BattleActor *defender)
{
    if(!defender)
        return;
    if(potency > 0 && defender->currentHealth < potency)
        defender->currentHealth = 0;
    else
        defender->currentHealth -= potency;
    if(defender->currentHealth > defender->maxHealth)
        defender->currentHealth = defender->maxHealth;
}

void ApplyStatus(Skill *skill, BattleActorWrapper *target)
{
    if(skill->skillProperties > 0)
    {
        for(int i = 0; i < MAX_STATUS_EFFECTS; ++i)
        {
            uint16_t bit = 1 << i;
            if(skill->skillProperties & bit && (!(target->battlerWrapper->battler.statusImmunities & bit)))
            {
                target->actor.statusEffectDurations[i] = skill->propertyDuration;
                if(i == STATUS_EFFECT_STUN)
                {
                    target->actor.skillQueued = false;
                    target->actor.currentTime = 0;
                }
            }
        }
    }
}

const char *ExecuteSkill(Skill *skill, BattleActorWrapper *attacker, BattleActorWrapper *defender)
{
    static char description[30];
    DEBUG_VERBOSE_LOG("ExecuteSkill");
    
    switch(skill->target)
    {
        case SKILL_TARGET_ENEMY:
        {
            bool countered = false;
            if(defender->actor.counterSkill != INVALID_SKILL)
            {
                Skill *counterSkill = BattlerWrapper_GetSkillByIndex(defender->battlerWrapper, defender->actor.counterSkill);
                if(counterSkill->counterDamageType & skill->damageType)
                {
                    int potency = ComputeSkillPotency(counterSkill, defender, attacker);
                    DealDamage(potency, &attacker->actor);
                    snprintf(description, sizeof(description), "%s counters for %d damage", defender->battlerWrapper->battler.name, potency);
                    countered = true;
                    ApplyStatus(counterSkill, attacker);
                }
                defender->actor.counterSkill = INVALID_SKILL;
            }
            
            if(!countered)
            {
                int potency = ComputeSkillPotency(skill, attacker, defender);
                DealDamage(potency, &defender->actor);
                snprintf(description, sizeof(description), "%s takes %d damage", defender->battlerWrapper->battler.name, potency);
                ApplyStatus(skill, defender);
            }
            break;
        }
        case SKILL_TARGET_COUNTER:
        {
            attacker->actor.counterSkill = attacker->actor.activeSkill;
            snprintf(description, sizeof(description), "%s prepares %s", attacker->battlerWrapper->battler.name, skill->name);
            break;
        }
        case SKILL_TARGET_SELF:
        {
            int potency = ComputeSkillPotency(skill, attacker, NULL);
            DealDamage(-potency, &attacker->actor);
            ApplyStatus(skill, attacker);
            snprintf(description, sizeof(description), "%s heals %d damage", attacker->battlerWrapper->battler.name, potency);
            break;
        }
        default:
        {
            snprintf(description, sizeof(description), "Unhandled skill: %s", skill->name);
            break;
        }
    }
    DEBUG_VERBOSE_LOG("Setting description: %s", description);
    attacker->actor.skillCooldowns[attacker->actor.activeSkill] = GetSkillCooldown(skill);
    attacker->actor.activeSkill = INVALID_SKILL;
    return description;
}

void UpdateSkillCooldowns(uint16_t *skillCooldowns)
{
    for(int i = 0; i < MAX_SKILLS_IN_LIST; ++i)
    {
        if(skillCooldowns[i] > 0)
            skillCooldowns[i]--;
    }
}
