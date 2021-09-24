
class Color {
  private:
    int r;
    int g;
    int b;
    
  public:
    Color();
    Color(int r,int g,int b);
    int getR();
    int getG();
    int getB();
    void setR(int r);
    void setG(int g);
    void setB(int b);
};

Color::Color()
{
  this->r = 255;
  this->g = 0;
  this->b = 0;
}
Color::Color(int r,int g,int b)
{
  setR(r);
  setG(g);
  setB(b);
}
void Color::setR(int r)
{
  this->r = max(0,min(255,r));
}
void Color::setG(int g)
{
  this->g = max(0,min(255,g));
}
void Color::setB(int b)
{
  this->b = max(0,min(255,b));
}
int Color::getR(){return r;};
int Color::getG(){return g;};
int Color::getB(){return b;};
#include <FastLED.h>
#define doubleLeds 1
int doubleLedPositions[doubleLeds] ={3};
#define LED_PIN     13
#define NUM_LEDS    8
#define ADMPIN  A0
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB

#define msCampioneAudio  10 // tempo di campionamento audio in ms 
#define BAUD 9600
#define CMD_MAX_LEN 100
#define minDecibelTrigger 40
#define minBrightnessAnimVUMETER 50
CRGB leds[NUM_LEDS];

int BRIGHTNESS  = 255;
//int UPDATES_PER_SECOND = 100;

Color fixedColor = Color(255,255,255);
Color animColor = Color(255,0,0);

int micAnimCode = 0;
bool micMode = false;

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;
#include <SoftwareSerial.h>

SoftwareSerial BTserial(6, 7); // RX | TX
// Connect the HC-08 TX to Arduino pin 2 RX.
// Connect the HC-08 RX to Arduino pin 3 TX through a voltage divider.

String cmd="";

void setup(){
  Serial.begin(BAUD);
  BTserial.begin(BAUD);
  delay(3000); // delay iniziale per sicurezza
  Serial.println("ready...");
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;

  initialSetup();
}
unsigned long lastMillis = 0;
bool incomingCmd=false;
bool neverReceivedCommands = true;
void loop() {  
  
  if (BTserial.available()) {
    neverReceivedCommands = false;
    if(cmd.length() >= CMD_MAX_LEN)
    {
      Serial.println("max cmd length reached");
      cmd="";
      incomingCmd=false;
    }
    //Serial.write(BTserial.read());
    char c = BTserial.read();
    if(c == '!') // cmd start
    {
      cmd = "!";
      incomingCmd=true;
    }
    else if(c == '-' && incomingCmd) // cmd end
    {
      cmd+="-";
      incomingCmd = false;
      execute(cmd);
    }
    else if(incomingCmd)
    {
      cmd+=c;
    }
    //Serial.print(c);
  }
  else if(neverReceivedCommands)return;
  // ogni tot millis esegui o la mic routine o la colorPalette Routine
  else if(!incomingCmd && millis()-lastMillis >= 20)
  {
    if(micMode)
    {
      // la mic routine impiega 'tempoCampionamentoAudio' millisecondi per essere eseguita
      // quindi puo' capitare che viene perso qualche comando bluetooth
      micRoutine();
    }
    else 
    {
      colorPaletteRoutine();
    }
    

    lastMillis = millis();
    //FastLED.delay(1000 / UPDATES_PER_SECOND);
    
  } 
}
void micRoutine()
{
  int PTPAmp = ottieniPTPAmp(ADMPIN);
  int decibel = ottieniDecibel(PTPAmp);
  int decibelMapped = map(decibel,minDecibelTrigger,120,0,NUM_LEDS);
  //Serial.print("decibel mapped = ");
  //Serial.println(decibelMapped);
  switch(micAnimCode)
  {
    case 0:
      ledStripVUMETER_RightAnim(decibelMapped);
    break;
    case 1:
      ledStripVUMETER_LeftAnim(decibelMapped);
    break;
    case 2:
      ledStripVUMETER_CenterAnim(decibelMapped);
    break;
    case 3:
      ledStripVUMETER_BrightAnim(decibelMapped);
    break;
    case 4:
      ledStripVUMETER_SemaphoreRightAnim(decibelMapped);
    break;
    case 5:
      ledStripVUMETER_SemaphoreLeftAnim(decibelMapped);
    break;
    case 6:
      ledStripVUMETER_SemaphoreCenterAnim(decibelMapped);
    break;
    case 7:
      ledStripVUMETER_SemaphoreBrightAnim(decibelMapped);
    break;
    case 8:
      ledStripVUMETER_KingAnim(decibelMapped);
    break;
    case 9:
      ledStripVUMETER_KingStaticAnim(decibelMapped);
    break;
    case 10:
      ledStripVUMETER_ItalyAnim(decibelMapped);
    break;
    case 11:
      ledStripVUMETER_ItalyStaticAnim(decibelMapped);
    break;
    default:
    break;
  }
}
void colorPaletteRoutine()
{
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */
  
  FillLEDsFromPaletteColors( startIndex);
  
  FastLED.show();
}
void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < NUM_LEDS; ++i) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}

