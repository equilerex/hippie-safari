#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <algorithm>
#include <cstring>
#include <ArduinoJson.h>
#include "ContentManagerImpl.h"
#include "../include/Config.h"

bool ContentManagerImpl::initialize() {
  // Initialize SPI for SD card
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

  if (!SD.begin(PIN_SD_CS, SPI, 1000000)) {
    snprintf(lastError, sizeof(lastError), "SD.begin() failed");
    state.sdAvailable = false;
    state.contentDiscovered = false;
    return false;
  }

  state.sdAvailable = true;
  state.lastScanTimeMs = millis();

  // Perform initial content discovery (normal + easter eggs)
  if (!discoverContent()) {
    state.contentDiscovered = false;
    return false;
  }

  // Discover easter egg patterns (non-fatal; continues if not found)
  discoverEasterEggs();

  return true;
}

bool ContentManagerImpl::discoverContent() {
  if (!state.sdAvailable) {
    snprintf(lastError, sizeof(lastError), "SD not available");
    return false;
  }

  cachedTypes.clear();
  state.typesFound = 0;
  state.lastScanTimeMs = millis();

  File root = SD.open(PATH_TYPES_ROOT);
  if (!root) {
    snprintf(lastError, sizeof(lastError), "Cannot open %s", PATH_TYPES_ROOT);
    state.contentDiscovered = false;
    return false;
  }

  if (!root.isDirectory()) {
    snprintf(lastError, sizeof(lastError), "%s is not a directory", PATH_TYPES_ROOT);
    root.close();
    state.contentDiscovered = false;
    return false;
  }

  // Collect all type folders
  std::vector<std::pair<std::string, File>> typeFolders;
  File typeFolder = root.openNextFile();

  while (typeFolder) {
    if (typeFolder.isDirectory()) {
      const char* folderName = typeFolder.name();
      typeFolders.push_back({std::string(folderName), typeFolder});
    }
    typeFolder = root.openNextFile();
  }

  // Sort alphabetically
  std::sort(typeFolders.begin(), typeFolders.end(),
    [](const std::pair<std::string, File>& a, const std::pair<std::string, File>& b) {
      return a.first < b.first;
    });

  // Scan each type folder (limit to NUM_BUTTON_TYPES)
  for (size_t i = 0; i < typeFolders.size() && i < NUM_BUTTON_TYPES; i++) {
    TypeContent content;
    content.typeIndex = i;
    content.folderName = typeFolders[i].first;

    if (scanTypeFolder(i, typeFolders[i].second, content)) {
      cachedTypes.push_back(content);
      state.typesFound++;
    }

    typeFolders[i].second.close();
  }

  root.close();

  if (state.typesFound == 0) {
    snprintf(lastError, sizeof(lastError), "No type folders found or no variants discovered");
    state.contentDiscovered = false;
    return false;
  }

  state.contentDiscovered = true;
  return true;
}

bool ContentManagerImpl::scanTypeFolder(uint8_t typeIndex, File& typeFolder, TypeContent& outContent) {
  outContent.typeIndex = typeIndex;

  // Scan for mode folders within this type
  typeFolder.rewindDirectory();
  File modeFolder = typeFolder.openNextFile();
  bool foundVariants = false;

  while (modeFolder) {
    if (modeFolder.isDirectory()) {
      const char* modeName = modeFolder.name();

      // Collect variants with mode folder prefix
      std::vector<std::string> modeVariants;
      if (collectVariants(modeFolder, modeVariants)) {
        foundVariants = true;

        // Prepend mode folder name to each variant (e.g., "default/variant.wav")
        for (const auto& variant : modeVariants) {
          std::string prefixed = std::string(modeName) + "/" + variant;
          outContent.variants.push_back(prefixed);
        }
      }

      // Only process optional mode configs for non-default folders
      if (strcmp(modeName, "default") != 0) {
        ModeConfig modeConfig;
        if (loadModeFolderConfig(typeIndex, outContent.folderName.c_str(), modeName, modeConfig)) {
          outContent.modes.push_back(modeConfig);
        }
      }
    }
    modeFolder = typeFolder.openNextFile();
  }

  return foundVariants;
}

bool ContentManagerImpl::collectVariants(File& folder, std::vector<std::string>& outVariants) {
  outVariants.clear();

  folder.rewindDirectory();
  File file = folder.openNextFile();

  while (file) {
    if (!file.isDirectory()) {
      const char* fileName = file.name();
      // Check if .wav file
      if (fileName && strlen(fileName) > 4) {
        const char* ext = fileName + strlen(fileName) - 4;
        if (strcasecmp(ext, ".wav") == 0) {
          outVariants.push_back(std::string(fileName));
        }
      }
    }
    file = folder.openNextFile();
  }

  // Sort variants alphabetically
  std::sort(outVariants.begin(), outVariants.end());

  return outVariants.size() > 0;
}

