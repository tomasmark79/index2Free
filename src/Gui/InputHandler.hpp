#ifndef INPUTHANDLER_HPP
#define INPUTHANDLER_HPP

#include <SDL.h>
#include <functional>
#include <unordered_map>

enum class InputAction {
  VolumeUp,
  VolumeDown,
  Mute,
  ScaleUp,
  ScaleDown,
  ToggleFullscreen,
};

class InputHandler {
public:
  InputHandler ();
  ~InputHandler ();

  using FunctionWithFloatCallback = std::function<void (float)>;
  using ActionCallback = std::function<void ()>;

  void setScaleCallback (FunctionWithFloatCallback callback);
  void setActionCallback (InputAction action, ActionCallback callback);

  bool processEvent (const SDL_Event& event);

protected:
  void processKeyboardInput (const SDL_Event& event);
  void processWindowEvent (const SDL_Event& event);

private:
  FunctionWithFloatCallback scaleCallback_;
  std::unordered_map<InputAction, ActionCallback> actionCallbacks_;
  std::unordered_map<SDL_Keycode, InputAction> keyMappings_;
  void initializeDefaultKeyMappings ();
};

#endif