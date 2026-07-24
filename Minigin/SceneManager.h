#pragma once
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include "Scene.h"
#include "Singleton.h"

namespace portfolio
{
    class Scene;
    class SceneManager final : public Singleton<SceneManager>
    {
    public:
        Scene& CreateScene();
        Scene& GetScene(size_t index);

        void SetActiveScene(size_t index) { m_ActiveSceneIndex = index; }
        size_t GetActiveScene() const { return m_ActiveSceneIndex; }

        void TransitionToScene(size_t index, std::function<void()> onTransitionComplete);

        void Update(float deltaTime);
        void Render();
    private:
        friend class Singleton<SceneManager>;
        SceneManager() = default;
        std::vector<std::unique_ptr<Scene>> m_scenes{};
        size_t m_ActiveSceneIndex{ 0 };

        bool m_IsFading{ false };
        bool m_IsFadingOut{ false };
        float m_FadeAlpha{ 0.0f };
        float m_FadeSpeed{ 300.0f };
        size_t m_NextSceneIndex{ 0 };
        std::function<void()> m_OnTransitionComplete;
    };
}