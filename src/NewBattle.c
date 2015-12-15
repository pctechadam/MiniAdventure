#include <pebble.h>
#include "BattleActor.h"
#include "Character.h"
#include "DescriptionFrame.h"
#include "GlobalState.h"
#include "Logging.h"
#include "NewBattle.h"
#include "MainImage.h"
#include "Menu.h"
#include "Monsters.h"
#include "NewMenu.h"
#include "Skills.h"
#include "Story.h"
#include "BattleQueue.h"
#include "UILayers.h"

typedef struct NewBattleState
{
	uint16_t currentMonsterHealth;
	BattleActor *player;
	BattleActor *monster;
} NewBattleState;

static NewBattleState gBattleState = {0};
static MonsterDef *currentMonster = NULL;

bool gUpdateNewBattle = false;
bool gPlayerTurn = false;
int gSkillDelay = 0;
const char *gEffectDescription = NULL;

void CloseNewBattleWindow(void)
{
	INFO_LOG("Ending battle.");
	ResetBattleQueue();
	PopGlobalState();
}

BattleActor *GetPlayerActor(void)
{
	return gBattleState.player;
}

BattleActor *GetMonsterActor(void)
{
	return gBattleState.monster;
}

void PlayerPushFastAttack(void)
{
	SkillInstance *newInstance = CreateSkillInstance(GetFastAttack(), GetPlayerActor(), GetMonsterActor());
	BattleQueuePush(SKILL, newInstance);
	BattleQueuePush(ACTOR, GetPlayerActor());
	gPlayerTurn = false;
}

void PlayerPushSlowAttack(void)
{
	SkillInstance *newInstance = CreateSkillInstance(GetSlowAttack(), GetPlayerActor(), GetMonsterActor());
	BattleQueuePush(SKILL, newInstance);
	BattleQueuePush(ACTOR, GetPlayerActor());
	gPlayerTurn = false;
}

static uint16_t BattleScreenCount(void)
{
	DEBUG_LOG("BattleScreenCount");
	if(!gPlayerTurn)
		return 0;
	
	DEBUG_LOG("Returning good value");
	return 2;
}

static const char *BattleScreenNameCallback(int row)
{
	if(!gPlayerTurn)
		return NULL;

	switch(row)
	{
		case 0:
		{
			return "Fast Attack";
			break;
		}
		case 1:
		{
			return "Slow Attack";
			break;
		}
	}
	return "";
}

static const char *BattleScreenDescriptionCallback(int row)
{
	if(!gPlayerTurn)
		return NULL;

	switch(row)
	{
		case 0:
		{
			return "Quick stab";
			break;
		}
		case 1:
		{
			return "Heavy slash";
			break;
		}
	}
	return "";
}

static void BattleScreenSelectCallback(int row)
{
	if(!gPlayerTurn)
		return;

	switch(row)
	{
		case 0:
		{
			PlayerPushFastAttack();
			break;
		}
		case 1:
		{
			PlayerPushSlowAttack();
			break;
		}
	}
}

const char  *UpdateNewMonsterHealthText(void)
{
	static char monsterHealthText[] = "00000"; // Needs to be static because it's used by the system later.

	IntToString(monsterHealthText, 5, BattleActor_GetHealth(gBattleState.monster));
	return monsterHealthText;
}

void BattleScreenAppear(void)
{
	SetDescription(currentMonster->name);
	RegisterMenuCellCallbacks(BattleScreenCount, BattleScreenNameCallback, BattleScreenDescriptionCallback, BattleScreenSelectCallback);
	ReloadMenu();
}

int NewComputeMonsterHealth(int level)
{
	int baseHealth = 20 + ((level-1)*(level)/2) + ((level-1)*(level)*(level+1)/(6*2));
	return ScaleMonsterHealth(currentMonster, baseHealth);
}

void NewBattleInit(void)
{
	currentMonster = NULL;

	if(!currentMonster)
	{
		currentMonster = GetRandomMonster();
		gBattleState.currentMonsterHealth = NewComputeMonsterHealth(GetCurrentBaseLevel());
	}
	CharacterData *character = GetCharacter();
	gBattleState.player = InitBattleActor(true, character->level, character->speed, character->stats.maxHealth);
	gBattleState.monster = InitBattleActor(false, GetCurrentBaseLevel(), currentMonster->speed, gBattleState.currentMonsterHealth);
	
	BattleQueuePush(ACTOR, gBattleState.player);
	BattleQueuePush(ACTOR, gBattleState.monster);

	RegisterMenuCellCallbacks(BattleScreenCount, BattleScreenNameCallback, BattleScreenDescriptionCallback, BattleScreenSelectCallback);

	SetForegroundImage(currentMonster->imageId);
#if defined(PBL_COLOR)
	SetBackgroundImage(RESOURCE_ID_IMAGE_BATTLE_FLOOR);
#endif
	SetMainImageVisibility(true, true, true);
}

void UpdateNewBattle(void)
{
	bool entryRemoved = false;
	void *data = NULL;
	BattleQueueEntryType type = ACTOR;
	
	if(gSkillDelay > 0)
	{
		--gSkillDelay;
		if(gSkillDelay == 0)
			SetDescription(currentMonster->name);
		return;
	}
	
	if(gPlayerTurn)
		return;
	
	if(BattleActor_GetHealth(gBattleState.player) <= 0 || BattleActor_GetHealth(gBattleState.monster) <= 0)
	{
		CloseNewBattleWindow();
		return;
	}
	
	entryRemoved = UpdateBattleQueue(&type, &data);
	
	if(entryRemoved)
	{
		switch(type)
		{
			case SKILL:
			{
				SkillInstance *instance = (SkillInstance*)data;
				gEffectDescription = ExecuteSkill(instance);
				SetDescription(gEffectDescription);
				gSkillDelay = SKILL_DELAY;
				break;
			}
			case ACTOR:
			{
				BattleActor *actor = (BattleActor*)data;
				if(BattleActor_IsPlayer(actor))
				{
					gPlayerTurn = true;
					ReloadMenu();
				}
				else
				{
					// Fake monster AI
					SkillInstance *newInstance = CreateSkillInstance(GetFastAttack(), GetMonsterActor(), GetPlayerActor());
					BattleQueuePush(SKILL, newInstance);
					BattleQueuePush(ACTOR, GetMonsterActor());
				}
				break;
			}
		}
	}
}

void BattleScreenPush(void)
{
	NewBattleInit();
}

void TriggerBattleScreen(void)
{
	if(CurrentLocationAllowsCombat())
		PushGlobalState(BATTLE, SECOND_UNIT, UpdateNewBattle, BattleScreenPush, BattleScreenAppear, NULL, NULL);
}
