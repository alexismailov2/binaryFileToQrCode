#pragma once

struct Header
{
  uint32_t chunkId;
  uint32_t chunksCount;
  uint16_t payloadSize;
  uint16_t reserved;
};
