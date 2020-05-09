
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

const uint16_t channelTable[] PROGMEM = {
    // Channel 1 - 8
    0x281D, 0x288F, 0x2902, 0x2914, 0x2987, 0x2999, 0x2A0C, 0x2A1E, // Raceband
    0x2A05, 0x299B, 0x2991, 0x2987, 0x291D, 0x2913, 0x2909, 0x289F, // Band A
    0x2903, 0x290C, 0x2916, 0x291F, 0x2989, 0x2992, 0x299C, 0x2A05, // Band B
    0x2895, 0x288B, 0x2881, 0x2817, 0x2A0F, 0x2A19, 0x2A83, 0x2A8D, // Band E
    0x2906, 0x2910, 0x291A, 0x2984, 0x298E, 0x2998, 0x2A02, 0x2A0C, // Band F / Airwave
    0x2609, 0x261C, 0x268E, 0x2701, 0x2713, 0x2786, 0x2798, 0x280B // Band D / 5.3
};

const uint16_t channelFreqTable[] PROGMEM = {
    // Channel 1 - 8
    5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917, // Raceband
    5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // Band A
    5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // Band B
    5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // Band E
    5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // Band F / Airwave
    5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621 // Band D / 5.3
};

void SERIAL_SENDBIT1() {
    digitalWrite(SPI_CLOCK_PIN, LOW);
    delayMicroseconds(1);

    digitalWrite(SPI_DATA_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(SPI_CLOCK_PIN, HIGH);
    delayMicroseconds(1);

    digitalWrite(SPI_CLOCK_PIN, LOW);
    delayMicroseconds(1);
}

void SERIAL_SENDBIT0() {
    digitalWrite(SPI_CLOCK_PIN, LOW);
    delayMicroseconds(1);

    digitalWrite(SPI_DATA_PIN, LOW);
    delayMicroseconds(1);
    digitalWrite(SPI_CLOCK_PIN, HIGH);
    delayMicroseconds(1);

    digitalWrite(SPI_CLOCK_PIN, LOW);
    delayMicroseconds(1);
}

void SERIAL_ENABLE_LOW() {
    delayMicroseconds(1);
    digitalWrite(SPI_SS_PIN, LOW);
    delayMicroseconds(1);
}

void SERIAL_ENABLE_HIGH() {
    delayMicroseconds(1);
    digitalWrite(SPI_SS_PIN, HIGH);
    delayMicroseconds(1);
}

void setChannelModule(uint8_t channel, uint8_t band) {
    uint8_t i;
    uint16_t channelData;

    channelData = pgm_read_word_near(channelTable + channel-1 + (8 * band));

    // bit bang out 25 bits of data
    // Order: A0-3, !R/W, D0-D19
    // A0=0, A1=0, A2=0, A3=1, RW=0, D0-19=0
    SERIAL_ENABLE_HIGH();
    delayMicroseconds(1);
    SERIAL_ENABLE_LOW();

    SERIAL_SENDBIT0();
    SERIAL_SENDBIT0();
    SERIAL_SENDBIT0();
    SERIAL_SENDBIT1();

    SERIAL_SENDBIT0();

    // remaining zeros
    for (i = 20; i > 0; i--) {
        SERIAL_SENDBIT0();
    }

    // Clock the data in
    SERIAL_ENABLE_HIGH();
    delayMicroseconds(1);
    SERIAL_ENABLE_LOW();

    // Second is the channel data from the lookup table
    // 20 bytes of register data are sent, but the MSB 4 bits are zeros
    // register address = 0x1, write, data0-15=channelData data15-19=0x0
    SERIAL_ENABLE_HIGH();
    SERIAL_ENABLE_LOW();

    // Register 0x1
    SERIAL_SENDBIT1();
    SERIAL_SENDBIT0();
    SERIAL_SENDBIT0();
    SERIAL_SENDBIT0();

    // Write to register
    SERIAL_SENDBIT1();

    // D0-D15
    //   note: loop runs backwards as more efficent on AVR
    for (i = 16; i > 0; i--) {
        // Is bit high or low?
        if (channelData & 0x1) {
            SERIAL_SENDBIT1();
        }
        else {
            SERIAL_SENDBIT0();
        }
        // Shift bits along to check the next one
        channelData >>= 1;
    }

    // Remaining D16-D19
    for (i = 4; i > 0; i--) {
        SERIAL_SENDBIT0();
    }

    // Finished clocking data in
    SERIAL_ENABLE_HIGH();
    delayMicroseconds(1);

    digitalWrite(SPI_SS_PIN, LOW);
    digitalWrite(SPI_CLOCK_PIN, LOW);
    digitalWrite(SPI_DATA_PIN, LOW);
}

byte ChannelMaxRssi() {
    byte Channel = 0;
    int   rssiMax = 0;
    for (byte i = 0; i < 8; i++) 
    {
      rssi = 0;
      setChannelModule(i, 0);
      // Leave the time to switch frequency to RX5808
      delay (30);
      for (int j = 0; j < RSSI_AVERAGE_READS; j++) {
        rssi += analogRead(RSSI_ADC);
      }
      rssi = rssi / RSSI_AVERAGE_READS;
      //Serial.println(rssi);
      if (rssi > rssiMax && rssi > RSSI_THRESHOLD) {
          Channel = i;
          rssiMax = rssi;
          }
    }
  return Channel;
} 

int RaceBandScan(byte channel) {
      setChannelModule(channel, bandSelect);
      // Leave the time to switch frequency to RX5808
      delay (30);
      rssi = 0;
      for (int j = 0; j < RSSI_AVERAGE_READS; j++) {
        rssi += analogRead(RSSI_ADC);
      }
      rssi = rssi / RSSI_AVERAGE_READS;
  return rssi;
} 

int GetRssi() {
    rssi = 0;
    for (int i = 0; i < RSSI_AVERAGE_READS; i++) {
        rssi += analogRead(RSSI_ADC);
    }
    rssi = rssi / RSSI_AVERAGE_READS;
  return rssi;
}
