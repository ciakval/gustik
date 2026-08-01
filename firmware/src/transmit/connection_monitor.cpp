#include "transmit/connection_monitor.h"

void ConnectionMonitor::recordSendSuccess() {
    justRecovered_ = !healthy_;
    healthy_ = true;
}

void ConnectionMonitor::recordSendFailure() {
    healthy_ = false;
    justRecovered_ = false;
}

bool ConnectionMonitor::isHealthy() const {
    return healthy_;
}

bool ConnectionMonitor::justRecovered() const {
    return justRecovered_;
}
