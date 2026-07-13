#pragma once

#include <Arduino.h>

#include "DisplayDevice.h"

class BootAssetManager final {
 public:
  explicit BootAssetManager(DisplayDevice& display);
  bool play(uint32_t maximumDurationMs);
  bool beginPlayback();
  void updatePlayback(uint32_t now);
  void endPlayback();
  bool isPlaying() const;
  static String currentPath();
  static String currentType();
  static size_t currentSize();
  static void removeAll();

 private:
  enum class PlaybackType : uint8_t { None, StaticImage, Gif };
  DisplayDevice& display_;
  PlaybackType playbackType_ = PlaybackType::None;
  uint32_t nextGifFrameAtMs_ = 0;
  bool drawStatic(const String& path);
  bool openGif(const char* path);
};
