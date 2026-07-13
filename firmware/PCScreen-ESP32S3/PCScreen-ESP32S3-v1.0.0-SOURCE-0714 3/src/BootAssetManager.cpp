#include "BootAssetManager.h"

#include <AnimatedGIF.h>
#include <LittleFS.h>

namespace {
AnimatedGIF gif;
File gifFile;
DisplayDevice* gifDisplay = nullptr;
int gifOffsetX = 0;
int gifOffsetY = 0;

bool readImageSize(const String& path, int32_t& width, int32_t& height) {
  File file = LittleFS.open(path, "r");
  if (!file) return false;
  uint8_t header[24] = {};
  const size_t count = file.read(header, sizeof(header));
  if (count >= 24 && header[0] == 0x89 && header[1] == 'P' &&
      header[2] == 'N' && header[3] == 'G') {
    width = (static_cast<uint32_t>(header[16]) << 24) |
            (static_cast<uint32_t>(header[17]) << 16) |
            (static_cast<uint32_t>(header[18]) << 8) | header[19];
    height = (static_cast<uint32_t>(header[20]) << 24) |
             (static_cast<uint32_t>(header[21]) << 16) |
             (static_cast<uint32_t>(header[22]) << 8) | header[23];
    file.close();
    return width > 0 && height > 0;
  }

  if (count >= 2 && header[0] == 0xFF && header[1] == 0xD8) {
    file.seek(2);
    while (file.available() >= 4) {
      if (file.read() != 0xFF) continue;
      int marker = file.read();
      while (marker == 0xFF) marker = file.read();
      if (marker == 0xD9 || marker == 0xDA) break;
      const int high = file.read();
      const int low = file.read();
      const int length = (high << 8) | low;
      if (length < 2) break;
      const bool isSof = marker >= 0xC0 && marker <= 0xCF &&
                         marker != 0xC4 && marker != 0xC8 && marker != 0xCC;
      if (isSof && length >= 7) {
        file.read();
        height = (file.read() << 8) | file.read();
        width = (file.read() << 8) | file.read();
        file.close();
        return width > 0 && height > 0;
      }
      file.seek(file.position() + length - 2);
    }
  }
  file.close();
  return false;
}

void* gifOpen(const char* filename, int32_t* size) {
  gifFile = LittleFS.open(filename, "r");
  if (!gifFile) return nullptr;
  *size = static_cast<int32_t>(gifFile.size());
  return &gifFile;
}

void gifClose(void* handle) {
  File* file = static_cast<File*>(handle);
  if (file) file->close();
}

int32_t gifRead(GIFFILE* file, uint8_t* buffer, int32_t length) {
  File* source = static_cast<File*>(file->fHandle);
  if (!source || length <= 0) return 0;
  const int32_t remaining = file->iSize - file->iPos;
  if (length > remaining) length = remaining;
  const int32_t read = static_cast<int32_t>(source->read(buffer, length));
  file->iPos = static_cast<int32_t>(source->position());
  return read;
}

int32_t gifSeek(GIFFILE* file, int32_t position) {
  File* source = static_cast<File*>(file->fHandle);
  if (!source || !source->seek(position)) return -1;
  file->iPos = static_cast<int32_t>(source->position());
  return file->iPos;
}

void gifDraw(GIFDRAW* draw) {
  if (!gifDisplay) return;
  int destinationX = draw->iX + gifOffsetX;
  const int sourceStart = max(0, -destinationX);
  destinationX = max(0, destinationX);
  int width = min(draw->iWidth - sourceStart, 240 - destinationX);
  const int y = draw->iY + draw->y + gifOffsetY;
  if (width <= 0 || y < 0 || y >= 240) return;

  uint8_t* source = draw->pPixels + sourceStart;
  uint16_t* palette = draw->pPalette;
  uint16_t line[240];
  if (draw->ucDisposalMethod == 2) {
    for (int x = 0; x < width; ++x) {
      if (source[x] == draw->ucTransparent) source[x] = draw->ucBackground;
    }
    draw->ucHasTransparency = 0;
  }

  if (!draw->ucHasTransparency) {
    for (int x = 0; x < width; ++x) line[x] = palette[source[x]];
    gifDisplay->pushImage(destinationX, y, width, 1, line);
    return;
  }

  int runStart = 0;
  while (runStart < width) {
    while (runStart < width && source[runStart] == draw->ucTransparent) ++runStart;
    if (runStart >= width) break;
    int runEnd = runStart;
    while (runEnd < width && source[runEnd] != draw->ucTransparent) {
      line[runEnd - runStart] = palette[source[runEnd]];
      ++runEnd;
    }
    gifDisplay->pushImage(destinationX + runStart, y,
                          runEnd - runStart, 1, line);
    runStart = runEnd;
  }
}
}  // namespace

