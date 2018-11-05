// Import NicoHood's MSGEQ7 driver
#include <MSGEQ7.h>

//#define DEBUG
#define NEOPIXEL_ENABLE

#ifdef NEOPIXEL_ENABLE
#include <Adafruit_NeoPixel.h>
#define pinNeopixel 2
#define STRIPLENGTH 100
#endif

#define MSGEQ7_MAPNOISE

#define pinDCIn A0
#define pinReset 4
#define pinStrobe 3
#define MSGEQ7_INTERVAL 33333 // 1000000/30
#define MSGEQ7_SMOOTH 10
#define MSGEQ7_FREQCOUNT 7

#define FIXEDBRIGHTNESS 110
#define VOLUMEBAR

CMSGEQ7<MSGEQ7_SMOOTH, pinReset, pinStrobe, pinDCIn> MSGEQ7;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(STRIPLENGTH, pinNeopixel, NEO_RGB + NEO_KHZ800);



// Frequency volume values
byte freqVolume[MSGEQ7_FREQCOUNT];

// RGB values
byte red=0;
byte green=0;
byte blue=0;

// 0th order polynomial coefficient
// This is not broken out because, when the dependent terms are summed, that would waste time
int   red0   = 0;
int green0   = 0;
int  blue0   = 0;

int   red1[] = {40,60,30,0,0,0,0};
int green1[] = {0,0,30,60,30,0,0};
int  blue1[] = {0,0,0,0,30,60,40};

#ifdef VOLUMEBAR
const byte redOff=10;
const byte greenOff=10;
const byte blueOff=10;
const long volume0=-20;
const long volume1=240;
#endif

void setup() {
  
  #ifdef DEBUG  
  Serial.begin(115200);
  #endif

  #ifdef NEOPIXEL_ENABLE
  strip.begin();
  strip.show();
  #endif

  // Initialize the IC for reading frequencies
  MSGEQ7.begin();
}

void updateFreqVolume(){
  for(int i=0; i<MSGEQ7_FREQCOUNT; i++){
    #ifdef MSGEQ7_MAPNOISE
    freqVolume[i]=mapNoise(MSGEQ7.get(i));
    #else
    freqVolume[i]=MSGEQ7.get(i);
    #endif
  }
}

long calculateColorValue(byte sample[], int color1[]){
  long y=0;
  for(int i=0; i<MSGEQ7_FREQCOUNT; i++){
    y+=sample[i]*color1[i];
  }
  return y;
}

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

void normalizeColors(){
  long r = calculateColorValue(freqVolume, red1)/255+red0;
  long g = calculateColorValue(freqVolume, green1)/255+red0;
  long b = calculateColorValue(freqVolume, blue1)/255+red0;
  #ifdef FIXEDBRIGHTNESS
  long sum = r + g + b;
  red=coerceToByte(FIXEDBRIGHTNESS*r/sum);
  green=coerceToByte(FIXEDBRIGHTNESS*g/sum);
  blue=coerceToByte(FIXEDBRIGHTNESS*b/sum);
  #else
  red=coerceToByte(r);
  green=coerceToByte(g);
  blue=coerceToByte(b);
  #endif
//  Serial.print(red);
//  Serial.print(" ");
//  Serial.print(green);
//  Serial.print(" ");
//  Serial.println(blue);
}

#ifdef VOLUMEBAR
long calculateVolume(){
//  long sum=0;
//  for(int i=0; i<MSGEQ7_FREQCOUNT; i++){
//    sum+=long(freqVolume[i])*long(freqVolume[i]);
//  }
//  long y=sqrt(sum/MSGEQ7_FREQCOUNT);
  long y=MSGEQ7.getVolume()*volume1/255+volume0;
  if(y<0){
    y=0;
  }
  return y;
}
#endif

void setOutput(){
  for(int i=0; i<STRIPLENGTH; i++){
    #ifdef VOLUMEBAR
    if(i < calculateVolume()){
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
  // put your main code here, to run repeatedly:
//  MSGEQ7.reset();
  bool newReading = MSGEQ7.read();
  if(newReading){
    updateFreqVolume();
    normalizeColors();
    setOutput();
  }
  
  #ifdef DEBUG
  String output="";
  for(int i=0; i<MSGEQ7_FREQCOUNT; i++){
    output+=String(freqVolume[i])+" ";
  }
//  output+=String(red)+" ";
//  output+=String(green)+" ";
//  output+=String(blue)+" ";
  Serial.println(output);
  #endif
  #ifdef DEBUG
//  delay(100);
  #endif
}