String getCmdName(String cmd)
{
  int start = cmd.indexOf('!');
  int dataStart = cmd.indexOf('{');
  if(start==-1 || dataStart==-1 ||
        dataStart<start+1 || cmd.length()<2)return "?";
  return cmd.substring(start+1,dataStart);
}
String getCmdData(String cmd)
{
  int start = cmd.indexOf('{');
  int dataStart = cmd.indexOf('}');
  if(start==-1 || dataStart==-1 ||
        dataStart<start+1 || cmd.length()<2)return "?";
  return cmd.substring(start+1,dataStart);
}
int getDataLen(String data)
{
  int cont=1;
  for(int i=0;i<data.length();i++)
    if(data.charAt(i) == ';')cont++;
}
String getDataField(String data, int index)
{
  int len = getDataLen(data);
  if(len == -1)return "?";
  if(len == 1)return data;
  else
  {
    if(index ==0)return data.substring(0,data.indexOf(';'));
    else
    {
      int startTrim = indexOfCar(data,';',index);
      if(startTrim ==-1)return "?";
      int endTrim = indexOfCar(data,';',index+1);
      if(endTrim ==-1)return data.substring(startTrim+1);
      else return data.substring(startTrim+1,endTrim);
    }
  }
}
int indexOfCar(String str,char c,int occorrenza)
  {
    int lastIndex=0;
    for(int i=0;i<occorrenza;i++)
    {
      if(lastIndex == -1)break;
      lastIndex = str.indexOf(c,lastIndex+1);
    }
    return lastIndex;
  }
