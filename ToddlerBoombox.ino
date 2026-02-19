// ========================================
// TODDLER BOOMBOX v3.0 - Multi-Mode
// ========================================
// Modes (hold button during power-on):
//   Normal boot     = SD Card Music Player
//   Hold SHUFFLE    = WiFi SD Card Manager
//   Hold NEXT       = Bluetooth Speaker
// ========================================
// NOTE: Do NOT use GPIO 12 (SFX) for boot detection!
// GPIO 12 is a strapping pin.
// ========================================
// Hardware: ESP32, PCM5102A DAC, PAM8403 Amp, SD Card, Buttons
// Libraries: ESP32-audioI2S, ESP32-A2DP
// ESP32 Core: 2.0.17
// Partition: Huge APP (3MB No OTA/1MB SPIFFS)
// ========================================

#include <SPI.h>
#include <SD.h>
#include "Audio.h"               // ESP32-audioI2S library
#include "BluetoothA2DPSink.h"   // ESP32-A2DP library
#include <WiFi.h>
#include <WebServer.h>
#include "esp_system.h" // esp_random()

// ========================================
// PIN DEFINITIONS
// ========================================

// SD Card Pins (SPI) - ESP32 VSPI defaults usually work, but we define anyway
#define SD_CS    5
#define SD_MOSI  23
#define SD_MISO  19
#define SD_SCK   18

// Button Pins (INPUT_PULLUP: pressed = LOW)
#define BTN_PLAY      32
#define BTN_PREVIOUS  33
#define BTN_NEXT      27
#define BTN_SHUFFLE   14
#define BTN_SFX       12   // DO NOT use for boot detection (strap pin)

// Settings
#define VOLUME_LEVEL  3    // 0-21 range
#define MUSIC_FOLDER  "/Music"
#define SFX_FOLDER    "/SFX"

// WiFi AP Credentials
#define WIFI_SSID     "ToddlerBoombox"
#define WIFI_PASS     "boombox123"

// Bluetooth Name
#define BT_NAME       "Toddler Boombox"

// Limits
static const int MAX_TRACKS = 50;
static const int MAX_SFX    = 20;

// ========================================
// MODE ENUM
// ========================================

enum BootMode {
  MODE_PLAYER,
  MODE_WIFI,
  MODE_BLUETOOTH
};

BootMode currentMode = MODE_PLAYER;

// ========================================
// GLOBAL VARIABLES (used across tabs)
// ========================================

// Audio object (SD playback mode only)
Audio audio;

// Bluetooth sink (BT mode only)
BluetoothA2DPSink *a2dp_sink = nullptr;

// WiFi web server (WiFi mode only)
WebServer *server = nullptr;

// Player state
bool isPlaying = false;
bool isPaused  = false;
bool shuffleMode = false;

int currentTrack = 0;

// Playlist + SFX lists (used by Playlist.ino + Audio.ino)
int totalTracks = 0;
String playlist[MAX_TRACKS];

int totalSFX = 0;
String sfxList[MAX_SFX];

// Debounce / timing
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 300;

// BT play state (for toggle)
bool btIsPlaying = false;

// Prevent "same song" behavior when random mode is used
bool randomSeeded = false;

// ========================================
// FORWARD DECLARATIONS
// ========================================
void initPins();
bool initSDCard();
void initAudio();

void loadPlaylist();
void loadSFXList();
void shufflePlaylist();

void handleButtons();
void startWiFiManager();

void startBluetoothMode();
void handleButtonsBT();

void playTrack(int trackNumber);
void playNextTrack();
void playPreviousTrack();
void playSoundEffect();
void togglePlayPause();
void stopPlayback();

// ========================================
// SETUP
// ========================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n\n=== TODDLER BOOMBOX v3.0 ===");

  initPins();
  delay(150);

  if (!randomSeeded) {
    randomSeed(esp_random());
    randomSeeded = true;
    Serial.println("Random generator seeded");
  }

  // --- Detect boot mode from held buttons ---
  if (digitalRead(BTN_SHUFFLE) == LOW) {
    currentMode = MODE_WIFI;
    Serial.println(">> WiFi Management Mode selected");

    if (!initSDCard()) {
      Serial.println("SD Card Failed - Cannot manage files");
      while (1) delay(1000);
    }

    startWiFiManager();
    return;
  }

  if (digitalRead(BTN_NEXT) == LOW) {
    currentMode = MODE_BLUETOOTH;
    Serial.println(">> Bluetooth Speaker Mode selected");
    startBluetoothMode();
    return;
  }

  // Default: SD player mode
  currentMode = MODE_PLAYER;
  Serial.println(">> SD Music Player Mode selected");

  if (!initSDCard()) {
    Serial.println("SD Card Failed - Cannot play music");
    while (1) delay(1000);
  }

  loadPlaylist();
  loadSFXList();
  initAudio();

  if (totalTracks > 0) {
    currentTrack = random(0, totalTracks);
    playTrack(currentTrack);
  } else {
    Serial.println("No MP3 files found in /Music");
  }
}

// ========================================
// LOOP
// ========================================

void loop() {
  switch (currentMode) {
    case MODE_PLAYER:
      handleButtons();
      audio.loop();

      // auto-advance when track ends (if not paused)
      if (isPlaying && !isPaused && !audio.isRunning()) {
        playNextTrack();
      }
      break;

    case MODE_BLUETOOTH:
      handleButtonsBT();
      yield();
      break;

    case MODE_WIFI:
      if (server) server->handleClient();
      break;
  }

  // WiFi mode needs a small yield for the TCP stack
  // Player mode: NO delay - audio.loop() must run flat out
  //   to keep the I2S DMA buffer fed
  // BT mode: yield() already called above
  if (currentMode == MODE_WIFI) {
    delay(10);
  }
}
