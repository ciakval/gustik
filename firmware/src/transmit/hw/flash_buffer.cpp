#include "transmit/hw/flash_buffer.h"
#include <LittleFS.h>

namespace {
constexpr char kBufferDir[] = "/buf";
} // namespace

void FlashBuffer::begin(size_t capacity) {
    LittleFS.begin(/*formatOnFail=*/true);
    LittleFS.mkdir(kBufferDir);
    index_.reset(capacity);
}

String FlashBuffer::slotPath(size_t slot) const {
    return String(kBufferDir) + "/" + String(slot) + ".txt";
}

void FlashBuffer::push(const Reading &reading) {
    size_t slot = index_.push();
    File f = LittleFS.open(slotPath(slot), "w");
    if (!f) {
        return; // best-effort (FR-4) - a failed local write is still not
                // allowed to crash/block the sampling loop
    }
    f.printf(
        "%s|%s|%d|%f|%d|%d|%d\n",
        reading.clientId.c_str(),
        reading.capturedAt.c_str(),
        reading.clockSynced ? 1 : 0,
        reading.windSpeedMs,
        reading.windDirOctant,
        reading.rssiValid ? 1 : 0,
        reading.rssiDbm);
    f.close();
}

std::vector<Reading> FlashBuffer::peekAll() {
    std::vector<Reading> result;
    for (size_t slot : index_.oldestToNewestSlots()) {
        File f = LittleFS.open(slotPath(slot), "r");
        if (!f) {
            continue; // best-effort - skip a slot that failed to read back
        }
        String line = f.readStringUntil('\n');
        f.close();

        Reading r;
        int fieldStart = 0;
        int fieldIndex = 0;
        String fields[7];
        for (int i = 0; i <= line.length() && fieldIndex < 7; i++) {
            if (i == line.length() || line[i] == '|') {
                fields[fieldIndex++] = line.substring(fieldStart, i);
                fieldStart = i + 1;
            }
        }
        r.clientId = fields[0].c_str();
        r.capturedAt = fields[1].c_str();
        r.clockSynced = fields[2].toInt() != 0;
        r.windSpeedMs = fields[3].toDouble();
        r.windDirOctant = fields[4].toInt();
        r.rssiValid = fields[5].toInt() != 0;
        r.rssiDbm = fields[6].toInt();
        result.push_back(r);
    }
    return result;
}

void FlashBuffer::clear() {
    index_.drain();
}
