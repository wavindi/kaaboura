#include <SPI.h>
#include <Wire.h>


#include <BitBang_I2C.h>
#include <OneBitDisplay.h>
#include <AnimatedGIF.h>

#include "animation.h"           


OBDISP obd;
AnimatedGIF gif;
static uint8_t ucOLED[4096]; // holds current frame for 128x64 OLED

// ESP32-C6 Zero
#define RESET_PIN -1
#define SDA_PIN 21
#define SCL_PIN 20
#define OLED_ADDR -1
#define MY_OLED OLED_128x64
#define USE_HW_I2C 1
#define FLIP180 0
#define INVERT 0

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

//
// This doesn't have to be super efficient
// 
void DrawPixel(int x, int y, uint8_t ucColor)
{
    uint8_t ucMask;
    int index;

    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
        return;
    ucMask = 1 << (y & 7);
    index = x + ((y >> 3) << 7);
    if (ucColor)
        ucOLED[index] |= ucMask;
    else
        ucOLED[index] &= ~ucMask;
}

// Draw a line of image directly on the LCD
void GIFDraw(GIFDRAW* pDraw)
{
    uint8_t* s;
    int x, y, iWidth;
    static uint8_t ucPalette[4096]; // thresholded palette


    if (pDraw->y == 0) // first line, convert palette to 0/1
    {
        for (x = 0; x < 256; x++)
        {
            uint16_t usColor = pDraw->pPalette[x];
            int gray = (usColor & 0xf800) >> 8; // red
            gray += ((usColor & 0x7e0) >> 2); // plus green*2
            gray += ((usColor & 0x1f) << 3); // plus blue
            //ucPalette[x] = (gray >> 9); // 0->511 = 0, 512->1023 = 1
            if (gray>800) ucPalette[x]=1; else ucPalette[x]=0;
        }
    }
    y = pDraw->iY + pDraw->y; // current line
    iWidth = pDraw->iWidth;
    if (iWidth > DISPLAY_WIDTH)
        iWidth = DISPLAY_WIDTH;

    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) // restore to background color
    {
        for (x = 0; x < iWidth; x++)
        {
            if (s[x] == pDraw->ucTransparent)
                s[x] = pDraw->ucBackground;
        }
        pDraw->ucHasTransparency = 0;
    }
    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) // if transparency used
    {
        uint8_t c, ucTransparent = pDraw->ucTransparent;
        int x;
        for (x = 0; x < iWidth; x++)
        {
            c = *s++;
            if (c != ucTransparent)
                DrawPixel(pDraw->iX + x, y, ucPalette[c]);
        }
    }
    else
    {
        s = pDraw->pPixels;
        // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
        for (x = 0; x < pDraw->iWidth; x++)
            DrawPixel(pDraw->iX + x, y, ucPalette[*s++]);
    }
    if (pDraw->y == pDraw->iHeight - 1) // last line, render it to the display
        obdDumpBuffer(&obd, ucOLED, 1, 1, 0);

        
} /* GIFDraw() */


uint8_t last_animation = 0; // to prevent 2 animation loop after idle. just make it feels , more "random"??


void playWrapper(uint8_t* gifinput, int size)
{

    if (gif.open(gifinput, size, GIFDraw))
    {
        //    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
        while (gif.playFrame(true, NULL))
        {
        }
        gif.close();
    }

}

struct Anime {
    uint8_t* ptr;
    int size;
};


#define NUMBEROFANIMATION 32
Anime anime;

int n = NUMBEROFANIMATION;

int r;
int debugRandom = 0;  //choose between random or i++ animation  (0 = random / 1 = i++)
int counter = 99;



