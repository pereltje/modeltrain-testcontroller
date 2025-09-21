/**************************************************************************
 * Nano_atm_modeltrain_testconroller (c) Fenna Pel, february 2024, updated september 2025
 * 
    Nano_atm_modeltrain_testconroller is free software: you can redistribute it and/or modify it under the terms of the 
    GNU General Public License as published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program. 
    If not, see <https://www.gnu.org/licenses/>. 
 * 
 * 
 * this software is designed to be used on arduino nano with atmega328p microcontroller. 
 * Untested, but may possibly be used on the UNO variant with changing the A6 pin assignment
 * 
  ToDo:

  changelog:
  0.6:  - changed version number
        - added olikraus u8g2/u8x8 library to the project
          (no graphics needed and the u8x8 does not requere ram of the arduino)
          the adafruit libraries may interfere with the ram usage by the program itself.
          Setting up the run-in program with an array broke the program.
        - removed adafruit libraries from project. result, smaller program firmware
        - initial rewrite to use the u8x8 display library
        - reworked display layout to use u8x8 by olikraus (ie a 16x8 grid character display)
        - modified the displayed language all to dutch

  0.5   - changed version number
        - speedFade function: changed the not-used frequency mode option to be used
          as timestep option
        - speedLoop function: - removed the not-used frequence mode option
                              - included a step counter 
        - changed the run-in speed secuence, smaller steps higher starting value, 
          higher ending value. loop time and fadespeed are set with #defines

  0.4:  - changed version number
        - rewrote code for cleaner seperation between local and global variables
        - implemented structural naming scheme for better human readability:
              THIS_IS_A_DEFINED_VALUE
              _THIS_IS_A_GLOBAL_VARIABLE
              otherVariablesAndFuntionsAreLikeThis
              function scoped variables get the function initials prefixed in lower case
        - implemented operating modes:
          - pwm manual
          - setting of PWM frequency
          - external (non PWM) powersupply for tracks
          - run-in program

  0.3: - added sh1106 oled driver (changable with commenting out the driver not in use)
       - rotated screen orentation 180 degrees
       - textual changes to reflect it is a speed controller for the testbench
       - added 31Khz pwm timer prescaler option (commented out)
       - converted ino sketch to vscode/platformIO project

  0.2: - swapped over pins 5 and 11 for pwm timer prescaling without messing up
         the nano internaly used timing functions like delay()
  
  0.1: - initial release
         
 **************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8x8lib.h>

// general defines (used by functions)
#define VERSION_NUMBER "0.6"
#define MAX_SPEED 127
#define MIN_SPEED -127
#define DEBOUNCE_TIME  300
#define DEFAULT_FREQUENCY_MODE 7

// pin defenitions:
#define OLED_MOSI       9 // oled pin D1 / data
#define OLED_CLK        10 //oled pin: D0 / clock
#define OLED_DC         5 
#define OLED_CS         12
#define OLED_RESET      8
#define ENABLE_PIN      11 
#define IN_1_PIN        4
#define IN_2_PIN        6
#define RED_BUTTON_PIN  3
#define BLUE_BUTTON_PIN 2
#define POT_PIN         A6
#define RELAY_PIN       7

// screen related:
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define LINE_HEIGHT 8
//U8X8_SH1106_128X64_VCOMH0_4W_SW_SPI display(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC); // U8X8_SH1106_128X64_XXXX_4W_SW_SPI(clock, data, cs, dc [, reset])
//U8X8_SH1106_128X64_VCOMH0_4W_SW_SPI display(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC);
U8X8_SH1106_128X64_NONAME_4W_SW_SPI display(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC);
/*
  global variables and consts:
*/
bool _RED_BUTTON = false;
bool _BLUE_BUTTON = false;
const char *_FREQUENCY_NAME[7] = {"31372.55 Hz", "3921.16 Hz", "980.39 Hz", "490.20 Hz", "245.10 Hz", "122.55 Hz","30.64 Hz"};
byte _FREQUENCY_MODE = DEFAULT_FREQUENCY_MODE;

