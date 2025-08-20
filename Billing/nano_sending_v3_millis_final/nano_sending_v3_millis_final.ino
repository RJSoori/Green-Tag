#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

SoftwareSerial hc12(7, 6); // RX, TX
SoftwareSerial mySerial(2, 3);

#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

unsigned long previousHCTime = 0;
unsigned long hcInterval = 50;
unsigned long previousRFIDTime = 0;
unsigned long rfidInterval = 100;

String str;
String rfidUID = "";
unsigned long uidAsNumber = 0;
unsigned long checkUID = 0;

unsigned long ID_NO = 0;
int IDNO = 0;
int code = 0;
float weight = 0.0;
float price = 0.0;

String tempLines[4];
int lineCount = 0;

String hcBuffer = "";

void setup() {
  Serial.begin(9600);
  hc12.begin(9600);
  Serial.println("HC-12 module initialized and ready.");
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID module initialized and ready.");
  mySerial.begin(9600);
}

void loop() {
  unsigned long currentMillis = millis();

  // HC-12 read (non-blocking)
  if (currentMillis - previousHCTime >= hcInterval) {
    previousHCTime = currentMillis;

    hc12.listen(); // make HC-12 active
    while (hc12.available()) {
      String line = hc12.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        tempLines[lineCount] = line;
        lineCount++;

        if (lineCount == 4) {
          parseHC12Data();
          lineCount = 0;
        }
      }
    }
  }

  // RFID check
  if (currentMillis - previousRFIDTime >= rfidInterval) {
    previousRFIDTime = currentMillis;
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      Serial.print("RFID UID: ");

      rfidUID = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) rfidUID += "0";
        rfidUID += String(rfid.uid.uidByte[i], HEX);
      }

      rfidUID.toUpperCase();
      checkUID = strtoul(rfidUID.c_str(), NULL, 16);

      if (checkUID == 0) return;

      Serial.print("UID (DEC): ");
      Serial.println(checkUID);

      mySerial.listen(); // make mySerial active
      mySerial.println(checkUID);

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }
}

void parseHC12Data() {
  ID_NO  = tempLines[0].toInt();
  code   = tempLines[1].toInt();
  weight = tempLines[2].toFloat();
  price  = tempLines[3].toFloat();

  Serial.println("==== Parsed Numeric Data ====");
  Serial.print("ID_NO  : "); Serial.println(ID_NO);
  Serial.print("Code   : "); Serial.println(code);
  Serial.print("Weight : "); Serial.println(weight, 3);
  Serial.print("Price  : "); Serial.println(price, 2);
  Serial.println("=============================");

  str = "ID_NO= " + String(ID_NO) +
        " Code= " + String(code) +
        " Weight= " + String(weight) +
        " Price= " + String(price);

  mySerial.listen(); // make mySerial active
  mySerial.println(str);
}


