#include "core.h"

/*  Typedefs  */

typedef struct {
    uint16_t    h;
    uint8_t     v;
} WALL_STATE_T;

typedef struct {
    uint16_t    h;
    uint8_t     pos;
    int8_t      dir;
    uint8_t     life;
} STAR_STATE_T;

union STATE_T {
    WALL_STATE_T    walls[4];
    STAR_STATE_T    stars[8];
};

/*  Local Functions  */

static void onChangePattern(void);
static void updateWhite(void);
static void updateRainbow(void);
static void updateWalls(void);
static void updateDisco(void);
static void updateWhirlpool(void);

/*  Local Functions (macros)  */

#define callUpdateFunc(n)   ((void (*)(void)) pgm_read_ptr(updateFuncTable + n))()

/*  Local Constants  */

PROGMEM static void(*const updateFuncTable[])(void) = {
    updateWhite, updateRainbow, updateWalls, updateDisco, updateWhirlpool
};

PROGMEM static const char titleTable[][11] = {
    "WHITE", "RAINBOW", "WALLS", "DISCO HALL", "WHIRLPOOL"
};

/*  Local Variables  */

static STATE_T state;
static uint8_t pattern;
static char timeString[] = "PATTERN 1";
static bool isInitial;

/*---------------------------------------------------------------------------*/

void initIllumi(void)
{
    setString(1, 10, timeString);
    pattern = 0;
    onChangePattern();
}

void updateIllumi(void)
{
    int8_t v = isButtonDown(RIGHT_BUTTON) - isButtonDown(LEFT_BUTTON);
    if (v != 0) {
        pattern = circulate(pattern, v, (sizeof(updateFuncTable) / sizeof(void *)));
        onChangePattern();
    }
    callUpdateFunc(pattern);
    isInitial = false;
}

/*---------------------------------------------------------------------------*/

static void onChangePattern(void)
{
    timeString[8] = '1' + pattern;
    const char *pTitle = titleTable[pattern];
    setString(2, 64 - strlen_P(pTitle) * 6, (__FlashStringHelper *)pTitle);
    isInitial = true;
    isInvalid = true;

}

static void updateWhite(void)
{
    if (isInitial || ++counter == 120) counter = 0;
    uint8_t v = ((counter < 60) ? counter : (120 - counter)) * 4;
    fillNeoPixelsRGB(v, v, v);
}

static void updateRainbow(void)
{
    if (isInitial) counter = 0;
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        setNeoPixelHSV(i, i * HUE_MAX / LED_COUNT + counter * 30);
    }
    counter++;
}

static void updateWalls(void)
{
    if (isInitial) {
        for (uint8_t i = 0; i < 4; i++) {
            state.walls[i].v = 0;
        }
    }
    for (uint8_t i = 0; i < 4; i++) {
        WALL_STATE_T *p = &state.walls[i];
        if (p->v > 0) p->v -= 16;
        if (tinyRandom(30) == 0) {
            p->h = tinyRandom(HUE_MAX);
            p->v = 240;
        }
        uint32_t c = hsv2Color(p->h, 255, p->v);
        for (uint8_t j = 0; j < 8; j++) {
            setNeoPixel(i * 8 + j, c);
        }
    }
}

static void updateDisco(void)
{
    if (isInitial) {
        for (uint8_t i = 0; i < 8; i++) {
            STAR_STATE_T *p = &state.stars[i];
            p->pos = tinyRandom(256);
            p->life = 0;
        }
    }
    fillNeoPixels(0);
    for (uint8_t i = 0; i < 8; i++) {
        STAR_STATE_T *p = &state.stars[i];
        if (p->life-- == 0) {
            p->h = tinyRandom(HUE_MAX);
            p->dir = (tinyRandom(8) + 1) * (tinyRandom(2) * 2 - 1);
            p->life = tinyRandom(211) + 30;
        }
        p->pos += p->dir;
        setNeoPixelHSV(p->pos >> 3, p->h);
    }
}

static void updateWhirlpool(void)
{
    if (isInitial) counter = 0;
    fillNeoPixels(0);
    for (uint8_t i = 1; i <= 8; i++) {
        uint8_t pos = (i * counter >> 3) % LED_COUNT;
        setNeoPixel(pos, digitColor(i));
    }
    counter++;
}
