#include "config/station_config.h"
#include <map>
#include <sstream>
#include <cctype>

namespace {
std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Parses "networkN.ssid" / "networkN.password" into (N, field). Returns
// false if key doesn't match that pattern.
bool parseNetworkKey(const std::string &key, int &index, std::string &field) {
    if (key.rfind("network", 0) != 0) {
        return false;
    }
    size_t i = 7; // length of "network"
    size_t digitsStart = i;
    while (i < key.size() && std::isdigit(static_cast<unsigned char>(key[i]))) {
        i++;
    }
    if (i == digitsStart || i >= key.size() || key[i] != '.') {
        return false;
    }
    index = std::stoi(key.substr(digitsStart, i - digitsStart));
    field = key.substr(i + 1);
    return true;
}
} // namespace

StationConfig parseStationConfig(const std::string &fileContents) {
    StationConfig config;
    std::map<int, WifiNetwork> networksByIndex;

    std::istringstream stream(fileContents);
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue;
        }

        size_t eq = trimmedLine.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = trim(trimmedLine.substr(0, eq));
        std::string value = trim(trimmedLine.substr(eq + 1));

        int networkIndex;
        std::string field;
        if (parseNetworkKey(key, networkIndex, field)) {
            if (field == "ssid") {
                networksByIndex[networkIndex].ssid = value;
            } else if (field == "password") {
                networksByIndex[networkIndex].password = value;
            }
        } else if (key == "backend.url") {
            config.backendBaseUrl = value;
        } else if (key == "backend.token") {
            config.ingestToken = value;
        }
    }

    for (const auto &entry : networksByIndex) {
        config.networks.push_back(entry.second);
    }
    return config;
}

int selectNetworkIndex(const std::vector<WifiNetwork> &configured, const std::vector<std::string> &availableSsids) {
    for (size_t i = 0; i < configured.size(); i++) {
        for (const std::string &ssid : availableSsids) {
            if (configured[i].ssid == ssid) {
                return static_cast<int>(i);
            }
        }
    }
    return -1;
}
