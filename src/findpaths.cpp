#include "platform.h"
#include "dirmap.h"

//////////////////////////////////////////////////////////////////////////

struct Unit
{
  uint16_t positionX, positionY, destinationX, destinationY;
};

void DrawObstacles(_In_ uint32_t *pPixels, _In_ uint8_t *pObstacles, const size_t worldX, const size_t worldY);
void DrawDirMap(_In_ uint32_t *pPixels, _In_ uint8_t *pDirMap, const size_t worldX, const size_t worldY);

//////////////////////////////////////////////////////////////////////////

constexpr size_t WorldX = 1024;
constexpr size_t WorldY = 1024;
constexpr size_t WorldSubSample = 3;
constexpr size_t NavLutSizeX = WorldX >> WorldSubSample;
constexpr size_t NavLutSizeY = WorldY >> WorldSubSample;
constexpr size_t UnitCount = 1024;

static_assert(WorldX % 16 == 0 && WorldY % 16 == 0 && WorldX < INT_MAX && WorldY < INT_MAX && NavLutSizeX > 0 && NavLutSizeY > 0, "Invalid Configuration");

int main(const int /*argc*/, char ** /*ppArgv*/)
{
  fpResult result = fpR_Success;

  SDL_Window *pWindow = nullptr;
  uint32_t *pPixels = nullptr;
  uint8_t *pObstacles = nullptr;
  //Unit *pUnits = nullptr;
  uint8_t *pDirMap = nullptr;

  AppState appState = { 0 };

  // Initialize Window.
  {
    FP_ERROR_IF(0 != SDL_Init(SDL_INIT_VIDEO), fpR_NotSupported);

    pWindow = SDL_CreateWindow("findpath", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int)WorldX, (int)WorldY, SDL_WINDOW_SHOWN);
    FP_ERROR_IF(pWindow == nullptr, fpR_NotSupported);

    pPixels = (uint32_t *)SDL_GetWindowSurface(pWindow)->pixels;
    FP_ERROR_IF(pPixels == nullptr, fpR_NotSupported);
  }

  // Initialize Buffers.
  {
    pObstacles = (uint8_t *)calloc((WorldX * WorldY) >> 3, sizeof(uint8_t)); // 1 bit per obstacle.
    FP_ERROR_IF(pObstacles == nullptr, fpR_MemoryAllocationFailure);

    pDirMap = (uint8_t *)malloc(WorldX * WorldY); // no need to be zeroed.
    FP_ERROR_IF(pDirMap == nullptr, fpR_MemoryAllocationFailure);
  }

  while (HandleWindowEvents(&appState))
  {
    const auto start = std::chrono::high_resolution_clock::now();

    // Add Obstacles.
    if (appState.keyDown && appState.key == SDLK_LSHIFT)
    {
      constexpr size_t rowWidth = WorldX >> 3;
      uint8_t *pItem = pObstacles + (rowWidth * appState.mouseY) + (appState.mouseX >> 3);

      if (appState.leftMouseDown)
        *pItem |= (1 << (appState.mouseX & 7));
      else if (appState.rightMouseDown)
        *pItem &= ~(1 << (appState.mouseX & 7));
    }

    if (appState.keyDown && appState.key == SDLK_LCTRL && appState.leftMouseDown)
    {
      dirmap(pDirMap, pObstacles, WorldX, WorldY, appState.mouseX, appState.mouseY);
      DrawDirMap(pPixels, pDirMap, WorldX, WorldY);
    }
    else
    {
      DrawObstacles(pPixels, pObstacles, WorldX, WorldY);
    }

    printf("\r%f ms       ", 0.0001 * 0.0001 * std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count());

    // End frame.
    SDL_UpdateWindowSurface(pWindow);
  }

  goto epilogue;

epilogue:
  fpFreePtr(&pObstacles);
  fpFreePtr(&pDirMap);

  if (pWindow)
    SDL_DestroyWindow(pWindow);

  if (FP_FAILED(result))
  {
    printf("Application failed with error code 0x%" PRIx64 ".\nPress any key to exit.\n", (uint64_t)result);
    getchar();
  }

  SDL_Quit();
  return 0;
}

//////////////////////////////////////////////////////////////////////////

void DrawObstacles(_In_ uint32_t *pPixels, _In_ uint8_t *pObstacles, const size_t worldX, const size_t worldY)
{
  const __m128i maskHi = _mm_set_epi32(0b10000000, 0b1000000, 0b100000, 0b10000);
  const __m128i maskLo = _mm_set_epi32(0b1000, 0b100, 0b10, 0b1);
  const __m128i maskRemoveUpper64 = _mm_set_epi64x(0, 0xFF);

  __m128i *pPixels128 = (__m128i *)pPixels;

  for (size_t i = 0; i < worldX * worldY; i += 8)
  {
    const __m128i obstacles = _mm_loadu_si128((__m128i *)pObstacles);
    const __m128i lowest8 = _mm_and_si128(maskRemoveUpper64, _mm_cvtepu8_epi64(obstacles));
    const __m128i spreadTo64 = _mm_or_si128(lowest8, _mm_slli_si128(lowest8, 8));
    const __m128i spreadTo32 = _mm_or_si128(spreadTo64, _mm_slli_si128(spreadTo64, 4));

    __m128i low = _mm_and_si128(maskLo, spreadTo32);
    __m128i high = _mm_and_si128(maskHi, spreadTo32);

    low = _mm_cmpeq_epi32(low, maskLo);
    high = _mm_cmpeq_epi32(high, maskHi);

    _mm_storeu_si128(pPixels128, low);
    pPixels128++;

    _mm_storeu_si128(pPixels128, high);
    pPixels128++;

    pObstacles++;
  }
}

void DrawDirMap(_In_ uint32_t *pPixels, _In_ uint8_t *pDirMap, const size_t worldX, const size_t worldY)
{
  __m128i *pPixels128 = (__m128i *)pPixels;

  for (size_t i = 0; i < worldX * worldY; i += 4)
  {
    const __m128i dirmap = _mm_loadu_si128((__m128i *)pDirMap);

    __m128i map = _mm_cvtepu8_epi32(dirmap);

    map = _mm_or_si128(_mm_slli_epi32(map, 6), _mm_slli_epi32(map, 7 + 8));

    _mm_storeu_si128(pPixels128, map);

    pPixels128++;
    pDirMap += 4;
  }
}