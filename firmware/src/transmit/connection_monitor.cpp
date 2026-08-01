#include "transmit/connection_monitor.h"

void ConnectionMonitor::recordSendSuccess() {
    healthy_ = true;
}

void ConnectionMonitor::recordSendFailure() {
    healthy_ = false;
}

bool ConnectionMonitor::isHealthy() const {
    return healthy_;
}
