/*
 * RC Reciever LED Level Indicator
 * 
 * Shows current levels on four main RC channels via 3 LED array (one at a time).
 * Toggle channel shown via momentary (.25 second) high level on another channel.
 * Requires EnableInterrupt.
 * 
 * First Arduino "project", code could probably be better.
 * 
 * created 27 Oct 2015
 * by James Watts
 */

#include <EnableInterrupt.h>

// RC Channels in
#define THROTTLE_SIGNAL_IN 2
#define YAW_SIGNAL_IN      5
#define PITCH_SIGNAL_IN    4
#define ROLL_SIGNAL_IN     3
#define TOGGLE_SIGNAL_IN   10

// LED level out
#define LEVEL_OUT_LOW  A0
#define LEVEL_OUT_MED  A1
#define LEVEL_OUT_HIGH A2

// Signal LED
#define SIGNAL_OUT 13

// Update Flags
#define THROTTLE_FLAG 1
#define YAW_FLAG      2
#define PITCH_FLAG    4
#define ROLL_FLAG     8
#define TOGGLE_FLAG   16

volatile uint8_t updateFlagsShared;
uint8_t currentFlag;

// Shared ISR channel values
volatile uint16_t throttleInShared;
volatile uint16_t yawInShared;
volatile uint16_t pitchInShared;
volatile uint16_t rollInShared;
volatile uint16_t toggleInShared;

// Channel pulse times
uint32_t throttleStart;
uint32_t yawStart;
uint32_t pitchStart;
uint32_t rollStart;
uint32_t toggleStart;

// Signal LED pulse time / state
uint32_t signalStart = 0;
int signalState = LOW;

// Toggle time / state
uint32_t toggleLastChange = 0;
int toggleLastState = 0;

void setup() {
  // Setup input (RC) pins
  pinMode(THROTTLE_SIGNAL_IN, INPUT);
  pinMode(YAW_SIGNAL_IN, INPUT);
  pinMode(PITCH_SIGNAL_IN, INPUT);
  pinMode(ROLL_SIGNAL_IN, INPUT);
  pinMode(TOGGLE_SIGNAL_IN, INPUT);
  
  // Setup output LED pins
  pinMode(LEVEL_OUT_LOW, OUTPUT);
  pinMode(LEVEL_OUT_MED, OUTPUT);
  pinMode(LEVEL_OUT_HIGH, OUTPUT);
  pinMode(SIGNAL_OUT, OUTPUT);

  // Setup current channel
  currentFlag = THROTTLE_FLAG;
  
  // Attach Interrupts
  enableInterrupt(THROTTLE_SIGNAL_IN, calcThrottle, CHANGE);
  enableInterrupt(YAW_SIGNAL_IN, calcYaw, CHANGE);
  enableInterrupt(PITCH_SIGNAL_IN, calcPitch, CHANGE);
  enableInterrupt(ROLL_SIGNAL_IN, calcRoll, CHANGE);
  enableInterrupt(TOGGLE_SIGNAL_IN, calcToggle, CHANGE);
}

