#include <U8g2lib.h>
#include <SD.h>
#include <SPI.h>
#include <Keypad.h>
#include "HX711.h"
#include <SoftwareSerial.h>

// Initialize the ST7920-based LCD in SPI mode
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* MISO=RS*/ 13, /* MOSI=RW*/ 11, /* E=SCK=*/ 10);

// SD card module Chip Select (CS) pin
const int chipSelect = 53; // Update this based on your setup

// HX711 load cell module pins
#define LOADCELL_DOUT_PIN  A0
#define LOADCELL_SCK_PIN   A1
HX711 scale;

// Calibration factor for HX711 (adjust based on your load cell)
float calibration_factor = 104;

// Keypad configuration
const byte ROWS = 4; // 4 rows
const byte COLS = 4; // 4 columns
char keys[ROWS][COLS] = {
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '#'},
  {'A', 'B', 'C', 'D'}
};
byte rowPins[ROWS] = {30, 31, 32, 33}; // Connect to row pins
byte colPins[COLS] = {26, 27, 28, 29}; // Connect to column pins
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

SoftwareSerial nanoserial(18, 19);
String str;
String codef;

float weightGrams;
float weightKg;
float totalPrice;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  nanoserial.begin(9600);
  // Initialize the LCD
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr); // Set a default font
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "Initializing...");
  u8g2.sendBuffer();

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "SD init failed!");
    u8g2.sendBuffer();
    while (true);
  }
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "SD Initialized.");
  u8g2.sendBuffer();

  // Initialize HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor); // Set the calibration factor
  scale.tare(); // Reset the scale to 0
  Serial.println("HX711 initialized.");
}

void loop() {
  char key = keypad.getKey(); // Get the key pressed

  if (key) { // If a key is pressed
    static String inputCode = ""; // Store the code being entered

    if (key == '#') { // '#' is used to confirm input
      Serial.print("Searching for Code: ");
      Serial.println(inputCode);
      // Display "Searching" on the LCD
      u8g2.clearBuffer();
      u8g2.drawStr(0, 10, "Searching: ");
      u8g2.drawStr(70, 10, inputCode.c_str());
      u8g2.sendBuffer();
      codef = inputCode;
      searchItem(inputCode); // Call the function to search for the item
      inputCode = ""; // Clear the input for the next entry
    } else if (key == '*') { // '*' is used to clear input
      inputCode = "";
      Serial.println("Input cleared.");
      // Display "Input cleared" on the LCD
      u8g2.clearBuffer();
      u8g2.drawStr(0, 10, "Input cleared.");
      u8g2.sendBuffer(); 
    } else if (key == 'A'){
      u8g2.clearBuffer();
      u8g2.drawStr(0, 10, "Sending...");
      u8g2.drawStr(0, 25, "Place the RFID Card...");
      u8g2.sendBuffer(); 
      str = String("coming from arduino: ")+String("Code= ")+String(codef)+String("Weight= ")+String(weightKg)+String("Price= ")+String(totalPrice);
      SE_COM();
    }else { // Append numeric input
      inputCode += key;
      Serial.print("Input: ");
      Serial.println(inputCode);

      // Display current input on the LCD
      u8g2.clearBuffer();
      u8g2.drawStr(0, 10, "Input: ");
      u8g2.drawStr(50, 10, inputCode.c_str());
      u8g2.sendBuffer();
    }
  }

}

void searchItem(String code) {
  // Open the file containing the item list
  File itemListFile = SD.open("products.txt");

  if (itemListFile) {
    bool found = false;
    while (itemListFile.available()) {
      String line = itemListFile.readStringUntil('\n'); // Read each line

      // Parse the line
      int firstComma = line.indexOf(',');
      int colonIndex = line.indexOf(':');

      if (firstComma != -1 && colonIndex != -1) {
        String fileCode = line.substring(0, firstComma); // Extract code
        String item = line.substring(firstComma + 1, colonIndex); // Extract item
        String priceStr = line.substring(colonIndex + 1); // Extract price
        float unitPrice = priceStr.toFloat();

        // Compare with input code
        if (fileCode == code) {
          Serial.print("Code: ");
          Serial.println(fileCode);
          Serial.print("Item: ");
          Serial.println(item);
          Serial.print("Unit Price: ");
          Serial.println(unitPrice);

          // Get weight from load cell
          weightGrams = scale.get_units();
          weightKg = weightGrams / 1000.0; // Convert to kilograms
          totalPrice = weightKg * unitPrice; // Calculate total price

          Serial.print("Weight (kg): ");
          Serial.println(weightKg, 3);
          Serial.print("Total Price: ");
          Serial.println(totalPrice, 2);

          // Display item details, weight, and total price on the LCD
          u8g2.clearBuffer();
          u8g2.drawStr(0, 10, "Code:");
          u8g2.drawStr(40, 10, fileCode.c_str());
          u8g2.drawStr(0, 25, "Item:");
          u8g2.drawStr(40, 25, item.c_str());
          u8g2.drawStr(0, 40, "Weight:");
          u8g2.drawStr(60, 40, String(weightKg, 3).c_str());
          u8g2.drawStr(0, 55, "Price:");
          u8g2.drawStr(60, 55, String(totalPrice, 2).c_str());
          u8g2.sendBuffer();

          found = true;
          break;
        }
      }
    }
    itemListFile.close(); // Close the file

    if (!found) {
      Serial.println("Item not found.");

      // Display "Item not found" on the LCD
      u8g2.clearBuffer();
      u8g2.drawStr(0, 10, "Item not found.");
      u8g2.sendBuffer();
    }
  } else {
    Serial.println("Error opening file.");

    // Display "Error opening file" on the LCD
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "Error opening file.");
    u8g2.sendBuffer();
  }
}

void SE_COM(){
  nanoserial.println(str);
}

