/*
 * PCScreen AppleSMC read-only helper for Intel macOS.
 * Copyright (c) 2026 Nguyen Tien Thanh (GitHub: TienThanhDEV).
 * SPDX-License-Identifier: MIT
 *
 * This program accepts no arguments, opens AppleSMC read-only, permanently
 * drops elevated credentials, reads a bounded list of sensor keys and exits.
 */
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <libkern/OSByteOrder.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum {
  kSmcUserClientMethod = 2,
  kSmcReadBytes = 5,
  kSmcReadKeyInfo = 9,
};

typedef struct {
  uint8_t major;
  uint8_t minor;
  uint8_t build;
  uint8_t reserved;
  uint16_t release;
} SmcVersion;

typedef struct {
  uint16_t version;
  uint16_t length;
  uint32_t cpuPLimit;
  uint32_t gpuPLimit;
  uint32_t memPLimit;
} SmcPLimitData;

typedef struct {
  uint32_t dataSize;
  uint32_t dataType;
  uint8_t dataAttributes;
} SmcKeyInfoData;

typedef struct {
  uint32_t key;
  SmcVersion version;
  SmcPLimitData pLimitData;
  SmcKeyInfoData keyInfo;
  uint8_t result;
  uint8_t status;
  uint8_t data8;
  uint32_t data32;
  uint8_t bytes[32];
} SmcKeyData;

typedef struct {
  uint32_t dataType;
  uint32_t dataSize;
  uint8_t bytes[32];
} SmcValue;

static uint32_t key_code(const char key[5]) {
  return ((uint32_t)(uint8_t)key[0] << 24) |
         ((uint32_t)(uint8_t)key[1] << 16) |
         ((uint32_t)(uint8_t)key[2] << 8) |
         (uint32_t)(uint8_t)key[3];
}

static kern_return_t smc_call(io_connect_t connection, SmcKeyData *input,
                              SmcKeyData *output) {
  size_t outputSize = sizeof(*output);
  memset(output, 0, sizeof(*output));
  return IOConnectCallStructMethod(connection, kSmcUserClientMethod, input,
                                   sizeof(*input), output, &outputSize);
}

static int read_key(io_connect_t connection, const char key[5], SmcValue *value) {
  SmcKeyData input;
  SmcKeyData output;
  SmcKeyInfoData keyInfo;
  memset(&input, 0, sizeof(input));
  input.key = key_code(key);
  input.data8 = kSmcReadKeyInfo;
  if (smc_call(connection, &input, &output) != KERN_SUCCESS || output.result != 0)
    return 0;

  keyInfo = output.keyInfo;
  input.keyInfo = keyInfo;
  input.data8 = kSmcReadBytes;
  if (smc_call(connection, &input, &output) != KERN_SUCCESS || output.result != 0)
    return 0;

  // Lệnh READ_BYTES không đảm bảo trả lại keyInfo; dùng metadata đã lấy từ
  // READ_KEYINFO ở trên để giải mã đúng sp78/fpe2/ui16.
  value->dataType = keyInfo.dataType;
  value->dataSize = keyInfo.dataSize;
  memcpy(value->bytes, output.bytes, sizeof(value->bytes));
  return value->dataSize > 0 && value->dataSize <= sizeof(value->bytes);
}

static double decode_value(const SmcValue *value) {
  const uint32_t sp78 = key_code("sp78");
  const uint32_t fpe2 = key_code("fpe2");
  const uint32_t flt = key_code("flt ");
  const uint32_t ui8 = key_code("ui8 ");
  const uint32_t ui16 = key_code("ui16");

  if (value->dataType == sp78 && value->dataSize >= 2)
    return (double)(int8_t)value->bytes[0] + value->bytes[1] / 256.0;
  if (value->dataType == fpe2 && value->dataSize >= 2) {
    const uint16_t raw = ((uint16_t)value->bytes[0] << 8) | value->bytes[1];
    return raw / 4.0;
  }
  if (value->dataType == flt && value->dataSize >= 4) {
    uint32_t raw;
    float decoded;
    memcpy(&raw, value->bytes, sizeof(raw));
    raw = OSSwapBigToHostInt32(raw);
    memcpy(&decoded, &raw, sizeof(decoded));
    return decoded;
  }
  if (value->dataType == ui8 && value->dataSize >= 1) return value->bytes[0];
  if (value->dataType == ui16 && value->dataSize >= 2)
    return ((uint16_t)value->bytes[0] << 8) | value->bytes[1];
  return NAN;
}

static double first_valid(io_connect_t connection, const char *const keys[],
                          size_t count, double minimum, double maximum) {
  for (size_t index = 0; index < count; ++index) {
    SmcValue value;
    if (!read_key(connection, keys[index], &value)) continue;
    const double decoded = decode_value(&value);
    if (isfinite(decoded) && decoded >= minimum && decoded <= maximum)
      return decoded;
  }
  return NAN;
}

int main(void) {
  io_service_t service = IOServiceGetMatchingService(
      MACH_PORT_NULL, IOServiceMatching("AppleSMC"));
  if (service == IO_OBJECT_NULL) {
    fprintf(stderr, "AppleSMC service not found\n");
    return 2;
  }

  io_connect_t connection = IO_OBJECT_NULL;
  const kern_return_t openResult =
      IOServiceOpen(service, mach_task_self(), 0, &connection);
  IOObjectRelease(service);
  if (openResult != KERN_SUCCESS) {
    fprintf(stderr, "AppleSMC user client unavailable: 0x%x\n", openResult);
    return 3;
  }

  // Chỉ cần đặc quyền lúc mở AppleSMC. Hạ quyền vĩnh viễn trước khi đọc dữ liệu.
  if (geteuid() == 0 && getuid() != 0) {
    if (setgid(getgid()) != 0 || setuid(getuid()) != 0) {
      fprintf(stderr, "Unable to drop elevated credentials\n");
      IOServiceClose(connection);
      return 5;
    }
  }

  const char *cpuKeys[] = {"TC0D", "TC0P", "TC0H", "TC0E", "TC1C"};
  const char *gpuKeys[] = {"TG0D", "TG0P", "TG0H", "TG1D", "TG1P"};
  const double cpuTemp = first_valid(connection, cpuKeys,
                                     sizeof(cpuKeys) / sizeof(cpuKeys[0]), 5, 125);
  const double gpuTemp = first_valid(connection, gpuKeys,
                                     sizeof(gpuKeys) / sizeof(gpuKeys[0]), 5, 125);

  double fans[8];
  size_t fanCount = 0;
  for (unsigned index = 0; index < 8; ++index) {
    char key[5];
    snprintf(key, sizeof(key), "F%uAc", index);
    SmcValue value;
    if (!read_key(connection, key, &value)) continue;
    const double rpm = decode_value(&value);
    if (isfinite(rpm) && rpm >= 0 && rpm <= 20000) fans[fanCount++] = rpm;
  }
  IOServiceClose(connection);

  printf("{");
  int comma = 0;
  if (isfinite(cpuTemp)) {
    printf("\"cpuTemp\":%.2f", cpuTemp);
    comma = 1;
  }
  if (isfinite(gpuTemp)) {
    printf("%s\"gpuTemp\":%.2f", comma ? "," : "", gpuTemp);
    comma = 1;
  }
  printf("%s\"fans\":[", comma ? "," : "");
  for (size_t index = 0; index < fanCount; ++index)
    printf("%s%.0f", index ? "," : "", fans[index]);
  printf("]}\n");
  return (isfinite(cpuTemp) || isfinite(gpuTemp) || fanCount > 0) ? 0 : 4;
}
