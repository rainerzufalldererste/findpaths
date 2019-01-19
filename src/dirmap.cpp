#include "dirmap.h"

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

void dirmap(_Out_ uint8_t *pDirectionBuffer, _In_ uint8_t *pObstacleBuffer, const size_t worldSizeX, const size_t worldSizeY, const size_t startPositionX, const size_t startPositionY)
{
  constexpr uint8_t obstacleFlag = 0b10000000;
  constexpr uint8_t touchedFlag = 0b1000;
  constexpr uint8_t freshDataFlag = 0b10000;
  constexpr uint8_t leftFlag = 0b00 | touchedFlag;
  constexpr uint8_t rightFlag = 0b01 | touchedFlag;
  constexpr uint8_t upFlag = 0b10 | touchedFlag;
  constexpr uint8_t downFlag = 0b11 | touchedFlag;
  constexpr uint8_t newLeftFlag = 0b00 | touchedFlag | freshDataFlag;
  constexpr uint8_t newRightFlag = 0b01 | touchedFlag | freshDataFlag;
  constexpr uint8_t newUpFlag = 0b10 | touchedFlag | freshDataFlag;
  constexpr uint8_t newDownFlag = 0b11 | touchedFlag | freshDataFlag;

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
    pDirectionBuffer[startPositionY * worldSizeX + startPositionX] |= touchedFlag;
  }

  // Expand and Store direction.
  {
    uint8_t **ppData = (uint8_t **)alloca(sizeof(uint8_t *) * worldSizeY);
    size_t *pStart = (size_t *)alloca(sizeof(size_t) * worldSizeY);
    size_t *pEnd = (size_t *)alloca(sizeof(size_t) * worldSizeY);

    // Initialize memory.
    {
      uint8_t *pData = pDirectionBuffer;

      for (size_t i = 0; i < worldSizeY; i++)
      {
        pStart[i] = 0;
        pEnd[i] = worldSizeX;
        ppData[i] = pData;
        pData += worldSizeX;
      }
    }

    while (true)
    {
      register bool changeFound = false;

      // handle first line without checking above.
      {
        bool lastWasTouched = true;

        for (size_t i = 0; i < worldSizeX; i++)
        {
          if (pDirectionBuffer[i] & touchedFlag)
          {
            if (!lastWasTouched)
            {
              pDirectionBuffer[i - 1] = newLeftFlag;
              changeFound = true;
            }

            lastWasTouched = true;
          }
          else
          {
            if (lastWasTouched && i > 0)
            {
              pDirectionBuffer[i] = newRightFlag;
              changeFound = true;
            }

            lastWasTouched = false;
          }
        }
      }
      
      uint8_t *pLastLine = pDirectionBuffer;
      uint8_t *pNextLastLine = pLastLine + worldSizeX;

      for (size_t i = 1; i < worldSizeX; i++)
      {
        pLastLine = pNextLastLine;
        pNextLastLine = ppData[i];

        if (pStart[i] == pEnd[i])
          continue;

        if (ppData[i][pStart[i]] & touchedFlag) // try to move start forward.
        {
          size_t nextStart = pStart[i] + 1;
          size_t furthestMoveForward = 0;

          for (; nextStart < pEnd[i]; nextStart++)
          {
            if (!(ppData[i][nextStart] & touchedFlag))
            {
              break;
            }
            else if (pLastLine[nextStart] == 0)
            {
              pLastLine[nextStart] = newUpFlag;
              changeFound = true;

              // Order intended!
              nextStart++;
              furthestMoveForward = nextStart;

              // We no longer have to check for changeFound or set furthestMoveForward.
              for (; nextStart < pEnd[i]; nextStart++)
              {
                if (!(ppData[i][nextStart] & touchedFlag))
                {
                  break;
                }
                else if (pLastLine[nextStart] == 0)
                {
                  pLastLine[nextStart] = newUpFlag;
                  changeFound = true;
                }
              }

              break;
            }
          }

          if (furthestMoveForward > pStart[i])
            pStart[i] = furthestMoveForward;
          else
            pStart[i] = nextStart;

          if (pEnd[i] < pStart[i])
          {
            ppData[i][pStart[i]] = newRightFlag;
            changeFound = true;
          }
          else
          {
            continue;
          }
        }

        size_t pos = pStart[i];

      empty_region: // We're inside an 'empty region'.
        {
          size_t nextPos = pos;

          for (; nextPos < pEnd[i]; nextPos++)
          {
            if (ppData[i][nextPos] & touchedFlag)
            {
              break;
            }
            else if (!(ppData[i][nextPos] & obstacleFlag) && pLastLine[nextPos] & touchedFlag && !(pLastLine[nextPos] & freshDataFlag))
            {
              ppData[i][nextPos] = newDownFlag;
              changeFound = true;
            }
          }
          
          if (nextPos < pEnd[i])
          {
            if (!(ppData[i][nextPos - 1] & obstacleFlag))
            {
              ppData[i][nextPos - 1] = newLeftFlag;
              changeFound = true;
            }

            pos = nextPos;
            goto solid_region; // redundant.
          }
          else
          {
            continue;
          }
        }

      solid_region: // We're inside an 'solid region'.
        {
          size_t furthestEndPosition = pos;
          size_t nextPos = pos;

          for (; nextPos < pEnd[i]; nextPos++)
          {
            if (!(ppData[i][nextPos] & touchedFlag))
            {
              break;
            }
            else if (pLastLine[nextPos] == 0)
            {
              pLastLine[nextPos] = newUpFlag;
              changeFound = true;
            }
            else if (!(pLastLine[nextPos] & touchedFlag))
            {
              furthestEndPosition = pos;
            }
          }

          if (nextPos < pEnd[i])
          {
            if (!(ppData[i][nextPos] & obstacleFlag))
            {
              ppData[i][nextPos] = newRightFlag;
              changeFound = true;
            }

            pos = nextPos + 1;
            goto empty_region;
          }
          else
          {
            pEnd[i] = furthestEndPosition;
          }
        }
      }

      if (!changeFound)
        break;

      // Remove new data flag.
      {
        const __m128i mask = _mm_set1_epi8(~freshDataFlag);

        __m128i *pDirections128 = (__m128i *)pDirectionBuffer;

        for (size_t i = 0; i < worldSizeX * worldSizeY; i += 16)
        {
          __m128i value = _mm_loadu_si128((__m128i *)pDirections128);
          value = _mm_and_si128(value, mask);
          _mm_storeu_si128(pDirections128, value);

          pDirections128++;
        }
      }
    }
  }
}
