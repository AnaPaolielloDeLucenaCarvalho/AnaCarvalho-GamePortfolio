#include "SpriteComponent.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "GameObject.h"
#include "Texture2D.h"

namespace portfolio
{
    SpriteComponent::SpriteComponent(GameObject* pOwner, const std::string& filename, int cols, int rows, float frameTime)
        : Component(pOwner), m_Cols(cols), m_Rows(rows), m_FrameTime(frameTime)
    {
        m_texture = ResourceManager::GetInstance().LoadTexture(filename);

        auto size = m_texture->GetSize();
        m_FrameWidth = size.x / m_Cols;
        m_FrameHeight = size.y / m_Rows;
    }

    void portfolio::SpriteComponent::Update(float deltaTime)
    {
        if (!m_WasMovedThisFrame)
        {
            m_CurrentCol = 0;
            m_AccumulatedTime = 0.0f;
            return;
        }

        m_AccumulatedTime += deltaTime;
        if (m_AccumulatedTime >= m_FrameTime)
        {
            m_CurrentCol++;
            if (m_CurrentCol >= m_Cols)
            {
                m_CurrentCol = 1;
            }
            m_AccumulatedTime -= m_FrameTime;
        }

        m_WasMovedThisFrame = false;
    }

    void portfolio::SpriteComponent::Render() const
    {
        if (m_texture == nullptr) return;

        const auto& pos = GetOwner()->GetTransform().GetPosition();

        SDL_FRect srcRect{};
        srcRect.w = m_FrameWidth;
        srcRect.h = m_FrameHeight;
        srcRect.x = m_CurrentCol * m_FrameWidth;
        srcRect.y = m_CurrentRow * m_FrameHeight;

        float scaledW = m_OverrideSize ? m_CustomWidth : (m_FrameWidth * m_Scale);
        float scaledH = m_OverrideSize ? m_CustomHeight : (m_FrameHeight * m_Scale);
        SDL_FRect dstRect{ pos.x, pos.y, scaledW, scaledH };

        const auto flip = m_isFlipped ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

        SDL_RenderTextureRotated(
            portfolio::Renderer::GetInstance().GetSDLRenderer(),
            m_texture->GetSDLTexture(),
            &srcRect,
            &dstRect,
            0.0,
            nullptr,
            flip
        );
    }

    void portfolio::SpriteComponent::SetDirection(const glm::vec2& dir)
    {
        if (dir.y < 0) m_CurrentRow = 0; // Up
        else if (dir.y > 0) m_CurrentRow = 1; // Down
        else if (dir.x != 0) m_CurrentRow = 2; // Left/Right

        // Handle flipping
        if (dir.x < 0) SetFlip(true);
        else if (dir.x > 0) SetFlip(false);
    }
}