void setup() {

    // sorry for ths long hard code, just to make simple I want to put the animation inside array , as the library fn is using ptr and size I cant just put the addresses I also need the size
  // as i dont know how to read sizeof a pointer .  bear with ths :(
    //anime[0].ptr = (uint8_t*)_1;
    //anime[0].size = sizeof(_1);
    /*
    anime[1].ptr = (uint8_t*)_2;
    anime[1].size = sizeof(_2);
    anime[2].ptr = (uint8_t*)_3;
    anime[2].size = sizeof(_3);
    anime[3].ptr = (uint8_t*)_4;
    anime[3].size = sizeof(_4);
    anime[4].ptr = (uint8_t*)_5;
    anime[4].size = sizeof(_5);
    anime[5].ptr = (uint8_t*)_6;
    anime[5].size = sizeof(_6);
    anime[6].ptr = (uint8_t*)_7;
    anime[6].size = sizeof(_7);
    anime[7].ptr = (uint8_t*)jojos;
    anime[7].size = sizeof(jojos);
    anime[8].ptr = (uint8_t*)_9;
    anime[8].size = sizeof(_9);
    anime[9].ptr = (uint8_t*)_10;
    anime[9].size = sizeof(_10);

    anime[10].ptr = (uint8_t*)_11;
    anime[10].size = sizeof(_11);
    anime[11].ptr = (uint8_t*)_12;
    anime[11].size = sizeof(_12);
    anime[12].ptr = (uint8_t*)_13;
    anime[12].size = sizeof(_13);
    
    anime[13].ptr = (uint8_t*)_14;
    anime[13].size = sizeof(_14);
    
    anime[14].ptr = (uint8_t*)_15; 
    anime[14].size = sizeof(_15);

    anime[15].ptr = (uint8_t*)_16;
    anime[15].size = sizeof(_16);

    anime[16].ptr = (uint8_t*)_17;
    anime[16].size = sizeof(_17);
   
    anime[17].ptr = (uint8_t*)_18;
    anime[17].size = sizeof(_18);
   
    anime[18].ptr = (uint8_t*)_19;
    anime[18].size = sizeof(_19);
    
    anime[19].ptr = (uint8_t*)_20;
    anime[19].size = sizeof(_20);
    
    
    anime[20].ptr = (uint8_t*)_21;
    anime[20].size = sizeof(_21);
    anime[21].ptr = (uint8_t*)_22;
    anime[21].size = sizeof(_22);
    anime[22].ptr = (uint8_t*)_23;
    anime[22].size = sizeof(_23);    
    anime[23].ptr = (uint8_t*)_24;
    anime[23].size = sizeof(_24);    
    anime[24].ptr = (uint8_t*)_25; 
    anime[24].size = sizeof(_25);
    anime[25].ptr = (uint8_t*)_26;
    anime[25].size = sizeof(_26);
    anime[26].ptr = (uint8_t*)_27;
    anime[26].size = sizeof(_27);   
    anime[27].ptr = (uint8_t*)_28;
    anime[27].size = sizeof(_28);   
    anime[28].ptr = (uint8_t*)_29;
    anime[28].size = sizeof(_29);    
    anime[29].ptr = (uint8_t*)_30;
    anime[29].size = sizeof(_30);*/
    

    Serial.begin(115200);


    int rc = obdI2CInit(&obd, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 800000L); // use standard I2C bus at 400Khz
    Serial.print(rc);
   
    obdFill(&obd, 0, 1);
    
    gif.begin(LITTLE_ENDIAN_PIXELS);
// obdWriteString(&obd,0,0,0,(char *)"GIF Demo", FONT_NORMAL, 0, 1);
  //delay(1000);
if (gif.open((uint8_t*)_31, sizeof(_31), GIFDraw))
    {
             Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());

    while (gif.playFrame(true, NULL))
       {
          
       }
        gif.close();
    }
     
}

