#include <Arduino.h>
#include "SystemManagerImpl.h"

// Global system instance
std::unique_ptr<SystemManager> audioSystem;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Hippy Safari Audio Installation ===");
  Serial.println("Initializing subsystems...");

  // Create and initialize system
  audioSystem.reset(new SystemManagerImpl());
  if (!audioSystem->initialize()) {
    Serial.print("ERROR: System initialization failed: ");
    Serial.println(audioSystem->getLastError());
    audioSystem->getPlaybackLogger()->logSDError(audioSystem->getLastError());
    return;
  }

  Serial.println("System initialized successfully");
  Serial.print("Types discovered: ");
  Serial.println(audioSystem->getContentManager()->getTypeCount());
  Serial.println("Ready for button input");
}

void loop() {
  // Main system update
  audioSystem->update();

  // Small delay to prevent CPU hogging
  delay(10);
}
