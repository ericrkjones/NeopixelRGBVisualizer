// Import NicoHood's MSGEQ7 driver
#include <MSGEQ7.h>
#include <EEPROM.h>
//
#include "MSGEQ7NeopixelTree4.h"
/*
  Persistant customization variables
*/

#define CONFIG_DELAY 50
// Length of the Neopixel display
unsigned long length = 50;
const unsigned long defaultlength = 50;
const int length_eepromaddress = 0;
// Global input linear gain as a fraction of 255
unsigned long gain = 240;
const int gain_eepromaddress = 4;
const long defaultgain = 240;
// Maximum and minimum brightness per channel
byte maxbrightness = 100; 
const byte defaultmaxbrightness = 100;
byte minbrightness = 5;
const byte defaultminbrightness = 5;
// Color 1 for use in fixed-color displays, RGB order
word hue1 = 0;
const int defaulthue1 = 0;
// Color 2 for use in fixed color displays, RGB order
word hue2 = 255;
const byte defaulthue2 = 255;
/*
  Permutation index of RGB for all-color displays.
  0: RGB
  1: GBR
  2: BRG
  3: RBG
  4: GRB
  5: BGR
  Notice that 0-2 are rotated RGB order and 3-5 are rotated RBG order
  So, for any moving displays that use a fade through all colors,
  all that matters is those ranges.
*/
byte rgbpermutationindex;
const byte defaultrgbperumtationindex = 0;
// Speed for scrolling displays in units of milliseconds/dot
byte speed;
const byte defaultspeed = 30;
// 0-index point for bar-style displays
unsigned long startpoint;
const long defaultstartpoint = 0;
// Operating mode
byte mode = 0;

// Global variables

// Define Neopixel connection
Adafruit_NeoPixel strip = Adafruit_NeoPixel(length, STRIP_PIN, NEO_RGB + NEO_KHZ800);

// Define MSGEQ7 connection
CMSGEQ7<MSGEQ7_SMOOTH, MSGEQ7_PINRESET, MSGEQ7_PINSTROBE, MSGEQ7_PINDCIN> MSGEQ7;

// Frequency volume values
byte freqVolume[MSGEQ7_FREQCOUNT];
long meanVolume = 0;

// Button variables
bool button_mode;
bool button_mode_last = 0;
bool button_mode_rising = 0;
bool button_mode_falling = 0;
unsigned long button_mode_start_time = 0;
unsigned long button_mode_held_time;
bool button_minus;
bool button_minus_last = 0;
bool button_minus_rising = 0;
bool button_minus_falling = 0;
unsigned long button_minus_start_time = 0;
unsigned long button_minus_held_time;
bool button_plus;
bool button_plus_last = 0;
bool button_plus_rising = 0;
bool button_plus_falling = 0;
unsigned long button_plus_start_time = 0;
unsigned long button_plus_held_time;
bool button_custom;
bool button_custom_last = 0;
bool button_custom_rising = 0;
bool button_custom_falling = 0;
unsigned long button_custom_start_time = 0;
unsigned long button_custom_held_time;
unsigned long config_fastadd_start_time = 0;
unsigned long current_time = 0;

void readbuttons() {
  button_mode = ! digitalRead(BUTTON_MODE);
  button_minus = ! digitalRead(BUTTON_MINUS);
  button_plus = ! digitalRead(BUTTON_PLUS);
  button_custom = ! digitalRead(BUTTON_CUSTOM);
  // Set rising edge booleans
  if (button_mode && ! button_mode_last) {
    button_mode_rising = 1;
  } else {
    if (!button_mode && button_mode_last) {
      button_mode_falling = 1;
    } else {
      button_mode_rising = 0;
      button_mode_falling = 0;
    }
  }
  if (button_minus && ! button_minus_last) {
    button_minus_rising = 1;
  } else {
    if (!button_minus && button_mode_last) {
      button_mode_falling = 1;
    } else {
      button_minus_rising = 0;
      button_minus_falling = 0;
    }
  }
  if (button_plus && ! button_plus_last) {
    button_plus_rising = 1;
  } else {
    if (!button_plus && button_mode_last) {
      button_mode_falling = 1;
    } else {
      button_plus_rising = 0;
      button_plus_falling = 0;
    }
  }
  if (button_custom && ! button_custom_last) {
    button_custom_rising = 1;
  } else {
    if (!button_custom && button_mode_last) {
      button_mode_falling = 1;
    } else {
      button_custom_rising = 0;
      button_custom_falling = 0;
    }
  }
  // Set the last time that the buttons were not pressed
  // Calculate difference in time between now and when buttons were last not pressed
  if (button_mode) {
    button_mode_held_time = current_time - button_mode_start_time;
  } else {
    button_mode_start_time = current_time;
    button_mode_held_time = 0;
  }
  if (button_minus) {
    button_minus_held_time = current_time - button_minus_start_time;
  } else {
    button_minus_start_time = current_time;
    button_minus_held_time = 0;
  }
  if (button_plus) {
    button_plus_held_time = current_time - button_plus_start_time;
  } else {
    button_plus_start_time = current_time;
    button_plus_held_time = 0;
  }
  if (button_custom) {
    button_custom_held_time = current_time - button_custom_start_time;
  } else {
    button_custom_start_time = current_time;
    button_custom_held_time = 0;
  }
  // Set loop variables
  button_mode_last = button_mode;
  button_minus_last = button_minus;
  button_plus_last = button_plus;
  button_custom_last = button_custom;
}

