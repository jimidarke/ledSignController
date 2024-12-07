#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <Arduino.h>

void checkForUpdates(const char* currentVersion, const char* versionURL, const char* serverURL);

#endif // OTA_UPDATE_H
