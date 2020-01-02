/*****************************************************************************************
 * 
 *  DESHYDRATEUR DE FILAMENT
 *  L.MERCET 01/01/2020
 * 
 ****************************************************************************************/


/* Dépendances */
#include <LiquidCrystal.h>
#include <OneWire.h>

/** Broche pour le contrôle du rétroéclairage */
const byte BACKLIGHT_PWM_PIN = 10;
const byte RELAY_PIN = A5;

/** Variable */
float DS18B20_temperature;
bool mode = false;
byte selectMode = 0;
int backlight = 10, tempsRestant, tempsMin = 1;
unsigned long long tempo, tempoBase;

// Variables propres au DS18B20
const int DS18B20_PIN=A1;
const int DS18B20_ID=0x28;
// Déclaration de l'objet ds
OneWire ds(DS18B20_PIN); // on pin DS18B20_PIN (a 4.7K resistor is necessary)

/** Objet LiquidCrystal pour communication avec l'écran LCD */
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

/** Énumération des boutons utilisables et des selections possibles*/
enum {
  BUTTON_NONE,  /*!< Pas de bouton appuyé */
  BUTTON_UP,    /*!< Bouton UP (haut) */
  BUTTON_DOWN,  /*!< Bouton DOWN (bas) */
  BUTTON_LEFT,  /*!< Bouton LEFT (gauche) */
  BUTTON_RIGHT, /*!< Bouton RIGHT (droite) */
  BUTTON_SELECT, /*!< Bouton SELECT */
  OFF, /*!< Selection OFF */
  BACKLIGHT, /*!< Selection BAKLIGHT */
  MINUTES, /*!< Selection TEMPS DE TRAVAIL */
  TEMPERATURE /*!< Selection TEMPERATURE DE CONSIGNE */
};

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  lcd.begin(16, 2);
  lcd.print("Deshydrateur");
  analogWrite(BACKLIGHT_PWM_PIN, backlight); // Pas de rétroéclairage
  pinMode(RELAY_PIN, OUTPUT);
  delay(2000);
  lcd.clear();
  lcd.print("240min hu20% OFF");
  lcd.setCursor(0,1);
  lcd.print("T 00.00  Rel OFF");
  lcd.setCursor(1,1);
  lcd.print((char)223);
}

void loop() {
  // put your main code here, to run repeatedly:
  /* Affiche le bouton appuyé */

  switch (getPressedButton()) {
  case BUTTON_NONE:
    if(!modeSel){
      lcd.setCursor(2, 1);
      lcd.print(DS18B20_temperature);
    }
    break;

  case BUTTON_UP:
    if(!modeSel){
      backlight++;
    }else{
      tempsMin++;
      delay(200);
    }
    break;

  case BUTTON_DOWN:
    if(!modeSel){
      backlight--;
    }else{
      tempsMin--;
      delay(200);
    }
    break;

  case BUTTON_LEFT:
    lcd.setCursor(13,0);
    lcd.print("OFF");
    mode = false;
    break;

  case BUTTON_RIGHT:
    lcd.setCursor(13,0);
    lcd.print("ON ");
    tempoBase = tempo;
    mode = true;
    modeSel = false; // evétier le bug
    lcd.setCursor(0,1);
    lcd.print("T");
    lcd.print((char)223);
    lcd.print(DS18B20_temperature);
    break;

  case BUTTON_SELECT:
    switch(selectMode){
      case OFF:
        selectMode = BACKLIGHT;
      break;
      case BACKLIGHT:
        selectMode = MINUTES;
      break;
      case MINUTES:
        selectMode = TEMPERATURE;
      break;
      case TEMPERATURE:
        selectMode = OFF;
      break;
    }
      /*
      modeSel = false;
      lcd.setCursor(0,1);
      lcd.print("T");
      lcd.print((char)223);
      lcd.print(DS18B20_temperature);
      modeSel = true;
      lcd.setCursor(0,1);
      lcd.print("SELECT ");
      delay(800);
      */
    break;
  }

  /* gestion du relai */
  relay();

  /* Délai pour l'affichage */
  analogWrite(BACKLIGHT_PWM_PIN, backlight);
  if( modeSel && !mode){
    
  }else{
    DS18B20_temperature = getTemperatureDS18b20(); // On lance la fonction d'acquisition de T°
    // on affiche la T°
    Serial.print("(DS18B20) =>\t temperature: "); 
    Serial.println(DS18B20_temperature);
  }
  
  /* gestion du temps restant */
  calculTime();

  if(tempsRestant == 0){
    lcd.setCursor(13,0);
    lcd.print("OFF");
    mode = false;
  }
}

