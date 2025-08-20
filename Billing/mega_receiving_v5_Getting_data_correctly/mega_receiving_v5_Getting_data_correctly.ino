#include <U8g2lib.h>
#include <SD.h>
#include <SPI.h>

U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, 13, 11, 10);
const int chipSelect = 53;

const int BTN_NEXT = 4;
const int BTN_PREV = 5;
const int BTN_ENTER = 6;
const int BTN_CLEAR = 7;

struct Rec { String uid; String code; float weight; float price; String name; };
Rec recs[80];
int recCount = 0;
int page = 0;
const int perPage = 4;
String activeUID = "";

String inBuf = "";
unsigned long lastBtnMs[4] = {0,0,0,0};
const unsigned long btnDebounce = 150;
unsigned long lastDisplayMs = 0;
const unsigned long displayInterval = 500;


void setup() {
  Serial.begin(9600);   // USB monitor
  Serial1.begin(9600);  // Nano serial on pins 19(RX)/18(TX)

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);
  pinMode(BTN_CLEAR, INPUT_PULLUP);

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);

  u8g2.clearBuffer();
  u8g2.drawStr(0,12,"Initializing...");
  u8g2.sendBuffer();
  delay(500);

  if (!SD.begin(chipSelect)) {
    u8g2.clearBuffer();
    u8g2.drawStr(0,12,"SD init failed");
    u8g2.sendBuffer();
    while (1) {}
  }

  u8g2.clearBuffer();
  u8g2.drawStr(0,12,"SD Initialized.");
  u8g2.sendBuffer();
  delay(1000);
}

void loop() {
  readNanoSerial();           // non-blocking serial
  handleButtonsNonBlocking(); // non-blocking buttons

  if (millis() - lastDisplayMs >= displayInterval) {
    drawPage();
    lastDisplayMs = millis();
  }
}

// ----------------- SERIAL (non-blocking) -----------------
void readNanoSerial() {
  if (Serial1.available()) {
    char c = Serial1.read();
    inBuf += c;

    if (c == '\n') {
      inBuf.trim();
      Serial.println("Received from Nano: " + inBuf);

      if (inBuf.startsWith("ID_NO=")) {
        parseNanoLine(inBuf);   // item data
      } else if (isUIDString(inBuf)) {
        handleUID(inBuf);       // UID only
      }

      inBuf = ""; // reset buffer
    }
  }
}

// ----------------- UID CHECK -----------------
bool isUIDString(const String &s) {
  if (s.length() == 0) return false;
  for (unsigned int i=0; i<s.length(); i++) {
    if (!isDigit(s[i])) return false;
  }
  return true;
}

// ----------------- HANDLE UID SCAN -----------------
void handleUID(const String &uid) {
  activeUID = uid;
  Serial.println("UID scanned: " + uid);
  loadUID(uid);
  if (recCount == 0) {
    Serial.println("No records found for UID " + uid);
  }
  drawPage();
}

// ----------------- PARSE NANO LINE -----------------
void parseNanoLine(const String &s) {
  int iID = s.indexOf("ID_NO=");     if(iID<0) return;
  int iCode = s.indexOf("Code=", iID);  if(iCode<0) return;
  int iWeight = s.indexOf("Weight=", iCode); if(iWeight<0) return;
  int iPrice = s.indexOf("Price=", iWeight); if(iPrice<0) return;

  String uidStr = s.substring(iID + 6, iCode); uidStr.trim();
  String codeStr = s.substring(iCode + 5, iWeight); codeStr.trim();
  String weightStr = s.substring(iWeight + 7, iPrice); weightStr.trim();
  String priceStr = s.substring(iPrice + 6); priceStr.trim();

  // Print parsed values
  Serial.println("==== Parsed Nano Data ====");
  Serial.println("ID_NO    : " + uidStr);
  Serial.println("Code     : " + codeStr);
  Serial.println("Weight   : " + weightStr);
  Serial.println("Price    : " + priceStr);
  Serial.println("==========================");

  // Log to SD file
  String line = uidStr + "," + codeStr + "," + weightStr + "," + priceStr;
  appendLine("data.txt", line);
  appendLine("log.txt", line);

  // Load into display buffer if UID matches activeUID
  if (activeUID == uidStr) loadUID(activeUID);
}

// ----------------- PARSE & LOG -----------------
/*void parseAndLog(const String &s) {
  int i1 = s.indexOf("ID_NO="); if (i1 < 0) return;
  int i2 = s.indexOf("Code=", i1); if (i2 < 0) return;
  int i3 = s.indexOf("Weight=", i2); if (i3 < 0) return;
  int i4 = s.indexOf("Price=", i3); if (i4 < 0) return;

  String uid = s.substring(i1 + 6, i2); uid.trim();
  String code = s.substring(i2 + 5, i3); code.trim();
  String wStr = s.substring(i3 + 7, i4); wStr.trim();
  String pStr = s.substring(i4 + 6);     pStr.trim();

  float w = wStr.toFloat();
  float p = pStr.toFloat();

  String line = uid + "," + code + "," + String(w,3) + "," + String(p,2);
  appendLine("data.txt", line);
  appendLine("log.txt",  line);

  Serial.println("Logged: " + line);
}*/

