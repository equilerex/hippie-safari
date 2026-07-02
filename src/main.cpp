#include <Arduino.h>
#include "SystemManagerImpl.h"

// Global system instance
std::unique_ptr<SystemManager> system;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Hippy Safari Audio Installation ===");
  Serial.println("Initializing subsystems...");

  // Create and initialize system
  system = std::make_unique<SystemManagerImpl>();
  if (!system->initialize()) {
    Serial.print("ERROR: System initialization failed: ");
    Serial.println(system->getLastError());
    system->getPlaybackLogger()->logSDError(system->getLastError());
    return;
  }

  Serial.println("System initialized successfully");
  Serial.print("Types discovered: ");
  Serial.println(system->getContentManager()->getTypeCount());
  Serial.println("Ready for button input");
}

void loop() {
  // Main system update
  system->update();

  // Small delay to prevent CPU hogging
  delay(10);
}
