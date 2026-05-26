# 七段数码管字体（DSEG7）

数字钟主时间行使用 **DSEG7 Classic Regular**（[keshikan/DSEG](https://github.com/keshikan/DSEG)，[SIL OFL 1.1](https://scripts.sil.org/OFL)），经 `lv_font_conv` 转为 LVGL 位图字体 `chrone_font_dseg56.c`。

## 重新生成

在 `components/chrone_ui/fonts/` 目录：

```bash
# 从官方 release 解压得到 DSEG7Classic-Regular.ttf，或复制：
# dseg_zip/fonts-DSEG_v046/DSEG7-Classic/DSEG7Classic-Regular.ttf

npx lv_font_conv \
  --font DSEG7Classic-Regular.ttf \
  --size 56 --bpp 4 \
  -r 0x30-0x3A \
  --format lvgl -o chrone_font_dseg56.c \
  --lv-include lvgl.h --no-compress
```

仅包含 `0-9` 与 `:`，约 60 KiB Flash。修改字号后请同步 `clock_layout.h` 中的 `CHRONE_TIME_FONT_LINE_H`。

## DS-Digital 等其它字体

可用同样流程：将 TTF 放入本目录，调整 `--size` / `-r` 范围后生成新 `.c`，在 `CMakeLists.txt` 中注册并在 `digital_clock.cpp` 里 `LV_FONT_DECLARE` 后绑定到时间 `lv_label`。

**注意**：DS-Digital 为商业字体，发布固件前请自行确认授权；DSEG 为开源 OFL，适合随仓库分发。

## 不纳入版本库的文件

`fonts-DSEG_v046.zip`、`dseg_zip/` 为下载/解压临时目录，已在根 `.gitignore` 中忽略。
