#ifndef PLAYER_CONTROLLER_COMPONENT_H
#define PLAYER_CONTROLLER_COMPONENT_H

#include "Component.h"
#include "GameObject.h"
#include "SpriteComponent.h"
#include "InputManager.h"
#include <glm/glm.hpp>
#include <vector>
#include <SDL3/SDL_rect.h>
#include "ServiceLocator.h"
#include "SoundSystem.h"

namespace portfolio
{
    class PlayerControllerComponent : public Component
    {
    public:
        PlayerControllerComponent(GameObject* owner) : Component(owner) {}

        void Update(float deltaTime) override
        {
            if (m_AutoWalk)
            {
                glm::vec2 pos = GetOwner()->GetTransform().GetPosition();
                glm::vec2 feetPos = pos + glm::vec2(44.0f, 110.0f);

                glm::vec2 dir = m_TargetPos - feetPos;
                float dist = glm::length(dir);

                if (dist < 5.0f)
                {
                    m_AutoWalk = false;
                    auto spriteComp = GetOwner()->GetComponent<portfolio::SpriteComponent>();
                    if (spriteComp) spriteComp->SetDirection({0, 0});
                }
                else
                {
                    dir = glm::normalize(dir);
                    MoveInDirection(dir, deltaTime);
                }
            }
        }

        void MoveInDirection(const glm::vec2& dir, float deltaTime)
        {
            auto posBefore = GetOwner()->GetTransform().GetPosition();

            // Simple movement logic with collision
            glm::vec2 movement = dir * (m_Speed * deltaTime);
            float newX = posBefore.x + movement.x;
            float newY = posBefore.y + movement.y;

            if (m_WalkableZones.empty())
            {
                float minX = 0.0f, maxX = 1366.0f - 88.0f;
                float minY = 0.0f, maxY = 768.0f - 120.0f;
                if (newX < minX) newX = minX;
                if (newX > maxX) newX = maxX;
                if (newY < minY) newY = minY;
                if (newY > maxY) newY = maxY;

                GetOwner()->SetLocalPosition(newX, newY);
            }
            else
            {
                float feetOffsetX = 88.0f / 2.0f;
                float feetOffsetY = 110.0f;

                bool canWalkX = false;
                bool canWalkY = false;

                float testX = newX + feetOffsetX;
                float testY = posBefore.y + feetOffsetY;
                for (const auto& zone : m_WalkableZones)
                {
                    if (testX >= zone.x && testX <= zone.x + zone.w &&
                        testY >= zone.y && testY <= zone.y + zone.h) canWalkX = true;
                }

                testX = posBefore.x + feetOffsetX;
                testY = newY + feetOffsetY;
                for (const auto& zone : m_WalkableZones)
                {
                    if (testX >= zone.x && testX <= zone.x + zone.w &&
                        testY >= zone.y && testY <= zone.y + zone.h) canWalkY = true;
                }

                float finalX = canWalkX ? newX : posBefore.x;
                float finalY = canWalkY ? newY : posBefore.y;
                
                // If we can't move in either direction, cancel autowalk to avoid getting stuck
                if (!canWalkX && !canWalkY)
                {
                     m_AutoWalk = false;
                }

                GetOwner()->SetLocalPosition(finalX, finalY);
            }

            auto posAfter = GetOwner()->GetTransform().GetPosition();
            if (std::abs(posBefore.x - posAfter.x) > 0.01f || std::abs(posBefore.y - posAfter.y) > 0.01f)
            {
                auto spriteComp = GetOwner()->GetComponent<portfolio::SpriteComponent>();
                if (spriteComp)
                {
                    spriteComp->SetDirection(dir);
                    spriteComp->MarkAsMoved();
                }

                HandleFootsteps(posAfter);
            }
        }

        void HandleFootsteps(const glm::vec2& posAfter)
        {
            // Similar surface logic as before
            int newSurface = 2; // Grass
            if (m_AlwaysWood)
            {
                newSurface = 1; // Wood
            }
            else
            {
                float feetX = posAfter.x + 44.0f;
                float feetY = posAfter.y + 115.0f;

                for (const auto& rect : m_WoodZones)
                {
                    if (feetX >= rect.x && feetX <= rect.x + rect.w &&
                        feetY >= rect.y && feetY <= rect.y + rect.h)
                    {
                        newSurface = 1;
                        break;
                    }
                }
            }

            auto& ss = portfolio::ServiceLocator::get_sound_system();

            if (m_CurrentSurface != newSurface)
            {
                if (newSurface == 1) ss.play(11, 0.4f);
                else ss.play(12, 0.4f);

                m_CurrentSurface = newSurface;
                m_LastFootstepTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
            }

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastFootstepTime).count() > 350)
            {
                m_LastFootstepTime = now;
                int soundId = (newSurface == 1) ? (1 + (std::rand() % 5)) : (6 + (std::rand() % 5));
                ss.play(static_cast<portfolio::sound_id>(soundId), 0.15f);
            }
        }

        void SetTarget(const glm::vec2& target)
        {
            m_TargetPos = target;
            m_AutoWalk = true;
        }

        void CancelAutoWalk() { m_AutoWalk = false; }
        void SetSpeed(float speed) { m_Speed = speed; }
        
        void ConfigureZones(const std::vector<SDL_FRect>& walkableZones, bool alwaysWood, const std::vector<SDL_FRect>& woodZones)
        {
            m_WalkableZones = walkableZones;
            m_AlwaysWood = alwaysWood;
            m_WoodZones = woodZones;
        }

    private:
        glm::vec2 m_TargetPos{0,0};
        bool m_AutoWalk = false;
        float m_Speed = 150.0f;
        std::vector<SDL_FRect> m_WalkableZones;
        std::vector<SDL_FRect> m_WoodZones;
        bool m_AlwaysWood = true;

        int m_CurrentSurface = 1; // 1 = Wood, 2 = Grass
        std::chrono::steady_clock::time_point m_LastFootstepTime = std::chrono::steady_clock::now();
    };
}
#endif
