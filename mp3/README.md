# 闹钟铃声素材（MetroGnome）

源文件：`MetroGnome.mp3`（44.1 kHz 立体声 320 kbps，约 3:18）。

## ESP32 / ChroneCore 播放要求

`chrone_audio` 通过 **I2S + `esp_codec_dev_write`** 直接写 **16-bit 有符号 PCM**，无 MP3 解码器：

| 参数 | 值 |
|------|-----|
| 采样率 | **44100 Hz**（与 `SAMPLE_RATE_HZ`、Codec 一致） |
| 声道 | **单声道**（`-ac 1`，立体声混为 mono） |
| 格式 | **s16le**（`int16_t`，小端） |
| 容器 | 烧录用 **`alarm_6s.pcm`**（裸 PCM，无 WAV 头）；试听可用 `alarm_6s.wav` |

6 秒裸 PCM 大小：`44100 × 6 × 2 ≈ 517 KB`。当前固件通过 **`EMBED_FILES`** 链入 **Flash `.rodata`**（见 `components/chrone_audio/alarm_6s.pcm`），播放时按 512 样本块拷到栈上再送 I2S，**不占用 PSRAM**。若 app 分区不够，需增大 `partitions.csv` 里 `factory` 或使用更短/更低采样率素材。

## 截取与优化（ffmpeg）

在项目根目录或本目录执行：

```bash
ffmpeg -y -ss 0 -i MetroGnome.mp3 -t 6 \
  -ac 1 -ar 44100 \
  -af "highpass=f=100,lowpass=f=12000,acompressor=threshold=-20dB:ratio=3:attack=5:release=80,afade=t=in:st=0:d=0.08,afade=t=out:st=5.72:d=0.28,loudnorm=I=-18:TP=-2:LRA=7" \
  -c:a pcm_s16le alarm_6s.wav

ffmpeg -y -ss 0 -i MetroGnome.mp3 -t 6 \
  -ac 1 -ar 44100 \
  -af "highpass=f=100,lowpass=f=12000,acompressor=threshold=-20dB:ratio=3:attack=5:release=80,afade=t=in:st=0:d=0.08,afade=t=out:st=5.72:d=0.28,loudnorm=I=-18:TP=-2:LRA=7" \
  -f s16le -c:a pcm_s16le alarm_6s.pcm
```

滤镜说明：

- **`-t 6`**：只保留前 6 秒（可从 `-ss` 调整起点）。
- **`highpass=100`**：去掉超低频，小喇叭更干净。
- **`lowpass=12000`**：略削高频，减轻刺耳与混叠。
- **`acompressor`**：动态压缩，响铃在嘈杂环境更易听见且不削波。
- **`afade` 入/出**：避免起止爆音；循环播放时尾部淡出更顺。
- **`loudnorm`**：统一响度（约 -18 LUFS），避免过大失真。

若需更短/更省 Flash，可把 `-ar 44100` 改为 `22050` 或 `16000`，但须同步修改 `chrone_audio.c` 里的 `SAMPLE_RATE_HZ` 并在打开 Codec 时使用同一采样率。

## 生成 C 头文件（可选，用于嵌入固件）

```bash
python ../tools/pcm_to_header.py alarm_6s.pcm ../components/chrone_audio/alarm_tone_pcm.h chrone_alarm_tone_pcm
```

## 产物

- `alarm_6s.wav` — 本地试听
- `alarm_6s.pcm` — 供固件嵌入或后续转换
