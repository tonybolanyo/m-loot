/*
 * PEDALERA MIDI TIPO LOOP4LIVE
 * Basado en Arduino Micro
 * Comunicación USB MIDI nativa
 * 
 * Características:
 * - 10 Footswitches programables
 * - 5 Encoders rotativos (knobs)
 * - Pantalla OLED 0.96" (SSD1306 I2C)
 * - 8 LEDs indicadores
 * - Control MIDI completo para Ableton Live
 * 
 * Autor: Tony G. Bolaño
 * Versión: 0.1
 * Fecha: Nov 2025
 */

// ============ LIBRERÍAS ============
#include <MIDIUSB.h>          // USB MIDI
#include <Wire.h>             // I2C para OLED
#include <Adafruit_SSD1306.h> // Pantalla OLED
#include <Adafruit_GFX.h>     // Gráficos OLED
#include <Encoder.h>          // Lectura de encoders

// ============ DEFINICIONES DE PANTALLA OLED ============
#define SCREEN_WIDTH 128      // Ancho pantalla OLED en píxeles
#define SCREEN_HEIGHT 64      // Alto pantalla OLED en píxeles
#define OLED_RESET -1         // Reset pin (no usado, -1)
#define SCREEN_ADDRESS 0x3C   // Dirección I2C 0x3C típica

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============ DEFINICIONES DE PINES ============

// Pines de Footswitches (10x)
const int FOOTSWITCH_PINS[10] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

// Pines de Encoders Rotativos (5x KY-040)
// Cada encoder: CLK, DT, SW
const int ENCODER_CLK[5] = {12, 14, 16, 18, 20};
const int ENCODER_DT[5] = {13, 15, 17, 19, 21};
const int ENCODER_SW[5] = {A0, A1, A2, A3, A4};

// Pines de LEDs (8x)
// Usando pines analógicos como digitales
const int LED_PINS[8] = {A5, A6, A7, A8, A9, A10, 14, 15};

// ============ VARIABLES DE ESTADO ============

// Variables para almacenar estado de Footswitches
boolean footswitchState[10] = {false, false, false, false, false, false, false, false, false, false};
boolean footswitchLastState[10] = {false, false, false, false, false, false, false, false, false, false};
unsigned long footswitchDebounceTime[10] = {0};
const unsigned long DEBOUNCE_DELAY = 50; // Debounce 50ms

// Variables para Encoders
Encoder *encoders[5];
long encoderValues[5] = {0, 0, 0, 0, 0};
long encoderLastValues[5] = {0, 0, 0, 0, 0};
boolean encoderSwitchState[5] = {false, false, false, false, false};
boolean encoderSwitchLastState[5] = {false, false, false, false, false};

// Variables de MIDI
byte midiChannel = 1;           // Canal MIDI (1-16)
byte currentScene = 0;          // Escena actual (0-7 típicamente)
unsigned long currentTempo = 90; // BPM del proyecto
unsigned long lastTempoTapTime = 0;
int tapTempoCounts = 0;

// Variables de timing para actualización de pantalla
unsigned long lastScreenUpdate = 0;
const unsigned long SCREEN_UPDATE_INTERVAL = 100; // Actualizar cada 100ms

// ============ SETUP - Configuración Inicial ============
void setup() {
  // Inicializar comunicación serial (para debug)
  Serial.begin(115200);
  delay(2000); // Esperar a que se establezca conexión serial
  Serial.println(F("Sistema de Pedalera MIDI m-loot iniciando..."));
  
  // ---- Configurar pines de Footswitches ----
  for (int i = 0; i < 10; i++) {
    pinMode(FOOTSWITCH_PINS[i], INPUT_PULLUP);
  }
  Serial.println(F("Footswitches configurados"));
  
  // ---- Configurar pines de LEDs ----
  for (int i = 0; i < 8; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW); // Apagar todos inicialmente
  }
  Serial.println(F("LEDs configurados"));
  
  // ---- Inicializar Encoders ----
  // Encoder(pin_CLK, pin_DT)
  encoders[0] = new Encoder(ENCODER_CLK[0], ENCODER_DT[0]);
  encoders[1] = new Encoder(ENCODER_CLK[1], ENCODER_DT[1]);
  encoders[2] = new Encoder(ENCODER_CLK[2], ENCODER_DT[2]);
  encoders[3] = new Encoder(ENCODER_CLK[3], ENCODER_DT[3]);
  encoders[4] = new Encoder(ENCODER_CLK[4], ENCODER_DT[4]);
  
  // Configurar switches de Encoders como entrada
  for (int i = 0; i < 5; i++) {
    pinMode(ENCODER_SW[i], INPUT_PULLUP);
  }
  Serial.println(F("Encoders configurados"));
  
  // ---- Inicializar Pantalla OLED ----
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("ERROR: No se puede inicializar pantalla OLED"));
    // Continuar de todos modos, solo sin pantalla
  } else {
    Serial.println(F("Pantalla OLED inicializada"));
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("M-LOOT"));
    display.setTextSize(1);
    display.println(F("MIDI LISTO"));
    display.display();
    delay(2000);
  }
  
  // ---- Mostrar inicialización completa ----
  Serial.println(F("Setup completado. Esperando entrada..."));
  updateDisplay();
}

