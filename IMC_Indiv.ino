  /*  RFID & Keypad Lock voor Arduino
   *  IMC Individueel deel
   *  Yoram van Spijk
   * 
   *  Alles, behalve de libraries in de includes, is zelf geschreven
   */

#include <SPI.h>
#include <Keypad.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>

#define SS_PIN 10
#define RST_PIN 9
#define MAX_TAGS 10

const byte ROWS = 4;
const byte COLS = 4;
byte cardRead[4];
char* knownTags[MAX_TAGS] = {};
char* knownTagsPasscode[MAX_TAGS] = {};
int tagCount = 0;
String tagID = "";
boolean readSuccess = false;
boolean correctTag = false;
boolean doorOpened = false;

char hexaKeys[ROWS][COLS] =
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {4, 3, 2, 0}; //row pins on arduino
byte colPins[COLS] = {8, 7, 6, 5}; //column pins on arduino
Keypad kp = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); //initialize keypad instance
MFRC522 rfid(SS_PIN, RST_PIN); //initialize cardReader instance
Servo myServo;

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  SPI.begin();
  myServo.attach(13);

  Serial.println("Creating admin..");
  Serial.println("Scan tag now!");

  while (!readSuccess) 
  {
    readSuccess = getCardID();
    if (readSuccess == true) 
    {
      knownTags[tagCount] = strdup(tagID.c_str());
      knownTagsPasscode[tagCount] = enterPasscode();
      Serial.println("Admin tag made!");
    }
  }
}

void loop()
{
  Serial.println("This is a RFID operated lock, please scan your card.");

  if ( ! rfid.PICC_IsNewCardPresent())
  { //Check for new card
    return;
  }

  if ( ! rfid.PICC_ReadCardSerial())
  { //Get serial code of card
    return;
  }

  tagID = "";
  for ( uint8_t i = 0; i < 4; i++)
  {
    cardRead[i] = rfid.uid.uidByte[i];
    tagID.concat(String(rfid.uid.uidByte[i], HEX)); // Read the 4 bytes on the card, and store them in a single tagID.
  }
  tagID.toUpperCase();
  rfid.PICC_HaltA(); // Stop reading, avoids spam
  correctTag = false;

  if (tagID == knownTags[0])
  {
    Serial.println("Admin tag found!");
    if (getPasscode(tagID, knownTagsPasscode[0])) // Passcode is accepted
    {
      Serial.println("You can add and remove tags now!");
      while (!readSuccess)
      {
        readSuccess = getCardID();
        if ( readSuccess == true)
        {
          for (int i = 1; i < MAX_TAGS; i++)
          {
            if (tagID == knownTags[i])
            {
              knownTags[i] = "";
              Serial.println("Tag removed!");
              return;
            }

            knownTags[tagCount] = strdup(tagID.c_str());
            knownTagsPasscode[tagCount] = enterPasscode();
            Serial.println("New tag added!");
            tagCount++;
            return;
          }
        }
      }
    }
  }

  readSuccess = false;

  for (int i = 0; i < 100; i++)
  { // Check if the scanned tag is known to the system
    if (tagID == knownTags[i])
    {
      if (getPasscode(tagID, knownTagsPasscode[i]))
      {
        Serial.println("Passcode correct! Access granted.");
        myServo.write(170);   // OPen lock
        correctTag = true;
      }
    }
  }

  if (correctTag == false)
  {
    Serial.println("Access denied!");
  }
  else
  {
    Serial.println("Door is unlocked! Locking again after 10 seconds...");
    delay(10000);
    myServo.write(10);
  }
}

uint8_t getCardID()
{
  if ( ! rfid.PICC_IsNewCardPresent())
  { //Look for new cards
    return 0;
  }

  if ( ! rfid.PICC_ReadCardSerial())
  { //Card was placed, get serial
    return 0;
  }

  tagID = "";

  for ( uint8_t i = 0; i < 4; i++)
  {
    cardRead[i] = rfid.uid.uidByte[i];
    tagID.concat(String(rfid.uid.uidByte[i], HEX));
  }

  tagID.toUpperCase();
  rfid.PICC_HaltA(); // Stop reading cards, avoids spam

  return 1;
}

void printNormalMessage() {
  delay(1500);
  Serial.println("This is a RFID operated lock, please scan your card.");
}

boolean getPasscode(String NUID, int passcode)
{
  for (int i = 0; i < MAX_TAGS; i++)
  {
    if (strcmp(knownTags[i], (NUID.c_str())))
    {
      Serial.println("Card found!");
      int enteredPasscode = enterPasscode();
      if (enteredPasscode == passcode)
      {
        return true;
      }
    }
    else
    {
      Serial.println("Card not found!");
      Serial.println("Register card first!");
      return false;
    }
  }
}

int enterPasscode()
{
  int num = 0;
  char key = kp.getKey();
  Serial.println("Enter a 4 digit passcode");
  Serial.println("Press the # when you're done");
  Serial.println("Use * to reset the passcode");

  while (key != '#')
  {
    switch (key)
    {
      case NO_KEY:
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        num = num * 10 + (key - 0);
        break;

      case '*':
        num = 0;
        break;
    }
    key = kp.getKey();
  }

  return num;
}