void execute(String cmd)
{
  Serial.print("executing: ");
  Serial.println(cmd);
  Serial.print("cmd name: ");
  String cmdName = getCmdName(cmd);
  Serial.println(cmdName);
  Serial.print("cmd data: ");
  String data = getCmdData(cmd);
  Serial.println(data);
  Serial.print("cmd data at pos 1: ");
  Serial.println(getDataField(data,0));
  Serial.print("cmd data at pos 2: ");
  Serial.println(getDataField(data,1));
  Serial.print("cmd data at pos 3: ");
  Serial.println(getDataField(data,2));
  
  if(cmdName == "setbright"){setBrightCommand(data);}
  else if(cmdName == "setrgb"){setRgbCommand(data);}
  else if(cmdName == "setargb"){setArgbCommand(data);}
  else if(cmdName == "setblend"){setBlendCommand(data);}
  else if(cmdName == "playanim"){playAnimCommand(data);}
  else if(cmdName == "micmode"){micModeCommand(data);}
  else if(cmdName == "micanim"){setMicAnimCommand(data);}
  else if(cmdName == "swap"){swap(fixedColor,animColor);}
}
void micModeCommand(String data)
{
  if(data == "Y")micMode = true;
  else micMode = false;
}
void setBrightCommand(String data)
{
  int bright = getDataField(data,0).toInt();
  bright = max(0,min(255,bright));
  BRIGHTNESS = bright;
  FastLED.setBrightness(  BRIGHTNESS );
}
void setRgbCommand(String data)
{
  int r = getDataField(data,0).toInt();
  int g = getDataField(data,1).toInt();
  int b = getDataField(data,2).toInt();
  fixedColor = Color(r,g,b);
  Serial.print("setto rgb: ");
  Serial.println(r);
  Serial.println(g);
  Serial.println(b);
  SetupSingleColorPalette(fixedColor);
}
void setArgbCommand(String data)
{
  int r = getDataField(data,0).toInt();
  int g = getDataField(data,1).toInt();
  int b = getDataField(data,2).toInt();
  animColor = Color(r,g,b);
  SetupFadeAnimColorPalette();
}
void setBlendCommand(String data)
{
  if(data == "Y")currentBlending = LINEARBLEND;
  else currentBlending = NOBLEND;
}
void setMicAnimCommand(String data)
{
  if(data == "Right"){micAnimCode =0;}
  else if(data == "Left"){micAnimCode =1;}
  else if(data == "Center"){micAnimCode =2;}
  else if(data == "Bright"){micAnimCode =3;}
  else if(data == "SemaphoreRight"){micAnimCode =4;}
  else if(data == "SemaphoreLeft"){micAnimCode =5;}
  else if(data == "SemaphoreCenter"){micAnimCode =6;}
  else if(data == "SemaphoreBright"){micAnimCode =7;}
  else if(data == "King"){micAnimCode =8;}
  else if(data == "KingStatic"){micAnimCode =9;}
  else if(data == "Italy"){micAnimCode =10;}
  else if(data == "ItalyStatic"){micAnimCode =11;}
}
void EpilessiaAnimation();
void playAnimCommand(String data)
{
  if(data == "Random"){SetupTotallyRandomPalette();}
  else if(data == "Rainbow"){currentPalette = RainbowStripeColors_p;}
  else if(data == "RainbowStripe"){currentPalette = RainbowColors_p;}
  else if(data == "Fade(ANIMCOL)"){SetupFadeAnimColorPalette();}
  else if(data == "Arrow(ANIMCOL)"){SetupAnimColorStripedPalette();}
  else if(data == "BlackWhite"){SetupBlackAndWhiteStripedPalette();}
  else if(data == "PurpleGreen"){SetupPurpleAndGreenPalette();}
  else if(data == "Cloud"){currentPalette = CloudColors_p;}
  else if(data == "Party"){currentPalette = PartyColors_p;}
  else if(data == "Italy"){currentPalette = myRedWhiteBluePalette_p;}
  else if(data == "Ocean"){currentPalette = OceanColors_p;}
  else if(data == "Lava"){currentPalette = LavaColors_p;}
  else if(data == "Forest"){currentPalette = ForestColors_p;}
  else if(data == "Epilessia"){EpilessiaAnimation();}
}
//---
CRGB getCRGBColor(Color c)
{
  return CRGB(c.getR(),c.getG(),c.getB());
}
void SetupSingleColorPalette(Color c)
{
  fill_solid( currentPalette, 16,getCRGBColor(c));
}
// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; ++i) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}
void SetupFadeAnimColorPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    for( int i = 0; i < 8; ++i) {
        currentPalette[i] = getCRGBColor(animColor);
    }
}

void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}
void SetupAnimColorStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = getCRGBColor(animColor);
    currentPalette[4] = getCRGBColor(animColor);
    currentPalette[8] =getCRGBColor(animColor);
    currentPalette[12] = getCRGBColor(animColor);
    
}
// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}


