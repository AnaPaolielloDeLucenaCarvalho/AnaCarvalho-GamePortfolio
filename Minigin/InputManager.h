#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "Singleton.h"
#include "Command.h"
#include "Gamepad.h"
#include <map>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/fwd.hpp>

namespace portfolio
{
    enum class KeyState
    { 
		Down, // pressed
		Up, // released
		Pressed // held
    };

    class InputManager final : public Singleton<InputManager>
    {
    public:
        InputManager();

        bool ProcessInput(float deltaTime);

        void BindCommand(SDL_Scancode key, KeyState state, std::unique_ptr<Command> command);
        void BindCommand(unsigned int controllerIndex, Gamepad::ControllerButton button, KeyState state, std::unique_ptr<Command> command);

		// teacher feedback - unbind commands
        void UnbindAll();

    private:
        std::map<std::pair<SDL_Scancode, KeyState>, std::unique_ptr<Command>> m_KeyboardCommands;
		std::vector<std::unique_ptr<Gamepad>> m_pGamepads; // so it supports multiple gamepads

        struct GamepadBinding
        {
            unsigned int controllerIndex;
            Gamepad::ControllerButton button;
            KeyState state;

            bool operator<(const GamepadBinding& other) const
            {
                return std::tie(controllerIndex, button, state) < std::tie(other.controllerIndex, other.button, other.state);
            }
        };
        std::map<GamepadBinding, std::unique_ptr<Command>> m_GamepadCommands;

        struct MouseBinding
        {
            int button; // e.g. 1 for Left, 2 for Middle, 3 for Right
            KeyState state;

            bool operator<(const MouseBinding& other) const
            {
                return std::tie(button, state) < std::tie(other.button, other.state);
            }
        };
        std::map<MouseBinding, std::unique_ptr<Command>> m_MouseCommands;

    public:
        void BindMouseCommand(int button, KeyState state, std::unique_ptr<Command> command);
        glm::vec2 GetMousePosition() const;
    };
}

#endif