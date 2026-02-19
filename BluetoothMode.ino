// ========================================
// BLUETOOTH SPEAKER MODE (A2DP Sink)
// ========================================
// Receives audio from phone via Bluetooth A2DP
// Outputs to PCM5102A DAC via I2S
// AVRCP controls: play/pause, next, previous
//
// Pin config uses designated initializers matching
// the official ESP32-A2DP Legacy I2S API wiki:
// https://github.com/pschatzmann/ESP32-A2DP/wiki/Legacy-I2S-API
//
// The global Audio object (ESP32-audioI2S v2.0.0) does
// NOT install the I2S driver in its constructor - that
// only happens in setPinout(), which is only called in
// player mode. So there is no I2S port conflict here.
// ========================================

#include <WiFi.h>
#include "esp_bt.h"        // esp_bt_sleep_disable()
#include "driver/i2s.h"    // i2s_driver_uninstall()

// ---- Debug callbacks (optional, helps troubleshoot) ----

void bt_connection_state(esp_a2d_connection_state_t state, void *ptr) {
  switch (state) {
    case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
      Serial.println("BT: A2DP disconnected");
      break;
    case ESP_A2D_CONNECTION_STATE_CONNECTING:
      Serial.println("BT: A2DP connecting...");
      break;
    case ESP_A2D_CONNECTION_STATE_CONNECTED:
      Serial.println("BT: A2DP connected");
      break;
    case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
      Serial.println("BT: A2DP disconnecting...");
      break;
  }
}

void bt_audio_state(esp_a2d_audio_state_t state, void *ptr) {
  switch (state) {
    case ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND:
      Serial.println("BT: Audio suspended by phone");
      break;
    case ESP_A2D_AUDIO_STATE_STOPPED:
      Serial.println("BT: Audio stopped");
      break;
    case ESP_A2D_AUDIO_STATE_STARTED:
      Serial.println("BT: Audio streaming started <<< you should hear sound now");
      break;
  }
}

// ---- Main BT setup ----

void startBluetoothMode() {
  Serial.println("Starting Bluetooth A2DP Sink...");

  // Kill WiFi completely (shared 2.4GHz radio causes glitches)
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  WiFi.setSleep(false);

  // Lock CPU at max and disable BT power saving
  setCpuFrequencyMhz(240);
  esp_bt_sleep_disable();

  // The global Audio object's constructor claims I2S port 0
  // at boot (before setup runs). We must release it so A2DP
  // can take ownership. Without this you get:
  //   "E I2S: register I2S object to platform failed"
  i2s_driver_uninstall(I2S_NUM_0);
  Serial.println("I2S port 0 released for Bluetooth");

  // Create sink
  a2dp_sink = new BluetoothA2DPSink();

  // Register debug callbacks so serial monitor shows connection progress
  a2dp_sink->set_on_connection_state_changed(bt_connection_state);
  a2dp_sink->set_on_audio_state_changed(bt_audio_state);

  // Pin config: exact pattern from official ESP32-A2DP Legacy I2S Wiki
  // For ESP32 core 2.0.17 (IDF v4), designated initializers zero-fill
  // unmentioned fields. mck_io_num = I2S_PIN_NO_CHANGE means no MCK pin
  // (PCM5102A generates its own clock via the SCK solder bridge)
  i2s_pin_config_t my_pin_config = {
    .mck_io_num   = I2S_PIN_NO_CHANGE,
    .bck_io_num   = 26,
    .ws_io_num    = 25,
    .data_out_num = 22,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };
  a2dp_sink->set_pin_config(my_pin_config);

  // Let the library handle I2S config internally (defaults work
  // correctly with PCM5102A - no custom i2s_config needed)

  // Start BT
  a2dp_sink->start(BT_NAME);

  btIsPlaying = true;

  Serial.println("Bluetooth ready. Pair from phone.");
  Serial.println("Watch serial monitor for connection state changes.");
}

// ---- BT button handler (AVRCP) ----

void handleButtonsBT() {
  unsigned long now = millis();
  if (now - lastButtonPress < DEBOUNCE_DELAY) return;
  if (!a2dp_sink) return;

  // Play/Pause (AVRCP)
  if (digitalRead(BTN_PLAY) == LOW) {
    lastButtonPress = now;

    if (btIsPlaying) {
      a2dp_sink->pause();
      btIsPlaying = false;
      Serial.println("BT: Pause");
    } else {
      a2dp_sink->play();
      btIsPlaying = true;
      Serial.println("BT: Play");
    }

    delay(30);
    return;
  }

  // Previous
  if (digitalRead(BTN_PREVIOUS) == LOW) {
    lastButtonPress = now;
    a2dp_sink->previous();
    Serial.println("BT: Previous");
    delay(30);
    return;
  }

  // Next
  if (digitalRead(BTN_NEXT) == LOW) {
    lastButtonPress = now;
    a2dp_sink->next();
    Serial.println("BT: Next");
    delay(30);
    return;
  }

  // Shuffle and SFX buttons not used in BT mode
}