bool ContentManagerImpl::loadModeFolderConfig(uint8_t typeIndex, const char* typeFolderName, const char* modeFolderName, ModeConfig& outConfig) {
  char configPath[256];
  snprintf(configPath, sizeof(configPath), "%s/%s/%s/%s", PATH_TYPES_ROOT, typeFolderName, modeFolderName, CONFIG_FILENAME);

  File configFile = SD.open(configPath);
  if (!configFile) {
    return false;
  }

  size_t fileSize = configFile.size();
  if (fileSize == 0 || fileSize > 8192) {
    configFile.close();
    return false;
  }

  char* jsonBuffer = new char[fileSize + 1];
  if (!jsonBuffer) {
    configFile.close();
    return false;
  }

  configFile.read((uint8_t*)jsonBuffer, fileSize);
  jsonBuffer[fileSize] = '\0';
  configFile.close();

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, jsonBuffer);
  delete[] jsonBuffer;

  if (error) {
    Serial.print("[WARNING] Failed to parse config.json for mode: ");
    Serial.println(modeFolderName);
    return false;
  }

  outConfig.modeName = modeFolderName;
  int rootPriority = doc["priority"] | 0;

  if (doc.containsKey("timeWindows")) {
    JsonArray windowsArray = doc["timeWindows"].as<JsonArray>();
    for (JsonObject windowObj : windowsArray) {
      TimeWindow window;
      window.priority = windowObj["priority"] | rootPriority;
      
      if (windowObj.containsKey("days")) {
        window.type = TimeWindowType::DAY_SPECIFIC;
        JsonArray daysArray = windowObj["days"].as<JsonArray>();
        window.dayMask = 0;
        for (const char* dayStr : daysArray) {
          if (strcasecmp(dayStr, "monday") == 0) window.dayMask |= (1 << 0);
          else if (strcasecmp(dayStr, "tuesday") == 0) window.dayMask |= (1 << 1);
          else if (strcasecmp(dayStr, "wednesday") == 0) window.dayMask |= (1 << 2);
          else if (strcasecmp(dayStr, "thursday") == 0) window.dayMask |= (1 << 3);
          else if (strcasecmp(dayStr, "friday") == 0) window.dayMask |= (1 << 4);
          else if (strcasecmp(dayStr, "saturday") == 0) window.dayMask |= (1 << 5);
          else if (strcasecmp(dayStr, "sunday") == 0) window.dayMask |= (1 << 6);
          else if (strcasecmp(dayStr, "test") == 0 || strcasecmp(dayStr, "test_odd") == 0) window.dayMask |= (1 << 7);
        }
        const char* startStr = windowObj["startTime"] | "00:00";
        const char* endStr = windowObj["endTime"] | "00:00";
        sscanf(startStr, "%hhu:%hhu", &window.startHour, &window.startMin);
        sscanf(endStr, "%hhu:%hhu", &window.endHour, &window.endMin);
      } else {
        window.type = TimeWindowType::DAILY_RECURRING;
        const char* startStr = windowObj["startTime"] | "00:00";
        const char* endStr = windowObj["endTime"] | "00:00";
        sscanf(startStr, "%hhu:%hhu", &window.startHour, &window.startMin);
        sscanf(endStr, "%hhu:%hhu", &window.endHour, &window.endMin);
      }
      outConfig.timeWindows.push_back(window);
    }
  } else if (doc.containsKey("startTime") && doc.containsKey("endTime")) {
    TimeWindow window;
    window.priority = rootPriority;
    window.type = TimeWindowType::DAILY_RECURRING;
    const char* startStr = doc["startTime"] | "00:00";
    const char* endStr = doc["endTime"] | "00:00";
    sscanf(startStr, "%hhu:%hhu", &window.startHour, &window.startMin);
    sscanf(endStr, "%hhu:%hhu", &window.endHour, &window.endMin);
    outConfig.timeWindows.push_back(window);
  }

  return outConfig.timeWindows.size() > 0;
}

bool ContentManagerImpl::loadModeConfig(uint8_t typeIndex, const char* folderPath, TypeContent& outContent) {
  // Deprecated: configs now per-mode-folder, not per-type
  outContent.defaultMode.modeName = "default";
  return true;
}

uint8_t ContentManagerImpl::getTypeCount() const {
  return cachedTypes.size();
}

const TypeContent* ContentManagerImpl::getType(uint8_t typeIndex) const {
  if (typeIndex < cachedTypes.size()) {
    return &cachedTypes[typeIndex];
  }
  return nullptr;
}

const std::vector<TypeContent>& ContentManagerImpl::getAllTypes() const {
  return cachedTypes;
}

const char* ContentManagerImpl::getVariantPath(uint8_t typeIndex, uint8_t variantIndex) const {
  static char path[256];
  const TypeContent* type = getType(typeIndex);
  if (!type || variantIndex >= type->variants.size()) {
    return nullptr;
  }

  snprintf(path, sizeof(path), "%s/%s/%s", PATH_TYPES_ROOT, type->folderName.c_str(),
           type->variants[variantIndex].c_str());
  return path;
}

