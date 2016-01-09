#include <pebble.h>
#include "Clock.h"
#include "Logging.h"
#include "MiniAdventure.h"
#include "BaseWindow.h"
#include "TextBox.h"

static TextBox *clockTextBox = NULL;
static TextBox *dateTextBox = NULL;
static TextBox *dayTextBox = NULL;

#define CLOCK_FRAME_WIDTH 62
#define CLOCK_FRAME_HEIGHT 36
#define CLOCK_TEXT_X_OFFSET 6
#define CLOCK_TEXT_Y_OFFSET -1
#if defined(PBL_RECT)
static GRect clockFrame = {.origin = {.x = 144 / 2 - CLOCK_FRAME_WIDTH / 2, .y = 168 - CLOCK_FRAME_HEIGHT}, .size = {.w = CLOCK_FRAME_WIDTH, .h = CLOCK_FRAME_HEIGHT}};
static GRect dayFrame = {.origin = {.x = 0, .y = 60}, .size = {.w = 30, .h = 18}};
static GRect dateFrame = {.origin = {.x = 0, .y = 100}, .size = {.w = 60, .h = 18}};
#elif defined(PBL_ROUND)
#define CLOCK_VERTICAL_OFFSET 10
static GRect clockFrame = {.origin = {.x = 180 / 2 - CLOCK_FRAME_WIDTH / 2, .y = 180 - CLOCK_FRAME_HEIGHT - CLOCK_VERTICAL_OFFSET}, .size = {.w = CLOCK_FRAME_WIDTH, .h = CLOCK_FRAME_HEIGHT}};
static GRect dayFrame = {.origin = {.x = 45, .y = 48}, .size = {.w = 30, .h = 18}};
static GRect dateFrame = {.origin = {.x = 15, .y = 114}, .size = {.w = 60, .h = 18}};
#endif

typedef struct TimeFormatData
{
    char buffer[10];
    const char *format;
} TimeFormatData;

static TimeFormatData formatData[3] =
{
    {
        .buffer = "00/00/00",
        .format = "%m/%d/%y"
    },
    {
        .buffer = "Sun",
        .format = "%a"
    },
    {
        .buffer = "00:00",
        .format = "%R"
    }
};

void UpdateTimeData(uint16_t index, struct tm *current_time)
{
    strftime(formatData[index].buffer, 10, formatData[index].format, current_time);
}

void UpdateClockData(void)
{
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
    
    for(int i = 0; i < 3; ++i)
    {
        UpdateTimeData(i, current_time);
    }
}

void UpdateClock(void)
{
    if(!TextBoxInitialized(clockTextBox))
        return;
    
    UpdateClockData();
    
    TextBoxSetText(clockTextBox, formatData[2].buffer);
    TextBoxSetText(dayTextBox, formatData[1].buffer);
    TextBoxSetText(dateTextBox, formatData[0].buffer);
}

void RemoveClockLayer(void)
{
    RemoveTextBox(clockTextBox);
    RemoveTextBox(dayTextBox);
    RemoveTextBox(dateTextBox);
}

void HideDateLayer(void)
{
    HideTextBox(dayTextBox);
    HideTextBox(dateTextBox);
}

void ShowDateLayer(void)
{
    ShowTextBox(dayTextBox);
    ShowTextBox(dateTextBox);
}

void InitializeClockLayer(Window *window)
{
    if(!clockTextBox)
    {
        clockTextBox = CreateTextBox(CLOCK_TEXT_X_OFFSET, CLOCK_TEXT_Y_OFFSET, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), clockFrame);
        GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
        dayTextBox = CreateTextBox(0, 0, font, dayFrame);
        dateTextBox = CreateTextBox(0, 0, font, dateFrame);
        if (clock_is_24h_style())
        {
            formatData[2].format = "%R";
        }
        else
        {
            formatData[2].format = "%I:%M";
        }
    }
    
    UpdateClockData();
    InitializeTextBox(window, clockTextBox, formatData[2].buffer);
    InitializeTextBox(window, dayTextBox, formatData[1].buffer);
    InitializeTextBox(window, dateTextBox, formatData[0].buffer);
}

void FreeClockLayer(void)
{
    FreeTextBox(clockTextBox);
    FreeTextBox(dayTextBox);
    FreeTextBox(dateTextBox);
    clockTextBox = NULL;
    dayTextBox = NULL;
    dateTextBox = NULL;
}
