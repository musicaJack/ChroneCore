#pragma once

#include <cstdint>
#include <string>

#include "nvs_flash.h"

namespace chrone::wifi {

class Settings {
public:
    explicit Settings(const char *ns, bool read_write = false);
    ~Settings();

    std::string GetString(const char *key, const std::string &default_value = "");
    void SetString(const char *key, const std::string &value);
    int32_t GetInt(const char *key, int32_t default_value = 0);
    void SetInt(const char *key, int32_t value);

private:
    std::string ns_;
    nvs_handle_t nvs_handle_{0};
    bool read_write_{false};
    bool dirty_{false};
};

}  // namespace chrone::wifi
