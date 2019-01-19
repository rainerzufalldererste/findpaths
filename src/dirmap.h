#ifndef dirmap_h__
#define dirmap_h__

#include "platform.h"

fpResult dirmap(_Out_ uint8_t *pDirectionBuffer, _In_ uint8_t *pObstacleBuffer, const size_t worldSizeX, const size_t worldSizeY, const size_t startPositionX, const size_t startPositionY);

#endif // dirmap_h__
