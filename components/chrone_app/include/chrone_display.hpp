#pragma once

#include "bsp/esp-bsp.h"

namespace chrone {

/** RAII wrapper for bsp_display_lock / unlock */
class DisplayLock {
public:
    explicit DisplayLock(uint32_t timeout_ms = 1000) : locked_(bsp_display_lock(timeout_ms)) {}
    ~DisplayLock()
    {
        if (locked_) {
            bsp_display_unlock();
        }
    }

    DisplayLock(const DisplayLock &) = delete;
    DisplayLock &operator=(const DisplayLock &) = delete;

    bool ok() const { return locked_; }

private:
    bool locked_;
};

}  // namespace chrone
