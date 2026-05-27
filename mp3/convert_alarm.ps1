# 从 MetroGnome.mp3 截取前 6 秒并生成 ESP32 用 PCM/WAV
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$af = "highpass=f=100,lowpass=f=12000,acompressor=threshold=-20dB:ratio=3:attack=5:release=80,afade=t=in:st=0:d=0.08,afade=t=out:st=5.72:d=0.28,loudnorm=I=-18:TP=-2:LRA=7"

ffmpeg -y -ss 0 -i MetroGnome.mp3 -t 6 -ac 1 -ar 44100 -af $af -c:a pcm_s16le alarm_6s.wav
ffmpeg -y -ss 0 -i MetroGnome.mp3 -t 6 -ac 1 -ar 44100 -af $af -f s16le -c:a pcm_s16le alarm_6s.pcm

Write-Host "OK: alarm_6s.wav (preview), alarm_6s.pcm (flash embed)"
Write-Host "Optional: python ..\tools\pcm_to_header.py alarm_6s.pcm ..\components\chrone_audio\alarm_tone_pcm.h chrone_alarm_tone_pcm"