/*
-------------- support functions
*/
void setSpeed(int ssSpeed)
{
  bool ssReverse = false;
  if (ssSpeed < 0) ssReverse = true;
  analogWrite(ENABLE_PIN, abs(ssSpeed*2));
  digitalWrite(IN_1_PIN, ssReverse);
  digitalWrite(IN_2_PIN, !ssReverse);
}

int getSpeedMapped(int gsmPin)
{
  // requieres #define for _MAX_SPEED and _MIN_SPEED
  int gsmSpeed = analogRead(gsmPin);
  int gsmSpeedMapped=map(gsmSpeed,0,1023,MIN_SPEED, MAX_SPEED);
  return gsmSpeedMapped;
}

int getSpeedPct(int gspSpeedMapped)
{
  // requieres #define for _MAX_SPEED
  int gspSpeedPct=map(abs(gspSpeedMapped), 0 , MAX_SPEED, 0, 100);
  return gspSpeedPct;
}

void displaySpeed(int dsSpeedMapped, int dsLine)
{
  bool dsReverse = false;
  if (dsSpeedMapped < 0) dsReverse = true;
  // requieres #define for _LINE_HEIGHT
  // needs 2 lines on the screen
  //display.setCursor(0, (dsLine*LINE_HEIGHT));
  display.setCursor(0, (dsLine));
  display.print(" snelheid: ");
  display.print(getSpeedPct(dsSpeedMapped));
  display.println("%   ");
  display.println();
  if (abs(dsSpeedMapped) > 3)
  {
    if (dsReverse) display.println("  <<<<--------  ");
    else display.println("  -------->>>>  ");
  }
  else display.println("  ------------  ");
  display.display();
  setSpeed(dsSpeedMapped);
}

void drawStartup(void) {
  //requieres #define for _VERSION_NUMBER
  display.clearDisplay();
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Rij"));
  display.println(F("regelaar"));
  display.println(F("rollenbank"));
  display.println(VERSION_NUMBER);
  display.display();
  delay(500);
}

void redButton()
{
  // requieres _RED_BUTTON global bool variablre
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  bool redButtonPinCheck = digitalRead(!RED_BUTTON_PIN);
  if ((interrupt_time - last_interrupt_time > DEBOUNCE_TIME)&& redButtonPinCheck) _RED_BUTTON = true;
  last_interrupt_time = interrupt_time;
}

void blueButton()
{
  // requieres _BLUE_BUTTON global bool variable
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  bool blueButtonPinCheck = digitalRead(!BLUE_BUTTON_PIN);
  if ((interrupt_time - last_interrupt_time > DEBOUNCE_TIME)&& blueButtonPinCheck) _BLUE_BUTTON = true;
  last_interrupt_time = interrupt_time;
}

void speedFade(int sfStartSpeed, int sfEndSpeed, unsigned long sfTimeStep)
{
  //const int sfTimeStep = 2;
  int sfSpeedStep = 1;
  int sfSpeed = sfStartSpeed;
  if (sfStartSpeed >= sfEndSpeed) sfSpeedStep = sfSpeedStep * -1;
  unsigned long sfNewTime = millis();
  unsigned long sfTime = sfNewTime;
  while ((sfSpeed != sfEndSpeed))
  {
    displaySpeed(sfSpeed, 5);
    sfNewTime=millis();
    if (sfNewTime >= (sfTime + sfTimeStep))
    {
      sfSpeed = sfSpeed + sfSpeedStep;
      sfTime = sfNewTime;
    }
  }
}