void loop() {
  // Local Channel Values
  static uint16_t throttleIn;
  static uint16_t yawIn;
  static uint16_t pitchIn;
  static uint16_t rollIn;
  static uint16_t toggleIn;

  // Local flag copy
  static uint8_t updateFlags;

  // Check channels for updates and copy to locals
  if (updateFlagsShared) {
    noInterrupts();
    updateFlags = updateFlagsShared;

    if (updateFlags & THROTTLE_FLAG) {
      throttleIn = throttleInShared;
    }

    if (updateFlags & YAW_FLAG) {
      yawIn = yawInShared;
    }

    if (updateFlags & PITCH_FLAG) {
      pitchIn = pitchInShared;
    }

    if (updateFlags & ROLL_FLAG) {
      rollIn = rollInShared;
    }

    if (updateFlags & TOGGLE_FLAG) {
      toggleIn = toggleInShared;
    }

    updateFlagsShared = 0;
    interrupts();
  }

  // Handle toggle state change
  if (updateFlags & TOGGLE_FLAG) {
    
    // Check if toggle is held at high level for more than .25 seconds
    // but only toggle once per switch (debounce)
    if (toggleIn > 1900) {   
      if (toggleLastState == 0) {
        if (toggleLastChange == 0) {
          toggleLastChange = millis();
        } else if ((millis() - toggleLastChange) > 250) {
          
          // Toggled high for > .25 seconds, switch channels
          if (currentFlag & ROLL_FLAG) {
            currentFlag = THROTTLE_FLAG;
          } else {
            currentFlag = currentFlag << 1;
          }

          // Save the toggled state to prevent another 
          // toggle until switch is released (channel level low)
          toggleLastState = 1;
        }
      }
    } else {
      toggleLastChange = 0;
      toggleLastState = 0;
    }
  }
  
  // Show channel selected by current mask
  if (currentFlag & THROTTLE_FLAG) {
    showSignalLevel(throttleIn);
  } else if (currentFlag & YAW_FLAG) {
    showSignalLevel(yawIn);
  } else if (currentFlag & PITCH_FLAG) {
    showSignalLevel(pitchIn);
  } else if (currentFlag & ROLL_FLAG) {
    showSignalLevel(rollIn);
  } else {
    // Default to Throttle
    showSignalLevel(throttleIn);
  }

  // Show current channel selected by flashing signal LED
  showCurrentChannel(currentFlag);
}

// Handle interrupts for each channel, could be rewritten as a single 
// function via EI_ARDUINO_INTERRUPTED_PIN detection
void calcThrottle() {
  if (digitalRead(THROTTLE_SIGNAL_IN) == HIGH) {
    throttleStart = micros();
  } else {
    throttleInShared = (uint16_t)(micros() - throttleStart);
    updateFlagsShared |= THROTTLE_FLAG;
  }
}

void calcYaw() {
  if (digitalRead(YAW_SIGNAL_IN) == HIGH) {
    yawStart = micros();
  } else {
    yawInShared = (uint16_t)(micros() - yawStart);
    updateFlagsShared |= YAW_FLAG;
  }
}

void calcPitch() {
  if (digitalRead(PITCH_SIGNAL_IN) == HIGH) {
    pitchStart = micros();
  } else {
    pitchInShared = (uint16_t)(micros() - pitchStart);
    updateFlagsShared |= PITCH_FLAG;
  }  
}

void calcRoll() {
  if (digitalRead(ROLL_SIGNAL_IN) == HIGH) {
    rollStart = micros();
  } else {
    rollInShared = (uint16_t)(micros() - rollStart);
    updateFlagsShared |= ROLL_FLAG;
  }    
}

void calcToggle() {
  if (digitalRead(TOGGLE_SIGNAL_IN) == HIGH) {
    toggleStart = micros();
  } else {
    toggleInShared = (uint16_t)(micros() - toggleStart);
    updateFlagsShared |= TOGGLE_FLAG;
  }
}

// Translate current channel's level to lighting the LED array
void showSignalLevel(uint16_t levelIn) {
 
  if (levelIn > 1100) {
    // Show LOW LED
    digitalWrite(LEVEL_OUT_LOW, HIGH);
  } else {
    digitalWrite(LEVEL_OUT_LOW, LOW);
  }

  if (levelIn > 1400) {
    // Show MED LED
    digitalWrite(LEVEL_OUT_MED, HIGH);
  } else {
    digitalWrite(LEVEL_OUT_MED, LOW);
  }
  
  if (levelIn > 1900) {
    // Show HIGH LED
    digitalWrite(LEVEL_OUT_HIGH, HIGH);
  } else {
    digitalWrite(LEVEL_OUT_HIGH, LOW);
  }
}

// Flash Signal LED, speed based on current channel
void showCurrentChannel(uint8_t currentChannelFlag) {
  uint16_t interval;
  uint16_t currentTime = millis();
  
  if (currentChannelFlag > 0) {
    interval = 1000 / currentChannelFlag;
  } else {
    interval = 2000;
  }

  if (currentTime - signalStart >= interval) {
    signalStart = currentTime;
    if (signalState == LOW) {
      digitalWrite(SIGNAL_OUT, HIGH);
      signalState = HIGH;
    } else {
      digitalWrite(SIGNAL_OUT, LOW);
      signalState = LOW;
    }
  }
}
