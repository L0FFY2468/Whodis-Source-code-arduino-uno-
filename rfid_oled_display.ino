/*
  RFID Card -> OLED Name Display
  ------------------------------
  Tap an RFID card/tag on an RC522 reader and show the matching
  person's name on a 0.96" SSD1306 OLED (I2C).

  Hardware:
    - Arduino Uno
    - MFRC522 RFID Reader Module (SPI)
    - 0.96" OLED Display, SSD1306 driver, 128x64, I2C

  Wiring - RC522 (SPI):
    RC522 pin   Uno pin
    ---------   -------
    SDA (SS)    10
    SCK         13
    MOSI        11
    MISO        12
    IRQ         (not connected)
    GND         GND
    RST         9
    3.3V        3.3V   <-- RC522 is 3.3V only, never wire it to 5V

  Wiring - OLED (I2C):
    OLED pin    Uno pin
    ---------   -------
    GND         GND
    VCC         5V (most breakout boards regulate this fine)
    SCL         A5
    SDA         A4

  Libraries required (install via Library Manager):
    - MFRC522        by GithubCommunity
    - Adafruit SSD1306
    - Adafruit GFX Library

  How to add your own cards:
    1. Upload this sketch and open Serial Monitor (9600 baud).
    2. Tap an unregistered card - its UID prints there, e.g. "DE AD BE EF".
    3. Copy those bytes into the knownCards[] list below with a name.
    4. Re-upload.
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- RFID (RC522) ----------
#define SS_PIN  10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

// ---------- OLED (SSD1306) ----------
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDRESS   0x3C   // change to 0x3D if the display fails to init
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- Known cards: edit this list ----------
struct KnownCard {
  byte uid[4];
  const char* name;
};

KnownCard knownCards[] = {
  {{0x23, 0x3D, 0x09, 0x03}, "Zia"},
  {{0x56, 0x8D, 0xF5, 0xA6}, "Danial"},
  {{0x3A, 0x84, 0x14, 0xCD}, "Adam"},
  // Add more like: {{0x.., 0x.., 0x.., 0x..}, "Name"},
};
const int numKnownCards = sizeof(knownCards) / sizeof(knownCards[0]);

void setup() {
  Serial.begin(9600);

  SPI.begin();
  rfid.PCD_Init();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("OLED not found - check wiring/address"));
    while (true); // halt
  }

  showMessage("READY", "Scan card");
}

void loop() {
  // Look for a new card; return early if none present
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Print the UID so you can register new cards
  Serial.print("UID:");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  String name = matchCard(rfid.uid.uidByte, rfid.uid.size);

  if (name != "") {
    showMessage("ACCESS GRANTED", name);
  } else {
    showMessage("UNKNOWN CARD", getUidCompact(rfid.uid.uidByte, rfid.uid.size));
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1500); // debounce - stops the same tap re-triggering instantly
}

// Look up a scanned UID in knownCards[]; returns "" if no match
String matchCard(byte *uid, byte size) {
  for (int i = 0; i < numKnownCards; i++) {
    bool match = true;
    for (byte j = 0; j < 4 && j < size; j++) {
      if (knownCards[i].uid[j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) return String(knownCards[i].name);
  }
  return "";
}

// Compact hex string for the OLED, e.g. "DEADBEEF"
String getUidCompact(byte *uid, byte size) {
  String result = "";
  for (byte i = 0; i < size; i++) {
    if (uid[i] < 0x10) result += "0";
    result += String(uid[i], HEX);
  }
  result.toUpperCase();
  return result;
}

// Two-line OLED message: small header, bigger detail line
void showMessage(String header, String detail) {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(header);
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 25);
  display.println(detail);

  display.display();
}
