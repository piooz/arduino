#include <Arduino.h>
#include <Wire.h>

#include <radio.h>
#include <RDA5807M.h>
#include <LiquidCrystal_I2C.h>
#include <RDSParser.h>

// ----- Fixed settings here. -----

#define FIX_BAND RADIO_BAND_FM ///< The band that will be tuned by this sketch is FM.
#define FIX_STATION 9350       ///< The station that will be tuned by this sketch is 89.30 MHz.
#define FIX_VOLUME 2           ///< The volume that will be set by this sketch is level 4.

RDA5807M radio; // Create an instance of Class for RDA5807M Chip
LiquidCrystal_I2C lcd(0x27, 20, 4);
RDSParser rds;

const int latchPin = 10;
const int clockPin = 11;
const int dataPin = 12;

void RDS_print(const char *text)
{
  Serial.println(text);
}

void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4)
{
  rds.processData(block1, block2, block3, block4);
}

void RDS_print_time(uint8_t hour, uint8_t minute)
{
  lcd.setCursor(0, 1);
  lcd.print(hour);
  lcd.print(minute);
}

int mapAnalogFreq(int val)
{
  return (((float)val / (float)1024) * 100 + 900);
}

/// Setup a FM only radio configuration
/// with some debugging on the Serial port

void setup()
{
  delay(3000);

  // shift register setup
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  //

  rds.attachTextCallback(RDS_print);
  rds.attachTimeCallback(RDS_print_time);

  // open the Serial port
  Serial.begin(9600);
  Serial.println("RDA5807M Radio...");
  delay(200);

  lcd.init(); // initialize the lcd
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("simea");

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
    delay(4000);
    // ESP.restart();
  };

  // Set all radio setting to the fixed values.
  int newVal = analogRead(A1);
  newVal = mapAnalogFreq(newVal);

  radio.setBandFrequency(FIX_BAND, newVal * 10);
  radio.setVolume(FIX_VOLUME);
  radio.setMono(false);
  radio.setMute(false);
  radio.setSoftMute(true);
  radio.setBassBoost(false);

  radio.attachReceiveRDS(RDS_process);
} // setup

void updateLcd()
{
  lcd.clear();
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  lcd.setCursor(0, 0);
  lcd.print(s);
}

int calcValue(int oldVal)
{
  int newVal = analogRead(A1);
  newVal = mapAnalogFreq(newVal);

  if (oldVal != newVal)
  {
    radio.setFrequency(newVal * 10);
    return newVal;
  }

  updateLcd();

  rds.init();

  return oldVal;
} // calcValue

/// show the current chip data every 3 seconds.
int val = 0;

void testSR()
{
  for (int number = 0; number < 256; number++)
  {
    digitalWrite(latchPin, LOW);

    shiftOut(dataPin, clockPin, MSBFIRST, number);

    digitalWrite(latchPin, HIGH);

    delay(100);
  }
}
void loop()
{

  struct RADIO_INFO info = {};
  radio.getRadioInfo(&info);

  lcd.setCursor(0, 1);
  lcd.print("rds ");
  lcd.print(info.rds);
  // lcd.print(info.rssi);

  // radio.checkRDS();

  val = calcValue(val);
  delay(100); // delay for
} // loop
