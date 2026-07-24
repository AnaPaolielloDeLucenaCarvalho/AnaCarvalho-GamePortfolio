#pragma once
#include <string>
#include <memory>
#include "Component.h"
#include <glm/glm.hpp>

namespace portfolio
{
    class Texture2D;

    class SpriteComponent : public Component
    {
    public:
        SpriteComponent(GameObject* pOwner, const std::string& filename, int cols, int rows, float frameTime = 0.1f);
        ~SpriteComponent() = default;

        void Update(float deltaTime) override;
        void Render() const override;

        void SetFlip(bool flip) { m_isFlipped = flip; }
        void SetScale(float scale) { m_Scale = scale; }
        void SetSize(float w, float h) { m_OverrideSize = true; m_CustomWidth = w; m_CustomHeight = h; }

        void SetDirection(const glm::vec2& dir);
        void MarkAsMoved() { m_WasMovedThisFrame = true; }

    private:
        std::shared_ptr<Texture2D> m_texture;

        int m_Cols;
        int m_Rows;
        int m_CurrentCol{ 0 };
        int m_CurrentRow{ 1 };

        float m_FrameTime;
        float m_AccumulatedTime{ 0.0f };
        float m_FrameWidth;
        float m_FrameHeight;
        bool m_isFlipped{ false };
        float m_Scale{ 1.0f };
        bool m_OverrideSize{ false };
        float m_CustomWidth{ 0.0f };
        float m_CustomHeight{ 0.0f };

        bool m_WasMovedThisFrame{ false };
    };
}