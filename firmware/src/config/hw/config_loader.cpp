#include "config/hw/config_loader.h"
#include <LittleFS.h>

StationConfig loadStationConfig() {
    LittleFS.begin(/*formatOnFail=*/true);

    File f = LittleFS.open("/config.txt", "r");
    if (!f) {
        return StationConfig{};
    }

    std::string contents;
    while (f.available()) {
        contents += static_cast<char>(f.read());
    }
    f.close();

    return parseStationConfig(contents);
}
