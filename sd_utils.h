#pragma once

#include <SD.h>

bool sdInit();
void listDir(const char *path);
void readFile(const char *filename);
bool writeFile(const char *filename, const char *data, bool append = false);

// Returns the number of confession files (CONF###.WAV) in the root directory.
int countConfessions();

// Fills buf with the path to a randomly selected confession file.
// Returns false if there are no confessions.
bool getRandomConfession(char *buf, size_t bufLen);

// Fills buf with the next available confession filename (e.g. "CONF003.WAV").
void nextConfessionFilename(char *buf, size_t bufLen);

// Deletes the lowest-numbered CONF###.WAV file (the oldest confession).
void pruneOldestConfession();
