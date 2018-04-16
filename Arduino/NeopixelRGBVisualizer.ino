/*
  Neopixel RGB Visualizer
  Controls a single strand of individually addressable Neopixel-compatible 
  RGB LEDs as a visualization of an audio input.  The frequency balance of 
  the audio input determines the active color of the display.  Depending 
  on the mode configuration, the total volume determines the height of the 
  filled bar.  

  Dependencies:
    Adafruit Neopixel Library: https://github.com/adafruit/Adafruit_NeoPixel
    NicoHood's MSGEQ7 library: https://github.com/NicoHood/MSGEQ7

  The color output can be adjusted by modifying the color scaling 
  configurations.  The default outputs red for low frequencies, green for 
  middle frequencies, and blue for high frequencies.  

*/

// Import Adafruit NeoPixel library
#include <Adafruit_NeoPixel.h>

// Import NicoHood's MSGEQ7 library
#include <MSGEQ7.h>

// Enable serial communication for debugging and define baud rate
// #define DEBUG 115200



// Mode configuration settings

// Used a fixed brightness level instead of a varying brightness.
// Color values are calculated so that the red, green, and blue outputs
// are scaled equally so that their sum is equal to FIXEDBRIGHTNESS.  
#define FIXEDBRIGHTNESS 110

// Fill a lower section of the Neopixel strip sized as a function of the 
// total input volume with the active color and the rest with the 
// inactive color
#define VOLUMEBAR



// Neopixel Configuration

// Number of LED elements in the strip
#define NEOPIXEL_LENGTH 100

// Define the pin used to send the Neopixel control signal
#define NEOPIXEL_PIN 2

// Neopixel mode, multiple options added together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
#define NEOPIXEL_MODE NEO_RGB + NEO_KHZ800


// MSGEQ7 Configuration

// Input pin for DC output of MSGEQ7 IC
#define MSGEQ7_DC_PIN A0

// Output pin for MSGEQ7 strobe signal
#define MSGEQ7_STROBE_PIN 3

// Output pin for MSGEQ7 reset signal
#define MSGEQ7_RESET_PIN 4

// Enable default noise mapping to remove noise inherent to the MSGEQ7
#define MSGEQ7_MAPNOISE

// Smooth the frequency inputs using an average weighted with
// MSGEQ7_SMOOTH/255 of the previous value
// value = ((MSGEQ7_SMOOTH * last) + ((255 - MSGEQ7_SMOOTH) * this)) / 255
#define MSGEQ7_SMOOTH 10

// Number of frequencies sampled by the DC input (7 for the MSGEQ7)
#define MSGEQ7_FREQCOUNT 7



// Define globals

// Color scaling configuration
// The active color values will be the sum of products of the frequency
// volumes and their 1st order coefficients plus the 0th order coefficient.  

// 0th order coefficients
const int   red0   = 0;
const int green0   = 0;
const int  blue0   = 0;

// 1st order coefficients
const int   red1[] = {40,60,30,0,0,0,0};
const int green1[] = {0,0,30,60,30,0,0};
const int  blue1[] = {0,0,0,0,30,60,40};


// Volume bar configuration
#ifdef VOLUMEBAR

// Inactive color
const byte redOff=10;
const byte greenOff=10;
const byte blueOff=10;

// Volume bar scaling configuration 0th and 1st order coefficients
const long volume0=-20;
const long volume1=240;
#endif



// Initialize Neopixel strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOPIXEL_LENGTH, NEOPIXEL_PIN, NEOPIXEL_MODE);

// Initialize MSGEQ7
CMSGEQ7<MSGEQ7_SMOOTH, MSGEQ7_RESET_PIN, MSGEQ7_STROBE_PIN, MSGEQ7_DC_PIN> MSGEQ7;

// Frequency volume values
byte freqVolume[MSGEQ7_FREQCOUNT];

// RGB values
byte red=0;
byte green=0;
byte blue=0;


// Initialize subsystems
void setup() {
  // Initialize Neopixel strip and turn all the elements off
  strip.begin();
  strip.show();

  // Initialize the MSGEQ7 IC
  MSGEQ7.begin();

  // If debugging, initialize serial communication and send headers
  #ifdef DEBUG  
  Serial.begin(DEBUG);
  String headers="";
  for(int i=0; i<MSGEQ7_FREQCOUNT; i++){
    headers+="Freq"+String(i)+" ";
  }
  headers+="R G B";
  Serial.println(headers);
  #endif
}

// Read and store MSGEQ7 output
void updateFreqVolume(){
  MSGEQ7.read();
  for(int i=0; i<MSGEQ7_FREQCOUNT; i++){
    #ifdef MSGEQ7_MAPNOISE
    freqVolume[i]=mapNoise(MSGEQ7.get(i));
    #else
    freqVolume[i]=MSGEQ7.get(i);
    #endif
  }
}

// 1st-order scale of frequencies to color values
long calculateColorValue(byte sample[], int color1[]){
  long y=0;
  for(int i=0; i<MSGEQ7_FREQCOUNT; i++){
    y+=sample[i]*color1[i];
  }
  return y;
}

// Coerce long integer to byte integer between 0 and 255
byte coerceToByte(long x){
  if(x<0){
    x=0;
  }else{
    if(x>255){
      x=255;
    }
  }
  return byte(x);
}

// Calculate the output colors using scaling functions
void calculateColors(){
  // 1st-order scaling functions
  long r = calculateColorValue(freqVolume, red1)/255+red0;
  long g = calculateColorValue(freqVolume, green1)/255+red0;
  long b = calculateColorValue(freqVolume, blue1)/255+red0;

  #ifdef FIXEDBRIGHTNESS
  // Scale color values to a fixed total value
  long sum = r + g + b;
  red=coerceToByte(FIXEDBRIGHTNESS*r/sum);
  green=coerceToByte(FIXEDBRIGHTNESS*g/sum);
  blue=coerceToByte(FIXEDBRIGHTNESS*b/sum);
  #else
  // Set color values from scaling function output
  red=coerceToByte(r);
  green=coerceToByte(g);
  blue=coerceToByte(b);
  #endif
}

// Calculate volume bar level using scaling function
#ifdef VOLUMEBAR
long calculateVolumeBarSize(){
  long y=MSGEQ7.getVolume()*volume1/255+volume0;
  if(y<0){
    y=0;
  }
  return y;
}
#endif


// Write output to the display
void setOutput(){
  for(int i=0; i<NEOPIXEL_LENGTH; i++){
    #ifdef VOLUMEBAR
    if(i < calculateVolumeBarSize()){
      strip.setPixelColor(i, strip.Color(red, green, blue));
    }else{
      strip.setPixelColor(i, strip.Color(redOff, greenOff, blueOff));
    }
    #else
    strip.setPixelColor(i, strip.Color(red, green, blue));
    #endif
  }
  strip.show();
}
    
void loop() {
  // Acquire input signals
  updateFreqVolume();

  // Control logic
  calculateColors();

  // Display output
  setOutput();
  
  // Send debugging information
  #ifdef DEBUG
  String output="";
  for(int i=0; i<MSGEQ7_FREQCOUNT; i++){
    output+=String(freqVolume[i])+" ";
  }
  output+=String(red)+" ";
  output+=String(green)+" ";
  output+=String(blue)+" ";
  Serial.println(output);
  #endif
}
