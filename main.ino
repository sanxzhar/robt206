#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <ArduinoSTL.h>
#include <set>

#define SS_PIN 10
#define RST_PIN 9
#define TRIG_PIN 7       // Ultrasonic sensor trigger pin
#define ECHO_PIN 6       // Ultrasonic sensor echo pin
#define MAX_DISTANCE 10  // Maximum distance to detect objects (in cm)
#define MAX_CARDS 10     // Maximum number of authorized cards
#define UID_SIZE 4       // Size of UID in bytes
#define EEPROM_START 0   // Starting address in EEPROM
#define GAS_PIN A0
#define THRESHOLD 260


Servo servo;
bool isGateOpen = false;
bool isMasterMode = false;
bool gasAlerted = false;
std::set<String> cardList;

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
LiquidCrystal_I2C lcd(0x27, 16, 2);

MFRC522::MIFARE_Key key;


void setup() { 
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init RC522 
  servo.attach(8);
  lcd.init();
  lcd.backlight();
  
  // Setup ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

// Function to measure distance using ultrasonic sensor
int getDistance() {
  // Clear the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Set the trigger pin HIGH for 10 microseconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read the echo pin, returns the sound wave travel time in microseconds
  long duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate distance in centimeters
  // Speed of sound is 343 m/s or 0.0343 cm/μs
  // Distance = (time × speed) / 2 (divide by 2 because sound travels to object and back)
  int distance = (duration * 0.0343) / 2;
  
  return distance;
}
 
void loop() {

  if (isGasDetected()) {
    if (!gasAlerted) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WARNING:");
      lcd.setCursor(0, 1);
      lcd.print("Gas Detected!");
      Serial.println("ALERT: FIRE AT PARKING!");
      gasAlerted = true;
    }

    return;
  } else {
    gasAlerted = false;
  }

  lcd.setCursor(0,0);
  lcd.print("Please attach");
  lcd.setCursor(0, 1);
  lcd.print("your ID card :)");

  // Check for objects using ultrasonic sensor
  int distance = getDistance();
  
  if (distance > 0 && distance <= MAX_DISTANCE) { 
    Serial.print("Object detected at distance: ");
    Serial.print(distance);
    
    if(isGateOpen){
      closeGate();
      isGateOpen = false;
    }else{
      openGate();
      delay(1500);
      closeGate();
    }
  }

if (rfid.PICC_IsNewCardPresent()) {
    // Verify if the NUID has been read
    if (rfid.PICC_ReadCardSerial()) {
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
     
      Serial.print(F("RFID Tag UID:"));
      
      String hexKeyValue = bytesToHexString(rfid.uid.uidByte, rfid.uid.size);
      printHex(hexKeyValue);

      if(isMasterCard(hexKeyValue)){
        isMasterMode = !isMasterMode;

        lcd.clear();
        if (isMasterMode) {
          Serial.println("Master mode activated");
          lcd.setCursor(0,0);
          lcd.print("MASTER MODE ON");
        } else {
          Serial.println("Master mode deactivated");
          lcd.setCursor(0,0);
          lcd.print("MASTER MODE OFF");
        }
        delay(2000);
        lcd.clear();
      } else if (isMasterMode) {
        if (isCardStored(hexKeyValue)) {
          cardList.erase(hexKeyValue);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Card removed!");
          Serial.println("Card removed from authorized list");
        } else {
          cardList.insert(hexKeyValue);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Card added!");
          Serial.println("Card added to authorized list");
        }
        delay(2000);
        lcd.clear();
      } else {
        if (isCardStored(hexKeyValue)) {
          Serial.print(" Valid");
          openGate();
          isGateOpen = true;

          lcd.clear();
          lcd.setCursor(1,0);
          lcd.print("Welcome home!");
          delay(3000);
          lcd.clear();
        } else {
          lcd.clear();
          lcd.setCursor(1,0);
          lcd.print("Access denied!");
          delay(3000);
          lcd.clear();
        }
      }
        
      }
    
      Serial.println("");
    
      rfid.PICC_HaltA(); // Halt PICC
  }
}

bool isCardStored(String uidString) {
  if (cardList.find(uidString) != cardList.end()) {
    return true;
  }

  return false;
}

bool isGasDetected(){
  int sensorValue = analogRead(GAS_PIN);
  Serial.println(sensorValue);

  return sensorValue > THRESHOLD;
}

bool isMasterCard(String keyValue) {
  keyValue.toUpperCase();
  return keyValue == "95 59 55 2E";
}

void openGate() {
    servo.write(0);
    delay(200);
    servo.write(90);
}

void closeGate() {
    servo.write(180);
    delay(200);
    servo.write(90);
}


String bytesToHexString(byte *buffer, byte bufferSize) {
  String hexString = "";
  for (byte i = 0; i < bufferSize; i++) {
    if (buffer[i] < 0x10) {
      hexString += "0";
    }
    hexString += String(buffer[i], HEX);
    if (i < bufferSize - 1) {
      hexString += " ";
    }
  }
  return hexString;
}

void printHex(String keyValue) {
    Serial.print(keyValue);
}