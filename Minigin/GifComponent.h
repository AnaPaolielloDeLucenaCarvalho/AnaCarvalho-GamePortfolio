#pragma once
#include "Component.h"
#include <string>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>

namespace portfolio
{
    class GifComponent final : public Component
    {
    public:
        GifComponent(GameObject* pOwner, const std::string& filename);
        ~GifComponent();

        void Update(float deltaTime) override;
        void Render() const override;

        void SetScale(float scale) { m_Scale = scale; }

        GifComponent(const GifComponent&) = delete;
        GifComponent(GifComponent&&) = delete;
        GifComponent& operator=(const GifComponent&) = delete;
        GifComponent& operator=(GifComponent&&) = delete;

    private:
        struct GifFrame {
            SDL_Texture* texture;
            float duration; // in seconds
        };

        std::vector<GifFrame> m_Frames;
        int m_CurrentFrame{ 0 };
        float m_AccumulatedTime{ 0.0f };
        float m_Scale{ 1.0f };
        int m_Width{ 0 };
        int m_Height{ 0 };
    };
}
