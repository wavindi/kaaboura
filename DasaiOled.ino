#include <SPI.h>
#include <Wire.h>
#include <BitBang_I2C.h>
#include <OneBitDisplay.h>
#include <AnimatedGIF.h>
#include "animation.h"

OBDISP obd;
AnimatedGIF gif;
static uint8_t ucOLED[4096];

// ─── ESP32-C3 Mini Pin Config ──────────────────────────────
#define RESET_PIN  -1
#define SDA_PIN    21    // UART0-RX repurposed for I2C SDA
#define SCL_PIN    20    // UART0-TX repurposed for I2C SCL
#define OLED_ADDR  -1
#define MY_OLED    OLED_128x64
#define USE_HW_I2C 1
#define FLIP180    0
#define INVERT     0
// ──────────────────────────────────────────────────────────

#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 64

#define NUMBEROFANIMATION 32

// ─── TTP223 ───────────────────────────────────────────────
#define TOUCH_PIN   3      // TTP223 OUT → GPIO3
#define DEBOUNCE_MS 300
// ──────────────────────────────────────────────────────────

uint8_t last_animation = 0;
int n = NUMBEROFANIMATION;
int r;
int debugRandom = 0;   // 0 = random | 1 = sequential
int counter = 99;
unsigned long lastTouchTime = 0;

// ──────────────────────────────────────────────────────────

void DrawPixel(int x, int y, uint8_t ucColor)
{
    uint8_t ucMask;
    int index;
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
    ucMask = 1 << (y & 7);
    index  = x + ((y >> 3) << 7);
    if (ucColor) ucOLED[index] |= ucMask;
    else         ucOLED[index] &= ~ucMask;
}

void GIFDraw(GIFDRAW* pDraw)
{
    uint8_t* s;
    int x, y, iWidth;
    static uint8_t ucPalette[4096];

    if (pDraw->y == 0)
    {
        for (x = 0; x < 256; x++)
        {
            uint16_t usColor = pDraw->pPalette[x];
            int gray  = (usColor & 0xf800) >> 8;
            gray     += (usColor & 0x07e0) >> 2;
            gray     += (usColor & 0x001f) << 3;
            ucPalette[x] = (gray > 800) ? 1 : 0;
        }
    }

    y      = pDraw->iY + pDraw->y;
    iWidth = pDraw->iWidth;
    if (iWidth > DISPLAY_WIDTH) iWidth = DISPLAY_WIDTH;

    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2)
    {
        for (x = 0; x < iWidth; x++)
            if (s[x] == pDraw->ucTransparent) s[x] = pDraw->ucBackground;
        pDraw->ucHasTransparency = 0;
    }

    if (pDraw->ucHasTransparency)
    {
        uint8_t c, ucTransparent = pDraw->ucTransparent;
        for (x = 0; x < iWidth; x++)
        {
            c = *s++;
            if (c != ucTransparent) DrawPixel(pDraw->iX + x, y, ucPalette[c]);
        }
    }
    else
    {
        s = pDraw->pPixels;
        for (x = 0; x < pDraw->iWidth; x++)
            DrawPixel(pDraw->iX + x, y, ucPalette[*s++]);
    }

    if (pDraw->y == pDraw->iHeight - 1)
        obdDumpBuffer(&obd, ucOLED, 1, 1, 0);
}

void playWrapper(uint8_t* gifinput, int size)
{
    if (gif.open(gifinput, size, GIFDraw))
    {
        while (gif.playFrame(true, NULL)) {}
        gif.close();
    }
}

void pickNextAnimation()
{
    if (debugRandom == 0)
    {
        r = random(0, n) + 1;
        while (r == last_animation) r = random(0, n) + 1;
        last_animation = r;
    }
    else
    {
        counter++;
        if (counter > NUMBEROFANIMATION) counter = 1;
        r = counter;
    }
}

// Waits up to `ms` ms but breaks out immediately on touch
void waitOrTouch(unsigned long ms)
{
    unsigned long start = millis();
    while (millis() - start < ms)
    {
        if (digitalRead(TOUCH_PIN) == HIGH)
        {
            unsigned long now = millis();
            if (now - lastTouchTime > DEBOUNCE_MS)
            {
                lastTouchTime = now;
                while (digitalRead(TOUCH_PIN) == HIGH) delay(10);
                Serial.println("Touch! Next animation.");
                break;
            }
        }
        delay(10);
    }
}

