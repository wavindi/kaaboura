#include <SPI.h>
#include <Wire.h>
#include <BitBang_I2C.h>
#include <OneBitDisplay.h>
#include <AnimatedGIF.h>
#include "animation.h"

OBDISP obd;
AnimatedGIF gif;
static uint8_t ucOLED[1024];

// ─── ESP32-C3 Mini Pin Config ──────────────────────────────
#define RESET_PIN  -1
#define SDA_PIN    21
#define SCL_PIN    20
#define TOUCH_PIN  7      // TTP223 OUT → GPIO7
// ──────────────────────────────────────────────────────────

#define OLED_ADDR  -1
#define MY_OLED    OLED_128x64
#define USE_HW_I2C 1
#define FLIP180    0
#define INVERT     0

#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 64

// ─── Sequence Configuration ───────────────────────────────
#define TOTAL_ANIMATIONS 44 // 1 to 42 + jojos(43) + intro(44)
int currentIndex = 44;      // Start with "intro" on boot
bool forceNext = false;     // Flag to break out of the GIF loop instantly
// ──────────────────────────────────────────────────────────

void DrawPixel(int x, int y, uint8_t ucColor)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) return;
    uint8_t ucMask = 1 << (y & 7);
    int index = x + ((y >> 3) << 7);
    if (ucColor) ucOLED[index] |= ucMask;
    else         ucOLED[index] &= ~ucMask;
}

