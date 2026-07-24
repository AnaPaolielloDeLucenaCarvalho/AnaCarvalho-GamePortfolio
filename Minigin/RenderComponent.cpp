#include "RenderComponent.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "GameObject.h"
#include "Texture2D.h"
#include <algorithm>

namespace portfolio
{
    RenderComponent::RenderComponent(GameObject* pOwner)
        : Component(pOwner), m_texture(nullptr)
    {
    }

    RenderComponent::RenderComponent(GameObject* pOwner, const std::string& filename)
        : Component(pOwner)
    {
        SetTexture(filename);
    }

    void RenderComponent::Update(float /*deltaTime*/) {}

    void portfolio::RenderComponent::Render() const
    {
        if (m_texture == nullptr) return;

        const auto& pos = GetOwner()->GetTransform().GetPosition();
        auto size = m_texture->GetSize();

        // Use the member variable scale
        float scaledW = size.x * m_Scale;
        float scaledH = size.y * m_Scale;

        if (m_UseFillSize && size.x > 0 && size.y > 0)
        {
            float scaleX = m_ForcedWidth / size.x;
            float scaleY = m_ForcedHeight / size.y;
            float maxScale = std::max(scaleX, scaleY);
            scaledW = size.x * maxScale;
            scaledH = size.y * maxScale;
        }

        const auto flip = m_isFlipped ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

        SDL_FRect dst{ pos.x, pos.y, scaledW, scaledH };
        SDL_RenderTextureRotated(
            portfolio::Renderer::GetInstance().GetSDLRenderer(),
            m_texture->GetSDLTexture(),
            nullptr,
            &dst,
            m_Angle,
            nullptr,
            flip
        );
    }

    void RenderComponent::SetTexture(const std::string& filename)
    {
        m_texture = ResourceManager::GetInstance().LoadTexture(filename);
    }
}