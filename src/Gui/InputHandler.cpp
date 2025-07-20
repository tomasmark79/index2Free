#include "InputHandler.hpp"
#include <Logger/Logger.hpp>

InputHandler::InputHandler () {
  initializeDefaultKeyMappings ();
}

InputHandler::~InputHandler () = default;

// Set a callback for scaling actions
void InputHandler::setScaleCallback (FunctionWithFloatCallback callback) {
  scaleCallback_ = std::move (callback);
}

// Set a callback for a specific input action
void InputHandler::setActionCallback (InputAction action, ActionCallback callback) {
  actionCallbacks_[action] = std::move (callback);
}

// Process an SDL event and dispatch it to the appropriate handler
bool InputHandler::processEvent (const SDL_Event& event) {
  const SDL_Keycode key = event.key.keysym.sym;
  switch (event.type) {
  case SDL_KEYDOWN:
    processKeyboardInput (const_cast<SDL_Event&> (event));
    // LOG_D_STREAM << "InputHandler::processEvent: Key down event for key: " << SDL_GetKeyName (key)
    //              << std::endl;
    break;
  case SDL_WINDOWEVENT:
    processWindowEvent (const_cast<SDL_Event&> (event));
    // LOG_D_STREAM << "InputHandler::processEvent: Window event type: "
    //              << static_cast<int> (event.window.event) << std::endl;
    break;
  default:
    // Handle other event types if necessary
    break;
  }
  return (event.type == SDL_QUIT);
}

// Process keyboard input events
void InputHandler::processKeyboardInput (const SDL_Event& event) {
  const SDL_Keycode key = event.key.keysym.sym;

  if ((key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_EQUALS) && scaleCallback_) {
    scaleCallback_ (0.05f);
    LOG_D_STREAM << "InputHandler::processKeyboardInput: Scale up by 0.1" << std::endl;
    return;
  }

  if ((key == SDLK_MINUS || key == SDLK_KP_MINUS) && scaleCallback_) {
    scaleCallback_ (-0.05f);
    LOG_D_STREAM << "InputHandler::processKeyboardInput: Scale down by 0.1" << std::endl;
    return;
  }

  if (key == SDLK_UP && actionCallbacks_.count (InputAction::VolumeUp)) {
    actionCallbacks_[InputAction::VolumeUp]();
    LOG_D_STREAM << "InputHandler::processKeyboardInput: Volume up action triggered" << std::endl;
    return;
  }

  // Handle other key events
  auto it = keyMappings_.find (key);
  if (it != keyMappings_.end ()) {
    auto actionIt = actionCallbacks_.find (it->second);
    if (actionIt != actionCallbacks_.end ()) {
      actionIt->second ();
    }
  }
}

// Process window events such as resizing, focus changes, and closing
void InputHandler::processWindowEvent (const SDL_Event& event) {
  if (event.window.event == SDL_WINDOWEVENT_RESIZED
      || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
  }
  if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
    // Handle focus gained event if needed
  } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
    // Handle focus lost event if needed
  } else if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
    // Handle window close event if needed
  }
}

void InputHandler::initializeDefaultKeyMappings () {
  keyMappings_[SDLK_F11] = InputAction::ToggleFullscreen;
  keyMappings_[SDLK_UP] = InputAction::VolumeUp;
  keyMappings_[SDLK_DOWN] = InputAction::VolumeDown;
  keyMappings_[SDLK_m] = InputAction::Mute;
  keyMappings_[SDLK_PLUS] = InputAction::ScaleUp;
  keyMappings_[SDLK_MINUS] = InputAction::ScaleDown;
}