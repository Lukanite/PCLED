/*
 * WS2812 display using SPI on various TI LaunchPads with Energia
 *
 * Connections:
 *   LaunchPad      LED Strip
 *   ---------      ---------
 *   3V3            5VDC
 *   Pin 15 (MOSI)  DIN
 *   GND            GND
 *
 * How to use:
 *  ledsetup ();         - Get ready to send.
 *                         Call once at the beginning of the program.
 *  sendPixel (r, g, ; - Send a single pixel to the string.
 *                         Call this once for each pixel in a frame.
 *                         Each colour is in the range 0 to 255. Turn off 
 *                         interrupts before use and turn on after all pixels
 *                         have been programmed.
 *  show ();             - Latch the recently sent pixels onto the LEDs .
 *                         Call once per frame.
 *  showColor (count, r, g, ; - Set the entire string of count Neopixels
 *                         to this one colour. Turn off interrupts before use
 *                         and remember to turn on afterwards.
 *
 * Derived from NeoPixel display library by Nick Gammon
 * https://github.com/nickgammon/NeoPixels_SPI
 * With ideas from:
 * http://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
 * Released for public use under the Creative Commons Attribution 3.0 Australia License
 * http://creativecommons.org/licenses/by/3.0/au/
 *
 * F Milburn  November 2016
 * Tested with Energia V17 and WS2812 8 pixel strip on launchpads shown below.
 */
#include <SPI.h>
#include "color.h"

#if defined(__MSP430G2553)
  #define SPIDIV     SPI_CLOCK_DIV2       // 16 MHz/2 gives 125 ns for each on bit in byte
  #define SPILONG    0b11111100           // 750 ns (acceptable "on" range 550 to 850 ns)
  #define SPISHORT   0b11100000           // 375 ns (acceptable "on" range 200 to 500 ns)
#elif defined(__MSP430F5529)
  #define SPIDIV     SPI_CLOCK_DIV4       // 25.6 MHz/4 gives 156.25 ns for each on bit in byte
  #define SPILONG    0b11110000           // 625 ns (acceptable "on" range 550 to 850 ns)
  #define SPISHORT   0b11000000           // 312.5 ns (acceptable "on" range 200 to 500 ns)
#else
  #error This microcontroller is not supported
#endif

#define PIXELS 96 // Pixels in the strip

color pixbuf[PIXELS];
String content = "";
char validchar[11] = {'0','1','2','3','4','5','6','7','8','9','.'};

int prev = 0;
int cur = 0;
int checkval = 0;
int checkval2 = 0;

char character;
int mode = 0;
//Color Wheel - Wheel 0-784, brightness 0-256
color WheelBright(uint16_t WheelPos, uint8_t brightness)
{
  uint16_t r, g, b;
  color res;
  switch(WheelPos / 256)
  {
    case 0:
      r = (255 - WheelPos % 256);   //Red down
      g = (WheelPos % 256);      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = (255 - WheelPos % 256);  //green down
      b = (WheelPos % 256);      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = (255 - WheelPos % 256);  //blue down 
      r = (WheelPos % 256);      //red up
      g = 0;                  //green off
      break; 
  }
  res.r = r*brightness >> 8;
  res.g = g*brightness >> 8;
  res.b = b*brightness >> 8;
  return(res);
}


bool isvalueinarray(char val, char *arr, int size){
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return true;
    }
    return false;
}

// Sends one byte to the LED strip by SPI.
void sendByte (unsigned char b){
    for (unsigned char bit = 0; bit < 8; bit++){
      if (b & 0x80) // is high-order bit set?
        SPI.transfer (SPILONG);     // long on bit (~700 ns) defined for each clock speed
      else
        SPI.transfer (SPISHORT);    // short on bit (~350 ns) defined for each clock speed
      b <<= 1;                      // shift next bit into high-order position
    } // end of for each bit
} // end of sendByte

// Wait long enough without sending any bits to allow the pixels to latch and
// display the last sent frame
void show(){
  delayMicroseconds (9);
} // end of show


// Send a single pixel worth of information.  Turn interrupts off while using.
void sendPixel (unsigned char r, unsigned char g, unsigned char b) {
  sendByte (g);        // NeoPixel wants colors in green-then-red-then-blue order
  sendByte (r);
  sendByte (b);
} // end of sendPixel

