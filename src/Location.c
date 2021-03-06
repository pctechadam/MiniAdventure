//
//  Location.c
//  
//
//  Created by Jonathan Panttaja on 1/25/16.
//
//

#include <pebble.h>
#include "AutoLocationConstants.h"
#include "AutoSizeConstants.h"
#include "BinaryResourceLoading.h"
#include "Events.h"
#include "ImageMap.h"
#include "Location.h"
#include "Logging.h"
#include "Story.h"
#include "Utils.h"

typedef struct Location
{
    char name[MAX_STORY_NAME_LENGTH];
    char menuName[MAX_STORY_NAME_LENGTH];
    char menuDescription[MAX_STORY_DESC_LENGTH];
    uint16_t adjacentLocationCount;
    uint16_t adjacentLocations[MAX_ADJACENT_LOCATIONS];
    uint16_t backgroundImageCount;
    uint16_t backgroundImages[MAX_BACKGROUND_IMAGES];
    uint16_t overrideBattleFloor;
    uint16_t battleFloorImageId;
    uint16_t locationProperties;
    uint16_t length;
    uint16_t baseLevel;
    uint16_t encounterChance;
    uint16_t monsterCount;
    uint16_t monsters[MAX_MONSTERS];
    uint16_t initialEventCount;
    uint16_t initialEvents[MAX_EVENTS];
    uint16_t localEventCount;
    uint16_t localEvents[MAX_EVENTS];
    uint16_t useActivityTracking; //Three values, false, true, default
    uint16_t inactiveSpeed;
    uint16_t activeSpeed;
    uint16_t skipEncountersIfActive;
    uint16_t grantXPForSkippedEncounters;
    uint16_t extendPathDuringActivity;
} Location;

static Location *currentLocation = NULL;
static Event *currentLocationInitialEvent = NULL;
static Location *adjacentLocations[MAX_ADJACENT_LOCATIONS] = {0};
static Event *adjacentLocationInitialEvents[MAX_ADJACENT_LOCATIONS][MAX_EVENTS];

static Location *Location_Load(uint16_t logical_index)
{
    ResHandle currentStoryData = Story_GetCurrentResHandle();
    Location *newLocation = calloc(sizeof(Location), 1);
    ResourceLoadStruct(currentStoryData, logical_index, (uint8_t*)newLocation, sizeof(Location), "Location");
    
    for(int i = 0; i < newLocation->backgroundImageCount; ++i)
    {
        newLocation->backgroundImages[i] = ImageMap_GetIdByIndex(newLocation->backgroundImages[i]);
    }
    
    return newLocation;
}

uint16_t Location_GetCurrentEncounterChance(void)
{
    return currentLocation->encounterChance;
}

uint16_t Location_GetCurrentBaseLevel(void)
{
    return currentLocation->baseLevel;
}

static void Location_Free(Location *location)
{
    if(location)
    {
        free(location);
    }
}

#if DEBUG_LOGGING > 1
static void Location_Log(Location *location)
{
    DEBUG_VERBOSE_LOG("Location: %s", location->name);
    for(int i = 0; i < location->adjacentLocationCount; ++i)
    {
        DEBUG_VERBOSE_LOG("Adjacent Location: %d", location->adjacentLocations[i]);
    }
    for(int i = 0; i < location->backgroundImageCount; ++i)
    {
        DEBUG_VERBOSE_LOG("Background: %d", location->backgroundImages[i]);
    }
}
#endif

void Location_LoadAdjacentLocations(void)
{
    for(int i = 0; i < currentLocation->adjacentLocationCount; ++i)
    {
        adjacentLocations[i] = Location_Load(currentLocation->adjacentLocations[i]);
        for(int j = 0; j < MAX_EVENTS; ++j)
        {
            adjacentLocationInitialEvents[i][j] = Event_Load(adjacentLocations[i]->initialEvents[j]);
        }
    }
}

void Location_FreeAdjacentLocations(void)
{
    for(int i = 0; i < MAX_ADJACENT_LOCATIONS; ++i)
    {
        if(adjacentLocations[i])
        {
            Location_Free(adjacentLocations[i]);
            adjacentLocations[i] = NULL;
        }
        // When we switch locations, we set adjacentLocations[i] to NULL for the location to which we are moving.
        // We have to make sure we clear out extra initial events for the new location.
        for(int j = 0; j < MAX_EVENTS; ++j)
        {
            Event_Free(adjacentLocationInitialEvents[i][j]);
            adjacentLocationInitialEvents[i][j] = NULL;
        }
    }
}

uint16_t Location_GetCurrentAdjacentLocations(void)
{
    uint16_t count = 0;
    for(int i = 0; i < currentLocation->adjacentLocationCount; ++i)
    {
        if(EventList_CheckPrerequisites(adjacentLocationInitialEvents[i], NULL))
            ++count;
    }
    return count;
}

uint16_t Location_GetCurrentAdjacentLocationByIndex(uint16_t index)
{
    return currentLocation->adjacentLocations[index];
}

const char *Location_GetAdjacentLocationName(uint16_t index)
{
    uint16_t currentIndex = 0;
    for(int i = 0; i < currentLocation->adjacentLocationCount; ++i)
    {
        if(EventList_CheckPrerequisites(adjacentLocationInitialEvents[i], NULL))
        {
            if(currentIndex == index)
                return adjacentLocations[i]->menuName;
            ++currentIndex;
        }
    }
    
    return "None";
}