/*-----------------------------------------------
 *  FONCTION ANNEXE
 */

void relay(){
  
  if(DS18B20_temperature < 30 && mode == true){
    digitalWrite(RELAY_PIN, HIGH);
    lcd.setCursor(13, 1);
    lcd.print("On ");
  }else{
    digitalWrite(RELAY_PIN, LOW);
    lcd.setCursor(13, 1);
    lcd.print("Off");
  }
  
}

/* Affichage du temps restant dans le déshydrateur */
void calculTime(){
  tempo = superMillis();
  tempo = tempo/1000;
  int temp = tempo - tempoBase;
  Serial.println(temp);
  if(mode){
    tempsRestant = tempsMin-(temp/60);
    Serial.println(tempsRestant);
    displayTime(tempsRestant);
  }else{
    displayTime(tempsMin);
  }
}

void displayTime(int t){
  if(t<10){
    lcd.setCursor(0,0);
    lcd.print("00");
    lcd.print(t);
  }else if(t<100){
    lcd.setCursor(0,0);
    lcd.print("0");
    lcd.print(t);
  }else{
    lcd.setCursor(0,0);
    lcd.print(t);
  }
}

/** Retourne le bouton appuyé (si il y en a un) */
byte getPressedButton() {

  /* Lit l'état des boutons */
  int value = analogRead(A0);
  //Serial.println(value);

  /* Calcul l'état des boutons */
  if (value < 50)
    return BUTTON_RIGHT;
  else if (value < 200)
    return BUTTON_UP;
  else if (value < 350)
    return BUTTON_DOWN;
  else if (value < 500)
    return BUTTON_LEFT;
  else if (value < 800)
    return BUTTON_SELECT;
  else
    return BUTTON_NONE;
}

/* --------------- Acquisition de la température ----------------------------------- */
float getTemperatureDS18b20(){
byte i;
byte data[12];
byte addr[8];
float temp =0.0;

//Il n'y a qu'un seul capteur, donc on charge l'unique adresse.
ds.search(addr);

// Cette fonction sert à surveiller si la transmission s'est bien passée
if (OneWire::crc8( addr, 7) != addr[7]) {
Serial.println("getTemperatureDS18b20 : <!> CRC is not valid! <!>");
return false;
}

// On vérifie que l'élément trouvé est bien un DS18B20
if (addr[0] != DS18B20_ID) {
Serial.println("L'équipement trouvé n'est pas un DS18B20");
return false;
}

// Demander au capteur de mémoriser la température et lui laisser 850ms pour le faire (voir datasheet)
ds.reset();
ds.select(addr);
ds.write(0x44);
delay(850);
// Demander au capteur de nous envoyer la température mémorisé
ds.reset();
ds.select(addr);
ds.write(0xBE);

// Le MOT reçu du capteur fait 9 octets, on les charge donc un par un dans le tableau data[]
for ( i = 0; i < 9; i++) {
data[i] = ds.read();
}
// Puis on converti la température (*0.0625 car la température est stockée sur 12 bits)
temp = ( (data[1] << 8) + data[0] )*0.0625;

return temp;
}

/** 
 * Retourne le nombre de millisecondes depuis le démarrage du programme.
 *
 * @return Le nombre de millisecondes depuis le démarrage du programme sous la forme d'un
 * nombre entier sur 64 bits (unsigned long long).
 */
unsigned long long superMillis() {
  static unsigned long nbRollover = 0;
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  
  if (currentMillis < previousMillis) {
     nbRollover++;
  }
  previousMillis = currentMillis;

  unsigned long long finalMillis = nbRollover;
  finalMillis <<= 32;
  finalMillis +=  currentMillis;
  return finalMillis;
}
