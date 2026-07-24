#include <SDL3/SDL.h>
#include <algorithm>
#include "InputManager.h"

namespace portfolio
{
    InputManager::InputManager()
    {
        // Player 1 and Player 2
        m_pGamepads.push_back(std::make_unique<Gamepad>(0));
        m_pGamepads.push_back(std::make_unique<Gamepad>(1));
    }

    void InputManager::BindCommand(SDL_Scancode key, KeyState state, std::unique_ptr<Command> command)
    {
        m_KeyboardCommands[std::make_pair(key, state)] = std::move(command);
    }

    void InputManager::BindCommand(unsigned int controllerIndex, Gamepad::ControllerButton button, KeyState state, std::unique_ptr<Command> command)
    {
        m_GamepadCommands[GamepadBinding{ controllerIndex, button, state }] = std::move(command);
    }

    void InputManager::UnbindAll()
    {
        m_KeyboardCommands.clear();
        m_GamepadCommands.clear();
    }

    void InputManager::BindMouseCommand(int button, KeyState state, std::unique_ptr<Command> command)
    {
        m_MouseCommands[MouseBinding{ button, state }] = std::move(command);
    }

    glm::vec2 InputManager::GetMousePosition() const
    {
        float x, y;
        SDL_GetMouseState(&x, &y);
        return glm::vec2(x, y);
    }

    bool InputManager::ProcessInput(float deltaTime)
    {
		// now it supports multiple gamepads
        for (auto& gamepad : m_pGamepads)
        {
            gamepad->Update();
        }

        // Process mouse states for Pressed (held)
        Uint32 mouseStateBits = SDL_GetMouseState(nullptr, nullptr);

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                return false;
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                auto it = m_MouseCommands.find({ e.button.button, KeyState::Down });
                if (it != m_MouseCommands.end())
                {
                    it->second->Execute(deltaTime);
                }
            }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                auto it = m_MouseCommands.find({ e.button.button, KeyState::Up });
                if (it != m_MouseCommands.end())
                {
                    it->second->Execute(deltaTime);
                }
            }
        }

        for (auto& [binding, command] : m_MouseCommands)
        {
            if (binding.state == KeyState::Pressed)
            {
                if (mouseStateBits & SDL_BUTTON_MASK(binding.button))
                {
                    command->Execute(deltaTime);
                }
            }
        }

        // Keyboard Commands
        const bool* state = SDL_GetKeyboardState(nullptr);
        for (auto& [keyBinding, command] : m_KeyboardCommands)
        {
            const auto& [key, keyState] = keyBinding;
            if (keyState == KeyState::Pressed && state[key])
            {
                command->Execute(deltaTime);
            }
        }

        // Gamepad Commands
        for (auto& [binding, command] : m_GamepadCommands)
        {
            if (binding.controllerIndex < m_pGamepads.size())
            {
                auto& gamepad = m_pGamepads[binding.controllerIndex];
                bool shouldExecute = false;
                switch (binding.state)
                {
                case KeyState::Down:    
                    shouldExecute = gamepad->IsDown(binding.button); 
                    break;
                case KeyState::Up:      
                    shouldExecute = gamepad->IsUp(binding.button); 
                    break;
                case KeyState::Pressed: 
                    shouldExecute = gamepad->IsPressed(binding.button); 
                    break;
                }

                if (shouldExecute)
                {
                    command->Execute(deltaTime);
                }
            }
        }

        return true;
    }
}