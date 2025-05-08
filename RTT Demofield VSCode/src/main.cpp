/* 
This code is made for the RoboTeam Twente (RTT) demofield, designed in 2025
This code is written to be used with an Arduino Mega (it can be reconfigured to use an arduino uno using PlatformIO, the board should be set to "uno", or "megaatmega2560")
by Dylan Bruggeman (RTT generation 9)


use https://github.com/FastLED/FastLED/wiki/Controlling-leds for information for controlling the LED Strip

 
TODO:
[x] add fastled animations using functions
[x] add goal trigger
[x] add scoreboard
[-] add speaker noise for non-goal situations (maybe)
[x] add crowd noise for goal situations
[x] implement speaker in code
[x] add turntable control
[x] determine speaker data pins (probably RX/TX)
[-] check all pin numbers --> possibly change to use an arduino mega
[x] add user input 
[x] determine what to do with the keypad
[-] check if the keypad needs the numbers or the keys (as indicated)
[x] add sound effects
[x] change PlatformIO board to arduino mega 
*/

// Libraries
#include <FastLED.h>            // used for the adressable LEDs
#include <Keypad.h>             // used for the keypad input
#include <SoftwareSerial.h>     // used for the speakers

// Defines
// pin numbers
#define speakerPinTX 18          // 1st pin for the speaker, this one is used as TX, connect to arduino RX
#define speakerPinRX 19          // 2nd pin for the speaker, this one is used as RX, connect to arduino TX
#define LEDDataPin 6            // pin for the adressable led strip data
#define ballSensorBluePin A0    // pin for the blue side goal sensor
#define ballSensorYellowPin A1  // pin for the yellow side goal sensor
#define turntablePin 5          // pin for the robot turntable
#define alwaysOnLightsPin 4     // pin to turn on/off the stationary lights
#define volumePotPin A2         // pin for the potentiometer for the speaker volume
// keypad definitions           // definitions of the row and colum pins
#define COL1 8
#define COL2 9
#define COL3 10
#define COL4 11
#define ROW1 12
#define ROW2 13
#define ROW3 17                 
#define ROW4 53                 

// definitions for the speakers
#define CMD_PLAY  0X01
#define CMD_SEL_DEV 0X35
#define DEV_TF 0X01
#define CMD_SET_VOLUME 0X31
#define CMD_PLAY_W_VOL 0X31
#define CMD_SET_PLAY_MODE 0X33
#define CMD_PLAY_COMBINE 0X45   //can play combination up to 15 songs

// other definitions
#define NUM_LEDS 60             // Number of LEDs in the adressable LED strip
#define deltaI 0.5              // Fractional change in intensity before ball sensor triggers, should be between 0 and 1 (0.8 is 80% of the original value)
#define LEDMaxMilliAmp 800      // set the max ampere the adressable LEDs can use
#define LEDMaxVoltage 5
#define debugMode false         // enable-disable debug mode
#define printSerial true       // enable-disable serial print mode
#define playFirstSound 9        // define the sound effect to play at startup 
#define speakerVolumeMax 24     // sets the max speaker volume (0-30)


// Variables
// bools
bool turntableON = true;
bool goalYellow = false;
bool goalBlue = false;
bool LEDLoop = false;
// integers
int ballSensorYellowValue = 0;
int ballSensorBlueValue = 0;
int ballSensorYellowValuePrevious = 0;
int ballSensorBlueValuePrevious = 0;
int scoreYellow = 0;
int scoreBlue = 0;
int LEDEffect = 0;    
int potmeterValue = 0;
int potToVolume = 0;
// bytes
const byte numRows= 4;
const byte numCols= 4;

// adressable LED related
CRGB leds[NUM_LEDS];

// keypad related
char keys[numRows][numCols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'0', 'F', 'E', 'D'}
};

byte rowPins[numRows] = {ROW1, ROW2, ROW3, ROW4};                                 // the pins the rows are connected to
byte colPins[numCols] = {COL1, COL2, COL3, COL4};                                 // the pins the colums are connected to
Keypad myKeypad = Keypad(makeKeymap(keys), rowPins, colPins, numRows, numCols);   // initialise the keypad