BootAssetManager::BootAssetManager(DisplayDevice& display) : display_(display) {}

bool BootAssetManager::play(uint32_t maximumDurationMs) {
  if (!beginPlayback()) return false;
  const uint32_t started = millis();
  while (millis() - started < maximumDurationMs) {
    updatePlayback(millis());
    delay(1);
    yield();
  }
  endPlayback();
  return true;
}

bool BootAssetManager::beginPlayback() {
  endPlayback();
  const String path = currentPath();
  if (path.isEmpty()) return false;
  display_.fillScreen(TFT_BLACK);
  if (path.endsWith(".gif")) return openGif(path.c_str());
  const bool ok = drawStatic(path);
  playbackType_ = ok ? PlaybackType::StaticImage : PlaybackType::None;
  return ok;
}

bool BootAssetManager::drawStatic(const String& path) {
  bool ok = false;
  int32_t sourceWidth = 240;
  int32_t sourceHeight = 240;
  readImageSize(path, sourceWidth, sourceHeight);
  const float coverScale = max(240.0f / sourceWidth, 240.0f / sourceHeight);
  if (path.endsWith(".png")) {
    ok = display_.drawPngFile(LittleFS, path.c_str(), 0, 0, 240, 240, 0, 0,
                              coverScale, coverScale, middle_center);
  } else {
    ok = display_.drawJpgFile(LittleFS, path.c_str(), 0, 0, 240, 240, 0, 0,
                              coverScale, coverScale, middle_center);
  }
  return ok;
}

bool BootAssetManager::openGif(const char* path) {
  gifDisplay = &display_;
  gif.begin(BIG_ENDIAN_PIXELS);
  if (!gif.open(path, gifOpen, gifClose, gifRead, gifSeek, gifDraw)) {
    gifDisplay = nullptr;
    return false;
  }
  gifOffsetX = (240 - gif.getCanvasWidth()) / 2;
  gifOffsetY = (240 - gif.getCanvasHeight()) / 2;
  playbackType_ = PlaybackType::Gif;
  nextGifFrameAtMs_ = 0;
  updatePlayback(millis());
  return true;
}

void BootAssetManager::updatePlayback(uint32_t now) {
  if (playbackType_ != PlaybackType::Gif ||
      static_cast<int32_t>(now - nextGifFrameAtMs_) < 0) {
    return;
  }
  int frameDelay = 0;
  if (gif.playFrame(false, &frameDelay) < 0) {
    endPlayback();
    return;
  }
  nextGifFrameAtMs_ = now + static_cast<uint32_t>(max(frameDelay, 20));
}

void BootAssetManager::endPlayback() {
  if (playbackType_ == PlaybackType::Gif) gif.close();
  gifDisplay = nullptr;
  playbackType_ = PlaybackType::None;
  nextGifFrameAtMs_ = 0;
}

bool BootAssetManager::isPlaying() const {
  return playbackType_ != PlaybackType::None;
}

String BootAssetManager::currentPath() {
  if (LittleFS.exists("/boot.gif")) return "/boot.gif";
  if (LittleFS.exists("/boot.png")) return "/boot.png";
  if (LittleFS.exists("/boot.jpg")) return "/boot.jpg";
  return "";
}

String BootAssetManager::currentType() {
  const String path = currentPath();
  if (path.endsWith(".gif")) return "gif";
  if (path.endsWith(".png")) return "png";
  if (path.endsWith(".jpg")) return "jpg";
  return "none";
}

size_t BootAssetManager::currentSize() {
  const String path = currentPath();
  if (path.isEmpty()) return 0;
  File file = LittleFS.open(path, "r");
  const size_t size = file ? file.size() : 0;
  file.close();
  return size;
}

void BootAssetManager::removeAll() {
  LittleFS.remove("/boot.gif");
  LittleFS.remove("/boot.png");
  LittleFS.remove("/boot.jpg");
  LittleFS.remove("/boot.tmp");
}
