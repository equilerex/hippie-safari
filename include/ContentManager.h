#pragma once

#include <vector>
#include <map>
#include "Config.h"
#include "ContentTypes.h"

class ContentManager {
public:
  virtual ~ContentManager() = default;

  // Initialize SD card and perform initial content discovery
  virtual bool initialize() = 0;

  // Scan /audio/types/ for content folders and cache results
  virtual bool discoverContent() = 0;

  // Get list of all discovered types
  virtual uint8_t getTypeCount() const = 0;

  // Get TypeContent for a specific type index
  virtual const TypeContent* getType(uint8_t typeIndex) const = 0;

  // Get all types
  virtual const std::vector<TypeContent>& getAllTypes() const = 0;

  // Get variant file path for type + variant index
  virtual const char* getVariantPath(uint8_t typeIndex, uint8_t variantIndex) const = 0;

  // Get total variant count for a type
  virtual uint8_t getVariantCount(uint8_t typeIndex) const = 0;

  // Check if type has any variants
  virtual bool typeHasContent(uint8_t typeIndex) const = 0;

  // Check current SD state
  virtual const ContentState& getContentState() const = 0;

  // Retry SD mount (called periodically if SD unavailable)
  virtual bool retrySDMount() = 0;

  // Get last discovery error
  virtual const char* getLastError() const = 0;
};

#endif // CONTENT_MANAGER_H
