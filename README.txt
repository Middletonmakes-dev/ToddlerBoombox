========================================
TODDLER BOOMBOX v3.0 - Multi-Mode
========================================

THREE BOOT MODES
================
  Normal power-on        = SD Card Music Player
  Hold SHUFFLE at boot   = WiFi SD Card Manager
  Hold NEXT at boot      = Bluetooth Speaker

  NOTE: GPIO 12 (SFX button) is an ESP32 strapping pin.
  Never use it for boot detection - holding it LOW at
  power-on changes flash voltage and can prevent booting.

LIBRARY INSTALLATION
====================
Install via Sketch > Include Library > Manage Libraries:

  1. ESP32-audioI2S v2.0.0 (by schreibfaul1)
     - For SD card audio playback via I2S

  2. ESP32-A2DP (by Phil Schatzmann) - latest version
     - For Bluetooth speaker mode
     - Search "ESP32-A2DP" in Library Manager

  Built-in (no install needed): SD, SPI, WiFi, WebServer

ESP32 BOARD SETUP
=================
  Board: ESP32 Dev Module
  Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)  <-- IMPORTANT!
  Upload Speed: 115200
  ESP32 Core: 2.0.17

  The "Huge APP" partition is required because the firmware
  includes SD audio + Bluetooth + WiFi code. Find it under:
  Tools > Partition Scheme > Huge APP (3MB No OTA/1MB SPIFFS)

PCM5102A SOLDER BRIDGES
========================
  FRONT SIDE:
    SCK pads: BRIDGE TOGETHER (forces internal PLL)

  BACK SIDE (4 sets of 3 pads):
    H1L (FLT):  CENTER to GND     (normal latency)
    H2L (DEMP): CENTER to GND     (de-emphasis off)
    H3L (XSMT): CENTER to +3.3V  (audio enabled!)
    H4L (FMT):  CENTER to GND     (I2S format)

WIRING DIAGRAM
==============

  ESP32 > PCM5102A:
  -----------------
  5V (VIN)    >  VIN
  GND         >  GND
  GPIO 25     >  LCK (LRCK)
  GPIO 22     >  DIN
  GPIO 26     >  BCK
  (nothing)      SCK (leave unconnected)

  PCM5102A > PAM8403 Amplifier:
  -----------------------------
  L pin       >  PAM8403 L input
  R pin       >  PAM8403 R input
  G pin       >  PAM8403 GND

  ESP32 > SD Card Module:
  -----------------------
  GPIO 5      >  CS
  GPIO 23     >  MOSI
  GPIO 19     >  MISO
  GPIO 18     >  SCK
  3.3V        >  VCC
  GND         >  GND

  ESP32 > Buttons (all to GND):
  -----------------------------
  GPIO 32     >  Play/Pause button  > GND
  GPIO 33     >  Previous button    > GND
  GPIO 27     >  Next button        > GND
  GPIO 14     >  Shuffle button     > GND
  GPIO 12     >  Sound FX button    > GND

  PAM8403 > Speakers:
  --------------------
  Keep existing speaker connections

SD CARD STRUCTURE
=================
  Format as FAT32, then create:

  /Music/
    song1.mp3
    song2.mp3
    (up to 50 songs)

  /SFX/
    effect1.mp3
    effect2.mp3
    (up to 20 sound effects)

  Or use WiFi Manager mode to upload files!

MODE 1: SD CARD PLAYER (Normal Boot)
=====================================
  Just power on normally (no buttons held).

  Controls:
    Play/Pause  - True pause/resume (does not restart song)
    Next        - Skip to next track
    Previous    - Go to previous track
    Shuffle     - Randomize playlist order
    Sound FX    - Play random SFX, then resume music where it left off

  Features:
    - Auto-advances to next track when song ends
    - Volume limited to 15% for hearing safety
    - SFX saves and restores music position

MODE 2: WIFI SD CARD MANAGER (Hold SHUFFLE at Boot)
====================================================
  Hold the SHUFFLE button while powering on the ESP32.

  1. ESP32 creates WiFi network:
     SSID:     ToddlerBoombox
     Password: boombox123

  2. Connect your phone/laptop to "ToddlerBoombox" WiFi

  3. Open browser and go to: http://192.168.4.1

  4. Web interface lets you:
     - View all songs and sound effects
     - Upload new MP3 files
     - Delete files
     - See SD card free space

  5. Power cycle (without holding button) to return to player mode

MODE 3: BLUETOOTH SPEAKER (Hold NEXT at Boot)
=============================================
  Hold the NEXT button while powering on the ESP32.

  1. ESP32 appears as "Toddler Boombox" in your phone's
     Bluetooth settings

  2. Pair and connect from your phone

  3. Play music from any app on your phone

  4. Button controls work via AVRCP:
     Play/Pause  - Pause/resume phone playback
     Next        - Skip to next track on phone
     Previous    - Go to previous track on phone
     (Shuffle and SFX buttons inactive in BT mode)

  5. Power cycle to return to player mode

  Note: Phone volume + PAM8403 gain both affect loudness.
  Test at low phone volume first before giving to child.

TROUBLESHOOTING
===============
  No sound in player mode?
    - Check PCM5102A H3L is bridged to +3.3V (unmute)
    - Check SD card has MP3 files in /Music folder

  No sound in Bluetooth mode?
    - Ensure phone is connected and playing
    - Check phone volume is not muted
    - Check PAM8403 connections

  WiFi page won't load?
    - Make sure you're connected to "ToddlerBoombox" WiFi
    - Try http://192.168.4.1 (not https)
    - Wait 10 seconds after boot for AP to start

  Upload fails?
    - SD card may be full (check free space on page)
    - Try smaller files or delete old ones first

  Won't compile / too large?
    - Change partition scheme to: Huge APP (3MB No OTA/1MB SPIFFS)
    - Tools > Partition Scheme in Arduino IDE

  Songs skip or stutter?
    - Use Class 10 SD card
    - SD card runs at 40MHz SPI for smooth streaming

SAFETY NOTES
============
  Volume is limited to ~15% (3/21) in SD player mode
  Bluetooth mode: set phone volume low and test first
  Ensure all wiring is secure and insulated
  Test audio level before giving to child

FILES
=====
  ToddlerBoombox.ino  - Main file, config, setup, loop
  Audio.ino            - SD playback functions
  BluetoothMode.ino   - Bluetooth A2DP speaker mode
  Buttons.ino          - Button handling (player mode)
  Hardware.ino         - Pin, SD card, audio initialization
  Playlist.ino         - Playlist loading and shuffling
  WebManager.ino       - WiFi AP and web file manager
