#include "sd_utils.h"

static bool isConfessionFile(const char *name) {
  if (strncmp(name, "CONF", 4) != 0) return false;
  const char *ext = strrchr(name, '.');
  return ext && strcmp(ext, ".WAV") == 0;
}

bool sdInit() {
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("SD init failed — is a card inserted?");
    return false;
  }
  Serial.println("SD card ready.");
  return true;
}

void listDir(const char *path) {
  File dir = SD.open(path);
  if (!dir) {
    Serial.print("Could not open directory: ");
    Serial.println(path);
    return;
  }
  Serial.print("Contents of ");
  Serial.println(path);
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    Serial.print("  ");
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
    } else {
      Serial.print("  (");
      Serial.print(entry.size());
      Serial.println(" bytes)");
    }
    entry.close();
  }
  dir.close();
}

void readFile(const char *filename) {
  File f = SD.open(filename);
  if (!f) {
    Serial.print("Could not open: ");
    Serial.println(filename);
    return;
  }
  Serial.print("--- "); Serial.print(filename); Serial.println(" ---");
  while (f.available()) Serial.write(f.read());
  Serial.println("\n---");
  f.close();
}

bool writeFile(const char *filename, const char *data, bool append) {
  File f = SD.open(filename, append ? FILE_WRITE : O_WRITE | O_CREAT | O_TRUNC);
  if (!f) {
    Serial.print("Could not open for writing: ");
    Serial.println(filename);
    return false;
  }
  size_t written = f.print(data);
  f.close();
  if (written == 0 && strlen(data) > 0) {
    Serial.print("Write failed: ");
    Serial.println(filename);
    return false;
  }
  return true;
}

int countConfessions() {
  File root = SD.open("/");
  int count = 0;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory() && isConfessionFile(entry.name())) count++;
    entry.close();
  }
  root.close();
  return count;
}

bool getRandomConfession(char *buf, size_t bufLen) {
  int total = countConfessions();
  if (total == 0) return false;

  int target = random(total);
  File root = SD.open("/");
  int idx = 0;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory() && isConfessionFile(entry.name())) {
      if (idx == target) {
        strncpy(buf, entry.name(), bufLen - 1);
        buf[bufLen - 1] = '\0';
        entry.close();
        root.close();
        return true;
      }
      idx++;
    }
    entry.close();
  }
  root.close();
  return false;
}

void nextConfessionFilename(char *buf, size_t bufLen) {
  for (int i = 0; i < 1000; i++) {
    snprintf(buf, bufLen, "CONF%03d.WAV", i);
    if (!SD.exists(buf)) return;
  }
  snprintf(buf, bufLen, "CONF999.WAV");
}

void pruneOldestConfession() {
  char buf[16];
  for (int i = 0; i < 1000; i++) {
    snprintf(buf, sizeof(buf), "CONF%03d.WAV", i);
    if (SD.exists(buf)) {
      SD.remove(buf);
      Serial.print("Pruned oldest confession: ");
      Serial.println(buf);
      return;
    }
  }
}
