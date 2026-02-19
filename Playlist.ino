// ========================================
// PLAYLIST MANAGEMENT
// ========================================

static bool isMp3Name(const String &name) {
  return name.endsWith(".mp3") || name.endsWith(".MP3");
}

void loadPlaylist() {
  Serial.println("Loading music playlist...");

  totalTracks = 0;

  File root = SD.open(MUSIC_FOLDER);
  if (!root) {
    Serial.println("Failed to open Music folder");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Music is not a directory");
    root.close();
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = String(file.name());
      if (isMp3Name(filename)) {
        if (totalTracks < MAX_TRACKS) {
          playlist[totalTracks] = String(MUSIC_FOLDER) + "/" + filename;
          totalTracks++;
        } else {
          Serial.println("Playlist full - increase MAX_TRACKS if needed");
          break;
        }
      }
    }
    file = root.openNextFile();
  }

  root.close();
  Serial.printf("Loaded %d music tracks\n", totalTracks);
}

void loadSFXList() {
  Serial.println("Loading SFX list...");

  totalSFX = 0;

  File root = SD.open(SFX_FOLDER);
  if (!root) {
    Serial.println("Failed to open SFX folder");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("SFX is not a directory");
    root.close();
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = String(file.name());
      if (isMp3Name(filename)) {
        if (totalSFX < MAX_SFX) {
          sfxList[totalSFX] = String(SFX_FOLDER) + "/" + filename;
          totalSFX++;
        } else {
          Serial.println("SFX list full - increase MAX_SFX if needed");
          break;
        }
      }
    }
    file = root.openNextFile();
  }

  root.close();
  Serial.printf("Loaded %d SFX files\n", totalSFX);
}

void shufflePlaylist() {
  Serial.println("Shuffling playlist...");

  for (int i = totalTracks - 1; i > 0; i--) {
    int j = random(0, i + 1);
    String temp = playlist[i];
    playlist[i] = playlist[j];
    playlist[j] = temp;
  }

  Serial.println("Playlist shuffled!");
}
