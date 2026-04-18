#include "phone.h"

static int  _pin;
static bool _stable;      // last debounced state
static bool _lastRead;    // last raw pin read
static bool _pickedUp;
static bool _hungUp;
static unsigned long _lastChangeMs;

static const unsigned long DEBOUNCE_MS = 50;

void phoneInit(int hookPin) {
  _pin = hookPin;
  pinMode(_pin, INPUT_PULLUP);
  _lastRead = _stable = (digitalRead(_pin) == HIGH);
  _pickedUp = _hungUp = false;
  _lastChangeMs = millis();
}

void phonePoll() {
  _pickedUp = false;
  _hungUp   = false;

  bool reading = (digitalRead(_pin) == HIGH);

  if (reading != _lastRead) {
    _lastRead    = reading;
    _lastChangeMs = millis();
  }

  if ((millis() - _lastChangeMs >= DEBOUNCE_MS) && (reading != _stable)) {
    _pickedUp = (reading && !_stable);
    _hungUp   = (!reading && _stable);
    _stable   = reading;
  }
}

bool isOffHook()    { return _stable; }
bool justPickedUp() { return _pickedUp; }
bool justHungUp()   { return _hungUp; }
