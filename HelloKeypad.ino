/* @file HelloKeypad.pde
|| @version 1.0
|| @author Alexander Brevig
|| @contact alexanderbrevig@gmail.com
||
|| @description
|| | Demonstrates the simplest use of the matrix Keypad library.
|| #
*/

#include <MD_MAX72xx.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Keypad.h>
#include <Homie.h>
#include <ArduinoOTA.h>

//////////////////////////////////////////////////////////////////////////

#define WIFI_SSID       "iPhone"
#define WIFI_PASSWORD   "0840432116"

#define FIREBASE_HOST "rmutl-bus.firebaseio.com"
#define FIREBASE_KEY "jzD40QuT0NDbQ7TuT2Qwwtj24NgildGIX3bUmRDC"

//////////////////////////////////////////////////////////////////////////

// Turn on debug statements to the serial output
#define  DEBUG  0

#if  DEBUG
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTD(x) Serial.println(x, DEC)

#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTD(x)

#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES  4

#define  CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // or SS

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary pins
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// We always wait a bit between updates of the display
#define  DELAYTIME  100  // in milliseconds

void scrollText(char *p)
{
  uint8_t charWidth;
  uint8_t cBuf[8];  // this should be ok for all built-in fonts

  PRINTS("\nScrolling text");
  mx.clear();

  while (*p != '\0')
  {
    charWidth = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);

    for (uint8_t i=0; i<=charWidth; i++)  // allow space between characters
    {
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
        mx.setColumn(0, cBuf[i]);
      delay(DELAYTIME);
    }
  }
}

void zeroPointSet()
// Demonstrates the use of setPoint and
// show where the zero point is in the display
{
  PRINTS("\nZero point highlight");
  mx.clear();

  if (MAX_DEVICES > 1)
    mx.setChar((2*COL_SIZE)-1, '0');

  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    mx.setPoint(i, i, true);
    mx.setPoint(0, i, true);
    mx.setPoint(i, 0, true);
    delay(DELAYTIME);
  }

  delay(DELAYTIME*3);
}

void rows()
// Demonstrates the use of setRow()
{
  PRINTS("\nRows 0->7");
  mx.clear();

  for (uint8_t row=0; row<ROW_SIZE; row++)
  {
    mx.setRow(row, 0xff);
    delay(2*DELAYTIME);
    mx.setRow(row, 0x00);
  }
}

void checkboard()
// nested rectangles spanning the entire display
{
  uint8_t chkCols[][2] = { { 0x55, 0xaa }, { 0x33, 0xcc }, { 0x0f, 0xf0 }, { 0xff, 0x00 } };

  PRINTS("\nCheckboard");
  mx.clear();

  for (uint8_t pattern = 0; pattern < sizeof(chkCols)/sizeof(chkCols[0]); pattern++)
  {
    uint8_t col = 0;
    uint8_t idx = 0;
    uint8_t rep = 1 << pattern;

    while (col < mx.getColumnCount())
    {
      for (uint8_t r = 0; r < rep; r++)
        mx.setColumn(col++, chkCols[pattern][idx]);   // use odd/even column masks
      idx++;
      if (idx > 1) idx = 0;
    }

    delay(10 * DELAYTIME);
  }
}

void columns()
// Demonstrates the use of setColumn()
{
  PRINTS("\nCols 0->max");
  mx.clear();

  for (uint8_t col=0; col<mx.getColumnCount(); col++)
  {
    mx.setColumn(col, 0xff);
    delay(DELAYTIME/MAX_DEVICES);
    mx.setColumn(col, 0x00);
  }
}