byte coerceToByte(long x) {
  if (x < 0) {
    x = 0;
  } else {
    if (x > 255) {
      x = 255;
    }
  }
  return byte(x);
}

unsigned long eepromReadLong(int address) {
  long value;
  for (int i = 0; i < 4; i++) {
    value = value << 8;
    value += EEPROM.read(address + i);
  }
  return value;
}

void eepromWriteLong(int address, unsigned long value) {
  for (int i = 0; i < 4; i++) {
    EEPROM.update(address + 3 - i, value >> (i * 8));
  }
}

void updateFreqVolume() {
  for (int i = 0; i < MSGEQ7_FREQCOUNT; i++) {
#ifdef MSGEQ7_MAPNOISE
    freqVolume[i] = mapNoise(MSGEQ7.get(i));
#else
    freqVolume[i] = MSGEQ7.get(i);
#endif
  }
}

long getMeanVolume() {
  long y = (MSGEQ7.getVolume() + GAIN_OFFSET) * gain / 255;
  if (y < 0) {
    y = 0;
  }
  return y;
}

void readAudio() {
  updateFreqVolume();
  meanVolume = getMeanVolume();
}

void startNeopixels() {
  strip = Adafruit_NeoPixel(length, STRIP_PIN, NEO_RGB + NEO_KHZ800);
  strip.begin();
  strip.show();
}

// RGBVu functions:

int   rgbvu_red0   = 0;
int rgbvu_green0   = 0;
int  rgbvu_blue0   = 0;
int   rgbvu_red1[] = {70, 70, 30, 0, 0, 0, 0};
int rgbvu_green1[] = {0, 0, 40, 60, 40, 0, 0};
int  rgbvu_blue1[] = {0, 0, 0, 0, 30, 60, 60};

// RGB values
byte rgbvu_red = 0;
byte rgbvu_green = 0;
byte rgbvu_blue = 0;

long rgbvu_calculateColor(byte sample[], int color1[]) {
  long y = 0;
  for (int i = 0; i < MSGEQ7_FREQCOUNT; i++) {
    y += sample[i] * color1[i];
  }
  return y;
}

void rgbvu_normalizeColors() {
  long r = rgbvu_calculateColor(freqVolume, rgbvu_red1) / 255 + rgbvu_red0;
  long g = rgbvu_calculateColor(freqVolume, rgbvu_green1) / 255 + rgbvu_red0;
  long b = rgbvu_calculateColor(freqVolume, rgbvu_blue1) / 255 + rgbvu_red0;
  long sum = r + g + b;
  rgbvu_red = coerceToByte(maxbrightness * r / sum);
  rgbvu_green = coerceToByte(maxbrightness * g / sum);
  rgbvu_blue = coerceToByte(maxbrightness * b / sum);
}
void rgbvu_output() {
  for (int i = 0; i < length; i++) {
    if (i < meanVolume) {
      strip.setPixelColor(i, strip.Color(rgbvu_red, rgbvu_green, rgbvu_blue));
    } else {
      strip.setPixelColor(i, strip.Color(minbrightness, minbrightness, minbrightness));
    }
  }
}

void visualizer_rgbvu() {
  // 0th order polynomial coefficient
  // This is not broken out because, when the dependent terms are summed, that would waste time
  rgbvu_normalizeColors();
  rgbvu_output();
  strip.show();
}

void visualizer_config_rgbvu(byte color[]) {
  // 0th order polynomial coefficient
  // This is not broken out because, when the dependent terms are summed, that would waste time
  rgbvu_normalizeColors();
  rgbvu_output();
  strip.setPixelColor(0, strip.Color(color[0], color[1], color[2]));
  strip.show();
}

