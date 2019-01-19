#include "dirmap.h"
#include "CyclicQueue.h"

uint8_t * memflag_u8(_In_ uint8_t *pData, const uint8_t flag, _Out_ uint8_t *pEnd)
{
  while (pData < pEnd)
  {
    if (*pData & flag)
      return pData;

    pData++;
  }

  return pEnd;
}

fpResult dirmap(_Out_ uint8_t *pDirectionBuffer, _In_ uint8_t *pObstacleBuffer, const size_t worldSizeX, const size_t worldSizeY, const size_t startPositionX, const size_t startPositionY)
{
  struct pos 
  {
    int16_t x, y;
    
    pos()
    { }

    pos(int16_t x, int16_t y) :
      x(x),
      y(y)
    { }
  };

  static CyclicQueue<pos> queue;
  queue.Clear();

  const fpResult result = fpR_Success;

  constexpr uint8_t obstacleFlag = 0b01000000;
  constexpr int8_t touched = 0b1000;
  constexpr int8_t left = 1;
  constexpr int8_t right = 2;
  constexpr int8_t up = 3;
  constexpr int8_t down = 6;
  constexpr int8_t constantSubtract = 1;

  // Put obstacles into the direction buffer.
  {
    const __m128i mask = _mm_set_epi8((char)(uint8_t)0b10000000, 0b1000000, 0b100000, 0b10000, 0b1000, 0b100, 0b10, 0b1, (char)(uint8_t)0b10000000, 0b1000000, 0b100000, 0b10000, 0b1000, 0b100, 0b10, 0b1);
    const __m128i setObstacleFlag = _mm_set1_epi8(obstacleFlag);

    __m128i *pDirections128 = (__m128i *)pDirectionBuffer;

    for (size_t i = 0; i < worldSizeX * worldSizeY; i += 16)
    {
      const __m128i obstacles = _mm_loadu_si128((__m128i *)pObstacleBuffer);
      const __m128i expandedTo64 = _mm_cvtepu8_epi64(obstacles);
      const __m128i spreadTo32 = _mm_or_si128(expandedTo64, _mm_slli_si128(expandedTo64, 4));
      const __m128i spreadTo16 = _mm_or_si128(spreadTo32, _mm_slli_si128(spreadTo32, 2));
      const __m128i spreadTo8 = _mm_or_si128(spreadTo16, _mm_slli_si128(spreadTo16, 1));

      const __m128i value = _mm_and_si128(setObstacleFlag, _mm_cmpeq_epi8(mask, _mm_and_si128(mask, spreadTo8)));

      _mm_storeu_si128(pDirections128, value);

      pDirections128++;
      pObstacleBuffer += 2;
    }
  }

  // Put startPosition into the direction buffer.
  {
    (queue.PushBack({(int16_t)startPositionX, (int16_t)startPositionY}));
  }

  while (queue.Any())
  {
    pos p;
    (queue.PopFront(&p));

    int8_t *pData = (int8_t *)&pDirectionBuffer[p.x + p.y * worldSizeX];

    const bool leftBounds = p.x > 0;
    const bool rightBounds = p.x + 1 < worldSizeX;
    const bool upBounds = p.y > 0;
    const bool downBounds = p.y + 1 < worldSizeY;

    if (leftBounds)
    {
      if (*(pData - 1) == 0)
      {
        *(pData - 1) = ((left) - constantSubtract) | touched;
        (queue.PushBack(pos(p.x - 1, p.y)));
      }

      if (upBounds && *(pData - 1 - worldSizeX) == 0)
      {
        *(pData - 1 - worldSizeX) = ((left + up) - constantSubtract) | touched;
        (queue.PushBack(pos(p.x - 1, p.y - 1)));
      }

      if (downBounds && *(pData - 1 + worldSizeX) == 0)
      {
        *(pData - 1 + worldSizeX) = ((left + down) - constantSubtract) | touched;
        (queue.PushBack(pos(p.x - 1, p.y + 1)));
      }
    }

    if (upBounds && *(pData - worldSizeX) == 0)
    {
      *(pData - worldSizeX) = ((up) - constantSubtract) | touched;
      (queue.PushBack(pos(p.x, p.y - 1)));
    }

    if (downBounds && *(pData + worldSizeX) == 0)
    {
      *(pData + worldSizeX) = ((down) - constantSubtract) | touched;
      (queue.PushBack(pos(p.x, p.y + 1)));
    }

    if (rightBounds)
    {
      if (*(pData + 1) == 0)
      {
        *(pData + 1) = ((right) - constantSubtract) | touched;
        (queue.PushBack(pos(p.x + 1, p.y)));
      }

      if (upBounds && *(pData + 1 - worldSizeX) == 0)
      {
        *(pData - 1 - worldSizeX) = ((right + up) - constantSubtract) | touched;
        (queue.PushBack(pos(p.x + 1, p.y - 1)));
      }

      if (downBounds && *(pData + 1 + worldSizeX) == 0)
      {
        *(pData - 1 + worldSizeX) = ((right + down) - constantSubtract) | touched;
        (queue.PushBack(pos(p.x + 1, p.y + 1)));
      }
    }
  }

  return result;
}
