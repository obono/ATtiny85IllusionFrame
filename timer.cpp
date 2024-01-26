#include "core.h"

/*  Defines  */

enum STATE_T : uint8_t {
    STATE_SETTING = 0,
    STATE_ACTIVE,
    STATE_PAUSED,
    STATE_FINISHED,
};

#define MINUTES_MAX     9
#define SECONDS_MAX     59
#define FRAMES_MAX      29

/*  Local Functions  */

static void onSetting(void);
static void controlSetting(void);
static void controlActive(void);
static void controlPaused(void);
static void controlFinished(void);
static uint8_t convertPosition(uint8_t n);
static void setSettingPixels(uint8_t m);
static void setTimerPixels(void);
static void setTimerString(uint8_t m, uint8_t s, bool isDot);

/*  Local Functions (macros)  */

#define callControlFunc(n)  ((void (*)(void)) pgm_read_ptr(controlFuncTable + n))()

/*  Local Constants  */

PROGMEM static void(*const controlFuncTable[])(void) = {
    controlSetting, controlActive, controlPaused, controlFinished
};

/*  Local Variables  */

static STATE_T state;
static uint8_t settingMinutes, minutes, seconds, frames;
static char timeString[5];

/*---------------------------------------------------------------------------*/

void initTimer(void)
{
    settingMinutes = 3;
    onSetting();
}

void updateTimer(void)
{
    callControlFunc(state);
}

/*---------------------------------------------------------------------------*/

static void onSetting(void)
{
    setString(0, 22, F("SETTING"));
    setString(1, 22, timeString);
    setSettingPixels(settingMinutes);
    setTimerString(settingMinutes, 0, true);
    state = STATE_SETTING;
    isInvalid = true;
}

static void controlSetting(void)
{
    if (isButtonDown(RIGHT_BUTTON)) {
        minutes = settingMinutes;
        seconds = 0;
        frames = FRAMES_MAX;
        state = STATE_ACTIVE;
        clearString(0);
        setTimerPixels();
        isInvalid = true;
    } else if (isButtonDown(LEFT_BUTTON)) {
        if (++settingMinutes > 9) settingMinutes = 1;
        setSettingPixels(settingMinutes);
        setTimerString(settingMinutes, 0, true);
        isInvalid = true;
    }
}

static void controlActive(void)
{
    if (frames-- == 0) {
        if (seconds-- == 0) {
            minutes--;
            seconds = SECONDS_MAX;
        }
        if (minutes == 0 && seconds == 0) {
            state = STATE_FINISHED;
        }
        frames = FRAMES_MAX;
    }
    if (frames == FRAMES_MAX || frames == 14) {
        setTimerString(minutes, seconds, (frames >= 15));
        isInvalid = true;
    }
    setTimerPixels();
    if (state != STATE_FINISHED && isButtonDown(RIGHT_BUTTON)) {
        state = STATE_PAUSED;
        setString(0, 28, F("PAUSED"));
        setTimerString(minutes, seconds, true);
        isInvalid = true;
    }
}

static void controlPaused(void)
{
    if (isButtonDown(RIGHT_BUTTON)) {
        state = STATE_ACTIVE;
        clearString(0);
        setTimerString(minutes, seconds, (frames >= 15));
        isInvalid = true;
    } else if (isButtonDown(LEFT_BUTTON)) {
        onSetting();
    }
}

static void controlFinished(void)
{
    if (isButtonDown(LEFT_BUTTON | RIGHT_BUTTON)) {
        onSetting();
    } else {
        if (frames-- == 0) {
            frames = FRAMES_MAX;
            setString(1, 22, timeString);
            isInvalid = true;
        } else if (frames == 14) {
            clearString(1);
            isInvalid = true;
        }
        uint8_t v = ((frames < 15) ? frames : (43 - frames)) * 8;
        fillNeoPixelsRGB(v, v, v);
    }
}

static uint8_t convertPosition(uint8_t n)
{
    return (26 - n) & 0x1F;
}

static void setSettingPixels(uint8_t m)
{
    fillNeoPixels(digitColor(m));
    uint32_t black = rgb2Color(0, 0, 0);
    setNeoPixel(27, black);
    setNeoPixel(28, black);
}

static void setTimerPixels(void)
{
    uint8_t lastFrame = (frames == FRAMES_MAX) ? 0 : frames + 1;
    setNeoPixel(convertPosition(lastFrame),
                digitColor(minutes + (lastFrame < seconds / 2)));
    setNeoPixelRGB(convertPosition(frames), 255, 255, 255);
}

static void setTimerString(uint8_t m, uint8_t s, bool isDot)
{
    timeString[0] = '0' + m;
    timeString[1] = (isDot) ? ':' : ' ';
    timeString[2] = '0' + s / 10;
    timeString[3] = '0' + s % 10;
    timeString[4] = '\0';
}
