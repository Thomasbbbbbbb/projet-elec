#include <EEPROM.h>

#include <RTClib.h>
#include<LedControl.h>
#include <HCSR04.h>

#include <SPI.h>

#include <Wire.h>

#include <Adafruit_GFX.h>

#include <Adafruit_SSD1306.h>

#include <Encoder.h>

#include <TimeLib.h>


#define SCREEN_WIDTH 128

#define SCREEN_HEIGHT 64

#define OLED_RESET -1

#define SCREEN_ADDRESS 0x3C

#define ERASE_BUTTON_PIN 6

#define ALARM_BUTTON_PIN 4

#define CONFIRM_BUTTON_PIN 5

const int NB_MAX7219 = 4;


RTC_DS1307 rtc;

LedControl lc = LedControl(11, 12, 13, NB_MAX7219);// DIN, CLK, CS de la matrice led


const int trigPin = 10;
const int echoPin = 9;

HCSR04 hcsr04(trigPin, echoPin);

const int seuilDistance = 15;
unsigned long startTime = 0;
unsigned long index = 0;
bool format12h = false; // Déclaration de format12h en tant que variable globale

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Encoder myEnc(2, 3); // Broches A et B de l'encodeur


int selected = 0;

int entered = -1;

long lastEncoded = 0;

long encoderValue = 0;

tmElements_t tm;

unsigned long lastButtonClickTime = 0;

const unsigned long doubleClickThreshold = 3000; // 3 secondes

boolean alarmOn = true; // État initial de l'alarme

void enregistrerTemps(unsigned long temps) {
  int address = index;
  EEPROM.put(address * sizeof(unsigned long), temps);
}

void afficherTemps() {
  entered = 0;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  for (int i = 0; i < index; i++) {
    unsigned long tempsLu;
    EEPROM.get(i * sizeof(unsigned long), tempsLu);
    display.setCursor(0, i * 8); // 8 pixels height per line
    display.print("Temps ");
    display.print(i + 1);
    display.print(": ");
    display.println(tempsLu);
  }

  delay(1000);

  while (entered == 0) {

    int confirmButtonState = digitalRead(CONFIRM_BUTTON_PIN);

    if (confirmButtonState == LOW) {

      // Logique pour traiter la sélection (12h ou 24h)

      // Mettez à jour entered pour sortir de la boucle

      entered = -1;

    }

  }

  display.display();
}

void effacerDonnees() {
  for (int i = 0; i < index; i++) {
    EEPROM.put(i * sizeof(unsigned long), 0);
  }
  index = 0;
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Donnees effacees");
  display.display();
  delay(2000);
  display.clearDisplay();
}
void displayMenu();

void formatHeure();

void reglerReveil();

void tempsDeSnooze();

void toggleAlarm();


void toggleAlarm() {

  alarmOn = !alarmOn;

}

void setup() {

  Serial.begin(57600);

if (!rtc.begin()) {

Serial.println("Couldn't find RTC");

Serial.flush();

abort();

}


if (!rtc.isrunning()) {

Serial.println("RTC is NOT running, setting the time!");

rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

}


for (int index = 0; index < NB_MAX7219; index++) {

lc.shutdown(index, false);

lc.setIntensity(index, 6);

lc.clearDisplay(index);

}



  pinMode(CONFIRM_BUTTON_PIN, INPUT_PULLUP); // Bouton pour valider

  pinMode(ALARM_BUTTON_PIN, INPUT_PULLUP);   // Bouton pour changer l'état de l'alarme

  pinMode(ERASE_BUTTON_PIN, INPUT_PULLUP); // Configure le pin du bouton comme une entrée avec résistance de tirage

  Serial.begin(9600);


  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {

    Serial.println(F("SSD1306 allocation failed"));

    for (;;)

      ;

  }

  display.clearDisplay();

  display.display();

  delay(2000);
  Serial.begin(57600);


}



