#include "pebble.h"

#include "Adventure.h"
#include "Credits.h"
#include "DescriptionFrame.h"
#include "DialogFrame.h"
#include "GlobalState.h"
#include "LargeImage.h"
#include "Logging.h"
#include "MainImage.h"
#include "MiniAdventure.h"
#include "OptionsMenu.h"
#include "NewBaseWindow.h"
#include "Slideshow.h"

#include "DungeonCrawl.h"
#include "DragonQuest.h"
#include "BattleTestStory.h"

#include "NewMenu.h"

void ChooseDungeonCrawl(void)
{
    LaunchDungeonCrawl();
}

void ChooseDragonQuest(void)
{
    LaunchDragonQuest();
}

void ChooseBattleTest(void)
{
    LaunchBattleTestStory();
}

void ChooseSlideshow(void)
{
    LaunchSlideshow();
}

void ChooseOptions(void)
{
    QueueOptionsScreen();
}

static void LoadText(int resourceId, int index)
{
    ResHandle creditsData = resource_get_handle(RESOURCE_ID_CREDITS_DATA);
    uint8_t buffer[256] = "";
    uint8_t int_bytes[2] = {0};
    resource_load_byte_range(creditsData, 0, int_bytes, 2);
//    int count = (int_bytes[1] << 8) + int_bytes[0];
    int location_index = 2 + (index) * 4;
    int location = 0;
    resource_load_byte_range(creditsData, location_index, int_bytes, 2);
    location = (int_bytes[1] << 8) + int_bytes[0];
    resource_load_byte_range(creditsData, location_index + 2, int_bytes, 2);
    int size = (int_bytes[1] << 8) + int_bytes[0];
    resource_load_byte_range(creditsData, location, buffer, size);
    DEBUG_LOG("Text: %s", (char*)buffer);
}

static DialogData credits[] =
{
    {
        .text = "Programming and art by Jonathan Panttaja",
        .allowCancel = false
    },
    {
        .text = "Additional Contributors: Belphemur and BlackLamb",
        .allowCancel = false
    },
    {
        .text = "Code located at https://Github.com/Torivon/MiniAdventure",
        .allowCancel = false
    },
};

void ChooseCredits(void)
{
    LoadText(RESOURCE_ID_CREDITS_DATA, CREDITS_PAGE_1);
    LoadText(RESOURCE_ID_CREDITS_DATA, CREDITS_PAGE_2);
    LoadText(RESOURCE_ID_CREDITS_DATA, CREDITS_PAGE_3);
    QueueDialog(&credits[0]);
    QueueDialog(&credits[1]);
    QueueDialog(&credits[2]);
}

void ChooseRepo(void)
{
    QueueLargeImage(RESOURCE_ID_IMAGE_REPOSITORY_CODE, true);
}

MenuCellDescription titleScreenMenuList[] = 
{
#if INCLUDE_DUNGEON_CRAWL
	{.name = "Dungeon", .description = "Simple dungeon delve", .callback = ChooseDungeonCrawl},
#endif
#if INCLUDE_DRAGON_QUEST
	{.name = "Dragon Quest", .description = "Extended adventure", .callback = ChooseDragonQuest},
#endif
#if INCLUDE_BATTLE_TEST_STORY
	{.name = "Battle Test", .description = "Battle arena", .callback = ChooseBattleTest},
#endif
#if INCLUDE_SLIDESHOW
	{.name = "Slideshow", .description = "Slideshow of all art", .callback = ChooseSlideshow},
#endif
	{.name = "Options", .description = "Options", .callback = ChooseOptions},
	{.name = "Credits", .description = "Credits", .callback = ChooseCredits},
	{.name = "Repository", .description = "QR code to Github", .callback = ChooseRepo}
};

static void TitleScreenAppear(void *data)
{
	RegisterMenuCellList(GetMainMenu(), titleScreenMenuList, sizeof(titleScreenMenuList)/sizeof(*titleScreenMenuList));
	SetForegroundImage(RESOURCE_ID_IMAGE_TITLE);
	SetMainImageVisibility(true, true, false);
	SetDescription("MiniAdventure");
}

static void TitleScreenPop(void *data)
{
	SetMainImageVisibility(false, false, false);
	SetDescription("");
}

DialogData introText[] =
{
    {
        .text = "MiniAdventure:\n Welcome to MiniAdventure. Press select to continue.",
        .allowCancel = true
    },
    {
        .text = "Use the select button to open the main menu.",
        .allowCancel = true
    },
    {
        .text = "Use the up and down buttons to make your selections inside menus.",
        .allowCancel = true
    },
    {
        .text = "The back button will exit games in progress.",
        .allowCancel = true
    },
};

void RegisterTitleScreen(void)
{
	INFO_LOG("RegisterTitleScreen");
    PushGlobalState(STATE_TITLE_SCREEN, 0, NULL, NULL, TitleScreenAppear, NULL, TitleScreenPop, NULL);
    TriggerDialog(&introText[0]);
    QueueDialog(&introText[1]);
    QueueDialog(&introText[2]);
    QueueDialog(&introText[3]);
}
