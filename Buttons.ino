// ========================================
// BUTTON HANDLING (SD Player Mode)
// ========================================

void handleButtons() {
  unsigned long currentTime = millis();

  if (currentTime - lastButtonPress < DEBOUNCE_DELAY) return;

  // Play/Pause
  if (digitalRead(BTN_PLAY) == LOW) {
    lastButtonPress = currentTime;
    togglePlayPause();
    delay(50);
  }

  // Previous
  else if (digitalRead(BTN_PREVIOUS) == LOW) {
    lastButtonPress = currentTime;
    playPreviousTrack();
    delay(50);
  }

  // Next
  else if (digitalRead(BTN_NEXT) == LOW) {
    lastButtonPress = currentTime;
    playNextTrack();
    delay(50);
  }

  // Shuffle toggle
  else if (digitalRead(BTN_SHUFFLE) == LOW) {
    lastButtonPress = currentTime;
    toggleShuffle();
    delay(50);
  }

  // Random SFX
  else if (digitalRead(BTN_SFX) == LOW) {
    lastButtonPress = currentTime;
    playSoundEffect();
    delay(50);
  }
}

void toggleShuffle() {
  shuffleMode = !shuffleMode;
  isPaused = false;

  if (shuffleMode) {
    Serial.println("Shuffle ON");
    shufflePlaylist();

    if (totalTracks > 0) {
      currentTrack = random(0, totalTracks);
      playTrack(currentTrack);
    }
  } else {
    Serial.println("Shuffle OFF");
    loadPlaylist();

    if (totalTracks > 0) {
      currentTrack = 0;
      playTrack(currentTrack);
    }
  }
}
