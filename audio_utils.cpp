#include "audio_utils.h"

// ── Audio graph ──────────────────────────────────────────────────────────────
// playback path:  player ──┐
//                          ├── mixer ── i2sOut (both channels, mono to stereo)
//                beepOsc ──┘
// recording path: i2sIn ── recordQueue

AudioPlaySdWav       player;
AudioSynthWaveform   beepOsc;
AudioMixer4          mixer;
AudioOutputI2S       i2sOut;
AudioInputI2S        i2sIn;
AudioRecordQueue     recordQueue;
AudioControlSGTL5000 codec;

AudioConnection c1(player,  0, mixer,       0);
AudioConnection c2(beepOsc, 0, mixer,       1);
AudioConnection c3(mixer,   0, i2sOut,      0);
AudioConnection c4(mixer,   0, i2sOut,      1); // mono → both ears
AudioConnection c5(i2sIn,   0, recordQueue, 0);

// ── WAV recording state ──────────────────────────────────────────────────────
static File     _recFile;
static uint32_t _bytesWritten;
static bool     _recording = false;

static const uint32_t SAMPLE_RATE    = 44100;
static const uint16_t NUM_CHANNELS   = 1;
static const uint16_t BITS_PER_SAMPLE = 16;

static void writeWavHeader(File &f, uint32_t dataBytes) {
  const uint32_t byteRate   = SAMPLE_RATE * NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
  const uint16_t blockAlign = NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
  const uint32_t fmtSize    = 16;
  const uint16_t pcmFormat  = 1;
  const uint32_t fileSize   = dataBytes + 36;

  f.write("RIFF", 4);
  f.write((uint8_t *)&fileSize,      4);
  f.write("WAVE", 4);
  f.write("fmt ", 4);
  f.write((uint8_t *)&fmtSize,       4);
  f.write((uint8_t *)&pcmFormat,     2);
  f.write((uint8_t *)&NUM_CHANNELS,  2);
  f.write((uint8_t *)&SAMPLE_RATE,   4);
  f.write((uint8_t *)&byteRate,      4);
  f.write((uint8_t *)&blockAlign,    2);
  f.write((uint8_t *)&BITS_PER_SAMPLE, 2);
  f.write("data", 4);
  f.write((uint8_t *)&dataBytes,     4);
}

// ── Public API ───────────────────────────────────────────────────────────────

void audioInit() {
  AudioMemory(60);

  codec.enable();
  codec.volume(0.8);
  codec.inputSelect(AUDIO_INPUT_MIC);
  codec.micGain(36);

  beepOsc.begin(0.0, 880.0, WAVEFORM_SINE); // start silent

  mixer.gain(0, 1.0); // player channel on
  mixer.gain(1, 0.0); // beep channel off
  mixer.gain(2, 0.0);
  mixer.gain(3, 0.0);
}

void playFile(const char *filename) {
  stopBeep();
  mixer.gain(0, 1.0);
  bool ok = player.play(filename);
  Serial.print("playFile(");
  Serial.print(filename);
  Serial.print(") -> ");
  Serial.println(ok ? "OK" : "FAILED");
  // Give AudioPlaySdWav a moment to confirm it started
  delay(10);
  Serial.print("  isPlaying after 10ms: ");
  Serial.println(player.isPlaying() ? "yes" : "no");
}

void stopPlayback() {
  player.stop();
  stopBeep();
}

bool isPlaying() {
  return player.isPlaying();
}

void startBeep(float freq, float amplitude) {
  player.stop();
  mixer.gain(0, 0.0);
  mixer.gain(1, 1.0);
  beepOsc.frequency(freq);
  beepOsc.amplitude(amplitude);
}

void stopBeep() {
  beepOsc.amplitude(0.0);
  mixer.gain(0, 1.0);
  mixer.gain(1, 0.0);
}

void startRecording(const char *filename) {
  if (SD.exists(filename)) SD.remove(filename);

  _recFile = SD.open(filename, FILE_WRITE);
  if (!_recFile) {
    Serial.print("Cannot open for recording: ");
    Serial.println(filename);
    return;
  }

  _bytesWritten = 0;
  writeWavHeader(_recFile, 0); // placeholder — fixed up in stopRecording()

  recordQueue.begin();
  _recording = true;
  Serial.print("Recording to ");
  Serial.println(filename);
}

void updateRecording() {
  if (!_recording) return;
  // Write in 512-byte chunks (2 audio blocks) to align with SD sector size.
  while (recordQueue.available() >= 2) {
    byte buf[512];
    memcpy(buf,       recordQueue.readBuffer(), 256);
    recordQueue.freeBuffer();
    memcpy(buf + 256, recordQueue.readBuffer(), 256);
    recordQueue.freeBuffer();
    _recFile.write(buf, 512);
    _bytesWritten += 512;
  }
}

void stopRecording() {
  if (!_recording) return;

  recordQueue.end();

  // Drain any remaining partial buffers
  while (recordQueue.available() > 0) {
    _recFile.write((byte *)recordQueue.readBuffer(), 256);
    recordQueue.freeBuffer();
    _bytesWritten += 256;
  }

  // Seek back and write the correct WAV header now that we know the data size
  _recFile.seek(0);
  writeWavHeader(_recFile, _bytesWritten);
  _recFile.close();

  _recording = false;
  Serial.print("Saved ");
  Serial.print(_bytesWritten / 1024);
  Serial.println(" KB");
}

bool isRecording() {
  return _recording;
}
