#include <util/delay.h>
#include "core.h"

/*  Defines  */

#define FPS     30
#define VERSION "1"

enum : uint8_t {
    MODE_SPLASH = 0,
    MODE_ILLUMI,
    MODE_TIMER,
};

/*  Typedefs  */

typedef struct {
    void(*initFunc)(void);
    void(*updateFunc)(void);
} MODULE_FUNCS;

/*  Global Functions  */

void initIllumi(void);
void updateIllumi(void);

void initTimer(void);
void updateTimer(void);

/*  Local functions  */

static void initSplash(void);
static void updateSplash(void);
static void modeControl(void);

/*  Local Functions (macros)  */

#define callInitFunc(idx)   ((void (*)(void)) pgm_read_word((uint16_t) &moduleTable[idx].initFunc))()
#define callUpdateFunc(idx) ((void (*)(void)) pgm_read_word((uint16_t) &moduleTable[idx].updateFunc))()

/*  Local Constants  */

PROGMEM static const MODULE_FUNCS moduleTable[] = {
    { initSplash,   updateSplash },
    { initIllumi,   updateIllumi },
    { initTimer,    updateTimer  },
};

/*  Local Variables  */

static uint8_t mode, modeCounter;
static uint32_t targetTime;

/*---------------------------------------------------------------------------*/

void setup(void)
{
    initCore();
    initSplash();
    targetTime = millis();
    mode = MODE_SPLASH;
    modeCounter = 0;
}

void loop(void)
{
    updateButtonState();
    callUpdateFunc(mode);
    refreshScreen((mode == MODE_TIMER));
    modeControl();
    targetTime += 1000 / FPS;
    uint32_t delayTime = targetTime - millis();
    if (!bitRead(delayTime, 31)) delay(delayTime);
}

/*---------------------------------------------------------------------------*/

static void initSplash(void)
{
    initStrings();
    setString(0, 16, F("ATTINY85"));
    setString(1, 16, F("ILLUSION"));
    setString(2, 34, F("FRAME"));
    setString(3, 68, F("VER " VERSION));
    fillNeoPixelsRGB(0, 0, 0);
}

static void updateSplash(void)
{
    // do nothing
}

static void modeControl(void)
{
    if (mode == MODE_SPLASH) {
        if (++modeCounter == FPS) {
            mode = MODE_ILLUMI;
            modeCounter = 0;
            initStrings();
            initIllumi();
        }
    } else {
        if (isButtonPressed(LEFT_BUTTON | RIGHT_BUTTON)) {
            if (modeCounter < FPS && ++modeCounter == FPS) {
                mode = 3 - mode;
                initStrings();
                callInitFunc(mode);
            }
        } else {
            modeCounter = 0;
        }
    }
}
