#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

class WebServerManager {
public:
  virtual ~WebServerManager() = default;

  // Start softAP + HTTP server. Non-fatal if it fails; caller continues without it.
  virtual bool initialize() = 0;

  // Poll for incoming HTTP requests. Call every loop() iteration.
  virtual void update() = 0;

  virtual const char* getLastError() const = 0;
};

#endif // WEBSERVERMANAGER_H
