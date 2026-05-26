#include "chrone_wifi.h"

#include "chrone_settings.hpp"

#include "chrone_time.h"
#include "weather_service.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ssid_manager.h>
#include <wifi_configuration_ap.h>
#include <wifi_station.h>

#include <cstring>
#include <string>

static const char *TAG = "chrone_wifi";

static chrone_wifi_state_t s_state = CHRONE_WIFI_STATE_IDLE;
static char s_ap_ssid[64];
static char s_ap_url[64];

namespace {

void enter_provisioning_mode(void)
{
    s_state = CHRONE_WIFI_STATE_PROVISIONING;

    auto &wifi_ap = WifiConfigurationAp::GetInstance();
    wifi_ap.SetSsidPrefix("ChroneCore");
    wifi_ap.Start();

    std::string ssid = wifi_ap.GetSsid();
    std::string url = wifi_ap.GetWebServerUrl();
    std::snprintf(s_ap_ssid, sizeof(s_ap_ssid), "%s", ssid.c_str());
    std::snprintf(s_ap_url, sizeof(s_ap_url), "%s", url.c_str());

    ESP_LOGI(TAG, "AP provisioning: SSID=%s URL=%s", s_ap_ssid, s_ap_url);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

bool start_station(void)
{
    s_state = CHRONE_WIFI_STATE_CONNECTING;

    auto &wifi_station = WifiStation::GetInstance();
    wifi_station.Start();

    if (!wifi_station.WaitForConnected(60 * 1000)) {
        wifi_station.Stop();
        ESP_LOGW(TAG, "STA connect timeout");
        return false;
    }

    s_state = CHRONE_WIFI_STATE_CONNECTED;
    ESP_LOGI(TAG, "WiFi connected: %s IP=%s", wifi_station.GetSsid().c_str(),
             wifi_station.GetIpAddress().c_str());
    return true;
}

/** FreeRTOS task must not return; keep WiFi worker alive after STA connect. */
static void wifi_task_idle_forever(void)
{
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

}  // namespace

extern "C" esp_err_t chrone_wifi_start(void)
{
    esp_netif_init();
    esp_event_loop_create_default();

    chrone::wifi::Settings settings("wifi", true);
    const bool force_ap = settings.GetInt("force_ap", 0) == 1;
    if (force_ap) {
        ESP_LOGI(TAG, "force_ap set, entering provisioning");
        settings.SetInt("force_ap", 0);
        enter_provisioning_mode();
        return ESP_OK;
    }

    auto &ssid_manager = SsidManager::GetInstance();
    if (ssid_manager.GetSsidList().empty()) {
        ESP_LOGI(TAG, "no saved SSID, entering provisioning");
        enter_provisioning_mode();
        return ESP_OK;
    }

    if (!start_station()) {
        enter_provisioning_mode();
        return ESP_OK;
    }

    chrone_time_sync_sntp_start();
    weather_service_start_query();
    wifi_task_idle_forever();
    return ESP_OK;  /* unreachable */
}

extern "C" chrone_wifi_state_t chrone_wifi_get_state(void)
{
    return s_state;
}

extern "C" bool chrone_wifi_is_connected(void)
{
    return s_state == CHRONE_WIFI_STATE_CONNECTED;
}

extern "C" esp_err_t chrone_wifi_get_provisioning_hint(char *ssid_buf, size_t ssid_sz, char *url_buf,
                                                       size_t url_sz)
{
    if (!ssid_buf || !url_buf || ssid_sz == 0 || url_sz == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    std::snprintf(ssid_buf, ssid_sz, "%s", s_ap_ssid);
    std::snprintf(url_buf, url_sz, "%s", s_ap_url);
    return ESP_OK;
}

extern "C" void chrone_wifi_reset_configuration(void)
{
    chrone::wifi::Settings settings("wifi", true);
    settings.SetInt("force_ap", 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}