void appendLine(const char *fname, const String &line) {
  File f = SD.open(fname, FILE_WRITE);
  if (!f) { Serial.print("Failed to open "); Serial.println(fname); return; }
  f.println(line);
  f.flush();
  f.close();
}

// ----------------- LOAD UID -----------------
/*void loadUID(const String &uid) {
  recCount = 0;
  File f = SD.open("data.txt", FILE_READ);
  if (!f) { Serial.println("Failed to open data.txt"); return; }

  while (f.available() && recCount < (int)(sizeof(recs)/sizeof(recs[0]))) {
    String line = f.readStringUntil('\n'); line.trim();
    if (line.length()==0) continue;

    int c1 = line.indexOf(','); if (c1<0) continue;
    int c2 = line.indexOf(',', c1+1); if (c2<0) continue;
    int c3 = line.indexOf(',', c2+1); if (c3<0) continue;

    String lu = line.substring(0,c1); lu.trim();
    //if (lu != uid) continue;
    if (lu.trim() != uid.trim()) continue;


    String code = line.substring(c1+1, c2); code.trim();
    String wStr = line.substring(c2+1, c3); wStr.trim();
    String pStr = line.substring(c3+1);     pStr.trim();

    recs[recCount].uid = lu;
    recs[recCount].code = code;
    recs[recCount].weight = wStr.toFloat();
    recs[recCount].price  = pStr.toFloat();
    recs[recCount].name = lookupName(code);
    recCount++;
  }
  f.close();
  Serial.print("Loaded "); Serial.print(recCount); Serial.println(" records for UID");
}*/

// ----------------- LOAD UID -----------------
void loadUID(const String &uid) {
  recCount = 0;
  File f = SD.open("data.txt", FILE_READ);
  if (!f) { 
    Serial.println("Failed to open data.txt"); 
    return; 
  }

  String trimmedUID = uid;
  trimmedUID.trim();  // trim the incoming UID

  while (f.available() && recCount < (int)(sizeof(recs)/sizeof(recs[0]))) {
    String line = f.readStringUntil('\n'); 
    line.trim();  // remove leading/trailing spaces or hidden characters
    if (line.length() == 0) continue;

    int c1 = line.indexOf(','); if (c1 < 0) continue;
    int c2 = line.indexOf(',', c1 + 1); if (c2 < 0) continue;
    int c3 = line.indexOf(',', c2 + 1); if (c3 < 0) continue;

    String lu = line.substring(0, c1); 
    lu.trim();  // trim the stored UID

    if (lu != trimmedUID) continue;  // compare trimmed UIDs

    String code = line.substring(c1 + 1, c2); code.trim();
    String wStr = line.substring(c2 + 1, c3); wStr.trim();
    String pStr = line.substring(c3 + 1); pStr.trim();

    recs[recCount].uid = lu;
    recs[recCount].code = code;
    recs[recCount].weight = wStr.toFloat();
    recs[recCount].price  = pStr.toFloat();
    recs[recCount].name = lookupName("00"+code);
    recCount++;
  }

  f.close();
  Serial.print("Loaded "); Serial.print(recCount); Serial.println(" records for UID");
}



/*String lookupName(const String &code) {
  File f = SD.open("products.txt", FILE_READ);
  if (!f) return String("");
  String name = "";
  while (f.available()) {
    String line = f.readStringUntil('\n'); line.trim();
    int c = line.indexOf(','); if (c<0) continue;
    int col = line.indexOf(':', c+1); if (col<0) continue;
    String ccode = line.substring(0,c); ccode.trim();
    if (ccode == code) { name = line.substring(c+1, col); name.trim(); break; }
  }
  f.close();
  return name;
}*/

String lookupName(const String &code) {
  File f = SD.open("products.txt", FILE_READ);
  if (!f) return "";

  String tcode = code;
  tcode.trim();
  String itemname = "";

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    int c = line.indexOf(',');
    int col = line.indexOf(':', c + 1);
    if (c < 0 || col < 0) continue;

    String ccode = line.substring(0, c);
    ccode.trim();

    if (ccode == tcode) {
      itemname = line.substring(c + 1, col); // between comma and colon
      itemname.trim();
      break;
    }
  }

  f.close();
  return itemname;
}





