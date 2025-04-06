/*
 * uno_lcd
 * Example firmware
 * Reads characters one by one from an external eeprom connected by I2C
 * and display them on a LCD
 */

// include the library code
#include <LiquidCrystal.h>
#include <Wire.h>

//Initialize the LCD library
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Character counter
uint16_t count = 0;

//I2C address of the external eeprom
const int EEPROM_ADDRESS = 5;

void setup() {
  //Set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  //Clear the LCD.
  lcd.clear();
  //Setup the I2C interface
  Wire.begin();
}

void loop() {
  //Read a character from the external EEPROM
  //First, send the position to read, MSB first
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write(byte(count >> 8));
  Wire.write(byte(count & 0xFF));
  Wire.endTransmission();
  //Then read the next character
  Wire.requestFrom(EEPROM_ADDRESS, 1);
  char c = Wire.read();

  ++count;

  //A null character marks the end of the text, wrap up the counter
  if (c == '\0') {
    count = 0;
    c = ' ';
  }
  //Replace new line character by spaces
  else if (c == '\n' || c == '\r') {
    c = ' ';
  }

  //Erase the counter digit
  lcd.setCursor(0, 1);
  lcd.print(' ');
  //Print the new character in the first position outside the screen
  lcd.setCursor(16, 0);
  lcd.print(c);
  //Scroll the display to reveal the new character
  lcd.scrollDisplayLeft();
  //Print the character count in the 2nd line
  lcd.setCursor(0, 1);
  lcd.print(count);
  //Wait 50ms until the next character
  delay(50);
}

