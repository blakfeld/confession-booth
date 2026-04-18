#pragma once

#include <Arduino.h>

// Hook switch wiring assumption: INPUT_PULLUP on hookPin.
// Receiver on hook  → switch shorts pin to GND → reads LOW.
// Receiver off hook → switch open              → reads HIGH.
// Flip the LOW/HIGH logic below if your switch is wired the other way.

void phoneInit(int hookPin);

// Call once per loop iteration to debounce and update state.
void phonePoll();

bool isOffHook();
bool justPickedUp(); // true for exactly one phonePoll() cycle after pickup
bool justHungUp();   // true for exactly one phonePoll() cycle after hangup
