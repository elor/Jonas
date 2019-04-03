#include <Wire.h>
/* !! ACHTUNG !!
 * Unbedingt "LiquidCrystal I2C" von <Frank de Brabander>
 * via Arduino Library Manager installieren.
 */
#include <LiquidCrystal_I2C.h>

/* PINS:
 * 
 * - A0: Potentiometer fuer Motorsteuerung (0V-5V)
 * - A4: Display SDA (Datensignal fuer Display. I2C-Bus)
 * - A5: Display SCL (Clocksignal fuer Display. I2C-Bus)
 * -  2: Speed Sensor digitaler Ausgang (LOW: dunkel, HIGH: hell. Wir warten auf Sprung
 *                                       LOW->HIGH und machen eine Drehzahl draus)
 * - ~3: H-Bruecke, Pin 2 (Steuersignal fuer Motor 1, PWM. 0-255 = 0V-5V)
 */

const int PIN_POTI = A0;
// LCD Pins werden ueber LiquidCrystal_I2C-Bibliothek gesetzt.
const int PIN_SENSOR = 2;
const int PIN_MOTOR = 3;

const int SPEED_SENSOR_SEGMENTS = 20;
const int BEATS_PRO_UMDREHUNG = 1;

const int SPEED_SENSOR_DEADTIME = 50000; // microseconds

/** Display in lcd-Variable einrichten. Initialisierung findet spaeter statt.
 * 0x27: I2C-Adresse des Displays
 * 16: 16 Spalten
 * 2: 2 Zeilen
 */
LiquidCrystal_I2C lcd(0x27, 16, 2);

int bpm_last_micros = 0;
double BPM = -123.45;
const char BPM_SUFFIX[] = " u/min"; // z.B. "BPM" oder "u/min". max. 8 Zeichen.


void setup()
{
  lcd.init(); // Display initialisieren
  lcd.backlight(); // Es werde Licht

  // obere Zeile im Display. Bleibt ja gleich.
  lcd.setCursor(0, 0);
  lcd.print("Drehzahl:");

  pinMode(PIN_POTI, INPUT);
  pinMode(PIN_MOTOR, OUTPUT);

  // Interrupt setzen, um BPM via Sensor zu zaehlen
  pinMode(PIN_SENSOR, INPUT_PULLUP); // alternativ: INPUT
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), bpmInterrupt, FALLING); // alternativ: RISING
}

/**
 * Diese Funktion wird per Interrupt aufgerufen, sobald der Sensor Licht sieht.
 */
void bpmInterrupt() {
  int bpm_diff_micros = micros() - bpm_last_micros;
  bpm_last_micros = micros();

  if (bpm_diff_micros == SPEED_SENSOR_DEADTIME) {
    //  oh gott, viel zu schnell
    return;
  }

  const double MICROS_PER_MINUTE = 60000000;

  BPM = MICROS_PER_MINUTE / double(bpm_diff_micros * SPEED_SENSOR_SEGMENTS);
}

void writeDisplay() {
  lcd.setCursor(0, 1);
  char buffer[17];
  sprintf(buffer, "%7.2lf%-8s", BPM, BPM_SUFFIX); // fuegt bpm und BPM_SUFFIX zu einer Zeile zusammen
  lcd.print(buffer);
}

void controlMotor() {
  int poti = analogRead(PIN_POTI); // von 0 bis 1023
  int motor = map(poti, 0, 1023, 0, 255); // Reskalierung auf 0 bis 255
	analogWrite(PIN_MOTOR, motor);
}

/** Arduino Hauptschleife
 * Aufgaben:
 *  1. Poti lesen und direkt an Motor uebergeben. Nix speichern.
 *  2. BPM auf Display ausgeben.
 *
 *  BPM werden vom Sensor per Interrupt ermittelt. Ist einfacher.
 */
void loop() 
{
  controlMotor();
  writeDisplay();
}
