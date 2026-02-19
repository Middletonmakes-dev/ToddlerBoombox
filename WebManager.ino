// ========================================
// WIFI SD CARD FILE MANAGER (Explorer-style)
// ========================================

#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>

#include "webpage.h"   // UI stored in PROGMEM

// Upload bookkeeping
static File uploadFile;
static String uploadPath = "/";

static String normPath(String p) {
  if (p.length() == 0) return "/";
  if (!p.startsWith("/")) p = "/" + p;
  // remove trailing slash except for root
  if (p.length() > 1 && p.endsWith("/")) p.remove(p.length() - 1);
  return p;
}

void startWiFiManager() {
  Serial.println("Starting WiFi Access Point...");

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  // Force the classic AP IP
  IPAddress ip(192, 168, 4, 1);
  IPAddress gw(192, 168, 4, 1);
  IPAddress mask(255, 255, 255, 0);
  WiFi.softAPConfig(ip, gw, mask);

  WiFi.softAP(WIFI_SSID, WIFI_PASS);

  Serial.printf("WiFi AP: %s\n", WIFI_SSID);
  Serial.printf("Password: %s\n", WIFI_PASS);
  Serial.printf("Open browser to: http://%s/\n", ip.toString().c_str());

  // Ensure folders exist
  if (!SD.exists(MUSIC_FOLDER)) SD.mkdir(MUSIC_FOLDER);
  if (!SD.exists(SFX_FOLDER)) SD.mkdir(SFX_FOLDER);

  server = new WebServer(80);

  server->on("/", HTTP_GET, []() {
    server->send_P(200, "text/html", WEB_PAGE);
  });

  // List directory: /api/list?path=/Music
  server->on("/api/list", HTTP_GET, []() {
    String path = normPath(server->arg("path"));

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
      if (dir) dir.close();
      server->send(400, "application/json", "{\"ok\":false,\"err\":\"not_a_dir\"}");
      return;
    }

    // Stream JSON so we don't build huge Strings
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send(200, "application/json", "");

    server->sendContent("{\"ok\":true,\"path\":\"" + path + "\",\"items\":[");
    bool first = true;

    File f = dir.openNextFile();
    while (f) {
      String name = String(f.name());

      // ESP32 SD sometimes returns full paths; keep only base name for display
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);

      if (!first) server->sendContent(",");
      first = false;

      String item = "{\"name\":\"" + name + "\",\"dir\":";
      item += (f.isDirectory() ? "true" : "false");
      item += ",\"size\":";
      item += String((uint32_t)f.size());
      item += "}";

      server->sendContent(item);

      f.close();                 // CRITICAL: close each file
      f = dir.openNextFile();
      yield();
    }

    dir.close();
    server->sendContent("]}");
  });

  // Space: /api/space
  server->on("/api/space", HTTP_GET, []() {
    // These exist in ESP32 SD library on core 2.x
    uint64_t total = SD.totalBytes() / (1024 * 1024);
    uint64_t used  = SD.usedBytes()  / (1024 * 1024);
    uint64_t free_mb = (total > used) ? (total - used) : 0;

    String json = "{\"ok\":true,\"total\":" + String((uint32_t)total) +
                  ",\"used\":" + String((uint32_t)used) +
                  ",\"free\":" + String((uint32_t)free_mb) + "}";

    server->send(200, "application/json", json);
  });

  // Delete: /api/delete?path=/Music/song.mp3
  server->on("/api/delete", HTTP_GET, []() {
    String path = normPath(server->arg("path"));

    if (path == "/" || path == MUSIC_FOLDER || path == SFX_FOLDER) {
      server->send(400, "application/json", "{\"ok\":false,\"err\":\"protected\"}");
      return;
    }

    bool ok = false;
    if (SD.exists(path)) ok = SD.remove(path);

    server->send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });

  // Mkdir: /api/mkdir?path=/Music/NewFolder
  server->on("/api/mkdir", HTTP_GET, []() {
    String path = normPath(server->arg("path"));
    bool ok = SD.mkdir(path);
    server->send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });

  // Rename: /api/rename?from=/Music/a.mp3&to=/Music/b.mp3
  server->on("/api/rename", HTTP_GET, []() {
    String from = normPath(server->arg("from"));
    String to   = normPath(server->arg("to"));
    bool ok = SD.rename(from, to);
    server->send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });

  // Download: /api/download?path=/Music/song.mp3
  server->on("/api/download", HTTP_GET, []() {
    String path = normPath(server->arg("path"));
    File f = SD.open(path, FILE_READ);
    if (!f || f.isDirectory()) {
      if (f) f.close();
      server->send(404, "text/plain", "Not found");
      return;
    }

    server->sendHeader("Content-Disposition", "attachment; filename=\"" + String(f.name()).substring(String(f.name()).lastIndexOf('/') + 1) + "\"");
    server->streamFile(f, "application/octet-stream");
    f.close();
  });

  // Upload: POST /api/upload?path=/Music  (multipart file field: "file")
  server->on("/api/upload", HTTP_POST,
    []() { server->send(200, "application/json", "{\"ok\":true}"); },
    []() {
      HTTPUpload &up = server->upload();

      if (up.status == UPLOAD_FILE_START) {
        uploadPath = normPath(server->arg("path"));
        if (!SD.exists(uploadPath)) SD.mkdir(uploadPath);

        // Check free space before accepting (reject if < 1 MB free)
        uint64_t freeBytes = SD.totalBytes() - SD.usedBytes();
        if (freeBytes < 1024 * 1024) {
          Serial.println("Upload rejected: SD card nearly full");
          // Can't send HTTP error from upload handler, but refusing
          // to open the file will cause write to silently fail.
          // The JS will still get 200 but the file won't be on disk.
          // Better than crashing mid-write.
          uploadFile = File();  // null file
          return;
        }

        String filename = up.filename;
        filename.replace("\\", "/");
        int slash = filename.lastIndexOf('/');
        if (slash >= 0) filename = filename.substring(slash + 1);

        String full = uploadPath + "/" + filename;

        if (SD.exists(full)) SD.remove(full);
        uploadFile = SD.open(full, FILE_WRITE);

        Serial.printf("Upload start: %s\n", full.c_str());
      }
      else if (up.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
          uploadFile.write(up.buf, up.currentSize);
          yield();  // feed watchdog during large writes
        }
      }
      else if (up.status == UPLOAD_FILE_END) {
        if (uploadFile) uploadFile.close();
        Serial.printf("Upload end: %u bytes\n", (unsigned)up.totalSize);
      }
    }
  );

  server->begin();
  Serial.println("Web server started");
}