void GIFDraw(GIFDRAW* pDraw)
{
    uint8_t* s;
    int x, y, iWidth;
    static uint8_t ucPalette[256];

    if (pDraw->y == 0)
    {
        for (x = 0; x < 256; x++)
        {
            uint16_t usColor = pDraw->pPalette[x];
            int gray = (usColor & 0xf800) >> 8;
            gray += ((usColor & 0x7e0) >> 2);
            gray += ((usColor & 0x1f) << 3);
            ucPalette[x] = (gray > 511) ? 1 : 0;
        }
    }

    y = pDraw->iY + pDraw->y;
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
        for (x = 0; x < iWidth; x++) {
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

// ============================================================
//  Serial Command Checker (Handles Numbers & Words)
// ============================================================
void checkSerial()
{
    if (Serial.available() > 0)
    {
        String input = Serial.readStringUntil('\n');
        input.trim();        // Remove whitespace/newlines
        input.toLowerCase(); // Make everything lowercase

        if (input == "jojos") { currentIndex = 43; forceNext = true; }
        else if (input == "intro") { currentIndex = 44; forceNext = true; }
        else 
        {
            int val = input.toInt();
            if (val >= 1 && val <= 42) 
            {
                currentIndex = val;
                forceNext = true;
            } 
            else if (input.length() > 0) 
            {
                Serial.println("❌ Invalid. Type a number 1-42, 'jojos', or 'intro'.");
            }
        }
    }
}

// ============================================================
//  GIF Playback Wrapper (Listens for Touch & Serial instantly)
// ============================================================
void playWrapper(uint8_t* gifinput, int size, String animName)
{
    Serial.print("▶️ Looping animation: ");
    Serial.println(animName);
    
    if (gif.open(gifinput, size, GIFDraw))
    {
        while (gif.playFrame(true, NULL)) 
        {
            // 1. Listen for Serial commands
            checkSerial();
            if (forceNext) break;

            // 2. Listen for Touch Sensor (GPIO 7)
            if (digitalRead(TOUCH_PIN) == HIGH)
            {
                delay(50); // Simple debounce
                if (digitalRead(TOUCH_PIN) == HIGH) 
                {
                    // Wait until finger is lifted
                    while (digitalRead(TOUCH_PIN) == HIGH) delay(10);
                    
                    // Move to the next face in the list
                    currentIndex++;
                    if (currentIndex > TOTAL_ANIMATIONS) currentIndex = 1; // Wrap around to 1
                    
                    forceNext = true;
                    Serial.println("👆 Touch detected! Skipping face.");
                    break;
                }
            }
        }
        gif.close();
    }
}

// ============================================================

void setup()
{
    pinMode(TOUCH_PIN, INPUT);
    Serial.begin(115200);

    int rc = obdI2CInit(&obd, MY_OLED, OLED_ADDR, FLIP180, INVERT,
                        USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 800000L);
    Serial.print("OLED Init Code: ");
    Serial.println(rc);

    obdFill(&obd, 0, 1);
    gif.begin(LITTLE_ENDIAN_PIXELS);

    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║                SYSTEM READY                ║");
    Serial.println("╠════════════════════════════════════════════╣");
    Serial.println("║ TAP SENSOR to cycle to the next face!      ║");
    Serial.println("║ TYPE: 1 to 42, 'jojos', or 'intro' in the  ║");
    Serial.println("║ serial monitor to jump instantly!          ║");
    Serial.println("╚════════════════════════════════════════════╝\n");
}

void loop()
{
    forceNext = false; // Reset the interrupt flag

    // Play whichever animation matches our current index
    switch (currentIndex)
    {
        case 1: playWrapper((uint8_t*)_1, sizeof(_1), "_1"); break;
        case 2: playWrapper((uint8_t*)_2, sizeof(_2), "_2"); break;
        case 3: playWrapper((uint8_t*)_3, sizeof(_3), "_3"); break;
        case 4: playWrapper((uint8_t*)_4, sizeof(_4), "_4"); break;
        case 5: playWrapper((uint8_t*)_5, sizeof(_5), "_5"); break;
        case 6: playWrapper((uint8_t*)_6, sizeof(_6), "_6"); break;
        case 7: playWrapper((uint8_t*)_7, sizeof(_7), "_7"); break;
        case 8: playWrapper((uint8_t*)_8, sizeof(_8), "_8"); break;
        case 9: playWrapper((uint8_t*)_9, sizeof(_9), "_9"); break;
        case 10: playWrapper((uint8_t*)_10, sizeof(_10), "_10"); break;
        case 11: playWrapper((uint8_t*)_11, sizeof(_11), "_11"); break;
        case 12: playWrapper((uint8_t*)_12, sizeof(_12), "_12"); break;
        case 13: playWrapper((uint8_t*)_13, sizeof(_13), "_13"); break;
        case 14: playWrapper((uint8_t*)_14, sizeof(_14), "_14"); break;
        case 15: playWrapper((uint8_t*)_15, sizeof(_15), "_15"); break;
        case 16: playWrapper((uint8_t*)_16, sizeof(_16), "_16"); break;
        case 17: playWrapper((uint8_t*)_17, sizeof(_17), "_17"); break;
        case 18: playWrapper((uint8_t*)_18, sizeof(_18), "_18"); break;
        case 19: playWrapper((uint8_t*)_19, sizeof(_19), "_19"); break;
        case 20: playWrapper((uint8_t*)_20, sizeof(_20), "_20"); break;
        case 21: playWrapper((uint8_t*)_21, sizeof(_21), "_21"); break;
        case 22: playWrapper((uint8_t*)_22, sizeof(_22), "_22"); break;
        case 23: playWrapper((uint8_t*)_23, sizeof(_23), "_23"); break;
        case 24: playWrapper((uint8_t*)_24, sizeof(_24), "_24"); break;
        case 25: playWrapper((uint8_t*)_25, sizeof(_25), "_25"); break;
        case 26: playWrapper((uint8_t*)_26, sizeof(_26), "_26"); break;
        case 27: playWrapper((uint8_t*)_27, sizeof(_27), "_27"); break;
        case 28: playWrapper((uint8_t*)_28, sizeof(_28), "_28"); break;
        case 29: playWrapper((uint8_t*)_29, sizeof(_29), "_29"); break;
        case 30: playWrapper((uint8_t*)_30, sizeof(_30), "_30"); break;
        case 31: playWrapper((uint8_t*)_31, sizeof(_31), "_31"); break;
        case 32: playWrapper((uint8_t*)_32, sizeof(_32), "_32"); break;
        case 33: playWrapper((uint8_t*)_33, sizeof(_33), "_33"); break;
        case 34: playWrapper((uint8_t*)_34, sizeof(_34), "_34"); break;
        case 35: playWrapper((uint8_t*)_35, sizeof(_35), "_35"); break;
        case 36: playWrapper((uint8_t*)_36, sizeof(_36), "_36"); break;
        case 37: playWrapper((uint8_t*)_37, sizeof(_37), "_37"); break;
        case 38: playWrapper((uint8_t*)_38, sizeof(_38), "_38"); break;
        case 39: playWrapper((uint8_t*)_39, sizeof(_39), "_39"); break;
        case 40: playWrapper((uint8_t*)_40, sizeof(_40), "_40"); break;
        case 41: playWrapper((uint8_t*)_41, sizeof(_41), "_41"); break;
        case 42: playWrapper((uint8_t*)_42, sizeof(_42), "_42"); break;
        
        // Custom named animations
        case 43: playWrapper((uint8_t*)jojos, sizeof(jojos), "jojos"); break;
        case 44: playWrapper((uint8_t*)intro, sizeof(intro), "intro"); break;
    }
}
