
/**
 * DIY RAceband Scanner by Jérémie Tondu (Jet FPV)
 * SPI driver based on fs_skyrf_58g-main.c by Simon Chambers
 * TVOUT by Myles Metzel

The MIT License (MIT)

Copyright (c) 2020 by Jérémie Tondu (Jet FPV)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// TVout setup
#include <avr/pgmspace.h>
#include <TVout.h>
#include <fontALL.h>

// Configuration of RX5808 VRX
#define RSSI_ADC A3 // RSSI pin on Arduino A3
#define SPI_DATA_PIN 10 // CH1 pin RX5808 => D10
#define SPI_SS_PIN 13 // CH2 pin RX5808 => D13
#define SPI_CLOCK_PIN 12 // CH3 pin RX5808 => D12
#define RSSI_AVERAGE_READS 5 // Number of ADC reads (averaged) per iteration
#define RSSI_THRESHOLD 200
#define MIN_RSSI 135 // Check RSSI when nothing is plugged
#define MAX_RSSI 270 // Check RSSI when drone is plugged next to RX5808
#define MIN_BAR_OFFSET 35
#define MAX_BAR_OFFSET 80

// Configuration of other pins
#define BUZZER_PIN 11 // Buzzer pin is defined by TVout on pin 11
#define BUTTON_M 3 // Button MODE on Pin 2
#define BUTTON_S 2 // Button SELECT on Pin 3

// Lipo monitor Input
#define LIPO_ADC A5 // Lipo voltage divider input on A5
#define LIPO_AVERAGE_READS 5 // Number of ADC reads (averaged) per iteration
#define VOLTAGE_CIRCUIT 5 // Voltage of input circuit for accurate reading of LIPO Voltage
#define LIPO_MIN_V 3.2 // Voltage minimum for each cell of lipo
#define LIPO_MAX_V 4.2 // Voltage maximum for each cell of lipo

// Global variable declaration and initialization
byte channelDetected = 1;
byte channelSelect = 1; // By default channel is 8 (only very first startup)
byte bandSelect = 0; // 0 is Raceband - Change for following : 1 Band A - 2 Band B - 3 Band E - 4 Band F - 5 Band D
uint16_t rssi = 0;
uint16_t rssiMax = 0;
uint16_t rssiScaled;
byte caddxPos = 0;
byte storedChannel;
byte channelTV;
bool lipoLow = false;
bool stateM;
bool stateO;
byte mode = 1; 
bool action = false;
bool out = false;
bool outM = false;
bool outO = false;

// LED Setup
#include "FastLED.h"
#include <avr/pgmspace.h>
#define DATA_PIN 5
#define NUM_LEDS 8
CRGB leds[NUM_LEDS];

// Include internal headers & other librairies
#include "rx5808spi.h"
#include "leds.h"
#include "lipo.h"
#include "pitches.h"
#include <EEPROM.h>

TVout TV;

//------ SETUP PART --------------------------------------------------------------------------------------

void setup() {
       
    // initialisation des pin
    pinMode(SPI_SS_PIN, OUTPUT);
    pinMode(SPI_DATA_PIN, OUTPUT);
    pinMode(SPI_CLOCK_PIN, OUTPUT);
    pinMode(BUTTON_M, INPUT_PULLUP);
    pinMode(BUTTON_S, INPUT_PULLUP);
    
    // LEDS setup
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    Clearleds();
    FastLED.show();

    // Display lipo status on LED
    ShowlipoStatus(); 

    // Start on mode 1 (manual)
    mode = 1;
  
    // Channel select on RX5808 based on EEPROM if exist & set in RX5808
    storedChannel = EEPROM.read(0);
    if ( storedChannel <=8 ){
        channelSelect = storedChannel;
    }
    setChannelModule(channelSelect, bandSelect);
    SetLed(channelSelect);

    //Setup display. (PAL/NTSC, resolution width, resolution height)
    TV.begin(PAL,128,96);
    TV.select_font(font4x6);
    TV.clear_screen();
}

//************************************* MAIN PROGRAM **********************************************************

void loop() {
  
//------------------- GET BUTTON STATUS & MODE / ACTION CHANGE ----------------

    stateM = digitalRead(BUTTON_M);
    stateO = digitalRead(BUTTON_S);

    if ((stateM == LOW && stateO == HIGH) || outM == true) {
        delay(50);
        if (mode<3) { mode++;}
          else { mode = 1; }
        SetLedMode(mode);
        delay(800);
        Clearleds();
        if (mode == 1 || mode == 2) 
        {
          setChannelModule(channelSelect, bandSelect);
          SetLed(channelSelect);
        }
        TV.draw_rect(2, 20,123, 70, BLACK, BLACK);
        outO = false;
        outM = false;
        out = false;
        }    
    else if ((stateO == LOW && stateM == HIGH)  || outO == true) {
        delay(50);
        action = true;
        outO = false;
        outM = false;
        out = false;
        }
    else { 
          action = false; 
        }

//-------------------- TV SCREEN INIT -----------------------------------------
  
      // Display border, tiltle, channel and rssi on the screen
      TV.select_font(font6x8);
      TV.print(15, 5,"RACEBAND SCANNER");
      TV.draw_rect(0,0,127,94,WHITE);
      TV.draw_line(0,14,127,14,WHITE);

       channelTV = 48 + channelSelect;
       TV.print(10, 22, "MODE : ");
       TV.draw_rect(90,22,35,8,BLACK, BLACK);
       if (mode == 1) { TV.print(50, 22, "MANUAL"); } 
       if (mode == 2) { TV.print(50, 22, "AUTO"); }
       if (mode == 3) { TV.print(50, 22, "SCAN"); }

       if (mode == 1 || mode == 2)
       {
          rssi = GetRssi();
          TV.print(10, 32, "CHANNEL : R");
          TV.print(78, 32, channelTV);
          rssi = GetRssi();
          TV.print(10, 42, "RSSI : ");
          rssi = constrain(rssi, MIN_RSSI, MAX_RSSI);
          rssiScaled = map(rssi, MIN_RSSI, MAX_RSSI, 0, 100);
          TV.print(50, 42, rssi);// Display raw number 
          //TV.print(105, 42, "%"); // Display RSSI in %
       }
                
 //--------------- MODE 1 = MANUAL CHANGE OF RACEBAND CHANNEL -----------------  
      if (mode == 1)
      {
          if (action == true)
              {
              channelSelect++;
              if (channelSelect == 9) { channelSelect = 1; }
              setChannelModule(channelSelect, bandSelect);
              SetLed(channelSelect);
              TV.tone(NOTE_G4,35);
              delay(35);
              TV.tone(NOTE_G5,35);
              delay(35);
              TV.tone(NOTE_G6,35);
              delay(35);
              TV.noTone(); 
              }      
          }
 //--------------- MODE 2 = AUTOMATIC : AUTO LOCK ON MAX RSSI DETECTED ---------
  
      if (mode == 2 && action == true)
      {
        // Detect Raceband Channel with higher RSSI & Set RX5808 to this channel
        channelDetected = ChannelMaxRssi();
        if ( (channelDetected != channelSelect) && (channelDetected != 0) )
        {
            channelSelect = channelDetected;
            setChannelModule(channelSelect, bandSelect);
            SetLed(channelSelect);
            TV.tone(NOTE_E6,125);
            delay(130);
            TV.tone(NOTE_G6,125);
            delay(130);
            TV.tone(NOTE_E7,125);
            delay(130);
            TV.tone(NOTE_C7,125);
            delay(130);
            TV.tone(NOTE_D7,125);
            delay(130);
            TV.tone(NOTE_G7,125);
            delay(125);
            TV.noTone();
        }
        else {
            setChannelModule(channelSelect, bandSelect);
            SetLed(channelSelect);
        }
      }
 //--------------- MODE 3 = SCANNER MODE -------------------------------------
   
      if (mode == 3)
      {
              TV.draw_rect(2, 30,123, 60, BLACK, BLACK);          
              for (byte i = 1; i < 9; i++) 
                  {
                  channelTV = 48 + i;
                  TV.select_font(font6x8);  
                  TV.print((15*i)-9, 85, "R");
                  TV.print((15*i)-3, 85, channelTV);
                  rssi = RaceBandScan(i);
                  if (rssi < RSSI_THRESHOLD) { 
                        leds[i-1] = CRGB::Green; }
                  else {
                        leds[i-1] = CRGB::Red; }
                  }
              outM = false;
              outO = false;
              out = false;
              FastLED.show();
              do {
               for (byte i = 1; i < 9; i++) 
                {
                    stateM = digitalRead(BUTTON_M);
                    stateO = digitalRead(BUTTON_S);
                    
                    if (stateM == LOW){ 
                      outM = true;}
                      else   {
                      outM = false;}
                    if (stateO == LOW){
                      outO = true;}
                      else   {
                      outO = false;}
                    if (outM == true || outO == true)
                    {
                        out = true;
                    }
                    rssi = RaceBandScan(i);     
                    rssi = constrain(rssi, MIN_RSSI, MAX_RSSI); 
                    rssiScaled = map(rssi, MIN_RSSI, MAX_RSSI, MAX_BAR_OFFSET, MIN_BAR_OFFSET);                  
                    TV.draw_rect((15*i)-9, MIN_BAR_OFFSET, 10, MAX_BAR_OFFSET-MIN_BAR_OFFSET, BLACK, BLACK);
                    TV.draw_rect((15*i)-9, rssiScaled , 10, MAX_BAR_OFFSET-rssiScaled, WHITE, WHITE);                 
                }
              }while (out == false);              
    }      
//-----------LIPO VOLTAGE CHECK & WRITE ON EEPROM  ----------------------------
     
      // Check if LIPO Voltage is low    
      lipoLow = CheckLipoVolt();
      if (lipoLow == true) {
          TV.tone(850, 200);
          delay (600);
          TV.tone(850, 200);
          delay (600);
      };
      EEPROM.write(0, channelSelect);
      TV.delay_frame(10);

}