void montrerChiffre(int chiffre, int panneau) {


byte chiffres[10][8] = {

{B00011100, B00100010, B00100010, B00100010, B00100010, B00100010, B00100010, B00011100}, // 0

{B00001000, B00011000, B00001000, B00001000, B00001000, B00001000, B00001000, B00011100}, // 1

{B00011100, B00100010, B00000010, B00000010, B00000100, B00001000, B00010000, B00111110}, // 2

{B00011100, B00100010, B00000010, B00001100, B00000010, B00000010, B00100010, B00011100}, // 3

{B00000100, B00001100, B00010100, B00100100, B00100100, B00111110, B00000100, B00000100}, // 4

{B00111110, B00100000, B00100000, B00111100, B00000010, B00000010, B00100010, B00011100}, // 5

{B00011100, B00100010, B00100000, B00111100, B00100010, B00100010, B00100010, B00011100}, // 6

{B00111110, B00000010, B00000010, B00000100, B00001000, B00010000, B00010000, B00010000}, // 7

{B00011100, B00100010, B00100010, B00011100, B00100010, B00100010, B00100010, B00011100}, // 8

{B00011100, B00100010, B00100010, B00100010, B00011110, B00000010, B00100010, B00011100} // 9

};

for (int i = 0; i < 8; i++) {

lc.setRow(panneau, i, chiffres[chiffre][i]);

}

}


void loop() {


  DateTime now = rtc.now();


int dizaineHeure = (now.hour() / 10);

montrerChiffre(dizaineHeure, 3);


int uniteHeure = (now.hour() % 10);

montrerChiffre(uniteHeure, 2);


int dizaineMinute = (now.minute() / 10);

montrerChiffre(dizaineMinute, 1);


int uniteMinute = (now.minute() % 10);

montrerChiffre(uniteMinute, 0);

 
  unsigned int distance = hcsr04.dist();


  displayMenu();

  int confirmButtonState = digitalRead(CONFIRM_BUTTON_PIN);

  if (distance <= seuilDistance && alarmOn == true) {
    Serial.println(distance);
    unsigned long endTime = millis();
    index = index + 1;
    unsigned long elapsedTime = endTime - startTime;
    Serial.print("Temps pour eteindre le reveil : ");
    Serial.print(elapsedTime);
    Serial.println(" microsecondes");
    toggleAlarm();

    enregistrerTemps(elapsedTime);
  }

  if (digitalRead(ERASE_BUTTON_PIN) == LOW) {

    effacerDonnees();

    delay(500); // Délai pour éviter les rebonds du bouton

  }


  // Appel de la fonction associée à l'option sélectionnée

  if (confirmButtonState == LOW) {

    unsigned long currentTime = millis();


    if (currentTime - lastButtonClickTime > doubleClickThreshold) {

      // Premier appui

      lastButtonClickTime = currentTime;

      entered = selected;


      switch (entered) {

        case 0:

          formatHeure();

          break;

        case 1:

          reglerReveil();

          break;

        case 2:

          afficherTemps();

          break;

        
      }

    } else {

      // Deuxième appui dans le délai, retour au menu

      entered = -1;

    }

  }


  // Vérifier si le bouton de l'alarme est pressé

  if (digitalRead(ALARM_BUTTON_PIN) == LOW) {

    toggleAlarm();

    delay(500); // Délai pour éviter les rebonds du bouton

  }

}


void displayMenu() {

  int confirmButtonState = digitalRead(CONFIRM_BUTTON_PIN);


  long encoderNew = myEnc.read();

  if (encoderNew != lastEncoded) {

    if ((encoderNew > lastEncoded) || (encoderNew < lastEncoded)) {

      selected += (encoderNew > lastEncoded) ? 1 : -1;

      if (selected < 0)

        selected = 2;

      if (selected > 2)

        selected = 0;

    }

  }

  lastEncoded = encoderNew;

  if (confirmButtonState == LOW) {

    entered = selected;


    // Appeler la fonction associée à l'option sélectionnée

    switch (entered) {

      case 0:

        formatHeure();

        break;

      case 1:

        reglerReveil();

        break;
      case 2:

          afficherTemps();

          break;

    }

  }


  const char *options[3] = {

      " 1.Format heure",

      " 2.Regler reveil",

      " 3.Temps pour eteindre reveil "};


  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);

  display.println(F("Menu"));

  display.println("");


  for (int i = 0; i < 3; i++) {

    if (i == selected) {

      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);

      display.println(options[i]);

    } else {

      display.setTextColor(SSD1306_WHITE);

      display.println(options[i]);

    }

  }


  // Afficher l'état de l'alarme en bas de l'écran

  display.setCursor(0, SCREEN_HEIGHT - 16);

  display.print(F("Alarm: "));

  display.print(alarmOn ? F("On") : F("Off"));


  display.display();

}