// ============ LOOP PRINCIPAL ============
void loop() {
  // Leer Footswitches con debounce
  handleFootswitches();
  
  // Leer Encoders y knobs
  handleEncoders();
  
  // Actualizar pantalla (cada 100ms para no saturar I2C)
  if (millis() - lastScreenUpdate > SCREEN_UPDATE_INTERVAL) {
    updateDisplay();
    lastScreenUpdate = millis();
  }
  
  // Procesar mensajes MIDI recibidos (opcional)
  midiEventPacket_t rx;
  rx = MidiUSB.read();
  if (rx.header != 0) {
    handleMidiInput(rx);
  }
  
  // Delay mínimo para evitar saturar CPU
  delay(10);
}

// ============ FUNCIÓN: LEER FOOTSWITCHES ============
void handleFootswitches() {
  for (int i = 0; i < 10; i++) {
    boolean reading = digitalRead(FOOTSWITCH_PINS[i]);
    
    // Aplicar debounce
    if (reading != footswitchLastState[i]) {
      footswitchDebounceTime[i] = millis();
    }
    
    if ((millis() - footswitchDebounceTime[i]) > DEBOUNCE_DELAY) {
      if (reading != footswitchState[i]) {
        footswitchState[i] = reading;
        
        // Si button presionado (LOW porque es INPUT_PULLUP)
        if (footswitchState[i] == LOW) {
          handleFootswitchPress(i);
          digitalWrite(LED_PINS[i % 8], HIGH); // Encender LED correspondiente
        } else {
          // Botón liberado
          handleFootswitchRelease(i);
          digitalWrite(LED_PINS[i % 8], LOW);  // Apagar LED
        }
      }
    }
    
    footswitchLastState[i] = reading;
  }
}

// ============ FUNCIÓN: PROCESAR PRESIÓN DE FOOTSWITCH ============
void handleFootswitchPress(int switchNum) {
  Serial.print(F("Footswitch "));
  Serial.print(switchNum + 1);
  Serial.println(F(" presionado"));
  
  // Mapeo de funciones para cada Footswitch
  // (Personalizar según necesidad)
  
  switch (switchNum) {
    // Footswitches 0-3: Lanzar escenas 0-3
    case 0:
    case 1:
    case 2:
    case 3:
      launchScene(switchNum);
      currentScene = switchNum;
      break;
    
    // Footswitches 4-5: Control de looper
    case 4:
      // Record/Overdub
      sendMidiNoteOn(midiChannel, 60, 127); // C3
      break;
    case 5:
      // Play/Stop
      sendMidiNoteOn(midiChannel, 61, 127); // C#3
      break;
    
    // Footswitches 6-7: Tempo control
    case 6:
      // Tap Tempo
      tapTempo();
      break;
    case 7:
      // Tap Tempo Reset
      tapTempoCounts = 0;
      break;
    
    // Footswitches 8-9: Controles adicionales
    case 8:
      // Bypass all effects
      sendMidiControlChange(midiChannel, 120, 0);
      break;
    case 9:
      // Master volume
      sendMidiControlChange(midiChannel, 7, 127);
      break;
  }
}

// ============ FUNCIÓN: PROCESAR LIBERACIÓN DE FOOTSWITCH ============
void handleFootswitchRelease(int switchNum) {
  // (Opcional: enviar Note Off si es necesario)
  switch (switchNum) {
    case 4:
    case 5:
      sendMidiNoteOff(midiChannel, 60 + switchNum - 4, 0);
      break;
  }
}

// ============ FUNCIÓN: LANZAR ESCENA ============
void launchScene(int sceneNum) {
  // Enviar MIDI Note On para lanzar escena
  // En Ableton, mapear escenas a notas (por ejemplo C1-C8 = Escenas 0-7)
  byte note = 36 + sceneNum; // C1 = MIDI nota 36
  sendMidiNoteOn(midiChannel, note, 127);
  
  Serial.print(F("Escena "));
  Serial.print(sceneNum);
  Serial.println(F(" lanzada"));
}

