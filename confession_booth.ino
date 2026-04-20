#include "sd_utils.h"
#include "audio_utils.h"
#include "phone.h"

// Pin connected to the telephone hook switch.
// Wiring: one side to this pin, other side to GND.
// Receiver lifted = switch open = pin reads HIGH (INPUT_PULLUP).
#define HOOK_PIN         24
#define MAX_CONFESSIONS 20

// ── Prompt audio files — put these on the SD card ───────────────────────────
// WELCOME.WAV  "Welcome back. Someone left you a message..."
// FIRST.WAV    "You're the first one here. Lucky you."
// PROMPT.WAV   "Now it's your turn. Leave your confession after the beep."

// ── State machine ────────────────────────────────────────────────────────────

enum State {
  IDLE,
  PLAYING_WELCOME,
  PLAYING_CONFESSION,
  PLAYING_PROMPT,
  PLAYING_BEEP,
  RECORDING
};

static State         state         = IDLE;
static unsigned long stateEnteredAt = 0;
static char          recFilename[16];

static void transitionTo(State next) {
  state          = next;
  stateEnteredAt = millis();
}

// ── Setup ────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) ;

  if (!sdInit()) {
    Serial.println("SD failed — halting.");
    while (true) ;
  }

  audioInit();
  phoneInit(HOOK_PIN);

  randomSeed(analogRead(A0)); // floating pin for entropy

  Serial.println("Confession booth ready. Waiting for receiver...");
}

// ── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
  phonePoll();

  // Hangup interrupts any active state
  if (state != IDLE && justHungUp()) {
    Serial.println("Receiver hung up.");
    if (isRecording()) {
      stopRecording();
      if (countConfessions() > MAX_CONFESSIONS) pruneOldestConfession();
    } else {
      stopPlayback();
    }
    transitionTo(IDLE);
    return;
  }

  switch (state) {

    case IDLE:
      if (justPickedUp()) {
        Serial.println("Playing Welcome");
        playFile("WELCOME.WAV");
        transitionTo(PLAYING_WELCOME);
      }
      break;

    case PLAYING_WELCOME:
      if (millis() - stateEnteredAt >= 100 && !isPlaying()) {
        char confPath[16];
        if (getRandomConfession(confPath, sizeof(confPath))) {
          playFile(confPath);
          transitionTo(PLAYING_CONFESSION);
        } else {
          // Race condition: no confessions found between the check and now
          playFile("PROMPT.WAV");
          transitionTo(PLAYING_PROMPT);
        }
      }
      break;

    case PLAYING_CONFESSION:
      if (millis() - stateEnteredAt >= 100 && !isPlaying()) {
        Serial.println("Playing Confession");
        playFile("PROMPT.WAV");
        transitionTo(PLAYING_PROMPT);
      }
      break;

    case PLAYING_PROMPT:
      if (millis() - stateEnteredAt >= 100 && !isPlaying()) {
        Serial.println("Playing Prompt");
        startBeep();
        transitionTo(PLAYING_BEEP);
      }
      break;

    case PLAYING_BEEP:
      if (millis() - stateEnteredAt >= 1000) {
        stopBeep();
        nextConfessionFilename(recFilename, sizeof(recFilename));
        startRecording(recFilename);
        transitionTo(RECORDING);
      }
      break;

    case RECORDING:
      updateRecording();
      break;
  }
}
