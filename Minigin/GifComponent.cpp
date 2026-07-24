#include "GifComponent.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "GameObject.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_GIF
#include "stb_image.h"

#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace portfolio
{
    GifComponent::GifComponent(GameObject* pOwner, const std::string& filename)
        : Component(pOwner)
    {
        std::string fullPath = (std::filesystem::path("Data") / filename).string();

        FILE* f = nullptr;
#ifdef _WIN32
        fopen_s(&f, fullPath.c_str(), "rb");
#else
        f = fopen(fullPath.c_str(), "rb");
#endif
        if (!f)
        {
            std::cerr << "Failed to open GIF: " << fullPath << "\n";
            return;
        }

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        std::vector<unsigned char> buffer(size);
        fread(buffer.data(), 1, size, f);
        fclose(f);

        int *delays = nullptr;
        int w, h, frames, comp;
        
        stbi_uc* gif_data = stbi_load_gif_from_memory(buffer.data(), (int)size, &delays, &w, &h, &frames, &comp, 4);
        
        if (!gif_data)
        {
            std::cerr << "Failed to decode GIF: " << fullPath << "\n";
            return;
        }

        m_Width = w;
        m_Height = h;

        SDL_Renderer* renderer = Renderer::GetInstance().GetSDLRenderer();

        for (int i = 0; i < frames; ++i)
        {
            SDL_Surface* surface = SDL_CreateSurfaceFrom(
                w, h, SDL_PIXELFORMAT_RGBA32,
                gif_data + (i * w * h * 4), w * 4
            );

            if (surface)
            {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_DestroySurface(surface);
                
                if (texture)
                {
                    GifFrame frame;
                    frame.texture = texture;
                    frame.duration = delays[i] / 1000.0f; 
                    if (frame.duration <= 0.01f) frame.duration = 0.1f;
                    
                    m_Frames.push_back(frame);
                }
            }
        }

        stbi_image_free(gif_data);
        if (delays) stbi_image_free(delays);
    }

    GifComponent::~GifComponent()
    {
        for (auto& frame : m_Frames)
        {
            if (frame.texture)
            {
                SDL_DestroyTexture(frame.texture);
            }
        }
    }

    void GifComponent::Update(float deltaTime)
    {
        if (m_Frames.empty()) return;

        m_AccumulatedTime += deltaTime;
        
        while (m_AccumulatedTime >= m_Frames[m_CurrentFrame].duration)
        {
            m_AccumulatedTime -= m_Frames[m_CurrentFrame].duration;
            m_CurrentFrame = (m_CurrentFrame + 1) % m_Frames.size();
        }
    }

    void GifComponent::Render() const
    {
        if (m_Frames.empty()) return;

        const auto& pos = GetOwner()->GetTransform().GetPosition();

        float scaledW = m_Width * m_Scale;
        float scaledH = m_Height * m_Scale;
        SDL_FRect dstRect{ pos.x, pos.y, scaledW, scaledH };

        SDL_RenderTexture(
            Renderer::GetInstance().GetSDLRenderer(),
            m_Frames[m_CurrentFrame].texture,
            nullptr,
            &dstRect
        );
    }
}
