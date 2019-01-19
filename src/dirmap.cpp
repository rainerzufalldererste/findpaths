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
  constexpr int8_t touched = 0b10000;

  // constexpr int8_t left = 1;
  // constexpr int8_t right = 2;
  // constexpr int8_t up = 3;
  // constexpr int8_t down = 6;
  // constexpr int8_t constantSubtract = 1;

  constexpr int8_t leftFlag = (1 - 1) | touched; // 0
  constexpr int8_t rightFlag = (2 - 1) | touched; // 1
  constexpr int8_t upFlag = (3 - 1) | touched; // 2
  constexpr int8_t upLeftFlag = (3 + 1 - 1) | touched; // 3
  constexpr int8_t upRightFlag = (3 + 2 - 1) | touched; // 4
  constexpr int8_t downFlag = (6 - 1) | touched; // 5
  constexpr int8_t downLeftFlag = (6 + 1 - 1) | touched; // 6
  constexpr int8_t downRightFlag = (6 + 2 - 1) | touched; // 7

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

    assert(p.x < worldSizeX && p.y < worldSizeY && p.x >= 0 && p.y >= 0);

    int8_t *pData = (int8_t *)&pDirectionBuffer[p.x + p.y * worldSizeX];

    const bool leftBounds = p.x > 0;
    const bool rightBounds = p.x + 1 < worldSizeX;
    const bool upBounds = p.y > 0;
    const bool downBounds = p.y + 1 < worldSizeY;

    if (rightBounds)
    {
      if (*(pData + 1) == 0)
      {
        *(pData + 1) = rightFlag;
        (queue.PushBack(pos(p.x + 1, p.y)));
      }
    }

    if (leftBounds)
    {
      if (*(pData - 1) == 0)
      {
        *(pData - 1) = leftFlag;
        (queue.PushBack(pos(p.x - 1, p.y)));
      }
    }

    if (rightBounds)
    {
      if (upBounds && *(pData + 1 - worldSizeX) == 0)
      {
        *(pData + 1 - worldSizeX) = upRightFlag;
        (queue.PushBack(pos(p.x + 1, p.y - 1)));
      }

      if (downBounds && *(pData + 1 + worldSizeX) == 0)
      {
        *(pData + 1 + worldSizeX) = downRightFlag;
        (queue.PushBack(pos(p.x + 1, p.y + 1)));
      }
    }

    if (leftBounds)
    {
      if (upBounds && *(pData - 1 - worldSizeX) == 0)
      {
        *(pData - 1 - worldSizeX) = upLeftFlag;
        (queue.PushBack(pos(p.x - 1, p.y - 1)));
      }

      if (downBounds && *(pData - 1 + worldSizeX) == 0)
      {
        *(pData - 1 + worldSizeX) = downLeftFlag;
        (queue.PushBack(pos(p.x - 1, p.y + 1)));
      }
    }

    if (upBounds && *(pData - worldSizeX) == 0)
    {
      *(pData - worldSizeX) = upFlag;
      (queue.PushBack(pos(p.x, p.y - 1)));
    }

    if (downBounds && *(pData + worldSizeX) == 0)
    {
      *(pData + worldSizeX) = downFlag;
      (queue.PushBack(pos(p.x, p.y + 1)));
    }
  }

  return result;
}
