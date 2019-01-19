#include "platform.h"

bool HandleWindowEvents(_Out_ AppState *pAppState)
{
  SDL_Event event;

  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {

    case SDL_KEYDOWN:
    {
      pAppState->keyDown = true;
      pAppState->key = event.key.keysym.sym;

      if (pAppState->key != SDLK_ESCAPE)
        break;
    }

    case SDL_QUIT:
    {
      pAppState->quit = true;
      return false;
    }

    case SDL_KEYUP:
    {
      pAppState->keyDown = false;
      break;
    }

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
      const bool setButton = (event.type == SDL_MOUSEBUTTONDOWN);

      switch (event.button.button)
      {
      case SDL_BUTTON_LEFT:
        pAppState->leftMouseDown = setButton;
        break;

      case SDL_BUTTON_RIGHT:
        pAppState->rightMouseDown = setButton;
        break;
      }

      break;
    }

    case SDL_MOUSEMOTION:
    {
      pAppState->mouseX = event.motion.x;
      pAppState->mouseY = event.motion.y;
      pAppState->relativeMouseX = event.motion.xrel;
      pAppState->relativeMouseY = event.motion.yrel;
      break;
    }

    }
  }

  return true;
}
