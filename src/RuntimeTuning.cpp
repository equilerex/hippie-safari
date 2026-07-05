#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "../include/Config.h"

RuntimeTuning g_tuning;

void RuntimeTuning::loadFromSD() {
  File file = SD.open("/safari-conf.json");
  if (!file) {
    return;  // No override file — keep compiled-in defaults
  }

  size_t fileSize = file.size();
  if (fileSize == 0 || fileSize > 8192) {
    file.close();
    return;
  }

  char* jsonBuffer = new char[fileSize + 1];
  file.read((uint8_t*)jsonBuffer, fileSize);
  jsonBuffer[fileSize] = '\0';
  file.close();

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, jsonBuffer);
  delete[] jsonBuffer;

  if (error) {
    Serial.print("[CONF] /safari-conf.json parse error: ");
    Serial.println(error.c_str());
    return;
  }

  buttonDebounceMs        = doc["buttonDebounceMs"]        | buttonDebounceMs;
  buttonClickMaxHoldMs    = doc["buttonClickMaxHoldMs"]    | buttonClickMaxHoldMs;

  eventHistoryWindowMs    = doc["eventHistoryWindowMs"]    | eventHistoryWindowMs;
  patternCooldownMs       = doc["patternCooldownMs"]       | patternCooldownMs;
  maxTapHoldMs            = doc["maxTapHoldMs"]            | maxTapHoldMs;
  loopResetMs             = doc["loopResetMs"]             | loopResetMs;

  sweepWindowMs           = doc["sweepWindowMs"]           | sweepWindowMs;
  sweepMinStepGapMs       = doc["sweepMinStepGapMs"]       | sweepMinStepGapMs;
  sosWindowMs             = doc["sosWindowMs"]             | sosWindowMs;
  sosTapThreshold         = doc["sosTapThreshold"]         | sosTapThreshold;
  hammerWindowMs          = doc["hammerWindowMs"]          | hammerWindowMs;
  hammerTapThreshold      = doc["hammerTapThreshold"]      | hammerTapThreshold;
  teamEffortWindowMs      = doc["teamEffortWindowMs"]      | teamEffortWindowMs;
  fastClickPairWindowMs   = doc["fastClickPairWindowMs"]   | fastClickPairWindowMs;
  longHoldMs              = doc["longHoldMs"]              | longHoldMs;
  multiHoldMs             = doc["multiHoldMs"]             | multiHoldMs;
  multiHoldJoinWindowMs   = doc["multiHoldJoinWindowMs"]   | multiHoldJoinWindowMs;
  allButtonsHeldMs        = doc["allButtonsHeldMs"]        | allButtonsHeldMs;
  allButtonsJoinWindowMs  = doc["allButtonsJoinWindowMs"]  | allButtonsJoinWindowMs;
  chaosBurstWindowMs      = doc["chaosBurstWindowMs"]      | chaosBurstWindowMs;
  chaosBurstThreshold     = doc["chaosBurstThreshold"]     | chaosBurstThreshold;
  multiClickWindowMs      = doc["multiClickWindowMs"]      | multiClickWindowMs;
  multiClickThreshold     = doc["multiClickThreshold"]     | multiClickThreshold;

  Serial.println("[CONF] Loaded /safari-conf.json overrides");
}
