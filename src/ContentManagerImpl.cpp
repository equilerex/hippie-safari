#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <algorithm>
#include <cstring>
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

  // Perform initial content discovery
  if (!discoverContent()) {
    state.contentDiscovered = false;
    return false;
  }

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
  // Build path: /audio/types/{typeFolderName}/{modeFolderName}/config.json
  char configPath[256];
  snprintf(configPath, sizeof(configPath), "%s/%s/%s/%s", PATH_TYPES_ROOT, typeFolderName, modeFolderName, CONFIG_FILENAME);

  File configFile = SD.open(configPath);
  if (!configFile) {
    // No config in this mode folder - skip it
    return false;
  }

  // TODO: Parse JSON and populate outConfig
  // For now, just set mode name and close file
  outConfig.modeName = modeFolderName;
  configFile.close();
  return true;
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

const char* ContentManagerImpl::getLastError() const {
  return lastError;
}
