#ifndef SharedBuffer_h__
#define SharedBuffer_h__

#include "platform.h"

struct Buffer
{
  ~Buffer();

  fpResult AddData(_In_ void *pAppendData, const size_t dataSize, _Out_ size_t *pIndex);

  uint8_t *pData = nullptr;
  size_t capacity = 0;
  size_t size = 0;
};

#endif // SharedBuffer_h__
