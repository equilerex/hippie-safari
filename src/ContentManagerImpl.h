#pragma once

#include <vector>
#include <SD.h>
#include "../include/ContentManager.h"

class ContentManagerImpl : public ContentManager {
private:
  std::vector<TypeContent> cachedTypes;
  ContentState state;
  char lastError[256] = {0};

  // Helper: scan single type folder
  bool scanTypeFolder(uint8_t typeIndex, File& typeFolder, TypeContent& outContent);

  // Helper: collect .wav files from folder
  bool collectVariants(File& folder, std::vector<std::string>& outVariants);

  // Helper: load mode config for type if exists
  bool loadModeConfig(uint8_t typeIndex, const char* folderPath, TypeContent& outContent);

public:
  ContentManagerImpl() = default;
  ~ContentManagerImpl() override = default;

  bool initialize() override;
  bool discoverContent() override;
  uint8_t getTypeCount() const override;
  const TypeContent* getType(uint8_t typeIndex) const override;
  const std::vector<TypeContent>& getAllTypes() const override;
  const char* getVariantPath(uint8_t typeIndex, uint8_t variantIndex) const override;
  uint8_t getVariantCount(uint8_t typeIndex) const override;
  bool typeHasContent(uint8_t typeIndex) const override;
  const ContentState& getContentState() const override;
  bool retrySDMount() override;
  const char* getLastError() const override;
};

#endif // CONTENT_MANAGER_IMPL_H