// speaker related
static int8_t Send_buf[6] = {0} ;
SoftwareSerial myMP3(speakerPinRX, speakerPinTX);
void sendCommand(int8_t command, int16_t dat );
int16_t speakerString;


void LEDOff(void){
  // turn off all adressable LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void LEDRainbow(uint8_t thisSpeed, uint8_t deltaHue) {                            // The fill_rainbow call doesn't support brightness levels.
 LEDOff();                                                                        // turn off all the adressable LEDs
 uint8_t thisHue = beat8(thisSpeed,255);                                          // A simple rainbow march.
 fill_rainbow(leds, NUM_LEDS, thisHue, deltaHue);                                 // Use FastLED's fill_rainbow routine.
 FastLED.show();
}

void LEDFlash(void){
  // turn on all lights to white for 50ms, then turn all lights off for 50ms
  LEDOff();
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(50);
  LEDOff();
  delay(50);
}

void LEDGoalYellow(void){
  // even nr of LED flash yellow, odd nr flash white (opposite each other)
  for (int j = 0; j < 10; j++){
    for (int i = 0; i < NUM_LEDS; i += 2){
      leds[i] = CRGB::Yellow;
    }
    FastLED.show();
    delay(50);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (int i = 1; i < NUM_LEDS; i += 2){
      leds[i] = CRGB::White;
    }
    FastLED.show();
    delay(50);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
}

void LEDGoalBlue(void){
  // even nr of LED flash blue, odd nr flash white (opposite each other)
  for (int j = 0; j < 10; j++){
    for (int i = 0; i < NUM_LEDS; i += 2){
      leds[i] = CRGB::Blue;
    }
    FastLED.show();
    delay(50);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (int i = 1; i < NUM_LEDS; i += 2){
      leds[i] = CRGB::White;
    }
    FastLED.show();
    delay(50);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
}

void printScoreboard(void){                                             
  // print the scoreboard including the names of the two goals
  Serial.println("\n------------------------");
  Serial.println("Scoreboard: \nYellow-Blue");                          
  Serial.print(scoreYellow);
  Serial.print("-");
  Serial.println(scoreBlue);
  Serial.println("------------------------\n");
}

void turntableControl(bool turntableState){    
  // turn on the turntable when the turntable variable is true                 
  if (turntableState== true){                 
    digitalWrite(turntablePin, HIGH);
  }
   // turn off the turntable when the turntable variable is false
  else if (turntableState == false){
    digitalWrite(turntablePin, LOW);
  }
}

void printRTTLogo(void){
  // print the following image on the terminal
  Serial.println("\n\n" 
  "------------------------------------------------------------------\n"
  "     ######  ####### #######                                      \n"
  "     #     #    #       #                                         \n"
  "     #     #    #       #                                         \n"
  "     ######     #       #                                         \n"
  "     #   #      #       #                                         \n"
  "     #    #     #       #                                         \n"
  "     #     #    #       #                                         \n"
  "                                                                  \n"
  "     ######                                                       \n"
  "     #     # ###### #    #  ####  ###### # ###### #      #####    \n"
  "     #     # #      ##  ## #    # #      # #      #      #    #   \n"
  "     #     # #####  # ## # #    # #####  # #####  #      #    #   \n"
  "     #     # #      #    # #    # #      # #      #      #    #   \n"
  "     #     # #      #    # #    # #      # #      #      #    #   \n"
  "     ######  ###### #    #  ####  #      # ###### ###### #####    \n" 
  "\n     |by Dylan Bruggeman - ninth generation (2024-2025)|        \n"
  "------------------------------------------------------------------\n");                                            
}

int16_t concatIntsToInt16(int value1, int value2) {
  // Ensure values are within 0-255 (1 byte)
  value1 = constrain(value1, 0, 255);
  value2 = constrain(value2, 0, 255);

  // Combine into a single int16_t
  return ((int16_t)value1 << 8) | (int16_t)value2;
}
void sendBytes(uint8_t nbytes)
{
  for(uint8_t i=0; i < nbytes; i++)//
  {
    myMP3.write(Send_buf[i]) ;
  }
}
void mp3Basic(int8_t command)
{
  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0x02; //the number of bytes of the command without starting byte and ending byte
  Send_buf[2] = command; 
  Send_buf[3] = 0xef; //
  sendBytes(4);
}
void mp3_5bytes(int8_t command, uint8_t dat)
{
  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0x03; //the number of bytes of the command without starting byte and ending byte
  Send_buf[2] = command; 
  Send_buf[3] = dat; //
  Send_buf[4] = 0xef; //
  sendBytes(5);
}
void mp3_6bytes(int8_t command, int16_t dat)
{
  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0x04; //the number of bytes of the command without starting byte and ending byte
  Send_buf[2] = command; 
  Send_buf[3] = (int8_t)(dat >> 8);//datah
  Send_buf[4] = (int8_t)(dat); //datal
  Send_buf[5] = 0xef; //
  sendBytes(6);
}
void playWithVolume(int16_t dat)
{
  mp3_6bytes(CMD_PLAY_W_VOL, dat);
}
void sendCommand(int8_t command, int16_t dat = 0)
{
  delay(20);
  if((command == CMD_PLAY_W_VOL)||(command == CMD_SET_PLAY_MODE)||(command == CMD_PLAY_COMBINE))
  	return;
  else if(command < 0x10) 
  {
	mp3Basic(command);
  }
  else if(command < 0x40)
  { 
	mp3_5bytes(command, dat);
  }
  else if(command < 0x50)
  { 
	mp3_6bytes(command, dat);
  }
  else return;
 
}

void speakerGoalSound(int team){
  if (team == 1){playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), 8)); delay(700); playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), 7));}
  if (team == 2){playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), 8)); delay(700); playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), 5));}
  delay(800);
  playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), 9)); 
  delay(5000);
}