// ──────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(115200);  // Uses USB CDC (GPIO18/19) on C3 Mini — GPIO20/21 stay free

    pinMode(TOUCH_PIN, INPUT);

    // Explicitly bind I2C to GPIO20/21 on C3 Mini
    Wire.begin(SDA_PIN, SCL_PIN);

    int rc = obdI2CInit(&obd, MY_OLED, OLED_ADDR, FLIP180, INVERT,
                        USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 800000L);
    Serial.print("OLED init: ");
    Serial.println(rc);

    obdFill(&obd, 0, 1);
    gif.begin(LITTLE_ENDIAN_PIXELS);

    // Startup animation
    if (gif.open((uint8_t*)_31, sizeof(_31), GIFDraw))
    {
        Serial.printf("Canvas size = %d x %d\n",
                      gif.getCanvasWidth(), gif.getCanvasHeight());
        while (gif.playFrame(true, NULL)) {}
        gif.close();
    }
}

void loop()
{
    unsigned long idleTime = (unsigned long)random(1, 3) * 7500;
    Serial.printf("Idle: %lu ms\n", idleTime);
    waitOrTouch(idleTime);

    pickNextAnimation();
    Serial.printf("Playing: %d\n", r);

    switch (r)
    {
        case 1:  playWrapper((uint8_t*)_1,  sizeof(_1));  break;
        case 2:  playWrapper((uint8_t*)_2,  sizeof(_2));  break;
        case 3:  playWrapper((uint8_t*)_3,  sizeof(_3));  break;
        case 4:  playWrapper((uint8_t*)_4,  sizeof(_4));  break;
        case 5:  playWrapper((uint8_t*)_5,  sizeof(_5));  break;
        case 6:  playWrapper((uint8_t*)_6,  sizeof(_6));  break;
        case 7:  playWrapper((uint8_t*)_40, sizeof(_40)); break;
        case 8:  playWrapper((uint8_t*)_8,  sizeof(_8));  break;
        case 9:  playWrapper((uint8_t*)_9,  sizeof(_9));  break;
        case 10: playWrapper((uint8_t*)_10, sizeof(_10)); break;
        case 11: playWrapper((uint8_t*)_36, sizeof(_36)); break;
        case 12: playWrapper((uint8_t*)_41, sizeof(_41)); break;
        case 13: playWrapper((uint8_t*)_13, sizeof(_13)); break;
        case 14: playWrapper((uint8_t*)_14, sizeof(_14)); break;
        case 15: playWrapper((uint8_t*)_34, sizeof(_34)); break;
        case 16: playWrapper((uint8_t*)_16, sizeof(_16)); break;
        case 17: playWrapper((uint8_t*)_35, sizeof(_35)); break;
        case 18: playWrapper((uint8_t*)_18, sizeof(_18)); break;
        case 19: playWrapper((uint8_t*)_19, sizeof(_19)); break;
        case 20: playWrapper((uint8_t*)_33, sizeof(_33)); break;
        case 21: playWrapper((uint8_t*)_21, sizeof(_21)); break;
        case 22: playWrapper((uint8_t*)_22, sizeof(_22)); break;
        case 23: playWrapper((uint8_t*)_23, sizeof(_23)); break;
        case 24: playWrapper((uint8_t*)_24, sizeof(_24)); break;
        case 25: playWrapper((uint8_t*)_25, sizeof(_25)); break;
        case 26: playWrapper((uint8_t*)_32, sizeof(_32)); break;
        case 27: playWrapper((uint8_t*)_37, sizeof(_37)); break;
        case 28: playWrapper((uint8_t*)_28, sizeof(_28)); break;
        case 29: playWrapper((uint8_t*)_29, sizeof(_29)); break;
        case 30: playWrapper((uint8_t*)_30, sizeof(_30)); break;
        case 31: playWrapper((uint8_t*)_42, sizeof(_42)); break;
        case 32: playWrapper((uint8_t*)_39, sizeof(_39)); break;
    }
}