// ----------------- DISPLAY -----------------
/*void drawPage() {
  u8g2.clearBuffer();

  if (recCount == 0) {
    u8g2.drawStr(0,12,"No Data Found");
    u8g2.sendBuffer();
    return;
  }

  int pages = (recCount + perPage - 1) / perPage;
  if (page < 0) page = 0;
  if (page >= pages) page = pages - 1;

  String hdr = "UID:" + activeUID + " P:" + String(page+1) + "/" + String(pages);
  u8g2.drawStr(0,12, hdr.c_str());

  int start = page * perPage;
  int y = 26;
  for (int i = 0; i < perPage; i++) {
    int idx = start + i;
    if (idx >= recCount) break;

    String line1 = recs[idx].code + " " + recs[idx].name;
    String line2 = "W:" + String(recs[idx].weight,3) + " P:" + String(recs[idx].price,2);
    u8g2.drawStr(0,y, line1.c_str()); y += 12;
    u8g2.drawStr(0,y, line2.c_str()); y += 12;
  }

  u8g2.sendBuffer();
}*/

void drawPage() {
  u8g2.clearBuffer();

  if (recCount == 0) {
    u8g2.drawStr(0,12,"No Data Found");
    u8g2.sendBuffer();
    return;
  }

  int pages = (recCount + perPage - 1) / perPage;
  if (page < 0) page = 0;
  if (page >= pages) page = pages - 1;

  String hdr = "P:" + String(page+1) + "/" + String(pages);
  u8g2.drawStr(0,12, hdr.c_str());

  int start = page * perPage;
  int y = 26;
  for (int i = 0; i < perPage; i++) {
    int idx = start + i;
    if (idx >= recCount) break;

    String line = recs[idx].name + " " 
                  + String(recs[idx].weight,3) + " " 
                  + String(recs[idx].price,2);

    u8g2.drawStr(0,y, line.c_str()); 
    y += 12;
  }

  u8g2.sendBuffer();
}


// ----------------- BUTTONS -----------------
void handleButtonsNonBlocking() {
  unsigned long now = millis();

  if (digitalRead(BTN_NEXT) == LOW && now - lastBtnMs[0] > btnDebounce) {
    if (recCount>0){ page++; drawPage(); }
    lastBtnMs[0]=now; Serial.println("BTN_NEXT");
  }
  if (digitalRead(BTN_PREV) == LOW && now - lastBtnMs[1] > btnDebounce) {
    if (recCount>0){ page--; drawPage(); }
    lastBtnMs[1]=now; Serial.println("BTN_PREV");
  }
  /*if (digitalRead(BTN_ENTER)==LOW && now - lastBtnMs[2] > btnDebounce) {
    if (recCount>0){
      Serial.println("BTN_ENTER pressed - Items:");
      for(int i=0;i<recCount;i++){
        Serial.print("UID="); Serial.print(recs[i].uid);
        Serial.print(" Code="); Serial.print(recs[i].code);
        Serial.print(" Name="); Serial.print(recs[i].name);
        Serial.print(" Weight="); Serial.print(recs[i].weight,3);
        Serial.print(" Price="); Serial.println(recs[i].price,2);
      }
    }
    lastBtnMs[2]=now;
  }*/
  if (digitalRead(BTN_ENTER) == LOW && now - lastBtnMs[2] > btnDebounce) {
  if (recCount > 0) {
    float total = 0;
    for (int i = 0; i < recCount; i++) {
      total += recs[i].price;
    }

    u8g2.clearBuffer();
    String line1 = "Total = " + String(total, 2);
    u8g2.drawStr(0, 12, line1.c_str());
    u8g2.drawStr(0, 28, "Items added to");
    u8g2.drawStr(0, 44, "the bill");
    u8g2.sendBuffer();
    delay(5000);
  }
  lastBtnMs[2] = now;
}
  if (digitalRead(BTN_CLEAR)==LOW && now - lastBtnMs[3] > btnDebounce) {
    if(activeUID.length()>0){
      Serial.print("BTN_CLEAR - Clearing UID "); Serial.println(activeUID);
      clearUIDFromFile(activeUID);
      recCount=0; page=0;
    }
    lastBtnMs[3]=now;
  }
}

// ----------------- CLEAR UID -----------------
void clearUIDFromFile(const String &uid) {
  File inF = SD.open("data.txt", FILE_READ);
  if(!inF){ Serial.println("data.txt error"); return; }
  File outF = SD.open("data.tmp", FILE_WRITE);
  if(!outF){ inF.close(); Serial.println("tmp file error"); return; }

  while(inF.available()){
    String line = inF.readStringUntil('\n'); String t=line; t.trim();
    if(t.length()==0){ outF.println(line); continue; }
    int c1=t.indexOf(','); if(c1<0){ outF.println(line); continue; }
    String lu = t.substring(0,c1); lu.trim();
    if(lu!=uid) outF.println(line);
  }

  inF.close(); outF.flush(); outF.close();

  SD.remove("data.txt");
  File src = SD.open("data.tmp", FILE_READ);
  File dst = SD.open("data.txt", FILE_WRITE);
  while(src.available()) dst.write(src.read());
  src.close(); dst.close();
  SD.remove("data.tmp");

  Serial.println("UID cleared from data.txt");
}






