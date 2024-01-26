#include "core.h"
#include "font.h"

#include <Adafruit_NeoPixel.h>
#include <avr/eeprom.h>

/*  Defines  */

#define PIN_SPI_CLK     0
#define PIN_SPI_DATA    2
#define PIN_SPI_DC      3
#define PIN_SPI_CS      4

#define PIN_NEOPIXEL    1
#define PIN_BUTTONS     A0

#define LED_BRIGHTNESS  48

#define STRINGS         4

/*  Typedefs  */

typedef struct {
    uint8_t x:7;
    uint8_t isPgm:1;
    char    *pString;
} STRING_T;

/*  Macro functions  */

#define pinLow(pin)         bitClear(PORTB, pin)
#define pinHigh(pin)        bitSet(PORTB, pin)
#define pinSpiClkLow()      pinLow(PIN_SPI_CLK)
#define pinSpiClkHigh()     pinHigh(PIN_SPI_CLK)
#define pinSpiDataLow()     pinLow(PIN_SPI_DATA)
#define pinSpiDataHigh()    pinHigh(PIN_SPI_DATA)
#define pinSpiDcLow()       pinLow(PIN_SPI_DC)
#define pinSpiDcHigh()      pinHigh(PIN_SPI_DC)
#define pinSpiCsLow()       pinLow(PIN_SPI_CS)
#define pinSpiCsHigh()      pinHigh(PIN_SPI_CS)

/*  Local functions  */

static void spiSendByte(const uint8_t b);
static void spiSendCommand(const uint8_t cmd);
static void spiSendCommands(const uint8_t *pCmd, uint8_t len);
static void spiSendData(uint8_t *pData, uint8_t len);
static void drawStrings(uint8_t idx, uint8_t row, bool isBig = false);
static void drawSprites(int16_t y);

/*  Global Variables  */

uint8_t counter;
bool    isInvalid;

/*  Local Constants  */

PROGMEM static const uint8_t ssd1306InitSequence[] = { // Initialization Sequence
    0xAE,           // Set Display ON/OFF - AE=OFF, AF=ON
    0xD5, 0xF0,     // Set display clock divide ratio/oscillator frequency, set divide ratio
    0xA8, 0x3F,     // Set multiplex ratio (1 to 64) ... (height - 1)
    0xD3, 0x00,     // Set display offset. 00 = no offset
    0x40 | 0x00,    // Set start line address, at 0.
    0x8D, 0x14,     // Charge Pump Setting, 14h = Enable Charge Pump
    0x20, 0x00,     // Set Memory Addressing Mode - 00=Horizontal, 01=Vertical, 10=Page, 11=Invalid
    0xA0,           // Set Segment Re-map
    0xC0,           // Set COM Output Scan Direction
    0xDA, 0x12,     // Set COM Pins Hardware Configuration - 128x32:0x02, 128x64:0x12
    0x81, 0xFF,     // Set contrast control register
    0xD9, 0x22,     // Set pre-charge period (0x22 or 0xF1)
    0xDB, 0x20,     // Set VCOMH Deselect Level - 0x00: 0.65 x VCC, 0x20: 0.77 x VCC (RESET), 0x30: 0.83 x VCC
    0xA4,           // Entire Display ON (resume) - output RAM to display
    0xA6,           // Set Normal/Inverse Display mode. A6=Normal; A7=Inverse
    0x2E,           // Deactivate Scroll command
    0xAF,           // Set Display ON/OFF - AE=OFF, AF=ON
    0xB0,           // Set page start, at 0.
    0x00, 0x10,     // Set column start, at 0.
};

PROGMEM static const uint8_t digitColorRGBTable[][3] {
    { 0,   0,   0   }, { 224, 0,   0   }, { 0,   0,   255 }, { 160, 160, 0   }, { 0,   192, 0   },
    { 192, 64,  64  }, { 192, 96,  0   }, { 128, 0,   255 }, { 128, 224, 0   }, { 0,   160, 255 },
};