void cross()
// Combination of setRow() and setColumn() with user controlled
// display updates to ensure concurrent changes.
{
  PRINTS("\nMoving cross");
  mx.clear();
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  // diagonally down the display R to L
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0xff);
      mx.setRow(j, i, 0xff);
    }
    mx.update();
    delay(DELAYTIME);
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0x00);
      mx.setRow(j, i, 0x00);
    }
  }

  // moving up the display on the R
  for (int8_t i=ROW_SIZE-1; i>=0; i--)
  {
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0xff);
      mx.setRow(j, ROW_SIZE-1, 0xff);
    }
    mx.update();
    delay(DELAYTIME);
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0x00);
      mx.setRow(j, ROW_SIZE-1, 0x00);
    }
  }

  // diagonally up the display L to R
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0xff);
      mx.setRow(j, ROW_SIZE-1-i, 0xff);
    }
    mx.update();
    delay(DELAYTIME);
    for (uint8_t j=0; j<MAX_DEVICES; j++)
    {
      mx.setColumn(j, i, 0x00);
      mx.setRow(j, ROW_SIZE-1-i, 0x00);
    }
  }
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void bullseye()
// Demonstrate the use of buffer based repeated patterns
// across all devices.
{
  PRINTS("\nBullseye");
  mx.clear();
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  for (uint8_t n=0; n<3; n++)
  {
    byte  b = 0xff;
    int   i = 0;

    while (b != 0x00)
    {
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx.setRow(j, i, b);
        mx.setColumn(j, i, b);
        mx.setRow(j, ROW_SIZE-1-i, b);
        mx.setColumn(j, COL_SIZE-1-i, b);
      }
      mx.update();
      delay(3*DELAYTIME);
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx.setRow(j, i, 0);
        mx.setColumn(j, i, 0);
        mx.setRow(j, ROW_SIZE-1-i, 0);
        mx.setColumn(j, COL_SIZE-1-i, 0);
      }

      bitClear(b, i);
      bitClear(b, 7-i);
      i++;
    }

    while (b != 0xff)
    {
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx.setRow(j, i, b);
        mx.setColumn(j, i, b);
        mx.setRow(j, ROW_SIZE-1-i, b);
        mx.setColumn(j, COL_SIZE-1-i, b);
      }
      mx.update();
      delay(3*DELAYTIME);
      for (uint8_t j=0; j<MAX_DEVICES+1; j++)
      {
        mx.setRow(j, i, 0);
        mx.setColumn(j, i, 0);
        mx.setRow(j, ROW_SIZE-1-i, 0);
        mx.setColumn(j, COL_SIZE-1-i, 0);
      }

      i--;
      bitSet(b, i);
      bitSet(b, 7-i);
    }
  }

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void stripe()
// Demonstrates animation of a diagonal stripe moving across the display
// with points plotted outside the display region ignored.
{
  const uint16_t maxCol = MAX_DEVICES*ROW_SIZE;
  const uint8_t stripeWidth = 10;

  PRINTS("\nEach individually by row then col");
  mx.clear();

  for (uint16_t col=0; col<maxCol + ROW_SIZE + stripeWidth; col++)
  {
    for (uint8_t row=0; row < ROW_SIZE; row++)
    {
      mx.setPoint(row, col-row, true);
      mx.setPoint(row, col-row - stripeWidth, false);
    }
    delay(DELAYTIME);
  }
}

void spiral()
// setPoint() used to draw a spiral across the whole display
{
  PRINTS("\nSpiral in");
  int  rmin = 0, rmax = ROW_SIZE-1;
  int  cmin = 0, cmax = (COL_SIZE*MAX_DEVICES)-1;

  mx.clear();
  while ((rmax > rmin) && (cmax > cmin))
  {
    // do row
    for (int i=cmin; i<=cmax; i++)
    {
      mx.setPoint(rmin, i, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    rmin++;

    // do column
    for (uint8_t i=rmin; i<=rmax; i++)
    {
      mx.setPoint(i, cmax, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    cmax--;

    // do row
    for (int i=cmax; i>=cmin; i--)
    {
      mx.setPoint(rmax, i, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    rmax--;

    // do column
    for (uint8_t i=rmax; i>=rmin; i--)
    {
      mx.setPoint(i, cmin, true);
      delay(DELAYTIME/MAX_DEVICES);
    }
    cmin++;
  }
}

void bounce()
// Animation of a bouncing ball
{
  const int minC = 0;
  const int maxC = mx.getColumnCount()-1;
  const int minR = 0;
  const int maxR = ROW_SIZE-1;

  int  nCounter = 0;

  int  r = 0, c = 2;
  int8_t dR = 1, dC = 1;  // delta row and column

  PRINTS("\nBouncing ball");
  mx.clear();

  while (nCounter++ < 200)
  {
    mx.setPoint(r, c, false);
    r += dR;
    c += dC;
    mx.setPoint(r, c, true);
    delay(DELAYTIME/2);

    if ((r == minR) || (r == maxR))
      dR = -dR;
    if ((c == minC) || (c == maxC))
      dC = -dC;
  }
}

void intensity()
// Demonstrates the control of display intensity (brightness) across
// the full range.
{
  uint8_t row;

  PRINTS("\nVary intensity ");

  mx.clear();

  // Grow and get brighter
  row = 0;
  for (int8_t i=0; i<=MAX_INTENSITY; i++)
  {
    mx.control(MD_MAX72XX::INTENSITY, i);
    if (i%2 == 0)
      mx.setRow(row++, 0xff);
    delay(DELAYTIME*3);
  }

  mx.control(MD_MAX72XX::INTENSITY, 8);
}

void blinking()
// Uses the test function of the MAX72xx to blink the display on and off.
{
  int  nDelay = 1000;

  PRINTS("\nBlinking");
  mx.clear();

  while (nDelay > 0)
  {
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::ON);
    delay(nDelay);
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::OFF);
    delay(nDelay);

    nDelay -= DELAYTIME;
  }
}

void scanLimit(void)
// Uses scan limit function to restrict the number of rows displayed.
{
  PRINTS("\nScan Limit");
  mx.clear();

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t row=0; row<ROW_SIZE; row++)
    mx.setRow(row, 0xff);
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);

  for (int8_t s=MAX_SCANLIMIT; s>=0; s--)
  {
    mx.control(MD_MAX72XX::SCANLIMIT, s);
    delay(DELAYTIME*5);
  }
  mx.control(MD_MAX72XX::SCANLIMIT, MAX_SCANLIMIT);
}

void transformation1()
// Demonstrates the use of transform() to move bitmaps on the display
// In this case a user defined bitmap is created and animated.
{
  uint8_t arrow[COL_SIZE] =
  {
    0b00001000,
    0b00011100,
    0b00111110,
    0b01111111,
    0b00011100,
    0b00011100,
    0b00111110,
    0b00000000
  };

  MD_MAX72XX::transformType_t  t[] =
  {
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TFLR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TRC,
    MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD,
    MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD, MD_MAX72XX::TSD,
    MD_MAX72XX::TFUD,
    MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU,
    MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU, MD_MAX72XX::TSU,
    MD_MAX72XX::TINV,
    MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC,
    MD_MAX72XX::TINV
  };

  PRINTS("\nTransformation1");
  mx.clear();

  // use the arrow bitmap
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t j=0; j<mx.getDeviceCount(); j++)
    mx.setBuffer(((j+1)*COL_SIZE)-1, COL_SIZE, arrow);
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  delay(DELAYTIME);

  // run through the transformations
  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::ON);
  for (uint8_t i=0; i<(sizeof(t)/sizeof(t[0])); i++)
  {
    mx.transform(t[i]);
    delay(DELAYTIME*4);
  }
  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::OFF);
}