// ============ FUNCIÓN: TAP TEMPO ============
void tapTempo() {
  unsigned long currentTime = millis();
  
  if (tapTempoCounts == 0) {
    // Primer tap
    lastTempoTapTime = currentTime;
    tapTempoCounts = 1;
  } else if (currentTime - lastTempoTapTime < 3000) {
    // Tiempo entre taps < 3 segundos, calcular BPM
    unsigned long timeDiff = currentTime - lastTempoTapTime;
    currentTempo = (60000 / timeDiff); // Convertir a BPM
    
    // Enviar MIDI Set Tempo (opcional)
    sendMidiControlChange(midiChannel, 11, map(currentTempo, 40, 200, 0, 127));
    
    Serial.print(F("Tempo: "));
    Serial.print(currentTempo);
    Serial.println(F(" BPM"));
    
    tapTempoCounts++;
    lastTempoTapTime = currentTime;
  } else {
    // Reiniciar contador (tap después de 3 segundos)
    tapTempoCounts = 0;
    tapTempo();
  }
}

// ============ FUNCIÓN: LEER ENCODERS ============
void handleEncoders() {
  for (int i = 0; i < 5; i++) {
    // Leer valor del encoder
    long newValue = encoders[i]->read();
    
    if (newValue != encoderLastValues[i]) {
      int direction = (newValue > encoderLastValues[i]) ? 1 : -1;
      handleEncoderRotation(i, direction);
      encoderLastValues[i] = newValue;
    }
    
    // Leer pulsador del encoder
    boolean reading = digitalRead(ENCODER_SW[i]);
    
    if (reading != encoderSwitchLastState[i]) {
      if (reading == LOW) {
        handleEncoderPress(i);
      }
    }
    
    encoderSwitchLastState[i] = reading;
  }
}

// ============ FUNCIÓN: PROCESAR ROTACIÓN DE ENCODER ============
void handleEncoderRotation(int encoderNum, int direction) {
  Serial.print(F("Encoder "));
  Serial.print(encoderNum + 1);
  Serial.print(F(" rotado: "));
  Serial.println(direction > 0 ? "CW" : "CCW");
  
  // Mapeo de encoders a parámetros MIDI
  // Encoder 0-4: CC 20-24 (o personalizar)
  
  byte ccNumber = 20 + encoderNum;
  byte value;
  
  if (direction > 0) {
    value = 127; // Máximo
  } else {
    value = 0;   // Mínimo
  }
  
  // Enviar CC con valores incrementales para mayor precisión
  sendMidiControlChange(midiChannel, ccNumber, value);
}

// ============ FUNCIÓN: PROCESAR PRESIÓN DE ENCODER ============
void handleEncoderPress(int encoderNum) {
  Serial.print(F("Encoder "));
  Serial.print(encoderNum + 1);
  Serial.println(F(" presionado"));
  
  // Enviar Note On con encoder como identificador
  sendMidiNoteOn(midiChannel, 48 + encoderNum, 100);
}

// ============ FUNCIONES MIDI - ENVÍO ============

/**
 * Enviar MIDI Note On
 * @param channel - Canal MIDI (1-16)
 * @param note - Nota MIDI (0-127)
 * @param velocity - Velocidad (0-127)
 */
void sendMidiNoteOn(byte channel, byte note, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | (channel - 1), note, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

/**
 * Enviar MIDI Note Off
 * @param channel - Canal MIDI (1-16)
 * @param note - Nota MIDI (0-127)
 * @param velocity - Velocidad (típicamente 0)
 */
void sendMidiNoteOff(byte channel, byte note, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | (channel - 1), note, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}

/**
 * Enviar MIDI Control Change (CC)
 * @param channel - Canal MIDI (1-16)
 * @param ccNumber - Número CC (0-127)
 * @param value - Valor (0-127)
 */
void sendMidiControlChange(byte channel, byte ccNumber, byte value) {
  midiEventPacket_t cc = {0x0B, 0xB0 | (channel - 1), ccNumber, value};
  MidiUSB.sendMIDI(cc);
  MidiUSB.flush();
}

// ============ FUNCIÓN: PROCESAR ENTRADA MIDI ============
void handleMidiInput(midiEventPacket_t rx) {
  byte status = rx.header & 0xF0;
  byte channel = rx.header & 0x0F;
  
  // Procesar mensajes MIDI recibidos (opcional)
  // Por ejemplo, cambiar indicadores de LED según estado de Ableton
}

// ============ FUNCIÓN: ACTUALIZAR PANTALLA OLED ============
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  // Línea 1: Escena actual
  display.print(F("Escena: "));
  display.println(currentScene);
  
  // Línea 2: Tempo
  display.print(F("Tempo: "));
  display.print(currentTempo);
  display.println(F(" BPM"));
  
  // Línea 3: Estado de Footswitches (visual)
  display.print(F("FS: "));
  for (int i = 0; i < 10; i++) {
    if (footswitchState[i] == LOW) {
      display.print(i + 1);
      display.print(" ");
    }
  }
  display.println();
  
  // Línea 4: Valores de Encoders
  display.print(F("ENC1: "));
  display.println(encoderValues[0] % 128);
  
  // Línea 5: Estado general
  display.print(F("Estado: OK"));
  display.println();
  
  // Mostrar en pantalla
  display.display();
}

// ============ FIN DEL CÓDIGO ============
