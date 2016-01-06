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
    if(skill->damageType & PHYSICAL)
    {
        attackPower = CombatantClass_GetStrength(BattlerWrapper_GetCombatantClass(attacker->battlerWrapper), attacker->actor.level);
        if(defender)
            defensePower = CombatantClass_GetDefense(BattlerWrapper_GetCombatantClass(defender->battlerWrapper), defender->actor.level);
    }
    else if(skill->damageType & MAGIC)
    {
        attackPower = CombatantClass_GetMagic(BattlerWrapper_GetCombatantClass(attacker->battlerWrapper), attacker->actor.level);
        if(defender)
            defensePower = CombatantClass_GetMagicDefense(BattlerWrapper_GetCombatantClass(defender->battlerWrapper), defender->actor.level);
    }
    
    if(defensePower == 0)
        defensePower = 1;
    
    int potency = skill->potency * attackPower / defensePower;
    
    if(potency <= 0)
        potency = 1;

    // TODO: Should also deal with damage types and resistances
    return potency;
}

static void DealDamage(int potency, BattleActor *defender)
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

const char *ExecuteSkill(Skill *skill, BattleActorWrapper *attacker, BattleActorWrapper *defender)
{
    static char description[30];
    DEBUG_VERBOSE_LOG("ExecuteSkill");
    
    switch(skill->type)
    {
        case SKILL_TYPE_ATTACK:
        {
            if(defender->actor.counterSkill != INVALID_SKILL)
            {
                //TODO: Add an attack type to counters, so that we can differentiate between counters to magic attacks and counters to physical attacks
                Skill *counterSkill = BattlerWrapper_GetSkillByIndex(defender->battlerWrapper, defender->actor.counterSkill);
                int potency = ComputeSkillPotency(counterSkill, defender, attacker);
                DealDamage(potency, &attacker->actor);
                snprintf(description, sizeof(description), "%s counters for %d damage", BattlerWrapper_GetName(defender->battlerWrapper), potency);

                defender->actor.counterSkill = INVALID_SKILL;
            }
            else
            {
                int potency = ComputeSkillPotency(skill, attacker, defender);
                DealDamage(potency, &defender->actor);
                snprintf(description, sizeof(description), "%s takes %d damage", BattlerWrapper_GetName(defender->battlerWrapper), potency);
            }
            break;
        }
        case SKILL_TYPE_COUNTER:
        {
            attacker->actor.counterSkill = attacker->actor.activeSkill;
            snprintf(description, sizeof(description), "%s prepares %s", BattlerWrapper_GetName(attacker->battlerWrapper), skill->name);
            break;
        }
        case SKILL_TYPE_HEAL:
        {
            int potency = ComputeSkillPotency(skill, attacker, NULL);
            DealDamage(-potency, &attacker->actor);
            snprintf(description, sizeof(description), "%s heals %d damage", BattlerWrapper_GetName(attacker->battlerWrapper), potency);
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
