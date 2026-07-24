#include <stdexcept>
#include <cstring>
#include <iostream>
#include "Renderer.h"
#include "SceneManager.h"
#include "Texture2D.h"


void portfolio::Renderer::Init(SDL_Window* window)
{
	m_window = window;
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	m_renderer = SDL_CreateRenderer(window, nullptr);
	if (m_renderer == nullptr)
	{
		std::cout << "Failed to create the renderer: " << SDL_GetError() << "\n";
		throw std::runtime_error(std::string("SDL_CreateRenderer Error: ") + SDL_GetError());
	}

	SDL_SetRenderLogicalPresentation(m_renderer, 1366, 768, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	SDL_SetRenderLogicalPresentation(m_renderer, 1366, 768, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

void portfolio::Renderer::Render() const
{
	const auto& color = GetBackgroundColor();
	SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
	SDL_RenderClear(m_renderer);

	SceneManager::GetInstance().Render();

	SDL_RenderPresent(m_renderer);
}

void portfolio::Renderer::Destroy()
{
	if (m_renderer != nullptr)
	{
		SDL_DestroyRenderer(m_renderer);
		m_renderer = nullptr;
	}
}

void portfolio::Renderer::RenderTexture(const Texture2D& texture, const float x, const float y) const
{
	SDL_FRect dst{};
	dst.x = x;
	dst.y = y;
	SDL_GetTextureSize(texture.GetSDLTexture(), &dst.w, &dst.h);
	SDL_RenderTexture(GetSDLRenderer(), texture.GetSDLTexture(), nullptr, &dst);
}

void portfolio::Renderer::RenderTexture(const Texture2D& texture, const float x, const float y, const float width, const float height) const
{
	SDL_FRect dst{};
	dst.x = x;
	dst.y = y;
	dst.w = width;
	dst.h = height;
	SDL_RenderTexture(GetSDLRenderer(), texture.GetSDLTexture(), nullptr, &dst);
}

// Fliping
void portfolio::Renderer::RenderTexture(const Texture2D& texture, float x, float y, SDL_FlipMode flip) const
{
	SDL_FRect dst{};
	dst.x = x;
	dst.y = y;
	auto size = texture.GetSize();
	dst.w = size.x;
	dst.h = size.y;

	SDL_RenderTextureRotated(m_renderer, texture.GetSDLTexture(), nullptr, &dst, 0.0, nullptr, flip);
}

// Fliping + Scaling
void portfolio::Renderer::RenderTexture(const Texture2D & texture, float x, float y, float width, float height, SDL_FlipMode flip) const
{
	SDL_FRect dst{};
	dst.x = x;
	dst.y = y;
	dst.w = width;
	dst.h = height;

	SDL_RenderTextureRotated(m_renderer, texture.GetSDLTexture(), nullptr, &dst, 0.0, nullptr, flip);
}

SDL_Renderer* portfolio::Renderer::GetSDLRenderer() const { return m_renderer; }