const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Green,
    CRGB::Green, 
    CRGB::Gray,
    CRGB::Gray,
    
    CRGB::Red,
    CRGB::Red,
    
    CRGB::Black, 
    CRGB::Black,
    
    CRGB::Green,
    CRGB::Green, 
    CRGB::Gray,
    CRGB::Gray,
    
    CRGB::Red,
    CRGB::Red, 
    
    
    CRGB::Black, 
    CRGB::Black
};
// Restituisce l'ampiezza Peak-to-Peak letta dal microfono
int ottieniPTPAmp(int pinMic)
{
  // Variabili di tempo per il campionamento
   unsigned long tempoInizio= millis();  // Ottengo l'istante di inizio
   unsigned int PTPAmp = 0; // Inizializzo variabile ampiezza Peek to peek
  // Signal variables to find the peak-to-peak amplitude
   unsigned int maxAmp = 0;
   unsigned int minAmp = 1023;
   int micOut = 0;
  
  // Trova il massimo e il minimo dell'output 
  // del microfono nella finestra di tempo di 50 ms
   while(millis() - tempoInizio < msCampioneAudio) 
   {
      micOut = analogRead(pinMic);
      if( micOut < 1023) //previeni letture errate
      {
        if (micOut > maxAmp)
        {
          maxAmp = micOut; //salva solo la lettura massima
        }
        else if (micOut < minAmp)
        {
          minAmp = micOut; //salva solo la lettura minima
        }
      }
   }

  PTPAmp = maxAmp - minAmp; // (max amp) - (min amp) = ampiezza peak-to-peak 
  //double micOut_Volts = (PTPAmp * 3.3) / 1024; // Converti ADC in voltaggio (non usato)
  // Ritorna l'ampiezza PTP 
  return PTPAmp;   
}
int ottieniDecibel(int PTPAmp)
{
  return 20.0  * log (PTPAmp+1.);
}
int prevBri =BRIGHTNESS;
int oldAudioLevel =0;
void audioBasedBrightness(int mappedAudioLevel)
{
  int bri =  (map(mappedAudioLevel,0,NUM_LEDS,minBrightnessAnimVUMETER,255 )+prevBri)/2;
  prevBri = bri;
  FastLED.setBrightness(bri);
}
void ledStripVUMETER_BrightAnim(int mappedAudioLevel)
{
  audioBasedBrightness(mappedAudioLevel);
  
  for(int i=0;i<NUM_LEDS;i++)
  {
    leds[i] = getCRGBColor(fixedColor);
  }
  FastLED.show(); 
}
void ledStripVUMETER_RightAnim(int mappedAudioLevel)
{
  // commenta queste due righe per non fare la media dell'audio letto
  //mappedAudioLevel = (oldAudioLevel + mappedAudioLevel)/2;
  //oldAudioLevel = mappedAudioLevel;
  
  //audioBasedBrightness(mappedAudioLevel);
  
  for(int i=0;i<NUM_LEDS;i++)
  {
    if(i<mappedAudioLevel)
      leds[i] = getCRGBColor(fixedColor);
    else leds[i] = CRGB::Black;
  }
  FastLED.show(); 
}
void ledStripVUMETER_LeftAnim(int mappedAudioLevel)
{
  // commenta queste due righe per non fare la media dell'audio letto
  //mappedAudioLevel = (oldAudioLevel + mappedAudioLevel)/2;
  //oldAudioLevel = mappedAudioLevel;
  
  //audioBasedBrightness(mappedAudioLevel);
  
  for(int i=0;i<NUM_LEDS;i++)
  {
    if(i>=NUM_LEDS-mappedAudioLevel)
      leds[i] = getCRGBColor(fixedColor);
    else leds[i] = CRGB::Black;
  }
  FastLED.show(); 
}
void ledStripVUMETER_CenterAnim(int mappedAudioLevel)
{
  // commenta queste due righe per non fare la media dell'audio letto
  //mappedAudioLevel = (oldAudioLevel + mappedAudioLevel)/2;
  //oldAudioLevel = mappedAudioLevel;
  
  //audioBasedBrightness(mappedAudioLevel);
  
  int center = NUM_LEDS/2;
  int newMappedAudioLevel = map(mappedAudioLevel,0,NUM_LEDS,0,center); 
  
  for(int i=0;i<NUM_LEDS;i++)
  {
    
    if((i<=(center+ newMappedAudioLevel) && i>=center) || (i>=(center- newMappedAudioLevel) && i<=center))
      leds[i] = getCRGBColor(fixedColor);
    else leds[i] = CRGB::Black;
  }
  FastLED.show(); 
}
float interp3(float t, float a, float b, float c)
{
  return pow((1-t),2)*a + 2*(1-t)*t*b + pow(t,2)*c;
}
float interp2(float t,float a,float b)
{
  return (1-t)*a + t*b;
}
Color lowAudioCol = Color(0,255,0);
Color mediumAudioCol = Color(255,255,0);
Color highAudioCol = Color(255,0,0);
void ledStripVUMETER_SemaphoreRightAnim(int mappedAudioLevel)
{
  for(int i=0;i<NUM_LEDS;i++)
  {
     if(i<mappedAudioLevel)
     {
        float normAudioLevel = ((float)i)/((float)(NUM_LEDS-1));
        float r = interp3(normAudioLevel,(float) lowAudioCol.getR(),(float)mediumAudioCol.getR(),(float)highAudioCol.getR());
        float g = interp3(normAudioLevel, (float)lowAudioCol.getG(),(float)mediumAudioCol.getG(),(float)highAudioCol.getG());
        float b = interp3(normAudioLevel,(float) lowAudioCol.getB(),(float)mediumAudioCol.getB(),(float)highAudioCol.getB());
        leds[i] = CRGB(r,g,b);//getCRGBColor(fixedColor);
     }
     else leds[i] = CRGB::Black;
    
  }
  FastLED.show(); 
}
void ledStripVUMETER_SemaphoreLeftAnim(int mappedAudioLevel)
{
  for(int i=0;i<NUM_LEDS;i++)
  {
     if(i>=NUM_LEDS-mappedAudioLevel)
     {
        float normAudioLevel = ((float)(NUM_LEDS-1-i))/((float)(NUM_LEDS-1));
        float r = interp3(normAudioLevel,(float) lowAudioCol.getR(),(float)mediumAudioCol.getR(),(float)highAudioCol.getR());
        float g = interp3(normAudioLevel, (float)lowAudioCol.getG(),(float)mediumAudioCol.getG(),(float)highAudioCol.getG());
        float b = interp3(normAudioLevel,(float) lowAudioCol.getB(),(float)mediumAudioCol.getB(),(float)highAudioCol.getB());
        leds[i] = CRGB(r,g,b);//getCRGBColor(fixedColor);
     }
     else leds[i] = CRGB::Black;
    
  }
  FastLED.show(); 
}
void ledStripVUMETER_SemaphoreCenterAnim(int mappedAudioLevel)
{
  int center = NUM_LEDS/2;
  int newMappedAudioLevel = map(mappedAudioLevel,0,NUM_LEDS,0,center); 
  for(int i=0;i<NUM_LEDS;i++)
  {
     if((i<=(center+ newMappedAudioLevel) && i>=center) || (i>=(center- newMappedAudioLevel) && i<=center))
     {
        float normAudioLevel = ((float)i)/((float)NUM_LEDS-1);
        float r = interp3(normAudioLevel,(float) highAudioCol.getR(),(float)lowAudioCol.getR(),(float)highAudioCol.getR());
        float g = interp3(normAudioLevel, (float)highAudioCol.getG(),(float)lowAudioCol.getG(),(float)highAudioCol.getG());
        float b = interp3(normAudioLevel,(float) highAudioCol.getB(),(float)lowAudioCol.getB(),(float)highAudioCol.getB());
        float t =0.3;
        leds[i] = CRGB(interp2(t,r,0),interp2(t,g,255),interp2(t,b,0));//getCRGBColor(fixedColor);
     }
     else leds[i] = CRGB::Black;
    
  }
  FastLED.show(); 
}
void ledStripVUMETER_SemaphoreBrightAnim(int mappedAudioLevel)
{
  audioBasedBrightness(mappedAudioLevel);
  float normAudioLevel = ((float)mappedAudioLevel)/((float)(NUM_LEDS-1));
  float r = interp3(normAudioLevel,(float) lowAudioCol.getR(),(float)mediumAudioCol.getR(),(float)highAudioCol.getR());
  float g = interp3(normAudioLevel, (float)lowAudioCol.getG(),(float)mediumAudioCol.getG(),(float)highAudioCol.getG());
  float b = interp3(normAudioLevel,(float) lowAudioCol.getB(),(float)mediumAudioCol.getB(),(float)highAudioCol.getB());
  for(int i=0;i<NUM_LEDS;i++)
  {
     leds[i] = CRGB(r,g,b);//getCRGBColor(fixedColor);
    
  }
  FastLED.show(); 
}
bool isAdoubleLed(int ledPos)
{
  for(int i=0;i<doubleLeds;i++)
    if(ledPos == doubleLedPositions[i])
      return true;
  return false;
    
}
void ledStripVUMETER_KingAnim(int mappedAudioLevel)
{
  audioBasedBrightness(mappedAudioLevel);
  
  for(int i=0;i<NUM_LEDS;i++)
  {
     if(isAdoubleLed(i))
      leds[i] = getCRGBColor(animColor);
     else leds[i] = getCRGBColor(fixedColor);
    
  }
  FastLED.show(); 
}
// i colori sono fissi, non c'e' luminosita' variabile
void ledStripVUMETER_KingStaticAnim(int mappedAudioLevel)
{
  FastLED.setBrightness(BRIGHTNESS);
  for(int i=0;i<NUM_LEDS;i++)
  {
     if(isAdoubleLed(i))
      leds[i] = getCRGBColor(animColor);
     else leds[i] = getCRGBColor(fixedColor);
    
  }
  FastLED.show(); 
}
void ledStripVUMETER_ItalyAnim(int mappedAudioLevel)
{
  audioBasedBrightness(mappedAudioLevel);
  int endVerde =NUM_LEDS/3;
  int startRosso = NUM_LEDS/1.4;
  for(int i=0;i<NUM_LEDS;i++)
  {
     if(i<=endVerde)
      leds[i] = getCRGBColor(Color(0,255,0));
     else if(i>=startRosso)leds[i] = getCRGBColor(Color(255,0,0));
     else leds[i] = getCRGBColor(Color(255,255,255));
    
  }
  FastLED.show(); 
}
void ledStripVUMETER_ItalyStaticAnim(int mappedAudioLevel)
{
  FastLED.setBrightness(BRIGHTNESS);
  int endVerde =NUM_LEDS/3;
  int startRosso = NUM_LEDS/1.4;
  for(int i=0;i<NUM_LEDS;i++)
  {
     if(i<=endVerde)
      leds[i] = getCRGBColor(Color(0,255,0));
     else if(i>=startRosso)leds[i] = getCRGBColor(Color(255,0,0));
     else leds[i] = getCRGBColor(Color(255,255,255));
    
  }
  FastLED.show(); 
}
int pauseStartupAnim = 120;
void initialSetup()
{
  for(int i=0;i<NUM_LEDS;i++)
    leds[i] = CRGB::Black;
  FastLED.show();
  
  FastLED.setBrightness(BRIGHTNESS);
  for(int i=0;i<NUM_LEDS;i++)
  {
     if(isAdoubleLed(i))
      leds[i] = getCRGBColor(animColor);
     else leds[i] = getCRGBColor(fixedColor);
     FastLED.show(); 
     delay(pauseStartupAnim);
  }
  for(int i=0;i<NUM_LEDS;i++)
    leds[i] = CRGB::Black;
  FastLED.show(); 
  delay(pauseStartupAnim);
  
  for(int i=0;i<NUM_LEDS;i++)
  {
     if(isAdoubleLed(i))
      leds[i] = getCRGBColor(animColor);
     else leds[i] = getCRGBColor(fixedColor);
  }
  FastLED.show(); 
  delay(pauseStartupAnim);

  for(int i=0;i<NUM_LEDS;i++)
    leds[i] = CRGB::Black;
  FastLED.show();
  delay(pauseStartupAnim);
  for(int i=0;i<NUM_LEDS;i++)
  {
     if(isAdoubleLed(i))
      leds[i] = getCRGBColor(animColor);
     else leds[i] = getCRGBColor(fixedColor);
  }
  FastLED.show(); 
  delay(pauseStartupAnim);
}
void swap(Color &a,Color &b)
{
  Color c=a;
  a=b;
  b=c;
}
int dimArrayFrequenze = 9;
float frequenzeHzBlink[] ={1,3,5,10,12,14,18,21,24};
int dimArrayFrequenzeMedie = 3;
float freqMedie[] ={14,18,21};
float HzToMillis(float hz)
{
  return (1/hz)*1000;
}
bool contains(float k,float a[],int dim)
{
  for(int i=0;i<dim;i++)
    if(a[i] == k)return true;
  return false;
}
void EpilessiaAnimation()
{
  for(int i=0;i<NUM_LEDS;i++)
        leds[i] = CRGB::Black;
  FastLED.show();
  FastLED.setBrightness(BRIGHTNESS);
  for(int i=0;i<dimArrayFrequenze;i++)
  {
    float delayBlink = HzToMillis(frequenzeHzBlink[i]), millisElapsed =0;
    bool isFreqMedia = false;
    if(contains(frequenzeHzBlink[i],freqMedie,dimArrayFrequenzeMedie))
      isFreqMedia =true;
    do
    {
      // attiva led 
      for(int i=0;i<NUM_LEDS;i++)
      {
        if(isFreqMedia)
          leds[i] = CRGB::Red;
        else leds[i] = CRGB::White;
      }
        
      FastLED.show(); 
      
      delay(delayBlink);
      
      //disattiva led
      for(int i=0;i<NUM_LEDS;i++)
        leds[i] = CRGB::Black;
      FastLED.show();
       
      delay(delayBlink);
      millisElapsed += delayBlink*2;
    }
    while(millisElapsed <= 10000);
    delay(5000);
  }
}
