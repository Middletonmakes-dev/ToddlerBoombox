// ========================================
// AUDIO PLAYBACK FUNCTIONS (SD Player Mode)
// ========================================

void playTrack(int trackNumber) {
  if (trackNumber < 0 || trackNumber >= totalTracks) {
    Serial.println("Invalid track number");
    return;
  }

  currentTrack = trackNumber;
  isPaused = false;
  String trackPath = playlist[currentTrack];

  Serial.printf("Playing [%d/%d]: %s\n",
                currentTrack + 1, totalTracks, trackPath.c_str());

  audio.connecttoSD(trackPath.c_str());
  isPlaying = true;
}

void playNextTrack() {
  if (totalTracks == 0) return;

  isPaused = false;

  if (shuffleMode && totalTracks > 1) {
    int nextTrack = currentTrack;
    while (nextTrack == currentTrack) {
      nextTrack = random(0, totalTracks);
    }
    currentTrack = nextTrack;
  } else {
    currentTrack = (currentTrack + 1) % totalTracks;
  }

  playTrack(currentTrack);
}

void playPreviousTrack() {
  if (totalTracks == 0) return;

  isPaused = false;

  if (shuffleMode && totalTracks > 1) {
    int prevTrack = currentTrack;
    while (prevTrack == currentTrack) {
      prevTrack = random(0, totalTracks);
    }
    currentTrack = prevTrack;
  } else {
    currentTrack = (currentTrack - 1 + totalTracks) % totalTracks;
  }

  playTrack(currentTrack);
}

void togglePlayPause() {
  if (isPaused) {
    // Resume from pause
    audio.pauseResume();
    isPaused = false;
    isPlaying = true;
    Serial.println("Resumed");
  } else if (isPlaying && audio.isRunning()) {
    // Pause current playback
    audio.pauseResume();
    isPaused = true;
    isPlaying = false;
    Serial.println("Paused");
  } else {
    // Nothing playing, start current track
    playTrack(currentTrack);
  }
}

void stopPlayback() {
  audio.stopSong();
  isPlaying = false;
  isPaused = false;
}

void playSoundEffect() {
  if (totalSFX == 0) {
    Serial.println("No sound effects available");
    return;
  }

  // Save current music state and byte position
  bool wasPlaying = isPlaying || isPaused;
  uint32_t savedPos = 0;

  if (wasPlaying) {
    savedPos = audio.getFilePos();
    Serial.printf("Saved byte position: %u\n", savedPos);
    audio.stopSong();
    isPlaying = false;
    isPaused = false;
  }

  // Pick and play random sound effect
  int sfxIndex = random(0, totalSFX);
  String sfxPath = sfxList[sfxIndex];
  Serial.printf("Playing SFX: %s\n", sfxPath.c_str());

  audio.connecttoSD(sfxPath.c_str());

  // Wait for SFX to finish
  while (audio.isRunning()) {
    audio.loop();
    delay(1);
  }

  delay(300);

  // Resume music at saved byte position
  if (wasPlaying) {
    String trackPath = playlist[currentTrack];
    Serial.printf("Resuming track at byte %u\n", savedPos);

    // connecttoFS with third param = byte offset to resume from
    audio.connecttoFS(SD, trackPath.c_str(), savedPos);

    isPlaying = true;
    isPaused = false;
  }

  Serial.println("SFX complete");
}
