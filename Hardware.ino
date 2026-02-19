// ========================================
// HARDWARE INITIALIZATION
// ========================================

void initPins() {
  Serial.println("Initializing pins...");

  pinMode(BTN_PLAY, INPUT_PULLUP);
  pinMode(BTN_PREVIOUS, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_SHUFFLE, INPUT_PULLUP);
  pinMode(BTN_SFX, INPUT_PULLUP);

  Serial.println("Pins initialized");
}

bool initSDCard() {
  Serial.println("Initializing SD Card...");

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, 40000000)) {
    Serial.println("SD Card mount failed!");
    return false;
  }

  Serial.println("SD Card initialized at 40MHz");
  return true;
}

void initAudio() {
  Serial.println("Initializing audio with PCM5102A DAC...");

  // I2S pins: BCK = 26, LCK = 25, DIN = 22
  audio.setPinout(26, 25, 22);
  audio.setVolume(VOLUME_LEVEL);

  // Larger internal buffer helps prevent stutter with
  // high bitrate MP3s and long-filename SD card lookups
  // Default is ~6KB; 16KB gives much more headroom
  audio.setBufsize(16 * 1024, 0);

  Serial.printf("PCM5102A initialized (Volume: %d/21)\n", VOLUME_LEVEL);
}
