#include <Arduino.h>
#include <Wire.h>

#include <radio.h> // RADIO by Matthias Hertel
#include <RDA5807M.h>
#include <LiquidCrystal_I2C.h> // LiquidCrystal_I2C by marcoSchwartz

// ----- Fixed settings here. -----

#define FIX_BAND RADIO_BAND_FM ///< The band that will be tuned by this sketch is FM.
#define FIX_STATION 9350       ///< The station that will be tuned by this sketch is 89.30 MHz.
#define FIX_VOLUME 5           ///< The volume that will be set by this sketch is level 4.

RDA5807M radio; // Create an instance of Class for RDA5807M Chip
LiquidCrystal_I2C lcd(0x27, 16, 2);

// rejestr przesowny
#define latchPin 10
#define clockPin 11
#define dataPin 12

/// przerwania
#define buttonPin 2
volatile bool muteRadioFlag = false;

///buzzer
#define buzzerPin 5

/*!
 *  @brief    IQR funtion on buttonClick
 *  @returns  void
 */
void muteRadio() {
  volatile static unsigned long lastDebounceTime = 0; //czas ostatniej zmiany stanu przycisku
  unsigned long debounceDelay = 100; //czas, przez który należy ignorować zmianę stanu przycisku po jego naciśnięciu
  unsigned long currentTime = millis(); //pobranie aktualnego czasu
  
  if (currentTime - lastDebounceTime > debounceDelay) { //jeśli upłynął czas debounceDelay od ostatniej zmiany stanu przycisku
    
    muteRadioFlag = !muteRadioFlag; //zmiana stanu przycisku (naciśnięty <-> zwolniony)
  }
  lastDebounceTime = currentTime; //zapisanie czasu ostatniej zmiany stanu przycisku
} // muteRadio


/*!
 *  @brief    setup project
 *  @returns  void
 */
void setup()
{
  delay(3000);

  //Buzzer
  pinMode(buzzerPin, OUTPUT);

  // shift register setup
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  // przerwania
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), muteRadio, LOW);

  // open the Serial port
  Serial.begin(9600);
  Serial.println("RDA5807M Radio...");
  delay(200);

  lcd.init(); // initialize the lcd
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0, 0);

  // Enable information to the Serial port
  radio.debugEnable(true);
  radio._wireDebug(false);

  // Set FM Options for Europe

  radio.setup(RADIO_FMSPACING, RADIO_FMSPACING_100);  // for EUROPE
  radio.setup(RADIO_DEEMPHASIS, RADIO_DEEMPHASIS_50); // for EUROPE

  // Initialize the Radio
  if (!radio.initWire(Wire))
  {
    Serial.println("no radio chip found.");
    delay(1000);
    // ESP.restart();
  };

  // Set all radio setting to the fixed values.
  int newVal = analogRead(A1);
  //newVal = mapAnalogFreq(newVal);
  newVal = map(newVal, 0, 1023, 900, 970);

  radio.setBandFrequency(FIX_BAND, newVal * 10);
  radio.setVolume(FIX_VOLUME);
  radio.setMono(false);
  radio.setMute(muteRadioFlag);
  radio.setSoftMute(true);
  radio.setBassBoost(true);
} // setup

/*!
 *  @brief    print frequency on lcd display
 *  @returns  void
 */
void printFreqOnLcd()
{
  //lcd.clear();
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  lcd.setCursor(0, 0);
  lcd.print(s);
}

/*!
 *  @brief    read analog and chagenge ferequency.
 *  @param oldVal  old analog value to comparison
 *  @returns  int updated value
 */
int calcValue(int oldVal)
{
  int newVal = analogRead(A1);
  newVal = map(newVal, 0, 1023, 900, 970);

  if (oldVal != newVal)
  {
    tone(buzzerPin, 500, 10);
    radio.setFrequency(newVal * 10);
    radio.setMute(muteRadioFlag);
    printFreqOnLcd();
    return newVal;
  }

  return oldVal;
} // calcValue

/*!
 *  @brief      shiftoutBits
 *  @param val  8 bit value 
 *  @returns    void
 */
void printOnLeds(int val)
{
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, val);
    digitalWrite(latchPin, HIGH);
    // delay(100);
}

int val = 0;

/*!
 *  @brief    main loop function
 *  @returns  void
 */
void loop()
{
  struct RADIO_INFO info = {};
  radio.getRadioInfo(&info);

  lcd.setCursor(0, 1);
  lcd.print("rssi ");
  lcd.print(info.rssi);
  lcd.setCursor(15, 1);

  if(muteRadioFlag != radio.getMute())
  {
    radio.setMute(muteRadioFlag);
  }

  lcd.print(muteRadioFlag);

  if(info.rssi > 20)  printOnLeds(0b1110);
  else if(info.rssi > 15) printOnLeds(0b0110);
  else if(info.rssi > 10) printOnLeds(0b0010);
  else printOnLeds(0b0);

  val = calcValue(val);
  // delay(50); // delay for LCD
} // loop