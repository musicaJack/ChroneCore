#include "chrone_settings.hpp"

namespace chrone::wifi {

Settings::Settings(const char *ns, bool read_write)
    : ns_(ns), read_write_(read_write)
{
    nvs_open(ns, read_write ? NVS_READWRITE : NVS_READONLY, &nvs_handle_);
}

Settings::~Settings()
{
    if (nvs_handle_) {
        if (read_write_ && dirty_) {
            nvs_commit(nvs_handle_);
        }
        nvs_close(nvs_handle_);
    }
}

std::string Settings::GetString(const char *key, const std::string &default_value)
{
    if (!nvs_handle_) {
        return default_value;
    }
    size_t len = 0;
    if (nvs_get_str(nvs_handle_, key, nullptr, &len) != ESP_OK) {
        return default_value;
    }
    std::string value(len, '\0');
    if (nvs_get_str(nvs_handle_, key, value.data(), &len) != ESP_OK) {
        return default_value;
    }
    while (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    return value;
}

void Settings::SetString(const char *key, const std::string &value)
{
    if (read_write_ && nvs_handle_) {
        nvs_set_str(nvs_handle_, key, value.c_str());
        dirty_ = true;
    }
}

int32_t Settings::GetInt(const char *key, int32_t default_value)
{
    if (!nvs_handle_) {
        return default_value;
    }
    int32_t v = default_value;
    if (nvs_get_i32(nvs_handle_, key, &v) != ESP_OK) {
        return default_value;
    }
    return v;
}

void Settings::SetInt(const char *key, int32_t value)
{
    if (read_write_ && nvs_handle_) {
        nvs_set_i32(nvs_handle_, key, value);
        dirty_ = true;
    }
}

}  // namespace chrone::wifi
