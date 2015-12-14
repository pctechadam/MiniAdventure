#include "pebble.h"

#include "Adventure.h"
#include "Battle.h"
#include "Character.h"
#include "Clock.h"
#include "GlobalState.h"
#include "Items.h"
#include "Logging.h"
#include "MainMenu.h"
#include "Menu.h"
#include "NewBattle.h"
#include "OptionsMenu.h"
#include "Persistence.h"
#include "Shop.h"
#include "Slideshow.h"
#include "Story.h"
#include "TitleScreen.h"
#include "UILayers.h"
#include "Utils.h"
#include "WorkerControl.h"

#include "NewBaseWindow.h"
#include "NewMenu.h"
	 
Window *baseWindow = NULL;

static bool hasFocus = true;

bool HasFocus(void)
{
	return hasFocus;
}

// Called once per minute
void handle_time_tick(struct tm* tick_time, TimeUnits units_changed) 
{
	DEBUG_LOG("Main App tick");
	if(!hasFocus)
		return;
	
	UpdateGlobalState(units_changed);
		
	if(units_changed & MINUTE_UNIT)
	{
		UpdateNewClock();
		UpdateClock();
	}
}

void focus_handler(bool in_focus) {
	hasFocus = in_focus;
	DEBUG_VERBOSE_LOG("Focus handler");
	if(hasFocus)
	{
		UpdateClock();
		SetUpdateDelay();
		INFO_LOG("Gained focus.");
	}
	else
	{
		INFO_LOG("Lost focus.");
	}
}

void handle_init() {
	
	INFO_LOG("Starting MiniAdventure");
	
#if ALLOW_WORKER_APP
	if(WorkerIsRunning())
	{
#if ALLOW_WORKER_APP_LISTENING
		app_worker_message_subscribe(WorkerMessageHandler);
#endif
		AppAwake();
	}
#endif

	SeedRandom();
	DEBUG_LOG("Srand");
	
	handle_time_tick(NULL, SECOND_UNIT);
	DEBUG_LOG("First handle second");
	
	// Just here so that the health and level fields are always filled in.
	InitializeCharacter();
#if USE_MENULAYER_PROTOTYPE	
	baseWindow = InitializeNewBaseWindow();
	window_stack_push(baseWindow, false);
	RegisterTitleScreen();
#else
	ShowTitleMenu();
#endif
	tick_timer_service_subscribe(SECOND_UNIT, &handle_time_tick);
	app_focus_service_subscribe(focus_handler);
}

void handle_deinit() 
{
	INFO_LOG("Cleaning up on exit.");
#if ALLOW_WORKER_APP		
	AppDying(ClosingWhileInBattle());
#endif
	UnloadBackgroundImage();
	UnloadMainBmpImage();
	UnloadTextLayers();
	tick_timer_service_unsubscribe();
	app_focus_service_unsubscribe();
#if ALLOW_WORKER_APP && ALLOW_WORKER_APP_LISTENING
	app_worker_message_unsubscribe();
#endif
	CleanupMenu();
	if(baseWindow)
		window_destroy(baseWindow);
}

// The main event/run loop for our app
int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}