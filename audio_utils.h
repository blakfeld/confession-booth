#pragma once

#include <Audio.h>
#include <Wire.h>
#include <SD.h>

// All audio objects are global — the audio library requires this.
extern AudioPlaySdWav       player;
extern AudioSynthWaveform   beepOsc;
extern AudioMixer4          mixer;
extern AudioOutputI2S       i2sOut;
extern AudioInputI2S        i2sIn;
extern AudioRecordQueue     recordQueue;
extern AudioControlSGTL5000 codec;

void audioInit();

void playFile(const char *filename);
void stopPlayback();
bool isPlaying();

// Synthesizes a tone through the headphone output.
void startBeep(float freq = 880.0, float amplitude = 0.5);
void stopBeep();

// Recording — call updateRecording() every loop iteration while recording.
void startRecording(const char *filename);
void updateRecording();
void stopRecording();
bool isRecording();