uint8_t ContentManagerImpl::getVariantCount(uint8_t typeIndex) const {
  const TypeContent* type = getType(typeIndex);
  return type ? type->variants.size() : 0;
}

bool ContentManagerImpl::typeHasContent(uint8_t typeIndex) const {
  const TypeContent* type = getType(typeIndex);
  return type && type->variants.size() > 0;
}

const ContentState& ContentManagerImpl::getContentState() const {
  return state;
}

bool ContentManagerImpl::retrySDMount() {
  if (state.sdAvailable) {
    return true;  // Already mounted
  }

  // Try to remount SD
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  if (!SD.begin(PIN_SD_CS, SPI, 1000000)) {
    snprintf(lastError, sizeof(lastError), "SD remount failed");
    return false;
  }

  state.sdAvailable = true;

  // Retry content discovery
  if (discoverContent()) {
    Serial.println("SD recovered, content rediscovered");
    return true;
  }

  return false;
}

bool ContentManagerImpl::discoverEasterEggs() {
  if (!SD.exists(PATH_EASTER_EGG)) {
    return true;  // No easter egg folder; not an error
  }

  File eggRoot = SD.open(PATH_EASTER_EGG);
  if (!eggRoot || !eggRoot.isDirectory()) {
    return true;  // Not a directory; skip
  }

  cachedEasterEggVariants.clear();

  // Iterate through easter egg pattern folders
  File patternFolder;
  while (patternFolder = eggRoot.openNextFile()) {
    if (!patternFolder.isDirectory()) {
      patternFolder.close();
      continue;
    }

    const char* patternName = patternFolder.name();

    // Map folder name to EasterEggPattern enum
    EasterEggPattern pattern = EasterEggPattern::NONE;
    if (strcmp(patternName, "SECRET_BUTTON") == 0) pattern = EasterEggPattern::SECRET_BUTTON;
    else if (strcmp(patternName, "ASCENDING_SWEEP") == 0) pattern = EasterEggPattern::ASCENDING_SWEEP;
    else if (strcmp(patternName, "SOS_MORSE") == 0) pattern = EasterEggPattern::SOS_MORSE;
    else if (strcmp(patternName, "HAMMER_SINGLE") == 0) pattern = EasterEggPattern::HAMMER_SINGLE;
    else if (strcmp(patternName, "TEAM_EFFORT") == 0) pattern = EasterEggPattern::TEAM_EFFORT;
    else if (strcmp(patternName, "LONG_HOLD_SUSTAINED") == 0) pattern = EasterEggPattern::LONG_HOLD_SUSTAINED;
    else if (strcmp(patternName, "MULTI_HOLD") == 0) pattern = EasterEggPattern::MULTI_HOLD;
    else if (strcmp(patternName, "ALL_BUTTONS_HELD") == 0) pattern = EasterEggPattern::ALL_BUTTONS_HELD;
    else if (strcmp(patternName, "CHAOS_BURST") == 0) pattern = EasterEggPattern::CHAOS_BURST;
    else if (strcmp(patternName, "MULTI_CLICK") == 0) pattern = EasterEggPattern::MULTI_CLICK;

    if (pattern != EasterEggPattern::NONE) {
      // Scan for .wav files in this folder
      std::vector<std::string> variants;
      File variantFile;
      while (variantFile = patternFolder.openNextFile()) {
        const char* fileName = variantFile.name();
        int len = strlen(fileName);
        if (len > 4 && strcasecmp(fileName + len - 4, ".wav") == 0) {
          variants.push_back(fileName);
        }
        variantFile.close();
      }

      if (!variants.empty()) {
        // Sort variants alphabetically for consistent ordering
        std::sort(variants.begin(), variants.end());
        cachedEasterEggVariants[pattern] = variants;
      }
    }

    patternFolder.close();
  }

  eggRoot.close();
  return true;
}

std::vector<std::string> ContentManagerImpl::getEasterEggVariants(EasterEggPattern pattern) {
  auto it = cachedEasterEggVariants.find(pattern);
  if (it != cachedEasterEggVariants.end()) {
    return it->second;
  }
  return std::vector<std::string>();
}

const char* ContentManagerImpl::getEasterEggVariantPath(EasterEggPattern pattern, uint8_t variantIndex) const {
  auto it = cachedEasterEggVariants.find(pattern);
  if (it == cachedEasterEggVariants.end() || variantIndex >= it->second.size()) {
    return nullptr;
  }

  static char fullPath[256];
  snprintf(fullPath, sizeof(fullPath), "%s/%s/%s",
           PATH_EASTER_EGG, getEasterEggPatternName(pattern), it->second[variantIndex].c_str());
  return fullPath;
}

const char* ContentManagerImpl::getLastError() const {
  return lastError;
}