// Set up SPI
void ledsetup(){
  SPI.begin ();
  SPI.setClockDivider (SPIDIV);  // defined for each clock speed
  SPI.setBitOrder (MSBFIRST);
  SPI.setDataMode (SPI_MODE1);   // MOSI normally low.
  show ();                       // in case MOSI went high, latch in whatever-we-sent
  sendPixel (0, 0, 0);           // now change back to black
  show ();                       // and latch that
}  // end of ledsetup



// Display a single color on the whole string.  Turn interrupts off while using.
void showColor (unsigned int count, unsigned char r , unsigned char g , unsigned char b) {
  noInterrupts ();
  for (unsigned int pixel = 0; pixel < count; pixel++)
    sendPixel (r, g, b);
  interrupts ();
  show ();  // latch the colours
} // end of showColor

//Start ANIMATIONS
void pulsedyn(int start, int end) {
  if (end>100) end=100; //Bounds check
  if (start > 100) start=100;
  int diff = end-start;
  for (int j=0; j<5; j++) {  //do 5 cycles of chasing
    for (int q=0; q < 6; q++) {
      uint16_t curval = start + (diff * (j*6+q) /(5*6));
      uint16_t curcolor = (100-curval)*(100-curval)*5/100; //Close enough for blue idle
      for (int i=0; i < PIXELS; i++) pixbuf[i] = WheelBright(curcolor,((i-q+6)%6)*42);
      noInterrupts();
      for (int i=0; i < PIXELS; i++) {
        sendPixel(pixbuf[i].r,pixbuf[i].g,pixbuf[i].b);   
      }
      interrupts();
      show();
      if (j==4 && q==5) {
          Serial.print("Done: ");
          Serial.print(checkval2);
          Serial.print("\r\n");
      }
      delay(105-curval); //Speed up when hot
    }
  }
}
void setup (){
    ledsetup();
    Serial.setTimeout(5);
    Serial.begin(9600);
    Serial.print("Online!");
    pinMode(GREEN_LED, OUTPUT);
    digitalWrite(GREEN_LED, HIGH);
} 

void loop (){
    while(Serial.available() > 2) {
        character = Serial.read();
        if (character == '*' && content != "") { //End flag
            checkval2 = content.toInt();
            if (checkval2 == checkval2) {
                cur = checkval;
                //Serial.print("Received: ");
                //Serial.print(cur);
                //Serial.print("\r\n");
                //Scale cur to 0-100
                cur = (cur-50)*2; //IDLE at 50C
                if (cur < 0) cur = 0;
                if (cur > 100) cur = 100;
            }
            else Serial.print("Rejected data");
            content = "";
        } else if (character == '^') {
            checkval = content.toInt();
            content = "";
        } else if (isvalueinarray(character,validchar,11)) {
            content.concat(character);
        } else if (character == '&') {
            mode = mode + 1;
            if (mode > 2) mode = 1; //MAX MODE GOES HERE
            Serial.print("Mode is now: ");
            Serial.print(mode);
            Serial.print("\r\n");
        }        
    }
    if (mode == 1) pulsedyn(prev, cur);
    else {
        //Off
        noInterrupts();                        // no interrupts while sending data
        showColor (PIXELS, 0x00, 0x00, 0x00);  // single color on entire strip
        interrupts(); 
        delay(1000);
    }
    prev = cur;
    
  /*
  // Show a solid color across the strip
  noInterrupts();                        // no interrupts while sending data
  showColor (PIXELS, 0xBB, 0x22, 0x22);  // single color on entire strip
  interrupts();                          // interrupts are OK now
  delay(1000);                           // hold it for a second
  
  // Show a different color on every pixel
  noInterrupts();                        // no interrupts while sending data
  sendPixel(0xBB, 0x00, 0x00);           // red
  sendPixel(0x00, 0xBB, 0x00);           // green
  sendPixel(0x00, 0x00, 0xBB);           // blue
  sendPixel(0xBB, 0xBB, 0xBB);           // white
  sendPixel(0xBB, 0x22, 0x22);           // pinkish
  sendPixel(0x22, 0xBB, 0x22);           // light green
  sendPixel(0x22, 0x22, 0xBB);           // purplish blue
  sendPixel(0x00, 0x00, 0x00);           // pixel off
  interrupts();                          // interrupts are OK now
  delay(1000);                           // hold it for a second                      
  */
} 
 