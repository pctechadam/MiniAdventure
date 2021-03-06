#include <pebble.h>

#include "Character.h"
#include "GlobalState.h"
#include "Logging.h"
#include "Battle.h"
#include "OptionsMenu.h"
#include "Persistence.h"
#include "Story.h"
#include "TitleScreen.h"
#include "WorkerControl.h"

void ClearStoryPersistedData(uint16_t storyId)
{
    int offset = ComputeStoryPersistedDataOffset(storyId);
    if(persist_exists(PERSISTED_STORY_IS_DATA_SAVED + offset))
    {
        DEBUG_LOG("Clearing game persisted data.");
        int maxKey = persist_read_int(PERSISTED_STORY_MAX_KEY_USED + offset);
        int i;
        for(i = offset; i < maxKey; ++i)
        {
            persist_delete(i);
        }
    }
}

void ClearCurrentStoryPersistedData(void)
{
    ClearStoryPersistedData(Story_GetCurrentStoryId());
}

void ClearGlobalPersistedData(void)
{
	if(persist_exists(PERSISTED_IS_DATA_SAVED))
	{
        INFO_LOG("Clearing global persisted data");
        if(persist_exists(PERSISTED_STORY_LIST_SIZE) && persist_exists(PERSISTED_STORY_LIST))
        {
            uint16_t oldcount = 0;
            uint16_t *oldbuffer = NULL;
            oldcount = persist_read_int(PERSISTED_STORY_LIST_SIZE);
            oldbuffer = calloc(sizeof(uint16_t), oldcount);
            persist_read_data(PERSISTED_STORY_LIST, oldbuffer, oldcount * sizeof(uint16_t));
            
            for(int i = 0; i < oldcount; ++i)
            {
                INFO_LOG("Deleting data for missing story: %d", oldbuffer[i]);
                ClearStoryPersistedData(oldbuffer[i]);
            }
            free(oldbuffer);
        }
        if(persist_exists(PERSISTED_MAX_KEY_USED))
        {
            int maxKey = persist_read_int(PERSISTED_MAX_KEY_USED);
            int i;
            for(i = 0; i <= maxKey; ++i)
            {
                persist_delete(i);
            }
        }
	}
}

bool SaveGlobalPersistedData(void)
{
    if(!IsGlobalPersistedDataCurrent())
    {
        WARNING_LOG("Persisted data does not match current version, clearing.");
        ClearGlobalPersistedData();
    }

    INFO_LOG("Saving global persisted data.");
    persist_write_bool(PERSISTED_IS_DATA_SAVED, true);
    persist_write_int(PERSISTED_CURRENT_DATA_VERSION, CURRENT_DATA_VERSION);
    persist_write_int(PERSISTED_MAX_KEY_USED, PERSISTED_GLOBAL_DATA_COUNT);
    persist_write_bool(PERSISTED_VIBRATION, GetVibration());
    persist_write_bool(PERSISTED_WORKER_APP, GetWorkerApp());
    persist_write_bool(PERSISTED_WORKER_CAN_LAUNCH, GetWorkerCanLaunch());
    persist_write_int(PERSISTED_CURRENT_GAME, Story_GetLastStoryId());
    persist_write_bool(PERSISTED_CURRENT_GAME_VALID, Story_IsLastStoryIdValid());
    persist_write_bool(PERSISTED_ALLOW_ACTIVITY, GetAllowActivity());
    
    uint16_t count = 0;
    uint16_t *buffer = NULL;
    Story_GetStoryList(&count, &buffer);
    persist_write_int(PERSISTED_STORY_LIST_SIZE, count);
    persist_write_data(PERSISTED_STORY_LIST, buffer, count * sizeof(uint16_t));
    free(buffer);
    
    persist_write_bool(PERSISTED_TUTORIAL_SEEN, GetTutorialSeen());
    
    return true;
}

