#ifndef platform_h__
#define platform_h__

#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <chrono>
#include <algorithm>

#ifdef _WIN32
#include "SDL2/include/SDL.h"
#include <intrin.h>
#else
#include <SDL2/SDL.h>
#include <x86intrin.h>
#endif

#ifndef _In_
#define _In_
#endif

#ifndef _Out_
#define _Out_
#endif

#ifndef _In_Out_
#define _In_Out_
#endif

enum fpResult
{
  fpR_Success,
  fpR_NotSupported,
  fpR_MemoryAllocationFailure,
};

#define FP_SUCCESS(errorCode) (errorCode == fpR_Success)
#define FP_FAILED(errorCode) (!(FP_SUCCESS(errorCode)))

#define FP_ERROR_SET(errorCode) \
  do \
  { result = errorCode; \
    goto epilogue; \
  } while (0) 

#define FP_ERROR_IF(booleanExpression, errorCode) \
  do \
  { if (booleanExpression) \
      FP_ERROR_SET(errorCode); \
  } while (0) \

template <typename T>
inline void fpFreePtr(_In_Out_ T **ppData)
{
  if (*ppData != nullptr)
    free(*ppData);

  *ppData = nullptr;
}

//////////////////////////////////////////////////////////////////////////

struct AppState
{
  int32_t mouseX, mouseY, relativeMouseX, relativeMouseY;
  SDL_Keycode key;
  bool rightMouseDown, leftMouseDown, keyDown, quit;
};

bool HandleWindowEvents(_Out_ AppState *pAppState);

#endif // platform_h__
