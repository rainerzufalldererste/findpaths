#include "Buffer.h"

Buffer::~Buffer()
{
  fpFreePtr(&pData);
}

fpResult Buffer::AddData(_In_ void *pAppendData, const size_t dataSize, _Out_ size_t *pIndex)
{
  fpResult result = fpR_Success;

  if (pData == nullptr || size + dataSize > capacity)
  {
    const size_t newCapacity = std::max(std::max(capacity * 2 + 1, capacity + dataSize), 1024ULL);

    pData = (uint8_t *)realloc(pData, newCapacity);
    FP_ERROR_IF(!pData, fpR_MemoryAllocationFailure);
    
    capacity = newCapacity;
  }

  *pIndex = size;
  memcpy(pData + size, pAppendData, dataSize);
  size += dataSize;

  goto epilogue;

epilogue:
  return result;
}