void transformation2()
// Demonstrates the use of transform() to move bitmaps on the display
// In this case font characters are loaded into the display for animation.
{
  MD_MAX72XX::transformType_t  t[] =
  {
    MD_MAX72XX::TINV,
    MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC, MD_MAX72XX::TRC,
    MD_MAX72XX::TINV,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TSD, MD_MAX72XX::TSU, MD_MAX72XX::TSD, MD_MAX72XX::TSU,
    MD_MAX72XX::TFLR, MD_MAX72XX::TFLR, MD_MAX72XX::TFUD, MD_MAX72XX::TFUD
  };

  PRINTS("\nTransformation2");
  mx.clear();
  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::OFF);

  // draw something that will show changes
  for (uint8_t j=0; j<mx.getDeviceCount(); j++)
  {
    mx.setChar(((j+1)*COL_SIZE)-1, '0'+j);
  }
  delay(DELAYTIME*5);

  // run thru transformations
  for (uint8_t i=0; i<(sizeof(t)/sizeof(t[0])); i++)
  {
    mx.transform(t[i]);
    delay(DELAYTIME*3);
  }
}

void wrapText()
// Display text and animate scrolling using auto wraparound of the buffer
{
  PRINTS("\nwrapText");
  mx.clear();
  mx.wraparound(MD_MAX72XX::ON);

  // draw something that will show changes
  for (uint16_t j=0; j<mx.getDeviceCount(); j++)
  {
    mx.setChar(((j+1)*COL_SIZE)-1, (j&1 ? 'M' : 'W'));
  }
  delay(DELAYTIME*5);

  // run thru transformations
  for (uint16_t i=0; i<3*COL_SIZE*MAX_DEVICES; i++)
  {
    mx.transform(MD_MAX72XX::TSL);
    delay(DELAYTIME/2);
  }
  for (uint16_t i=0; i<3*COL_SIZE*MAX_DEVICES; i++)
  {
    mx.transform(MD_MAX72XX::TSR);
    delay(DELAYTIME/2);
  }
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    mx.transform(MD_MAX72XX::TSU);
    delay(DELAYTIME*2);
  }
  for (uint8_t i=0; i<ROW_SIZE; i++)
  {
    mx.transform(MD_MAX72XX::TSD);
    delay(DELAYTIME*2);
  }

  mx.wraparound(MD_MAX72XX::OFF);
}

