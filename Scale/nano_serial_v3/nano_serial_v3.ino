#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

SoftwareSerial hc12(7, 6); // RX, TX (to HC-12 TX, RX)

#define SS_PIN 10
#define RST_PIN 9

#define RED_PIN   2
#define GREEN_PIN 3
#define BLUE_PIN  4

String incomingData = "";
int code = 0;
float weight = 0.0;
float price = 0.0;

MFRC522 rfid(SS_PIN, RST_PIN);

String rfidUID = "";
unsigned long uidAsNumber = 0;

void setup() {
  Serial.begin(9600);     // Match this with the sender's baud rate
  hc12.begin(9600);       // HC-12 default baud rate
  Serial.println("HC12 Initialize");
  Serial.println("Waiting for data...");
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID Reader Ready...");
}

void loop() {
  // Read incoming serial data character by character
  while (Serial.available()) {
    char c = Serial.read();
    incomingData += c;

    // If end of line, process the full message
    if (c == '\n') {
      parseData(incomingData);
      incomingData = ""; // Clear the buffer
    }
  }

  // Wait until a valid RFID tag is detected
  if (readRFID()) {
    SendHC12();
    delay(2000); // Short pause to avoid double reads
  }
}

void parseData(String data) {
  // Expected format:
  //   coming from arduino: Code= 123 Weight= 45.67 Price= 150.00

  int codeIndex   = data.indexOf("Code= ");
  int weightIndex = data.indexOf("Weight= ");
  int priceIndex  = data.indexOf("Price= ");

  if (codeIndex >= 0 && weightIndex > codeIndex && priceIndex > weightIndex) {
    // Extract substrings
    String codeStr   = data.substring(codeIndex + 6, weightIndex);
    String weightStr = data.substring(weightIndex + 8, priceIndex);
    String priceStr  = data.substring(priceIndex + 7);

    // Trim whitespace
    codeStr.trim();
    weightStr.trim();
    priceStr.trim();

    // Convert to numbers
    code   = codeStr.toInt();
    weight = weightStr.toFloat();
    price  = priceStr.toFloat();

    // Print parsed values
    Serial.println(F("----- Received Data -----"));
    Serial.print  (F("Code:   ")); Serial.println(code);
    Serial.print  (F("Weight: ")); Serial.println(weight, 2);
    Serial.print  (F("Total Price:  ")); Serial.println(price, 2);
    Serial.println(F("-------------------------"));
  } else {
    Serial.println(F("Invalid data format received!"));
  }
}

bool readRFID() {
  setRGB(true, false, false);
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) 
  return false;

  rfidUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) rfidUID += "0";
    rfidUID += String(rfid.uid.uidByte[i], HEX);
  }

  rfidUID.toUpperCase();
  uidAsNumber = strtoul(rfidUID.c_str(), NULL, 16);

  if (uidAsNumber == 0) 
  return false; // Still invalid


  Serial.print("UID (DEC): ");
  Serial.println(uidAsNumber);
  setRGB(false, true, false);
  delay(2000);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  return true;
}

void SendHC12() {
  hc12.println(uidAsNumber);
  hc12.println(code);
  hc12.println(weight);
  hc12.println(price);
  Serial.println("Sending Completed");
  setRGB(false, false, true);
}

void setRGB(bool r, bool g, bool b) {
  digitalWrite(RED_PIN,   r ? HIGH : LOW);
  digitalWrite(GREEN_PIN, g ? HIGH : LOW);
  digitalWrite(BLUE_PIN,  b ? HIGH : LOW);
}