void speedLoop(int slSpeed, unsigned long slLoopSeconds, int slStepCount, int slMaxSteps)
{
  unsigned long slStartTime = millis();
  unsigned long slEndTime = slStartTime + (slLoopSeconds * 1000);
  unsigned long slTime = slStartTime;
  unsigned long slTimeRemaining = slLoopSeconds;
  bool slCountDown = true;
  while (slCountDown && !_BLUE_BUTTON)
  {
    display.setCursor(0, 4);
    display.print("t: ");
    display.print(slTimeRemaining);
    display.print("s, ");
    display.print(slStepCount);
    display.print ("#");
    display.print(slMaxSteps);
    display.print ("   ");
    display.display();
    displaySpeed(slSpeed, 5);
    slTime = millis();
    slTimeRemaining = (slEndTime - slTime)/1000;
    if (slTime > slEndTime) slCountDown = false;
  }
  if (_BLUE_BUTTON)
  {
    speedFade(slSpeed, 0, 1);
  }
}

void pwmManual(byte pwmmFrequencyMode)
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Rijregelaar");
  display.println("hand PWM ");
  display.println(_FREQUENCY_NAME[pwmmFrequencyMode - 1]);
  display.display();
  speedFade(0, (getSpeedMapped(POT_PIN)), 1);
  while (!_BLUE_BUTTON)
  {
    displaySpeed((getSpeedMapped(POT_PIN)),5);
  }
  _BLUE_BUTTON = false;
  speedFade((getSpeedMapped(POT_PIN)), 0, 1);
  setSpeed(0);
}


void pwmRunin(byte pwmrFrequencyMode)
{
  #define SPEED_FADE 100
  #define LOOP_TIME 300
  int pwmrStepCount = 0;
  int pwmrSpeed = 0;
  bool pwmrDone = false;
  const unsigned long pwmrLoopTime = LOOP_TIME;
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Rijregelaar");
  display.println("inrij programma");
  display.print("PWM ");
  display.println(_FREQUENCY_NAME[pwmrFrequencyMode - 1]);
  display.display();
  _BLUE_BUTTON = false;
  while (!_BLUE_BUTTON && !pwmrDone)
  {
    for (int pwmrNewSpeed = 50; pwmrNewSpeed <= 90; pwmrNewSpeed = pwmrNewSpeed + 10)
    {
      pwmrStepCount++;
      speedFade(pwmrSpeed, pwmrNewSpeed, SPEED_FADE);
      speedLoop(pwmrNewSpeed, pwmrLoopTime, pwmrStepCount, 10);
      if (_BLUE_BUTTON) break;
      pwmrStepCount++;
      speedFade(pwmrNewSpeed, pwmrNewSpeed*-1, SPEED_FADE);
      speedLoop(pwmrNewSpeed*-1, pwmrLoopTime, pwmrStepCount, 10);
      if (_BLUE_BUTTON) break;
      pwmrSpeed = pwmrNewSpeed*-1;
    }
    pwmrDone = true;
  }
  speedFade(pwmrSpeed, 0, SPEED_FADE);
  setSpeed(0);
  if (_BLUE_BUTTON)
  {
    display.setCursor(0, 3);
    display.println("AFGEBROKEN");
    display.display();
    _BLUE_BUTTON = false;
  }
  else
  {
    display.setCursor(0, 3);
    display.println("klaar");
    display.display();
  }
  delay (500);
  display.clearDisplay();
  display.display();
}

int pwmFreq(byte pwmfFrequencyMode)
{
  /*
      TCCR2B = TCCR2B & B11111000 | B00000001;    // set timer 2 divisor to     1 for PWM frequency of 31372.55 Hz
      TCCR2B = TCCR2B & B11111000 | B00000010;    // set timer 2 divisor to     8 for PWM frequency of  3921.16 Hz
      TCCR2B = TCCR2B & B11111000 | B00000011;    // set timer 2 divisor to    32 for PWM frequency of   980.39 Hz
      TCCR2B = TCCR2B & B11111000 | B00000100;    // set timer 2 divisor to    64 for PWM frequency of   490.20 Hz (The default at nano start up)
      TCCR2B = TCCR2B & B11111000 | B00000101;    // set timer 2 divisor to   128 for PWM frequency of   245.10 Hz
      TCCR2B = TCCR2B & B11111000 | B00000110;    // set timer 2 divisor to   256 for PWM frequency of   122.55 Hz
      TCCR2B = TCCR2B & B11111000 | B00000111;    // set timer 2 divisor to  1024 for PWM frequency of    30.64 Hz
  */
  byte selectedFreq = pwmfFrequencyMode;
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Selecteer");
  display.setCursor(0,1);
  display.print("PWM frequentie");
  display.setCursor(0,2);
  display.print("Rood: selecteerd");
  display.setCursor(0,3);
  display.print("Blauw: bevestigd");
  display.display();
  while (!_BLUE_BUTTON)
  {
    if (_RED_BUTTON){
      _RED_BUTTON = false;
      selectedFreq++;
      if (selectedFreq > 7) selectedFreq = 1;
    }
    display.setCursor(0,5);
    display.print(selectedFreq);
    display.print(": ");
    display.print(_FREQUENCY_NAME[selectedFreq - 1]);
    display.println("");
    display.display();
  }
  TCCR2B = (TCCR2B & B11111000) | selectedFreq;
  display.println("geselecteerd");
  display.display();
  delay(500);
  _BLUE_BUTTON = false;
  return selectedFreq;
}