/*  Local Variables  */

Adafruit_NeoPixel strip(LED_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

static STRING_T string[STRINGS];
static uint8_t  buttonState = 0;
static uint8_t  lastButtonState;
static uint8_t  spiBuffer[WIDTH];
static bool     isInvalidPixels;

/*---------------------------------------------------------------------------*/
/*                                   Core                                    */
/*---------------------------------------------------------------------------*/

void initCore(void)
{
    DDRB = bit(PIN_SPI_CLK) | bit(PIN_SPI_DATA) | bit(PIN_SPI_DC) | bit(PIN_SPI_CS);
    pinSpiClkLow();
    pinSpiDcHigh();
    pinSpiCsHigh();
    delay(100);
    spiSendCommands(ssd1306InitSequence, sizeof(ssd1306InitSequence));
    isInvalid = true;

    strip.begin();
    strip.setBrightness(LED_BRIGHTNESS);
    strip.show();
    isInvalidPixels = false;

    initStrings();
}

void refreshScreen(bool isBig)
{
    if (isInvalidPixels) {
        strip.show();
        isInvalidPixels = false;
    }

    if (isInvalid) {
        for (uint8_t i = 0; i < PAGES; i++) {
            if (isBig && i >= 2) {
                drawStrings(1, i - 2, true);
            } else {
                drawStrings(i >> 1, i & 1, false);
            }
            spiSendData(spiBuffer, WIDTH);
        }
        isInvalid = false;
    }
}

void clearScreenBuffer(void)
{
    memset(spiBuffer, 0x00, WIDTH);
}

static void spiSendByte(const uint8_t b)
{
    for (uint8_t bit = 0x80; bit; bit >>= 1) {
        (b & bit) ? pinSpiDataHigh() : pinSpiDataLow();
        pinSpiClkHigh();
        pinSpiClkLow();
    }
}

static void spiSendCommand(const uint8_t cmd)
{
    pinSpiCsLow();
    pinSpiDcLow();
    spiSendByte(cmd);
    pinSpiDcHigh();
    pinSpiCsHigh();
}

static void spiSendCommands(const uint8_t *pCmd, uint8_t len)
{
    pinSpiCsLow();
    pinSpiDcLow();
    for (uint8_t i = 0; i < len; i++) {
        spiSendByte(pgm_read_byte(pCmd++));
    }
    pinSpiDcHigh();
    pinSpiCsHigh();
}

static void spiSendData(uint8_t *pData, uint8_t len)
{
    pinSpiCsLow();
    for (uint8_t i = 0 ; i < len; i++) {
        spiSendByte(*pData++);
    }
    pinSpiCsHigh();
}

uint32_t tinyRandom(uint16_t n)
{
    static uint32_t x = 2463534242;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (x >> 1) % n;
}

/*---------------------------------------------------------------------------*/
/*                               Button State                                */
/*---------------------------------------------------------------------------*/

void updateButtonState(void)
{
    lastButtonState = buttonState;
    buttonState = (1024 - analogRead(PIN_BUTTONS)) / 135;
}

bool isButtonPressed(uint8_t b)
{
    return (buttonState & b);
}

bool isButtonDown(uint8_t b)
{
    return (buttonState & ~lastButtonState & b);
}

bool isButtonUp(uint8_t b)
{
    return (~buttonState & lastButtonState & b);
}

/*---------------------------------------------------------------------------*/
/*                                NEO Pixels                                 */
/*---------------------------------------------------------------------------*/

uint32_t rgb2Color(uint8_t r, uint8_t g, uint8_t b)
{
    return strip.Color(r, g, b);
}

uint32_t hsv2Color(uint16_t h, uint8_t s, uint8_t v)
{
    int16_t h2 = h % HUE_MAX;
    uint16_t r = 0, g = 0, b = 0;
    if (h2 <= 512 || h2 >= 1024) {
        r = abs(768 - h2) - 256;
        if (r >= 256) r = 256;
    }
    if (h2 <= 1024) {
        g = 512 - abs(512 - h2);
        if (g >= 256) g = 256;
    }
    if (h2 >= 512) {
        b = 512 - abs(1024 - h2);
        if (b >= 256) b = 256;
    }
    uint16_t z = 256 - s;
    r = (r * s >> 8) + z;
    g = (g * s >> 8) + z;
    b = (b * s >> 8) + z;
    return strip.Color(r * v >> 8, g * v >> 8, b * v >> 8);
}

uint32_t digitColor(uint8_t n)
{
    return rgb2Color(pgm_read_byte(&digitColorRGBTable[n][0]),
                     pgm_read_byte(&digitColorRGBTable[n][1]),
                     pgm_read_byte(&digitColorRGBTable[n][2]));
}

void setNeoPixel(uint8_t pos, uint32_t c)
{
    strip.setPixelColor(pos, c);
    isInvalidPixels = true;
}

void setNeoPixelRGB(uint8_t pos, uint8_t r, uint8_t g, uint8_t b)
{
    setNeoPixel(pos, strip.Color(r, g, b));
}

void setNeoPixelHSV(uint8_t pos, uint16_t h, uint8_t s, uint8_t v)
{
    setNeoPixel(pos, hsv2Color(h, s, v));
}

void fillNeoPixels(uint32_t c)
{
    strip.fill(c);
    isInvalidPixels = true;
}

void fillNeoPixelsRGB(uint8_t r, uint8_t g, uint8_t b)
{
    fillNeoPixels(strip.Color(r, g, b));
}

void fillNeoPixelsHSV(uint16_t h, uint8_t s, uint8_t v)
{
    strip.fill(hsv2Color(h, s, v));
}

/*---------------------------------------------------------------------------*/
/*                                  Strings                                  */
/*---------------------------------------------------------------------------*/

void initStrings(void)
{
    memset(string, 0, sizeof(STRING_T) * STRINGS);
}

void setString(uint8_t idx, uint8_t x, const __FlashStringHelper *pFlashString)
{
    STRING_T *p = &string[idx];
    p->x = x;
    p->isPgm = true;
    p->pString = (char *)pFlashString;
}

void setString(uint8_t idx, uint8_t x, char *pString)
{
    STRING_T *p = &string[idx];
    p->x = x;
    p->isPgm = false;
    p->pString = pString;
}

void clearString(uint8_t idx)
{
    string[idx].pString = NULL;
}

static void drawStrings(uint8_t idx, uint8_t row, bool isBig)
{
    STRING_T *p = &string[idx];
    char c, *pChar = p->pString;
    uint8_t x = (pChar) ? p->x : WIDTH;
    memset(&spiBuffer[0], 0, x);

    while ((c = (p->isPgm) ? pgm_read_byte(pChar++) : *pChar++)) {
        uint8_t w = 0;
        const uint8_t *pBitmap = NULL;
        if (isBig) {
            if (c >= '0' && c <= '9') {
                w = IMG_BIG_DIGIT_W;
                if (row <= 4) pBitmap = &imgBigDigit[c - '0'][IMG_BIG_DIGIT_W * row];
            } else {
                w = IMG_BIG_DOT_W;
                if (c == ':' && row <= 4 && (row & 1)) pBitmap = imgBigDot;
            }
        } else {
            w = IMG_FONT_W;
            int8_t a = -1;
            if (c >= '0' && c <= '9') a = c - '0';
            if (c >= 'A' && c <= 'Z') a = c - 'A' + 10;
            if (c >= 'a' && c <= 'z') a = c - 'a' + 36;
            if (a >= 0) pBitmap = &imgFont[a][IMG_FONT_W * row];
        }

        if (x <= WIDTH - w) {
            if (pBitmap) {
                memcpy_P(&spiBuffer[x], pBitmap, w);
            } else {
                memset(&spiBuffer[x], 0, w);
            }
            x += w;
        } else {
            break;
        }
    }

    memset(&spiBuffer[x], 0, WIDTH - x);
}
