#include "SceneManager.h"
#include "Scene.h"
#include "Renderer.h"

void portfolio::SceneManager::Update(float deltaTime)
{
    if (m_IsFading)
    {
        if (m_IsFadingOut)
        {
            m_FadeAlpha += m_FadeSpeed * deltaTime;
            if (m_FadeAlpha >= 255.0f)
            {
                m_FadeAlpha = 255.0f;
                m_IsFadingOut = false;

                SetActiveScene(m_NextSceneIndex);
                if (m_OnTransitionComplete)
                    m_OnTransitionComplete();
            }
        }
        else
        {
            m_FadeAlpha -= m_FadeSpeed * deltaTime;
            if (m_FadeAlpha <= 0.0f)
            {
                m_FadeAlpha = 0.0f;
                m_IsFading = false;
            }
        }
    }

    if (m_ActiveSceneIndex < m_scenes.size())
    {
        m_scenes[m_ActiveSceneIndex]->Update(deltaTime);
    }
}

void portfolio::SceneManager::Render()
{
    if (m_ActiveSceneIndex < m_scenes.size())
    {
        m_scenes[m_ActiveSceneIndex]->Render();
    }

    if (m_IsFading)
    {
        auto renderer = portfolio::Renderer::GetInstance().GetSDLRenderer();

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, static_cast<Uint8>(m_FadeAlpha));
        SDL_FRect rect{ 0, 0, 1366, 768 };
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

void portfolio::SceneManager::TransitionToScene(size_t index, std::function<void()> onTransitionComplete)
{
    if (m_IsFading) return;

    m_IsFading = true;
    m_IsFadingOut = true;
    m_FadeAlpha = 0.0f;
    m_NextSceneIndex = index;
    m_OnTransitionComplete = onTransitionComplete;
}

portfolio::Scene& portfolio::SceneManager::CreateScene()
{
	m_scenes.push_back(std::unique_ptr<Scene>(new Scene()));
	return *m_scenes.back();
}

portfolio::Scene& portfolio::SceneManager::GetScene(size_t index)
{
    return *m_scenes[index];
}
