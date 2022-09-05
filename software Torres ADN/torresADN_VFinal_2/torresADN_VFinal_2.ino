#include <dummy.h>
#include <FastLED.h>
//Audio
#define SAMPLES1 10
#define SAMPLES2 1500       // 500 o 2000
#define AUDIO_IN_PIN A0     //Entrada analógica a la que se ha conectado el microfono
#define NOISE 115          //Los valores que estén por debajo de NOISE se ignoran
#define DC_OFFSET 0

int peak = 0;
int vol1[SAMPLES1];
int vol2[SAMPLES2];
byte volCount1 = 0;
byte volCount2 = 0;


//LEDs
#define NUM_LEDS 76
#define TOWER_NUM_LEDS 19
#define MAX_BAR_HEIGHT TOWER_NUM_LEDS
#define HALF_BAR_HEIGHT (MAX_BAR_HEIGHT/2)
#define LED_PIN D4
#define COLOR_ORDER GRB
#define MAX_BRIGHTNESS 220
#define LED_TYPE WS2812B
CRGB leds[NUM_LEDS];
byte base_hue = 0;  //HSV

//COLORPALETTE
#define UPDATES_PER_SECOND 100
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;


//AMBILIGHT
byte R[MAX_BAR_HEIGHT];
byte G[MAX_BAR_HEIGHT];
byte B[MAX_BAR_HEIGHT];
char recv[1];


//BOTÓN
const byte button = D6;
unsigned int operatingMode = 0;
unsigned long lastTime = 0;
const byte threshold = 100;


void setup() {
  // put your setup code here, to run once:
  delay(2000);
  Serial.begin(115200);

  //analogSetWidth(11);
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
  pinMode(button, INPUT);
  attachInterrupt(digitalPinToInterrupt(button), operatingModeHandler, RISING);

  for(int i=0; i<NUM_LEDS; i++){
    leds[i] = 0xff0000;
  }
  FastLED.show();
  delay(2000);

}


void loop() {
  // put your main code here, to run repeatedly:
  switch(operatingMode){
    case 0:
      vumeter1();
      delayMicroseconds(2500);
      break;

    case 1:
      vumeter2();
      delayMicroseconds(2500);
      break;

    case 2:
      colorPalette();
      break;

    case 3:
      ambilight();
      break;
  }
  FastLED.show();
}


//FUNCIONES VÚMETRO
void vumeter1(){ 
  static uint16_t minLvl1=0, maxLvl1=0;
  static uint16_t minLvl2=0, maxLvl2=0;
  int      n, height;
  static int oldHeight = 0;

  n   = analogRead(AUDIO_IN_PIN);                  // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  //n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum

  if(maxLvl2 > minLvl2 && maxLvl2 > NOISE)
    //height = MAX_BAR_HEIGHT * (maxLvl1 - minLvl2) /(maxLvl2 - minLvl2);
    //height = MAX_BAR_HEIGHT * (maxLvl1-minLvl1)/(maxLvl2-minLvl2);
    height = MAX_BAR_HEIGHT * (1*maxLvl1-NOISE)/(1.1*maxLvl2-NOISE);
  else
    height = 0;

  height = height*0.7 + oldHeight*0.3;

  
  if(height < 0L)       height = 0;      // Clip output
  else if(height >= (MAX_BAR_HEIGHT-1)){
    height = MAX_BAR_HEIGHT - 1;
    peak = height;
  }
  else if(height > peak) peak = height + 1;
  
  //Actualizamos los leds
  for(int i=0; i<TOWER_NUM_LEDS; i++){
    if(i > height || height == 0){
      //Torre principal
      leds[i] = 0x000000;
      leds[2*TOWER_NUM_LEDS-1-i] = 0x000000;

      //Torre secundaria
      leds[2*TOWER_NUM_LEDS+i] = 0x000000;
      leds[4*TOWER_NUM_LEDS-1-i] = 0x000000;
    }
    else{
      //Torre principal
      leds[i] = CHSV(base_hue, 255, 255);
      leds[2*TOWER_NUM_LEDS-1-i] = CHSV(base_hue, 255, 255);

      //Torre secundaria
      leds[2*TOWER_NUM_LEDS+i] = CHSV(base_hue, 255, 255);
      leds[4*TOWER_NUM_LEDS-1-i] = CHSV(base_hue, 255, 255);
    }
  }
  drawPeak(255, 255, 255);
  
  //Bajamos la cima
  EVERY_N_MILLISECONDS(100){
    if(peak>0)
      peak -= 1;
  }

  EVERY_N_MILLISECONDS(200){
    if(base_hue>245) base_hue = 0;
    else base_hue += 10;
  }


  vol1[volCount1] = n;
  if(++volCount1 >= SAMPLES1) volCount1 = 0;
  vol2[volCount2] = n;
  if(++volCount2 >= SAMPLES2) volCount2 = 0;
  

  minLvl1 = 9999;
  maxLvl1 = 0;
  for(int i=0; i<SAMPLES1; i++){
    if(vol1[i] < minLvl1){
      minLvl1 = vol1[i];
    }
    else if(vol1[i] > maxLvl1) maxLvl1 = vol1[i];
  }

  minLvl2 = 9999;
  maxLvl2 = 0;
  for(int i=0; i<SAMPLES2; i++){
    if(vol2[i] < minLvl2)      minLvl2 = vol2[i];
    else if(vol2[i] > maxLvl2) maxLvl2 = vol2[i];
  }

  oldHeight = height;
}


