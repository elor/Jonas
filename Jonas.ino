#include <Wire.h>
/* !! ACHTUNG !!
 * Unbedingt "LiquidCrystal I2C" von <Frank de Brabander>
 * via Arduino Library Manager installieren.
 */
#include <LiquidCrystal_I2C.h>
#include <math.h>

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

float DISPLAY_BPM_HALBWERTSZEIT = 100; // milliseconds

const int SPEED_SENSOR_DEADTIME = 50000; // microseconds



/** Display in lcd-Variable einrichten. Initialisierung findet spaeter statt.
 * 0x27: I2C-Adresse des Displays
 * 16: 16 Spalten
 * 2: 2 Zeilen
 */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/*
 * Variablen für BPM-Berechnungen und Displaywert-Konvergenz
 */
long bpm_last_micros = 0;
long BPM = -12345;

long DISPLAY_BPM = 0;
long DISPLAY_BPM_LAST_MILLIS = 0;

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
  long now_micros = micros();
  long bpm_diff_micros = now_micros - bpm_last_micros;

  if (bpm_diff_micros < SPEED_SENSOR_DEADTIME) {
    //  oh gott, viel zu schnell
    return;
  }

  const long SECONDS_PER_MINUTE = 60;
  const long MILLIS_PER_MINUTE = 1000 * SECONDS_PER_MINUTE;
  const long MICROS_PER_MINUTE = 1000 * MILLIS_PER_MINUTE;
  const long double_digit_factor = 100;

  int new_bpm = MICROS_PER_MINUTE / (bpm_diff_micros * SPEED_SENSOR_SEGMENTS / double_digit_factor);
  if (new_bpm <= 0) {
    // oh gott, unter null
    return;
  }
  BPM = 3*BPM/4 + new_bpm/4; // ueber die letzten 4 Werte "mitteln"
  bpm_last_micros = now_micros;
}

void writeDisplay() {
  lcd.setCursor(0, 1);
  char buffer[17];
  int vorkomma = BPM / 100;
  int nachkomma = abs(BPM % 100);
  sprintf(buffer, "%5d.%-2d%-8s", vorkomma, nachkomma, BPM_SUFFIX); // fuegt bpm und BPM_SUFFIX zu einer Zeile zusammen
  lcd.print(buffer);
}

void controlMotor() {
  int poti = analogRead(PIN_POTI); // von 0 bis 1023
  int motor = map(poti, 0, 1023, 0, 255); // Reskalierung auf 0 bis 255
	analogWrite(PIN_MOTOR, motor);
}

void convergeBPMValue() {
  long sollwert = BPM;
  long istwert = DISPLAY_BPM;

  long current_millis = millis();
  float delta_t = current_millis - DISPLAY_BPM_LAST_MILLIS;
  DISPLAY_BPM_LAST_MILLIS = current_millis;

  float zerfallskonstante = 0.693f / DISPLAY_BPM_HALBWERTSZEIT;

  DISPLAY_BPM += long((sollwert - istwert) / exp(zerfallskonstante * delta_t));
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
  convergeBPMValue();
  writeDisplay();
}