void externalPower(int epLine)
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Rijregelaar");
  display.println("Externe");
  display.println("voeding");
  display.display();
  digitalWrite(RELAY_PIN,0);
  while (!_BLUE_BUTTON)
  {
    display.setCursor(0, epLine);
    display.println("actief");
    display.display();
  }
  _BLUE_BUTTON = false;
  display.setCursor(0,epLine);
  display.println("beeindigd");
  display.display();
  digitalWrite(RELAY_PIN,1);
  delay(500);
  display.clearDisplay();
  display.display();
}
/*
-------------- end of support functions
*/

void setup() 
{
  TCCR2B = (TCCR2B & B11111000) | _FREQUENCY_MODE;
  //setup pin 11 AND 3 for phase correct PWM:
  TCCR2A = _BV(COM2A1) | _BV(WGM20);

  display.setFlipMode(0); // u8x8 specific
  //display.setFont(u8x8_font_5x7_f);
  //display.setFont(u8x8_font_pxplusibmcga_f);
  display.setFont(u8x8_font_pressstart2p_f);
  //display.begin(SSD1306_SWITCHCAPVCC);
  display.begin();
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(500); // Pause for 0.5 seconds

  // Clear the buffer
  display.clearDisplay();

  drawStartup();  
  pinMode(IN_1_PIN, OUTPUT);
  pinMode(IN_2_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BLUE_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RED_BUTTON_PIN), redButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BLUE_BUTTON_PIN), blueButton, FALLING);
  digitalWrite(RELAY_PIN,1);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("initiatie klaar");
  display.display();
  delay(500);
}

void loop() 
{
  int operatingMode = 1;
  display.clearDisplay();
  display.setCursor(0,0);             // Start at top-left corner
  display.print("selecteer Modus:");
  display.setCursor(0,1);   
  display.print("Rood: selecteerd");
  display.setCursor(0,2);   
  display.print("Blauw: bevestigd");
  display.display();
  _BLUE_BUTTON = false;
  _RED_BUTTON = false;
  while (!_BLUE_BUTTON){
    if (_RED_BUTTON){
      _RED_BUTTON = false;
      operatingMode++;
      if (operatingMode > 4) operatingMode = 1;
    }
    display.setCursor(0,5);
    switch (operatingMode)
    {
      case 1:
        display.print("PWM handbed.    ");
        break;
      case 2:
        display.print("PWM inrij autom.");
        break;
      case 3:
        display.print("kies PWM freq.  ");
        break;
      case 4:
        display.print("externe voeding ");
        break;
      default:
        break;
    }
    display.display();
  }
  _BLUE_BUTTON = false;

  switch (operatingMode)
  {
    case 1:
      pwmManual(_FREQUENCY_MODE);
      break;
    case 2:
      pwmRunin(_FREQUENCY_MODE);
      break;
    case 3:
      _FREQUENCY_MODE=pwmFreq(_FREQUENCY_MODE);
      break;
    case 4:
      externalPower(5);
      break;
    default:
      break;
  }
}
