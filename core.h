#pragma once

#include <Arduino.h>

/*  Defines  */

#define ENABLE_EEPROM

#if not defined __AVR_ATtiny85__ || F_CPU != 8000000UL || defined DISABLEMILLIS
#error Board Configuration is wrong...
#endif

#define WIDTH           128
#define HEIGHT          64
#define PAGES           8
#define LED_COUNT       32

#define LEFT_BUTTON     bit(0)
#define RIGHT_BUTTON    bit(1)

#define HUE_MAX         1536

/*  Global Functions  */

void    initCore(void);
void    refreshScreen(bool isBig);
void    clearScreenBuffer(void);

void    updateButtonState(void);
bool    isButtonPressed(uint8_t b);
bool    isButtonDown(uint8_t b);
bool    isButtonUp(uint8_t b);

uint32_t rgb2Color(uint8_t r, uint8_t g, uint8_t b);
uint32_t hsv2Color(uint16_t h, uint8_t s = 255, uint8_t v = 255);
uint32_t digitColor(uint8_t n);
void    setNeoPixel(uint8_t pos, uint32_t c);
void    setNeoPixelRGB(uint8_t pos, uint8_t r, uint8_t g, uint8_t b);
void    setNeoPixelHSV(uint8_t pos, uint16_t h, uint8_t s = 255, uint8_t v = 255);
void    fillNeoPixels(uint32_t c);
void    fillNeoPixelsRGB(uint8_t r, uint8_t g, uint8_t b);
void    fillNeoPixelsHSV(uint16_t h, uint8_t s = 255, uint8_t v = 255);

void    initStrings(void);
void    setString(uint8_t idx, uint8_t x, const __FlashStringHelper *pFlashString);
void    setString(uint8_t idx, uint8_t x, char *pString);
void    clearString(uint8_t idx);

uint32_t tinyRandom(uint16_t n);

/*  Global Functions (macros)  */

#define circulate(n, v, m)  (((n) + (v) + (m)) % (m))

/*  Global Variables  */

extern uint8_t  counter;
extern bool     isInvalid;