void vumeter2(){
  static uint16_t minLvl21=0, maxLvl21=0;
  static uint16_t minLvl22=0, maxLvl22=0;
  int      n, height;  
  static int oldHeight2 = 0;
  static int peak1=0, peak2=0;

  n   = analogRead(AUDIO_IN_PIN);                  // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero

  
  if(maxLvl22 > minLvl22 && maxLvl22 > NOISE){
    //height = MAX_BAR_HEIGHT * (maxLvl1 - minLvl2) /(maxLvl2 - minLvl2);
    //height = MAX_BAR_HEIGHT * (maxLvl1-minLvl1)/(maxLvl2-minLvl2);
    height = MAX_BAR_HEIGHT * (1*maxLvl21-NOISE)/(1.1*maxLvl22-NOISE);
    height = height*0.8 + oldHeight2*0.2;  
    height = height/2 + 1;
  }
  else
    height = 0;


  if(height < 0L)       height = 0;      // Clip output
  else if(height >= HALF_BAR_HEIGHT) height = HALF_BAR_HEIGHT - 1;
  if(height >= (HALF_BAR_HEIGHT-1))     peak1   = MAX_BAR_HEIGHT - 1, peak2=0;// Keep 'peak' dot at top 
  else if((height+HALF_BAR_HEIGHT) > peak1) peak1 = HALF_BAR_HEIGHT + height + 1, peak2 = HALF_BAR_HEIGHT - height - 1;


  
  //Actualizamos los leds
  for(int i=0; i<HALF_BAR_HEIGHT; i++){
    if(i > height || height == 0){
      //Torre principal
      leds[HALF_BAR_HEIGHT - i - 1] = 0x000000;
      leds[TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT + i + 1] = 0x000000;
      //Modificadas (arreglo)
      leds[HALF_BAR_HEIGHT + i + 1] = 0x000000;
      leds[TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT - i - 1] = 0x000000;

      //Torre secundaria
      leds[2*TOWER_NUM_LEDS+HALF_BAR_HEIGHT - i - 1] = 0x000000;
      leds[3*TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT + i + 1] = 0x000000;
      //Modificadas (arreglo)
      leds[2*TOWER_NUM_LEDS+HALF_BAR_HEIGHT + i + 1] = 0x000000;
      leds[3*TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT - i - 1] = 0x000000;
     
    }
    else{
      Wheel(map(i, 0, HALF_BAR_HEIGHT-1, 30, 150), i);
      //leds[HALF_BAR_HEIGHT-i-1] = 0xff0000;
      //leds[HALF_BAR_HEIGHT+i] = 0xff0000;
      //Serial.println(HALF_BAR_HEIGHT+i);
    }
  }

  
  leds[peak1] = 0xffffff;
  leds[2*TOWER_NUM_LEDS-1-peak1] = 0xffffff;
  leds[peak2] = 0xffffff;
  leds[2*TOWER_NUM_LEDS-1-peak2] = 0xffffff;

  leds[2*TOWER_NUM_LEDS+peak1] = 0xffffff;
  leds[4*TOWER_NUM_LEDS-1-peak1] = 0xffffff;
  leds[2*TOWER_NUM_LEDS+peak2] = 0xffffff;
  leds[4*TOWER_NUM_LEDS-1-peak2] = 0xffffff;
  

  //Bajamos la cima
  EVERY_N_MILLISECONDS(100){
    if(peak1 > HALF_BAR_HEIGHT){
      peak1 -= 1;
      peak2 += 1;
    }
  }

  vol1[volCount1] = n;
  if(++volCount1 >= SAMPLES1) volCount1 = 0;
  vol2[volCount2] = n;
  if(++volCount2 >= SAMPLES2) volCount2 = 0;
  
  minLvl21 = 9999;
  maxLvl21 = 0;
  for(int i=0; i<SAMPLES1; i++){
    if(vol1[i] < minLvl21){
      minLvl21 = vol1[i];
    }
    else if(vol1[i] > maxLvl21) maxLvl21 = vol1[i];
  }

  minLvl22 = 9999;
  maxLvl22 = 0;
  for(int i=0; i<SAMPLES2; i++){
    if(vol2[i] < minLvl22)      minLvl22 = vol2[i];
    else if(vol2[i] > maxLvl22) maxLvl22 = vol2[i];
  }

  oldHeight2 = height;
}


// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
void Wheel(byte WheelPos, int i) {
  if(WheelPos < 85) {
    leds[HALF_BAR_HEIGHT+i].setRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
    leds[TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT-i].setRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
    leds[HALF_BAR_HEIGHT-i-1].setRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
    leds[TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT+i+1].setRGB(WheelPos * 3, 255 - WheelPos * 3, 0);

    leds[2*TOWER_NUM_LEDS+HALF_BAR_HEIGHT+i].setRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
    leds[3*TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT-i].setRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
    leds[2*TOWER_NUM_LEDS+HALF_BAR_HEIGHT-i-1].setRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
    leds[3*TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT+i+1].setRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  
  else if(WheelPos < 170) {
    WheelPos -= 85;
    leds[HALF_BAR_HEIGHT+i].setRGB(255 - WheelPos * 3, 0, WheelPos * 3);
    leds[TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT-i].setRGB(255 - WheelPos * 3, 0, WheelPos * 3);
    leds[HALF_BAR_HEIGHT-i-1].setRGB(255 - WheelPos * 3, 0, WheelPos * 3);
    leds[TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT+i+1].setRGB(255 - WheelPos * 3, 0, WheelPos * 3);

    leds[2*TOWER_NUM_LEDS+HALF_BAR_HEIGHT+i].setRGB(255 - WheelPos * 3, 0, WheelPos * 3);
    leds[3*TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT-i].setRGB(255 - WheelPos * 3, 0, WheelPos * 3);
    leds[2*TOWER_NUM_LEDS+HALF_BAR_HEIGHT-i-1].setRGB(255 - WheelPos * 3, 0, WheelPos * 3);
    leds[3*TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT+i+1].setRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  
  else {
    WheelPos -= 170;
    leds[HALF_BAR_HEIGHT+i].setRGB(0, WheelPos * 3, 255 - WheelPos * 3);
    leds[TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT-i].setRGB(0, WheelPos * 3, 255 - WheelPos * 3);
    leds[HALF_BAR_HEIGHT-i-1].setRGB(0, WheelPos * 3, 255 - WheelPos * 3);
    leds[TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT+i+1].setRGB(0, WheelPos * 3, 255 - WheelPos * 3);

    leds[2*TOWER_NUM_LEDS+HALF_BAR_HEIGHT+i].setRGB(0, WheelPos * 3, 255 - WheelPos * 3);
    leds[3*TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT-i].setRGB(0, WheelPos * 3, 255 - WheelPos * 3);
    leds[2*TOWER_NUM_LEDS+HALF_BAR_HEIGHT-i-1].setRGB(0, WheelPos * 3, 255 - WheelPos * 3);
    leds[3*TOWER_NUM_LEDS-1+HALF_BAR_HEIGHT+i+1].setRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void drawPeak(byte R, byte G, byte B){
  //Torre principal
  leds[peak].setRGB(R, G, B);
  leds[2*TOWER_NUM_LEDS-peak-1].setRGB(R, G, B);

  //Torre secundaria
  leds[2*TOWER_NUM_LEDS+peak].setRGB(R, G, B);
  leds[4*TOWER_NUM_LEDS-peak-1].setRGB(R, G, B);
}




//FUNCIONES COLORPALETTE
void colorPalette(){
  FastLED.delay(1000 / UPDATES_PER_SECOND);
  ChangePalettePeriodically();
    
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */
  
  FillLEDsFromPaletteColors( startIndex);
}

void FillLEDsFromPaletteColors( uint8_t colorIndex){
    uint8_t brightness = 255;
    
    for( int i = 0; i < TOWER_NUM_LEDS; ++i) {
        //Torre principal
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        leds[2*TOWER_NUM_LEDS-1-i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);

        //Torre secundaria
        leds[2*TOWER_NUM_LEDS+i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        leds[4*TOWER_NUM_LEDS-1-i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}

void ChangePalettePeriodically(){
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
        if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
        if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
    }
}

void SetupTotallyRandomPalette(){
    for( int i = 0; i < 16; ++i) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

void SetupBlackAndWhiteStripedPalette(){
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

void SetupPurpleAndGreenPalette(){
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}

const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM ={
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};


//FUNCIONES AMBILIGHT
void ambilight(){
  if(Serial.available()>0){
    if(Serial.read()== 'w'){
      for(int i = TOWER_NUM_LEDS-1; i>=0; i--){

        
        while(!Serial.available());
        String rl = Serial.readStringUntil('r');
        int RL = rl.toInt();
        
        while(!Serial.available());
        String gl = Serial.readStringUntil('g');
        int GL = gl.toInt();
        
        while(!Serial.available());
        String bl = Serial.readStringUntil('b');
        int BL = bl.toInt();


        
        while(!Serial.available());
        String pio = Serial.readStringUntil('p');
        int PIO = pio.toInt();
        
        while(!Serial.available());
        String rte = Serial.readStringUntil('l');
        int RTE = rte.toInt();
        
        while(!Serial.available());
        String klj = Serial.readStringUntil('k');
        int KLJ = klj.toInt();
        
        
        leds[i].r = RL;
        leds[i].g = GL;
        leds[i].b = BL;    
        leds[2*TOWER_NUM_LEDS-1-i].r = RL;
        leds[2*TOWER_NUM_LEDS-1-i].g = GL;
        leds[2*TOWER_NUM_LEDS-1-i].b = BL;
        

        
        leds[2*TOWER_NUM_LEDS+i].r = PIO;
        leds[2*TOWER_NUM_LEDS+i].g = RTE;
        leds[2*TOWER_NUM_LEDS+i].b = KLJ;
        leds[4*TOWER_NUM_LEDS-1-i].r = PIO;
        leds[4*TOWER_NUM_LEDS-1-i].g = RTE;
        leds[4*TOWER_NUM_LEDS-1-i].b = KLJ;    
      }
    }
  }
}



//INTERRUPCIONES
ICACHE_RAM_ATTR void operatingModeHandler(){
  if((millis()-lastTime) > threshold){
    if(operatingMode < 3) operatingMode++;
    else operatingMode = 0;
    lastTime = millis();
  }
}
