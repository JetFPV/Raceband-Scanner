
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

// Variables declaration
byte numCell;
int lipoAdc = 0;
float lipoVolt = 0;
byte lipoStatus;

// function to read lipoVoltage
void getlipoVolt() {
    lipoAdc = 0;
    for (uint8_t i = 0; i < LIPO_AVERAGE_READS; i++) {
        lipoAdc += analogRead(LIPO_ADC);}
    lipoVolt = lipoAdc/LIPO_AVERAGE_READS; // average of Voltage readings
    lipoVolt = (lipoVolt * VOLTAGE_CIRCUIT / 1024) / 0.18; // 0.18 from voltage divider = 220ohms / 220ohms + 1000ohms
}

// function to get number of cell from Lipo
void getNbCell(){
    // Get lipoVoltage first
    getlipoVolt();
    
    // Define number of cells
    if (lipoVolt >= 21) {
          numCell = 6;}
        else if (lipoVolt < 21 && lipoVolt >= 17 ) {
          numCell = 5;}
        else if (lipoVolt < 17 && lipoVolt >= 13 ) {
          numCell = 4;}
        else if (lipoVolt < 13 && lipoVolt >= 9.6) {
          numCell = 3;}
        else if (lipoVolt < 9.6 ) {
          numCell = 0;}
}

// function to display the status on the leds
void ShowlipoStatus() {
    Clearleds();
    getNbCell();
    // Only display status if 3s to 6s, not if power from usb for example
    if (numCell >0) {  
      getlipoVolt();
      lipoStatus = (((lipoVolt - (LIPO_MIN_V * numCell))) / (((LIPO_MAX_V * numCell) - (LIPO_MIN_V * numCell)) / NUM_LEDS));    
      if (lipoStatus <=1){
              for(int dot = 0; dot <= NUM_LEDS-1; dot++) {
                 if (dot == 0){
                 leds[dot] = CRGB::Red;
                 delay(600);
                 }   
                 else{
                 leds[dot] = CRGB::Black;
                 }
                 FastLED.show();
              }
        }          
       else if (lipoStatus <= 3) {
              for(int dot = 0; dot <= NUM_LEDS-1; dot++) {
                 if (dot < lipoStatus){
                 leds[dot] = CRGB::DarkOrange;
                 delay(300);
                 }   
                 else {
                 leds[dot] = CRGB::Black;
                 }
                 FastLED.show();
              }
        }
        
       else if (lipoStatus > 3) {
              for(int dot = 0; dot <= NUM_LEDS-1; dot++) {
                 if (dot < lipoStatus){
                 leds[dot] = CRGB::Green;
                 delay(200);
                 }   
                 else{
                 leds[dot] = CRGB::Black;}
                FastLED.show();
              }    
        }
        delay(1000);
     }
}

// Check if lipo is low voltage & make sound if so
bool CheckLipoVolt() {
    bool lipoLow = false;
    getNbCell();
    getlipoVolt();
    lipoStatus = (((lipoVolt - (LIPO_MIN_V * numCell))) / (((LIPO_MAX_V * numCell) - (LIPO_MIN_V * numCell)) / NUM_LEDS));

    if (lipoStatus <=1 && numCell > 0){
           lipoLow = true;
           for(int dot = 0; dot <= NUM_LEDS-1; dot++) {
               if (dot == 0){
               leds[dot] = CRGB::Red;
               delay(1000);
               }   
               else{
               leds[dot] = CRGB::Black;
               }
               FastLED.show();
            }
     }   
    return lipoLow;
}