bool LoadGlobalPersistedData(void)
{
    bool useWorkerApp = false;
    
    INFO_LOG("Loading global persisted data.");
    if(!persist_exists(PERSISTED_IS_DATA_SAVED) || !persist_read_bool(PERSISTED_IS_DATA_SAVED))
    {
        INFO_LOG("No saved data to load.");
        return false;
    }
    
    if(!IsGlobalPersistedDataCurrent())
    {
        WARNING_LOG("Persisted data does not match current version, clearing.");
        ClearGlobalPersistedData();
        return false;
    }
    
    uint16_t newcount = 0;
    uint16_t *newbuffer = NULL;
    uint16_t oldcount = 0;
    uint16_t *oldbuffer = NULL;
    Story_GetStoryList(&newcount, &newbuffer);
    oldcount = persist_read_int(PERSISTED_STORY_LIST_SIZE);
    oldbuffer = calloc(sizeof(uint16_t), oldcount);
    persist_read_data(PERSISTED_STORY_LIST, oldbuffer, oldcount * sizeof(uint16_t));

    for(int i = 0; i < oldcount; ++i)
    {
        bool found = false;
        for(int j = 0; j < newcount; ++j)
        {
            if(newbuffer[j] == oldbuffer[i])
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            ClearStoryPersistedData(oldbuffer[i]);
        }
    }
    
    free(oldbuffer);
    free(newbuffer);
    
    if(persist_read_bool(PERSISTED_CURRENT_GAME_VALID))
    {
        Story_SetLastStoryId(persist_read_int(PERSISTED_CURRENT_GAME));
    }
    SetVibration(persist_read_bool(PERSISTED_VIBRATION));
    SetAllowActivity(persist_read_bool(PERSISTED_ALLOW_ACTIVITY));
    useWorkerApp = persist_read_bool(PERSISTED_WORKER_APP);
    if(useWorkerApp)
    {
        AttemptToLaunchWorkerApp();
    }
    else
    {
        // If the user has launched the worker app outside of MiniDungeon,
        // they want it on.
        if(WorkerIsRunning())
            SetWorkerApp(true);
    }
    SetWorkerCanLaunch(persist_read_bool(PERSISTED_WORKER_CAN_LAUNCH));
    
    SetTutorialSeen(persist_read_bool(PERSISTED_TUTORIAL_SEEN));
    
    return true;
}

bool SaveStoryPersistedData(void)
{
    uint16_t storyId = Story_GetCurrentStoryId();
    uint16_t storyVersion = Story_GetCurrentStoryVersion();
    uint16_t hash = Story_GetCurrentStoryHash();
    int offset = ComputeStoryPersistedDataOffset(storyId);

    if(!IsStoryPersistedDataCurrent(storyId, storyVersion, hash))
    {
        WARNING_LOG("Persisted data does not match current version, clearing.");
        ClearStoryPersistedData(storyId);
    }
    
    INFO_LOG("Saving story persisted data.");
    persist_write_bool(offset + PERSISTED_STORY_IS_DATA_SAVED, true);
    persist_write_int(offset + PERSISTED_STORY_CURRENT_DATA_VERSION, storyVersion);
    persist_write_int(offset + PERSISTED_STORY_HASH, hash);
    persist_write_int(offset + PERSISTED_STORY_MAX_KEY_USED, offset + PERSISTED_STORY_DATA_COUNT);
    uint16_t count;
    uint8_t *buffer;
    Story_GetPersistedData(&count, &buffer);
    persist_write_data(offset + PERSISTED_STORY_STORY_DATA, buffer, count);
    Character_WritePersistedData(offset + PERSISTED_STORY_CHARACTER_DATA);
    
    persist_write_bool(offset + PERSISTED_STORY_IN_COMBAT, ClosingWhileInBattle());
    if(ClosingWhileInBattle())
    {
        persist_write_int(offset + PERSISTED_STORY_MONSTER_TYPE, Battle_GetCurrentMonsterIndex());
        Battle_WritePlayerData(offset + PERSISTED_STORY_BATTLE_PLAYER);
        Battle_WriteMonsterData(offset + PERSISTED_STORY_BATTLE_MONSTER);
    }
    persist_write_bool(offset + PERSISTED_STORY_FORCE_RANDOM_BATTLE, false);
    
    return true;
}

bool LoadStoryPersistedData(void)
{
    uint16_t storyId = Story_GetCurrentStoryId();
    uint16_t storyVersion = Story_GetCurrentStoryVersion();
    uint16_t hash = Story_GetCurrentStoryHash();
    int offset = ComputeStoryPersistedDataOffset(storyId);
    
    if(!persist_exists(offset + PERSISTED_STORY_IS_DATA_SAVED) || !persist_read_bool(offset + PERSISTED_STORY_IS_DATA_SAVED))
    {
        INFO_LOG("No saved data to load.");
        return false;
    }
    
    if(!IsStoryPersistedDataCurrent(storyId, storyVersion, hash))
    {
        WARNING_LOG("Persisted data does not match current version, clearing.");
        ClearStoryPersistedData(storyId);
        return false;
    }
    
    INFO_LOG("Loading story persisted data.");
    uint16_t count;
    uint8_t *buffer;
    Story_GetPersistedData(&count, &buffer);
    persist_read_data(offset + PERSISTED_STORY_STORY_DATA, buffer, count);
    
    Story_UpdateStoryWithPersistedState();
    Character_ReadPersistedData(offset + PERSISTED_STORY_CHARACTER_DATA);
    
    if(persist_read_bool(offset + PERSISTED_STORY_IN_COMBAT))
    {
        Battle_ReadPlayerData(offset + PERSISTED_STORY_BATTLE_PLAYER);
        Battle_ReadMonsterData(offset + PERSISTED_STORY_BATTLE_MONSTER);
        ResumeBattle(persist_read_int(offset + PERSISTED_STORY_MONSTER_TYPE));
    }
    else if(persist_read_bool(offset + PERSISTED_STORY_FORCE_RANDOM_BATTLE))
    {
        ForceRandomBattle();
    }
    
    return true;
}