void setup() {
  // Set pinmode for buttons
  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(BUTTON_MINUS, INPUT_PULLUP);
  pinMode(BUTTON_PLUS, INPUT_PULLUP);
  pinMode(BUTTON_CUSTOM, INPUT_PULLUP);

  //  // Read EEPROM
  length = eepromReadLong(length_eepromaddress);
  gain = eepromReadLong(gain_eepromaddress);

  if (length > 65534) {
    length = defaultlength;
    eepromWriteLong(length_eepromaddress, length);
  }
  if (gain > 1020) {
    gain = defaultgain;
    eepromWriteLong(gain_eepromaddress, gain);
  }

  
  // Start Neopixels
  startNeopixels();

  // Initialize MSGEQ7
  MSGEQ7.begin();
}


//Rainbow Wheel

unsigned int hue = 0;

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(unsigned int hue) {
  if (hue > 765) {
    hue -= 765;
  }
  if (hue < 255) {
    return strip.Color(normalizeColor(255 - hue), normalizeColor(hue), 0);
  }
  if (hue < 510) {
    hue -= 255;
    return strip.Color(0, normalizeColor(255 - hue), normalizeColor(hue));
  }
  hue -= 510;
  return strip.Color(normalizeColor(hue), 0, normalizeColor(255 - hue));
}

byte normalizeColor(byte color) {
  word colorout = color * maxbrightness / 255;
  return colorout;
}

void rainbow() {
  for (unsigned int i = 0; i < length; i++) {
    strip.setPixelColor(i, Wheel(length - i - 1 + hue));
  }
  strip.show();
  if (hue == 764) {
    hue = 0;
  } else {
    hue ++;
  }
  delay(10);
}

bool statedelay(unsigned long delaytime) {
  if (current_time - config_fastadd_start_time > delaytime) {
    config_fastadd_start_time = current_time;
    return true;
  } else {
    return false;
  }
}

void loop() {
  current_time = millis();
  bool newReading = MSGEQ7.read();
  readbuttons();
  if (newReading) {
    readAudio();
  }
  if (button_mode_held_time > 3000) {
    mode = 253;
  }
  switch (mode) {
    case 0:
      if (button_mode_rising) {
        mode ++;
      } else {
        if (newReading) {
          visualizer_rgbvu();
        }
      }
      break;
    case 1:
      if (button_mode_rising) {
        mode ++;
      } else {
        if (newReading) {
          rainbow();
        }
      }
      break;
    // Global Length Setting
    case 253:
      if (button_mode_rising) {
        mode ++;
      } else {
        if (button_plus &&  button_minus) {
          if (button_plus_held_time > 5000 && button_minus_held_time > 3000) {
            length = defaultlength;
            startNeopixels();
          }
        } else {
          if (length < 65534) {
            if (button_plus_rising || button_plus_held_time > 3000) {
              if (statedelay(CONFIG_DELAY)) {
                length ++;
                startNeopixels();
              }
            }
          }
          if (length > 1) {
            if (button_minus_rising || button_minus_held_time > 3000) {
              if (statedelay(CONFIG_DELAY)) {
                length --;
                strip.setPixelColor(length, strip.Color(0, 0, 0));
                strip.show();
                startNeopixels();
              }
            }
          }
        }
      }
      if (newReading) {
        byte configcolor[] = {250, 0, 0};
        visualizer_config_rgbvu(configcolor);
      }
      break;
    // Global gain setting
    case 254:
      if (button_mode_rising) {
        mode ++;
      } else {
        if (button_plus &&  button_minus) {
          if (button_plus_held_time > 5000 && button_minus_held_time > 3000) {
            gain = defaultgain;
          }
        } else {
          if (gain < 1020) {
            if (button_plus_rising || button_plus_held_time > 500) {
              if (statedelay(CONFIG_DELAY)) {
                gain ++;
              }
            }
          }
          if (gain > 1) {
            if (button_minus_rising || button_minus_held_time > 500) {
              if (statedelay(CONFIG_DELAY)) {
                gain --;
              }
            }
          }
        }
      }
      if (newReading) {
        byte configcolor[] = {0, 250, 0};
        visualizer_config_rgbvu(configcolor);
      }
      break;
    case 255:
      eepromWriteLong(length_eepromaddress, length);
      eepromWriteLong(gain_eepromaddress, gain);
      mode ++;
      break;
    default:
      mode = 0;
  }
}
