#include "transmit/payload.h"
#include <sstream>

namespace {
void appendReadingJson(std::ostringstream &out, const Reading &r) {
    out << "{\"clientId\":\"" << r.clientId << "\","
        << "\"capturedAt\":\"" << r.capturedAt << "\","
        << "\"clockSynced\":" << (r.clockSynced ? "true" : "false") << ","
        << "\"windSpeedMs\":" << r.windSpeedMs << ","
        << "\"windDirOctant\":" << r.windDirOctant << ","
        << "\"rssiDbm\":" << (r.rssiValid ? std::to_string(r.rssiDbm) : "null")
        << "}";
}
} // namespace

std::string buildReadingsPayloadJson(const Reading *readings, size_t count) {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            out << ",";
        }
        appendReadingJson(out, readings[i]);
    }
    out << "]";
    return out.str();
}