const char *Location_GetAdjacentLocationDescription(uint16_t index)
{
    uint16_t currentIndex = 0;
    for(int i = 0; i < currentLocation->adjacentLocationCount; ++i)
    {
        if(EventList_CheckPrerequisites(adjacentLocationInitialEvents[i], NULL))
        {
            if(currentIndex == index)
                return adjacentLocations[i]->menuDescription;
            ++currentIndex;
        }
    }
    
    return "None";
}

int Location_GetCurrentBackgroundImageId(void)
{
    if(currentLocation && currentLocation->backgroundImageCount)
    {
        uint16_t index = Random(currentLocation->backgroundImageCount);
        return currentLocation->backgroundImages[index];
    }
    else
    {
        return -1;
    }
}

int Location_GetCurrentBattleFloorImageId(void)
{
    if(currentLocation->overrideBattleFloor)
        return currentLocation->battleFloorImageId;
    else
        return -1;
}

bool Location_CurrentLocationIsPath(void)
{
    if(currentLocation)
    {
        return currentLocation->length > 0;
    }
    
    return false;
}

uint16_t Location_GetCurrentLength(void)
{
    if(currentLocation)
    {
        return currentLocation->length;
    }
    
    return 0;
}

const char *Location_GetCurrentName(void)
{
    if(currentLocation)
    {
        return currentLocation->name;
    }
    else
    {
        return "None";
    }
}

bool Location_CurrentLocationHasMonster(void)
{
    return currentLocation && currentLocation->monsterCount > 0;
}

int Location_GetCurrentMonster(void)
{
    if(currentLocation && currentLocation->monsterCount)
    {
        uint16_t index = Random(currentLocation->monsterCount);
        return currentLocation->monsters[index];
    }
    else
    {
        return -1;
    }
}

void Location_SetCurrentLocation(uint16_t locationIndex)
{
    if(currentLocation)
    {
        Location_Free(currentLocation);
        Event_Free(currentLocationInitialEvent);
        currentLocationInitialEvent = NULL;
    }
    
    currentLocation = Location_Load(locationIndex);
    Location_FreeAdjacentLocations();
    Location_LoadAdjacentLocations();
    Event_FreeLocalEvents();
    Event_LoadLocalEvents(currentLocation->localEventCount, currentLocation->localEvents);
#if DEBUG_LOGGING > 1
    Location_Log(currentLocation);
#endif
}

uint16_t Location_MoveToAdjacentLocation(uint16_t menuIndex)
{
    uint16_t newIndex = 0;
    uint16_t indexToUse = 0;
    uint16_t initialEventIndex = 0;
    for(int i = 0; i < currentLocation->adjacentLocationCount; ++i)
    {
        if(EventList_CheckPrerequisites(adjacentLocationInitialEvents[i], &initialEventIndex))
        {
            if(newIndex == menuIndex)
            {
                indexToUse = i;
                break;
            }
            ++newIndex;
        }
    }

    if(currentLocation)
    {
        Location *newLocation = adjacentLocations[indexToUse];
        Event *newInitialEvent = adjacentLocationInitialEvents[indexToUse][initialEventIndex];
        adjacentLocations[indexToUse] = NULL;
        adjacentLocationInitialEvents[indexToUse][initialEventIndex] = NULL;
        
        uint16_t globalIndex = currentLocation->adjacentLocations[indexToUse];
        
        Event_FreeLocalEvents();
        Location_FreeAdjacentLocations();
        Location_Free(currentLocation);
        Event_Free(currentLocationInitialEvent);
        currentLocationInitialEvent = newInitialEvent;
        
        currentLocation = newLocation;
        if(currentLocationInitialEvent)
        {
            Event_TriggerEvent(currentLocationInitialEvent, true);
        }
        Location_LoadAdjacentLocations();
        Event_LoadLocalEvents(currentLocation->localEventCount, currentLocation->localEvents);
        return globalIndex;
    }
    
    return 0;
}

void Location_ClearCurrentLocation(void)
{
    if(currentLocation)
    {
        Location_Free(currentLocation);
        currentLocation = NULL;
        Event_Free(currentLocationInitialEvent);
        currentLocationInitialEvent = NULL;
        Location_FreeAdjacentLocations();
        Event_FreeLocalEvents();
    }
}

bool Location_CurrentLocationIsRestArea(void)
{
    return currentLocation && (currentLocation->locationProperties & LOCATION_PROPERTY_REST_AREA);
}

bool Location_CurrentLocationIsRespawnPoint(void)
{
    return currentLocation && (currentLocation->locationProperties & LOCATION_PROPERTY_RESPAWN_POINT);
}

bool Location_CurrentLocationIsLevelUp(void)
{
    return currentLocation && (currentLocation->locationProperties & LOCATION_PROPERTY_LEVEL_UP);
}

bool Location_CurrentLocationEndsGame(void)
{
    return currentLocation && (currentLocation->locationProperties & LOCATION_PROPERTY_GAME_WIN);
}

bool Location_CurrentLocationUseActivityTracking(void)
{
    return currentLocation && (currentLocation->useActivityTracking);
}

uint16_t Location_CurrentInactiveSpeed(void)
{
    if (!currentLocation)
        return 1;
    
    return currentLocation->inactiveSpeed;
}

uint16_t Location_CurrentActiveSpeed(void)
{
    if (!currentLocation)
        return 1;
    
    return currentLocation->activeSpeed;
}

bool Location_CurrentSkipEncountersIfActive(void)
{
    return currentLocation && (currentLocation->skipEncountersIfActive);
}

bool Location_CurrentGrantXPForSkippedEncounters(void)
{
    return currentLocation && (currentLocation->grantXPForSkippedEncounters);
}

bool Location_CurrentExtendPathDuringActivity(void)
{
    return currentLocation && (currentLocation->extendPathDuringActivity);
}