void speakerStaticSound(void){
  // Play Track 2 at 20% volume
}

void readBallSensorYellow(void){
  ballSensorYellowValuePrevious = ballSensorYellowValue;                // set past value to current value
  ballSensorYellowValue = analogRead(ballSensorYellowPin);              // read new current value and set it to the variable
  if(ballSensorYellowValue < (ballSensorYellowValuePrevious*deltaI)){   // check if the sensor value changes enough to trigger a goal
    goalYellow = true;                                                  // set goal trigger to true
    if(printSerial){Serial.println("YELLOW SCORED A GOAL");}                             // print that a goal was scored
    scoreYellow = scoreYellow + 1;                                      // increase the goal count
    if(printSerial){printScoreboard();}                                                  // print the scoreboard
    speakerGoalSound(1);
    LEDGoalYellow();                                                    // flash the adressable LEDs to celebrate the yellow goal
    
  }
  goalYellow = false;                                                   // set goal trigger to false
}

void readBallSensorBlue(void){                                               
  ballSensorBlueValuePrevious = ballSensorBlueValue;                    // set past value to current value
  ballSensorBlueValue = analogRead(ballSensorBluePin);                  // read new current value and set it to the variable
  if(ballSensorBlueValue < (ballSensorBlueValuePrevious*deltaI)){       // check if the sensor value changes enough to trigger a goal
    goalBlue = true;                                                    // set goal trigger to true
    speakerGoalSound(2);
    LEDGoalBlue();                                                      // flash the adressable LEDs to celebrate the blue goal
    scoreBlue = scoreBlue + 1;                                          // increase the goal count
    if(printSerial){Serial.println("BLUE SCORED A GOAL");}                               // print that a goal was scored
    if(printSerial){printScoreboard();}                                                  // print the scoreboard
  }
  goalBlue = false;                                                     // set goal trigger to false
}

void speakerSoundboard(int soundNumber){
  // play different sound effects using keypad
  switch (soundNumber)
  {
  case 1: playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), 2)); delay(5000); break;       // play sound 1: "ambulance" (2)
  case 2: playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), 3)); delay(5000); break;        // play sound 2: "victory" (3)
  case 3: playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), 1)); delay(1500); playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), 4)); delay(775); break;        // play sound 3: "whistle referee" (1)
  case 4: playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), 6)); delay(8000); break;        // play sound 4: "booooo" (4)
  }
  playWithVolume(concatIntsToInt16(0, 1));
}

