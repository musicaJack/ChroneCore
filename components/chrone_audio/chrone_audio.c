#include "chrone_audio.h"

#include "bsp/m5stack_core_2.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const char *TAG = "chrone_audio";

#define SAMPLE_RATE_HZ     44100
#define CHUNK_SAMPLES      512
#define ALARM_VOLUME       68.0f

/* 由 CMake EMBED_FILES 链接进 Flash .rodata，不占 PSRAM */
extern const uint8_t alarm_6s_pcm_start[] asm("_binary_alarm_6s_pcm_start");
extern const uint8_t alarm_6s_pcm_end[] asm("_binary_alarm_6s_pcm_end");

static esp_codec_dev_handle_t s_spk;
static TaskHandle_t s_play_task;
static volatile bool s_playing;
static bool s_audio_ready;

static size_t alarm_pcm_num_samples(void)
{
    const size_t bytes = (size_t)(alarm_6s_pcm_end - alarm_6s_pcm_start);
    return bytes / sizeof(int16_t);
}

static void fill_chunk_from_pcm(int16_t *out, size_t n, size_t *sample_pos)
{
    const int16_t *pcm = (const int16_t *)alarm_6s_pcm_start;
    const size_t total = alarm_pcm_num_samples();
    if (total == 0) {
        memset(out, 0, n * sizeof(int16_t));
        return;
    }

    for (size_t i = 0; i < n; i++) {
        out[i] = pcm[*sample_pos];
        *sample_pos = (*sample_pos + 1) % total;
    }
}

static void alarm_play_task(void *arg)
{
    (void)arg;

    if (!s_spk) {
        ESP_LOGE(TAG, "speaker codec not ready");
        s_playing = false;
        vTaskDelete(NULL);
        return;
    }

    const size_t num_samples = alarm_pcm_num_samples();
    if (num_samples == 0) {
        ESP_LOGE(TAG, "embedded alarm PCM is empty");
        s_playing = false;
        vTaskDelete(NULL);
        return;
    }

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = 1,
        .channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0),
        .sample_rate = SAMPLE_RATE_HZ,
    };

    if (esp_codec_dev_open(s_spk, &fs) != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "codec open failed");
        s_playing = false;
        vTaskDelete(NULL);
        return;
    }

    (void)esp_codec_dev_set_out_vol(s_spk, ALARM_VOLUME);

    int16_t buf[CHUNK_SAMPLES];
    size_t sample_pos = 0;
    const uint32_t duration_ms = (uint32_t)((num_samples * 1000U) / SAMPLE_RATE_HZ);

    ESP_LOGI(TAG, "alarm PCM playing (%u samples, %lu ms loop, flash rodata)",
             (unsigned)num_samples, (unsigned long)duration_ms);

    while (s_playing) {
        fill_chunk_from_pcm(buf, CHUNK_SAMPLES, &sample_pos);
        const int ret = esp_codec_dev_write(s_spk, buf, sizeof(buf));
        if (ret < 0) {
            ESP_LOGW(TAG, "codec write: %d", ret);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    (void)esp_codec_dev_close(s_spk);
    s_play_task = NULL;
    ESP_LOGI(TAG, "alarm tone stopped");
    vTaskDelete(NULL);
}

esp_err_t chrone_audio_init(void)
{
    s_playing = false;
    s_play_task = NULL;
    s_audio_ready = false;
    s_spk = NULL;

    const size_t pcm_bytes = (size_t)(alarm_6s_pcm_end - alarm_6s_pcm_start);
    ESP_LOGI(TAG, "alarm clip embedded: %u bytes in flash", (unsigned)pcm_bytes);

    esp_err_t ret = bsp_audio_init(NULL);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "bsp_audio_init: %s", esp_err_to_name(ret));
        return ret;
    }

    s_spk = bsp_audio_codec_speaker_init();
    if (!s_spk) {
        ESP_LOGW(TAG, "speaker codec init failed");
        return ESP_FAIL;
    }

    s_audio_ready = true;
    ESP_LOGI(TAG, "speaker ready (I2S %u Hz mono)", SAMPLE_RATE_HZ);
    return ESP_OK;
}

void chrone_audio_alarm_start(void)
{
    if (!s_audio_ready || !s_spk) {
        ESP_LOGW(TAG, "alarm start ignored (audio not ready)");
        return;
    }
    if (s_playing) {
        return;
    }

    s_playing = true;
    if (xTaskCreate(alarm_play_task, "chrone_alarm_audio", 4096, NULL, 5, &s_play_task) != pdPASS) {
        s_playing = false;
        s_play_task = NULL;
        ESP_LOGE(TAG, "alarm audio task create failed");
    }
}

void chrone_audio_alarm_stop(void)
{
    if (!s_playing) {
        return;
    }
    s_playing = false;

    for (int i = 0; i < 50 && s_play_task != NULL; ++i) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