void showCharset(void)
// Run through display of the the entire font characters set
{
  mx.clear();
  mx.update(MD_MAX72XX::OFF);

  for (uint16_t i=0; i<256; i++)
  {
    mx.clear(0);
    mx.setChar(COL_SIZE-1, i);

    if (MAX_DEVICES >= 3)
    {
      char hex[3];

      sprintf(hex, "%02X", i);

      mx.clear(1);
      mx.setChar((2*COL_SIZE)-1,hex[1]);
      mx.clear(2);
      mx.setChar((3*COL_SIZE)-1,hex[0]);
    }

    mx.update();
    delay(DELAYTIME*2);
  }
  mx.update(MD_MAX72XX::ON);
}

// Text parameters
#define CHAR_SPACING  1 // pixels between characters

// Global message buffers shared by Serial and Scrolling functions
#define BUF_SIZE  75
char message[BUF_SIZE] = {"Hello!"};
bool newMessageAvailable = true;

void printText(uint8_t modStart, uint8_t modEnd, char *pMsg)
// Print the text string to the LED matrix modules specified.
// Message area is padded with blank columns after printing.
{
  uint8_t   state = 0;
  uint8_t   curLen;
  uint16_t  showLen;
  uint8_t   cBuf[8];
  int16_t   col = ((modEnd + 1) * COL_SIZE) - 1;

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  do     // finite state machine to print the characters in the space available
  {
    switch(state)
    {
      case 0: // Load the next character from the font table
        // if we reached end of message, reset the message pointer
        if (*pMsg == '\0')
        {
          showLen = col - (modEnd * COL_SIZE);  // padding characters
          state = 2;
          break;
        }

        // retrieve the next character form the font file
        showLen = mx.getChar(*pMsg++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);
        curLen = 0;
        state++;
        // !! deliberately fall through to next state to start displaying

      case 1: // display the next part of the character
        mx.setColumn(col--, cBuf[curLen++]);

        // done with font character, now display the space between chars
        if (curLen == showLen)
        {
          showLen = CHAR_SPACING;
          state = 2;
        }
        break;

      case 2: // initialize state for displaying empty columns
        curLen = 0;
        state++;
        // fall through

      case 3:  // display inter-character spacing or end of message padding (blank columns)
        mx.setColumn(col--, 0);
        curLen++;
        if (curLen == showLen)
          state = 0;
        break;

      default:
        col = -1;   // this definitely ends the do loop
    }
  } while (col >= (modStart * COL_SIZE));

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

//////////////////////////////////////////////////////////////////////////

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {D4, D3, D2, D1}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {D0, D10, D9}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
//////////////////////////////////////////////////////////////////////////
#define buzzPin D6
#define NOTE__c 261
#define NOTE__d 294
#define NOTE__e 329
#define NOTE__f 349
#define NOTE__g 391
#define NOTE__gS 415
#define NOTE__a 440
#define NOTE__aS 455
#define NOTE__b 466
#define NOTE__cH 523
#define NOTE__cSH 554
#define NOTE__dH 587
#define NOTE__dSH 622
#define NOTE__eH 659
#define NOTE__fH 698
#define NOTE__fSH 740
#define NOTE__gH 784
#define NOTE__gSH 830
#define NOTE__aH 880
void beep(int f, int d) {
  tone(buzzPin, f);
  delay(d);noTone(buzzPin);delay(20);
}
void alarm() {
beep(NOTE__a, 500);
beep(NOTE__a, 500);
beep(NOTE__a, 500);
beep(NOTE__f, 350);
beep(NOTE__cH, 150);
beep(NOTE__a, 500);
beep(NOTE__f, 350);
beep(NOTE__cH, 150);
beep(NOTE__a, 1000);
beep(NOTE__eH, 500);
beep(NOTE__eH, 500);
beep(NOTE__eH, 500);
beep(NOTE__fH, 350);
beep(NOTE__cH, 150);
beep(NOTE__gS, 500);
beep(NOTE__f, 350);
beep(NOTE__cH, 150);
beep(NOTE__a, 1000);
beep(NOTE__aH, 500);
beep(NOTE__a, 350);
beep(NOTE__a, 150);
beep(NOTE__aH, 500);
beep(NOTE__gSH, 250);
beep(NOTE__gH, 250);
beep(NOTE__fSH, 125);
beep(NOTE__fH, 125);
beep(NOTE__fSH, 250);

delay(250);

beep(NOTE__aS, 250);
beep(NOTE__dSH, 500);
beep(NOTE__dH, 250);
beep(NOTE__cSH, 250);
beep(NOTE__cH, 125);
beep(NOTE__b, 125);
beep(NOTE__cH, 250);

delay(250);

beep(NOTE__f, 125);
beep(NOTE__gS, 500);
beep(NOTE__f, 375);
beep(NOTE__a, 125);
beep(NOTE__cH, 500);
beep(NOTE__a, 375);
beep(NOTE__cH, 125);
beep(NOTE__eH, 1000);
beep(NOTE__aH, 500);
beep(NOTE__a, 350);
beep(NOTE__a, 150);
beep(NOTE__aH, 500);
beep(NOTE__gSH, 250);
beep(NOTE__gH, 250);
beep(NOTE__fSH, 125);
beep(NOTE__fH, 125);
beep(NOTE__fSH, 250);

delay(250);

beep(NOTE__aS, 250);
beep(NOTE__dSH, 500);
beep(NOTE__dH, 250);
beep(NOTE__cSH, 250);
beep(NOTE__cH, 125);
beep(NOTE__b, 125);
beep(NOTE__cH, 250);

delay(250);

beep(NOTE__f, 250);
beep(NOTE__gS, 500);
beep(NOTE__f, 375);
beep(NOTE__cH, 125);
beep(NOTE__a, 500);
beep(NOTE__f, 375);
beep(NOTE__c, 125);
beep(NOTE__a, 1000);
}

//////////////////////////////////////////////////////////////////////////

int tick = 0;
int limit = 60 + 30;
int roundCount = 0;

bool isPlaySound = false;

Ticker blinker;

void changeState()
{
  tick++;
  if(tick >= limit * 100){
    roundCount++;
    isPlaySound = true;
    tick = 0;
  }
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

void showClock(){
  int now = tick;
  int sec = (now/100);
  int msec = now%100;
  int minute = (sec/60);
  int show_sec = sec%60;
  if(show_sec<10){
    String timeStr = (String(minute) + " : 0" + String(show_sec));
    printText(0, MAX_DEVICES-1, string2char(timeStr));
  }else{
    String timeStr = (String(minute) + " : " + String(show_sec));
    printText(0, MAX_DEVICES-1, string2char(timeStr));
  }
}

//////////////////////////////////////////////////////////////////////////

int state;
char key;
char key1;
char key2;
char key3;

void setup(){
  mx.begin();
  blinker.attach(0.01, changeState);
  state = 0;
}
  
void loop(){
  if(state == 0) {
    if (isPlaySound) {
      String timeStr = "R : " + String(roundCount);
      printText(0, MAX_DEVICES-1, string2char(timeStr));
      isPlaySound = false;
      beep(NOTE__aH, 1000);
    }
    showClock();
    key = keypad.getKey();
    if(key) {
      beep(NOTE__aH, 50);
      if(key == '*') {
        tick = 0;
        roundCount = 0;
      }else if(key == '#') {
        state = 1;
      }
    }
  }else if(state == 1) {
    key1 = keypad.getKey();
    if(key1) {
      beep(NOTE__aH, 50);
      if(key1 == '*') {
        tick = 0;
        roundCount = 0;
        state = 0;
      }else if(key1 == '#') {
        tick = 0;
        roundCount = 0;
        state = 0;
      }else {
        printText(0, MAX_DEVICES-1, string2char((String(key1) + " : __")));
        state = 2;
      }
    }
  }else if(state == 2) {
    key2 = keypad.getKey();
    if(key2) {
      beep(NOTE__aH, 50);
      if(key2 == '*') {
        tick = 0;
        roundCount = 0;
        state = 0;
      }else if(key2 == '#') {
        tick = 0;
        roundCount = 0;
        state = 0;
      }else {
        printText(0, MAX_DEVICES-1, string2char((String(key1) + " : "+String(key2)+"_")));
        state = 3;
      }
    }
  }else if(state == 3) {
    key3 = keypad.getKey();
    if(key3) {
      beep(NOTE__aH, 50);
      if(key3 == '*') {
        tick = 0;
        roundCount = 0;
        state = 0;
      }else if(key3 == '#') {
        tick = 0;
        roundCount = 0;
        state = 0;
      }else {
        printText(0, MAX_DEVICES-1, string2char((String(key1) + " : "+String(key2)+String(key3))));
        int ki1 = (key1 - '0') * 60;
        int ki2 = (key2 - '0') * 10;
        int ki3 = (key3 - '0') * 1;
        limit = ki1 + ki2 + ki3;
        state = 4;
      }
    }
  }else if(state == 4) {
    key3 = keypad.getKey();
    if(key3) {
      beep(NOTE__aH, 50);
      if(key3 == '*') {
        tick = 0;
        roundCount = 0;
        state = 0;
      }else if(key3 == '#') {
        tick = 0;
        roundCount = 0;
        state = 0;
      }
    }
  }

}
