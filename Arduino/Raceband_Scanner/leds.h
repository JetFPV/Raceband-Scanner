
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

void Clearleds() {
  for(byte dot = 0; dot < NUM_LEDS; dot++) 
  { 
            leds[dot] = CRGB::Black;
            FastLED.show();
  }
}

void SetLed(byte Channel) {
    Clearleds();
    for(byte dot = 1; dot <= NUM_LEDS; dot++) {
      if (dot == Channel && Channel > 0) {
           leds[dot-1] = CRGB::Blue; }
           else{
           leds[dot-1] = CRGB::Black;}
    FastLED.show();
    }
}

void SetLedMode(byte mode) {
    Clearleds();
    for(byte dot = 1; dot <= NUM_LEDS; dot++) {
      if (dot <= mode) {
           leds[dot-1] = CRGB::Purple; }
           else{
           leds[dot-1] = CRGB::Black;}          
    }
    FastLED.show();
}
