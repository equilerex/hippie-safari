#ifndef WEBSERVERMANAGERIMPL_H
#define WEBSERVERMANAGERIMPL_H

#include <WebServer.h>
#include "../include/WebServerManager.h"

class WebServerManagerImpl : public WebServerManager {
private:
  WebServer server{80};
  char lastError[128] = {0};

  void handleRoot();
  void handleLogList();
  void handleFile();
  void handleNotFound();

  // Streams a file from SD at PATH_LOGS_ROOT/<name> with the right content-type.
  void serveLogsFile(const char* name);

public:
  WebServerManagerImpl() = default;
  ~WebServerManagerImpl() override = default;

  bool initialize() override;
  void update() override;
  const char* getLastError() const override;
};

#endif // WEBSERVERMANAGERIMPL_H