void formatHeure() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Format Heure"));
  display.println("");
  display.println(F("1. 12 heures"));
  display.println(F("2. 24 heures"));
  display.display();
  delay(500);

  entered = 0;
  selected = 0;

  while (entered == 0) {
    long encoderNew = myEnc.read();

    if (encoderNew != lastEncoded) {
      if ((encoderNew > lastEncoded) || (encoderNew < lastEncoded)) {
        selected += (encoderNew > lastEncoded) ? 1 : -1;
        if (selected < 0)
          selected = 1;
        if (selected > 1)
          selected = 0;
      }
    }
    lastEncoded = encoderNew;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("Format Heure"));
    display.println("");
    if (selected == 0) {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      display.println(F("1. 12 heures"));
      display.setTextColor(SSD1306_WHITE);
      display.println(F("2. 24 heures"));
    } else {
      display.setTextColor(SSD1306_WHITE);
      display.println(F("1. 12 heures"));
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      display.println(F("2. 24 heures"));
    }
    display.display();

    int confirmButtonState = digitalRead(CONFIRM_BUTTON_PIN);
    if (confirmButtonState == LOW) {
      if (selected == 1) {
        format12h = true; // Sélection du format 12h
        Serial.println("format 12h");
        entered = -1;
      } else if (selected == 0) {
        format12h = false; // Sélection du format 24h
        Serial.println("format 24h");
        entered = -1;
      }
    }
  }
  delay(1000);
}


void reglerReveil() {

  entered = 0;

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);

  display.println(F("Regler Reveil"));

  display.println("");

  display.display();


  tm.Hour = 0;

  tm.Minute = 0;

  tm.Second = 0;


  // Multiplicateur de vitesse pour ajuster la sensibilité de l'encodeur

  int speedMultiplier = 1; // Vous pouvez ajuster ce nombre selon vos besoins

  delay(500);


  while (entered == 0) {
    if (format12h == true) {
      long encoderNew = myEnc.read();

    if (encoderNew != lastEncoded) {

      if ((encoderNew > lastEncoded) || (encoderNew < lastEncoded)) {

        // Logique pour traiter les variations de l'encodeur numérique

        tm.Minute += (encoderNew > lastEncoded) ? speedMultiplier : -speedMultiplier;

        if (tm.Minute > 59) {
           tm.Minute = 0;
        if (tm.Hour == 12) {
           tm.Hour = 1; // Si c'est midi (12:00), passer à 1:00
        } else {
           tm.Hour = (tm.Hour + 1) % 12; // Sinon, passer à l'heure suivante dans le format 12h
        }
        } else if (tm.Minute < 0) {
          tm.Minute = 59;
        if (tm.Hour == 1) {
           tm.Hour = 12; // Si c'est 1:00, revenir à midi (12:00)
        } else {
          tm.Hour = (tm.Hour + 11) % 12; // Sinon, revenir à l'heure précédente dans le format 12h
        }
        }


        display.clearDisplay();

        display.setTextSize(2);

        display.setTextColor(SSD1306_WHITE);

        display.setCursor(10, 10);

        display.print(String(tm.Hour) + ":" + String(tm.Minute));

        display.display();

      }
      

      

      lastEncoded = encoderNew;

    }
    }
    else {
    long encoderNew = myEnc.read();

    if (encoderNew != lastEncoded) {

      if ((encoderNew > lastEncoded) || (encoderNew < lastEncoded)) {

        // Logique pour traiter les variations de l'encodeur numérique

        tm.Minute += (encoderNew > lastEncoded) ? speedMultiplier : -speedMultiplier;

        if (tm.Minute > 59) {

          tm.Minute = 0;

          tm.Hour = (tm.Hour + 1) % 24; // Passage à une nouvelle heure

        } else if (tm.Minute < 0) {

          tm.Minute = 59;

          tm.Hour = (tm.Hour + 23) % 24; // Passage à une nouvelle heure

        }


        display.clearDisplay();

        display.setTextSize(2);

        display.setTextColor(SSD1306_WHITE);

        display.setCursor(10, 10);

        display.print(String(tm.Hour) + ":" + String(tm.Minute));

        display.display();

      }
      

      

      lastEncoded = encoderNew;

    } }
    int confirmButtonState = digitalRead(CONFIRM_BUTTON_PIN);

    if (confirmButtonState == LOW) {

        // Logique pour traiter la sélection (12h ou 24h)

        // Mettez à jour entered pour sortir de la boucle

        entered = -1;
             
        }

  }

   
  displayMenu();
  entered = -1;

}