void loop() {



    r = random(1, 3) * 7500;
    Serial.println(r);
    delay(r);

    if (debugRandom == 0)
    {
        //randomSeed(esp_random());
        r = random(0, n)+1;
        Serial.println(r);

        while (r == last_animation) {
            delay(10);
            //randomSeed(esp_random());
            r = random(0, n)+1;

            if (r != last_animation)
            {
                last_animation = r;
                break;
            }
        }

        Serial.println(r);
    }
    else
    {
        counter++;
        if (counter > NUMBEROFANIMATION)
        {
            counter = 1;
        }
        r = counter;
    }
    Serial.println(r);
    /*
    switch (r)
    {
      case 0:
        anime.ptr = (uint8_t*)_1;
        anime.size = sizeof(_1);
        break;
      case 1:
        anime.ptr = (uint8_t*)_2;
        anime.size = sizeof(_2);
        break;
      case 2:
        anime.ptr = (uint8_t*)_3;
        anime.size = sizeof(_3);
        break;
      case 3:
        anime.ptr = (uint8_t*)_4;
        anime.size = sizeof(_4);
        break;
      case 4:
        anime.ptr = (uint8_t*)_5;
        anime.size = sizeof(_5);
        break;
      case 5:
        anime.ptr = (uint8_t*)_6;
        anime.size = sizeof(_6);
        break;
      case 6:
        anime.ptr = (uint8_t*)_7;
        anime.size = sizeof(_7);
        break;
      case 7:
        //anime.ptr = (uint8_t*)jojos;
        //anime.size = sizeof(jojos);
        break;  
      case 8:
        anime.ptr = (uint8_t*)_9;
        anime.size = sizeof(_9);
        break;
      case 9:
        anime.ptr = (uint8_t*)_10;
        anime.size = sizeof(_10);
        break;
      case 10:
        anime.ptr = (uint8_t*)_11;
        anime.size = sizeof(_11);
        break;
      case 11:
        anime.ptr = (uint8_t*)_12;
        anime.size = sizeof(_12);
        break;
      case 12:
        anime.ptr = (uint8_t*)_13;
        anime.size = sizeof(_13);
        break;
      case 13:
        anime.ptr = (uint8_t*)_14;
        anime.size = sizeof(_14);
        break;
      case 14:
        anime.ptr = (uint8_t*)_15;
        anime.size = sizeof(_15);
        break;
      case 15:
        anime.ptr = (uint8_t*)_16;
        anime.size = sizeof(_16);
        break;
      case 16:
        anime.ptr = (uint8_t*)_17;
        anime.size = sizeof(_17);
        break;
      case 17:
        anime.ptr = (uint8_t*)_18;
        anime.size = sizeof(_18);
        break;
      case 18:
        anime.ptr = (uint8_t*)_19;
        anime.size = sizeof(_19);
        break;
     case 19:
        anime.ptr = (uint8_t*)_20;
        anime.size = sizeof(_20);
        break;
     case 20:
        anime.ptr = (uint8_t*)_21;
        anime.size = sizeof(_21);
        break;   
     case 21:
        anime.ptr = (uint8_t*)_22;
        anime.size = sizeof(_22);
        break;   
     case 22:
        anime.ptr = (uint8_t*)_23;
        anime.size = sizeof(_23);
        break;   
     case 23:
        anime.ptr = (uint8_t*)_24;
        anime.size = sizeof(_24);
        break;   
    case 24:
        anime.ptr = (uint8_t*)_25;
        anime.size = sizeof(_25);
        break;   
    case 25:
     //   anime.ptr = (uint8_t*)_26;
      //  anime.size = sizeof(_26);
        break;  
    case 26:
      //  anime.ptr = (uint8_t*)_27;
       // anime.size = sizeof(_27);
        break;    
    }
    playWrapper(anime.ptr, anime.size);
    */
   
switch (r)  
{
  case 1:
  playWrapper((uint8_t*)_1, sizeof(_1));
  break;
  case 2:
  playWrapper((uint8_t*)_2, sizeof(_2));
  break;
  case 3:
  playWrapper((uint8_t*)_3, sizeof(_3));
  break;
  case 4:
  playWrapper((uint8_t*)_4, sizeof(_4));
  break;
    case 5:
  playWrapper((uint8_t*)_5, sizeof(_5));
  break;
    case 6:
  playWrapper((uint8_t*)_6, sizeof(_6));
  break;
    case 7:
  playWrapper((uint8_t*)_40, sizeof(_40));
  break;
    case 8:
  playWrapper((uint8_t*)_8, sizeof(_8));
  break;
    case 9:
  playWrapper((uint8_t*)_9, sizeof(_9));
  break;
    case 10:
  playWrapper((uint8_t*)_10, sizeof(_10));
  break;
    case 11:
  playWrapper((uint8_t*)_36, sizeof(_36));
  break;
    case 12:
  playWrapper((uint8_t*)_41, sizeof(_41));
  break;
    case 13:
  playWrapper((uint8_t*)_13, sizeof(_13));
  break;
    case 14:
  playWrapper((uint8_t*)_14, sizeof(_14));
  break;
    case 15:
  playWrapper((uint8_t*)_34, sizeof(_34));
  break;
    case 16:
  playWrapper((uint8_t*)_16, sizeof(_16));
  break;
    case 17:
  playWrapper((uint8_t*)_35, sizeof(_35));
  break;
    case 18:
  playWrapper((uint8_t*)_18, sizeof(_18));
  break;
    case 19:
  playWrapper((uint8_t*)_19, sizeof(_19));
  break;
    case 20:
  playWrapper((uint8_t*)_33, sizeof(_33));
  break;
  case 21:
  playWrapper((uint8_t*)_21, sizeof(_21));
  break;
  case 22:
  playWrapper((uint8_t*)_22, sizeof(_22));
  break;
  case 23:
  playWrapper((uint8_t*)_23, sizeof(_23));
  break;
  case 24:
  playWrapper((uint8_t*)_24, sizeof(_24));
  break;
  case 25:
  playWrapper((uint8_t*)_25, sizeof(_25));
  break;
  case 26:
  playWrapper((uint8_t*)_32, sizeof(_32));
  break;
  case 27:
  playWrapper((uint8_t*)_37, sizeof(_37));
  break;
  case 28:
  playWrapper((uint8_t*)_28, sizeof(_28));
  break;
  case 29:
  playWrapper((uint8_t*)_29, sizeof(_29));
  break;
  case 30:
  playWrapper((uint8_t*)_30, sizeof(_30));
  break;
  case 31:
  playWrapper((uint8_t*)_42, sizeof(_42));
  break;
  case 32:
  playWrapper((uint8_t*)_39, sizeof(_39));
  break;
}


}