void keypadInput() {
  char keypressed = myKeypad.getKey();                            // get the keypress value and set the variable to this value

  if (keypressed != NO_KEY) {                                     // if a key is pressed, do something. Otherwise don't do something. 
    if(debugMode){Serial.print(keypressed);}                      // print the key that is pressed, for debug only
    switch (keypressed) {
      case '1': LEDOff(); break;                                  // if '1' is pressed, all adressable LEDs turn off
      case '2': LEDRainbow(10, 10); break;                        // if '2' is pressed, all adressable LEDs go in a rainbow pattern
      case '3': LEDFlash(); break;                                // if '3' is pressed, all adressable LEDs flash white
      case 'A': turntableControl(false); break;                   // if 'A' is pressed, turn turntable on
      case '4': speakerSoundboard(1); break;                      // if '4' is pressed, play sound effect 1: "ambulance"
      case '5': speakerSoundboard(2); break;                      // if '5' is pressed, play sound effect 2: "victory"
      case '6': speakerSoundboard(3); break;                      // if '6' is pressed, play sound effect 3: "whistle referee"
      case 'B': turntableControl(true); break;                    // if 'B' is pressed, turn turntable off
      case '7': scoreYellow++; if(printSerial){printScoreboard();} break; // if '7' is pressed, increase goal yellow by 1
      case '8': scoreBlue++; if(printSerial){printScoreboard();} break;   // if '8' is pressed, increase goal blue by 1
      case '9': speakerSoundboard(4); break;                      // if '9' is pressed, play sound 4: "booooo"
      case 'C': digitalWrite(alwaysOnLightsPin, LOW); break;      // if 'C' is pressed, turn on the always-on-lights
      case '0': scoreYellow--; if(printSerial){printScoreboard();} break;   // if '0' is pressed, decrease goal yellow by 1
      case 'F': scoreBlue--; if(printSerial){printScoreboard();} break;  // if 'F' is pressed, decrease goal blue by 1
      case 'E': break; // Add functionality here                  // if 'E' is pressed, 
      case 'D': digitalWrite(alwaysOnLightsPin, HIGH); break;     // if 'D' is pressed, turn off the always-on-lights
    }
  }
}

void setup() {
  if(printSerial){Serial.begin(9600);}
  FastLED.addLeds<NEOPIXEL, LEDDataPin>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(LEDMaxVoltage, LEDMaxMilliAmp);

  // setup the input/output pins
  pinMode(turntablePin, OUTPUT);
  pinMode(alwaysOnLightsPin, OUTPUT);
  pinMode(ballSensorBluePin, INPUT_PULLUP);
  pinMode(ballSensorYellowPin, INPUT_PULLUP);
  pinMode(volumePotPin, INPUT_PULLUP);
  turntableControl(true);
  digitalWrite(alwaysOnLightsPin, HIGH);

  // setup the speaker
  myMP3.begin(9600);
  delay(2000); // Wait for module initialization
  sendCommand(CMD_SEL_DEV, DEV_TF); // Select TF card
  delay(200);

  if(printSerial){printRTTLogo();}
  if(printSerial){printScoreboard();}

  potmeterValue = analogRead(volumePotPin);                                                            // read value from the potmeter
  playWithVolume(concatIntsToInt16(map(potmeterValue, 0, 1023, 0, speakerVolumeMax), playFirstSound)); // 
  delay(5000); 
}

void loop() {
  while(true){
    readBallSensorBlue();                                                       // read the blue goal sensor
    if (debugMode){Serial.println("Reading sensor blue"); delay(500);}          // for debug only
    delay(10);
    readBallSensorYellow();                                                     // read the yellow goal sensor
    if (debugMode){Serial.println("Reading sensor yellow"); delay(500);}        // for debug only
    delay(10);
    keypadInput();                                                              // read the keypad
    if (debugMode){Serial.println("checking keypad"); delay(500);}              // for debug only
    potmeterValue = analogRead(volumePotPin);                                   // read the potmeter value
    if(debugMode){Serial.println(potmeterValue);}
}
}