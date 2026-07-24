#define SDL_MAIN_HANDLED 

#include "Minigin.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <fstream>

#include "Scene.h"
#include "SceneManager.h"
#include "GameObject.h"
#include "RenderComponent.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "GifComponent.h"
#include <utility>
#include <filesystem>
#include <SDL3/SDL_scancode.h>
#include <MoveCommand.h>
#include <InputManager.h>
#include "TriggerComponent.h"
#include "MiniaudioSoundSystem.h"
#include "LoggingSoundSystem.h"
#include "ServiceLocator.h"
#include "PlayerControllerComponent.h"
#include <chrono>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace fs = std::filesystem;

// GLOBAL STATE & INTERACTION SYSTEM
std::vector<std::pair<portfolio::TriggerComponent*, std::function<void()>>> g_ProjectInteractions;

bool g_IsMuted = false;
std::vector<std::pair<portfolio::GameObject*, portfolio::GameObject*>> g_SoundIcons;

class ActionCommand : public portfolio::Command
{
    std::function<void()> m_Action;
public:
    ActionCommand(std::function<void()> action) : m_Action(action) {}
    void Execute(float /*deltaTime*/) override { if (m_Action) m_Action(); }
};

void ToggleMuteGlobal()
{
    static auto lastToggleTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastToggleTime).count() < 250) 
    {
        return;
    }
    lastToggleTime = currentTime;

    portfolio::ServiceLocator::get_sound_system().ToggleMute();
    g_IsMuted = !g_IsMuted;

    for (auto& icons : g_SoundIcons)
    {
        if (g_IsMuted)
        {
            icons.first->SetLocalPosition(-2000.0f, -2000.0f);
            icons.second->SetLocalPosition(1306.0f, 32.0f);
        }
        else
        {
            icons.first->SetLocalPosition(1306.0f, 20.0f);
            icons.second->SetLocalPosition(-2000.0f, -2000.0f);
        }
    }
}

void AddMuteIconsToScene(portfolio::Scene& scene)
{
    auto soundOn = std::make_unique<portfolio::GameObject>();
    soundOn->AddComponent<portfolio::RenderComponent>("SoundOn.png");
    auto soundOnPtr = soundOn.get();

    auto soundOff = std::make_unique<portfolio::GameObject>();
    soundOff->AddComponent<portfolio::RenderComponent>("SoundOff.png");
    auto soundOffPtr = soundOff.get();

    auto f2 = std::make_unique<portfolio::GameObject>();
    f2->AddComponent<portfolio::RenderComponent>("F2.png");
    f2->SetLocalPosition(1250.0f, 22.0f);

    if (g_IsMuted)
    {
        soundOnPtr->SetLocalPosition(-2000.0f, -2000.0f);
        soundOffPtr->SetLocalPosition(1306.0f, 32.0f);
    }
    else
    {
        soundOnPtr->SetLocalPosition(1306.0f, 20.0f);
        soundOffPtr->SetLocalPosition(-2000.0f, -2000.0f);
    }

    g_SoundIcons.push_back({ soundOnPtr, soundOffPtr });

    scene.Add(std::move(soundOn));
    scene.Add(std::move(soundOff));
    scene.Add(std::move(f2));
}

// SOUND EFFECTS MOVEMENT
enum class Surface { None, Wood, Grass };
Surface g_CurrentSurface = Surface::Wood;
std::chrono::steady_clock::time_point g_LastFootstepTime = std::chrono::steady_clock::now();

class PlayerMoveCommand : public portfolio::Command 
{
    portfolio::GameObject* m_Player;
    glm::vec2 m_Dir;

public:
    PlayerMoveCommand(portfolio::GameObject* player, glm::vec2 dir)
        : m_Player(player), m_Dir(dir)
    {
    }

    void Execute(float deltaTime) override
    {
        auto controller = m_Player->GetComponent<portfolio::PlayerControllerComponent>();
        if (controller)
        {
            controller->MoveInDirection(m_Dir, deltaTime);
        }
    }
};

void HandleGlobalWebAudioAutoplay()
{
#ifdef __EMSCRIPTEN__
    // Resume audio context on first interaction
    EM_ASM(
        if (typeof Module !== 'undefined' && Module.audioContext && Module.audioContext.state === 'suspended') 
        {
            Module.audioContext.resume();
        }
    );
#endif
}

class SprintCommand : public portfolio::Command
{
    portfolio::GameObject* m_Player;
    bool m_IsSprint;
public:
    SprintCommand(portfolio::GameObject* player, bool isSprint) : m_Player(player), m_IsSprint(isSprint) {}
    void Execute(float) override
    {
        auto controller = m_Player->GetComponent<portfolio::PlayerControllerComponent>();
        if (controller)
        {
            controller->SetSpeed(m_IsSprint ? 300.0f : 150.0f);
        }
    }
};

class CancelAutoWalkCommand : public portfolio::Command
{
    portfolio::GameObject* m_Player;
public:
    CancelAutoWalkCommand(portfolio::GameObject* player) : m_Player(player) {}
    void Execute(float) override
    {
        HandleGlobalWebAudioAutoplay();
        auto controller = m_Player->GetComponent<portfolio::PlayerControllerComponent>();
        if (controller) controller->CancelAutoWalk();
    }
};

// INPUT BINDINGS
void BindPlayerInputs(portfolio::GameObject* playerPtr, bool canInteract = false)
{
    auto& input = portfolio::InputManager::GetInstance();
    input.UnbindAll();

    input.BindCommand(SDL_SCANCODE_W, portfolio::KeyState::Pressed, std::make_unique<PlayerMoveCommand>(playerPtr, glm::vec2{ 0, -1 }));
    input.BindCommand(SDL_SCANCODE_S, portfolio::KeyState::Pressed, std::make_unique<PlayerMoveCommand>(playerPtr, glm::vec2{ 0, 1 }));
    input.BindCommand(SDL_SCANCODE_A, portfolio::KeyState::Pressed, std::make_unique<PlayerMoveCommand>(playerPtr, glm::vec2{ -1, 0 }));
    input.BindCommand(SDL_SCANCODE_D, portfolio::KeyState::Pressed, std::make_unique<PlayerMoveCommand>(playerPtr, glm::vec2{ 1, 0 }));

    input.BindCommand(SDL_SCANCODE_UP, portfolio::KeyState::Pressed, std::make_unique<PlayerMoveCommand>(playerPtr, glm::vec2{ 0, -1 }));
    input.BindCommand(SDL_SCANCODE_DOWN, portfolio::KeyState::Pressed, std::make_unique<PlayerMoveCommand>(playerPtr, glm::vec2{ 0, 1 }));
    input.BindCommand(SDL_SCANCODE_LEFT, portfolio::KeyState::Pressed, std::make_unique<PlayerMoveCommand>(playerPtr, glm::vec2{ -1, 0 }));
    input.BindCommand(SDL_SCANCODE_RIGHT, portfolio::KeyState::Pressed, std::make_unique<PlayerMoveCommand>(playerPtr, glm::vec2{ 1, 0 }));

    // Cancel autowalk when using WASD
    input.BindCommand(SDL_SCANCODE_W, portfolio::KeyState::Down, std::make_unique<CancelAutoWalkCommand>(playerPtr));
    input.BindCommand(SDL_SCANCODE_S, portfolio::KeyState::Down, std::make_unique<CancelAutoWalkCommand>(playerPtr));
    input.BindCommand(SDL_SCANCODE_A, portfolio::KeyState::Down, std::make_unique<CancelAutoWalkCommand>(playerPtr));
    input.BindCommand(SDL_SCANCODE_D, portfolio::KeyState::Down, std::make_unique<CancelAutoWalkCommand>(playerPtr));

    // Sprint bindings
    input.BindCommand(SDL_SCANCODE_LSHIFT, portfolio::KeyState::Down, std::make_unique<SprintCommand>(playerPtr, true));
    input.BindCommand(SDL_SCANCODE_LSHIFT, portfolio::KeyState::Up, std::make_unique<SprintCommand>(playerPtr, false));

    // Mouse bindings
    input.BindMouseCommand(1, portfolio::KeyState::Down, std::make_unique<ActionCommand>([playerPtr]() {
        HandleGlobalWebAudioAutoplay();
        
        glm::vec2 mousePos = portfolio::InputManager::GetInstance().GetMousePosition();
        if (mousePos.x > 1250.0f && mousePos.y < 80.0f)
        {
            ToggleMuteGlobal();
        }
        else
        {
            auto controller = playerPtr->GetComponent<portfolio::PlayerControllerComponent>();
            if (controller)
            {
                controller->SetTarget(mousePos);
            }
        }
    }));

    input.BindCommand(SDL_SCANCODE_F2, portfolio::KeyState::Pressed, std::make_unique<ActionCommand>(ToggleMuteGlobal));

    if (canInteract)
    {
        input.BindCommand(SDL_SCANCODE_E, portfolio::KeyState::Pressed, std::make_unique<ActionCommand>([]()
            {
                for (auto& interaction : g_ProjectInteractions)
                {
                    if (interaction.first->IsInside())
                    {
                        interaction.second();
                        break;
                    }
                }
            }));
    }
}

// BIND PROJECTS (ESC to leave, Q/E carousels)
void BindProjectViewInputs(std::function<void()> onEsc, std::function<void()> onQ, std::function<void()> onE)
{
    auto& input = portfolio::InputManager::GetInstance();
    input.UnbindAll();

    input.BindCommand(SDL_SCANCODE_ESCAPE, portfolio::KeyState::Pressed, std::make_unique<ActionCommand>(onEsc));
    input.BindCommand(SDL_SCANCODE_Q, portfolio::KeyState::Pressed, std::make_unique<ActionCommand>(onQ));
    input.BindCommand(SDL_SCANCODE_E, portfolio::KeyState::Pressed, std::make_unique<ActionCommand>(onE));

    input.BindCommand(SDL_SCANCODE_F2, portfolio::KeyState::Pressed, std::make_unique<ActionCommand>(ToggleMuteGlobal));

    input.BindMouseCommand(1, portfolio::KeyState::Down, std::make_unique<ActionCommand>([onEsc, onQ, onE]() 
    {
        HandleGlobalWebAudioAutoplay();
        glm::vec2 mousePos = portfolio::InputManager::GetInstance().GetMousePosition();

        if (mousePos.x > 1250.0f && mousePos.y < 80.0f)
        {
            ToggleMuteGlobal();
            return;
        }

        if (mousePos.x >= 48.0f && mousePos.x <= 184.0f && mousePos.y >= 544.0f && mousePos.y <= 660.0f)
        {
            onQ();
        }
        else if (mousePos.x >= 824.0f && mousePos.x <= 960.0f && mousePos.y >= 544.0f && mousePos.y <= 660.0f)
        {
            onE();
        }
        else if (mousePos.x < 150.0f && mousePos.y < 150.0f)
        {
            onEsc();
        }
        else if (mousePos.x > 500.0f && mousePos.x < 800.0f && mousePos.y < 100.0f)
        {
            onEsc();
        }
    }));
}

// SCENE BUILDERS
void LoadMainMenu(portfolio::GameObject*& outPlayer, portfolio::TriggerComponent*& tAbout, portfolio::TriggerComponent*& tContact, portfolio::TriggerComponent*& tProj)
{
    auto& scene = portfolio::SceneManager::GetInstance().GetScene(0);

    auto bg = std::make_unique<portfolio::GameObject>();
    bg->AddComponent<portfolio::RenderComponent>("MainMenuBackground.png");
    scene.Add(std::move(bg));

    auto player = std::make_unique<portfolio::GameObject>();
    player->AddComponent<portfolio::SpriteComponent>("PlayerSprite.png", 3, 3, 0.1f);
    player->AddComponent<portfolio::PlayerControllerComponent>();
    player->SetLocalPosition(642.5f, 400.0f);
    outPlayer = player.get();
    scene.Add(std::move(player));

    auto treeTop = std::make_unique<portfolio::GameObject>();
    treeTop->AddComponent<portfolio::RenderComponent>("MainMenuTop.png");
    treeTop->SetLocalPosition(159.0f, 442.0f);
    scene.Add(std::move(treeTop));

    auto tr1 = std::make_unique<portfolio::GameObject>();
    tr1->SetLocalPosition(624.0f, 0);
    tAbout = tr1->AddComponent<portfolio::TriggerComponent>(124.0f, 48.0f);
    tAbout->SetTarget(outPlayer, 88.0f, 120.0f);
    scene.Add(std::move(tr1));

    auto tr2 = std::make_unique<portfolio::GameObject>();
    tr2->SetLocalPosition(1280.0f, 388.0f);
    tContact = tr2->AddComponent<portfolio::TriggerComponent>(86.0f, 168.0f);
    tContact->SetTarget(outPlayer, 88.0f, 120.0f);
    scene.Add(std::move(tr2));

    auto tr3 = std::make_unique<portfolio::GameObject>();
    tr3->SetLocalPosition(624.0f, 684.0f);
    tProj = tr3->AddComponent<portfolio::TriggerComponent>(124.0f, 84.0f);
    tProj->SetTarget(outPlayer, 88.0f, 120.0f);
    scene.Add(std::move(tr3));

    AddMuteIconsToScene(scene);
}

void LoadAboutScene(portfolio::GameObject*& outPlayer, portfolio::TriggerComponent*& tMain)
{
    auto& scene = portfolio::SceneManager::GetInstance().GetScene(1);

    auto bg = std::make_unique<portfolio::GameObject>();
    bg->AddComponent<portfolio::RenderComponent>("AboutBackground.png");
    scene.Add(std::move(bg));

    auto player = std::make_unique<portfolio::GameObject>();
    player->AddComponent<portfolio::SpriteComponent>("PlayerSprite.png", 3, 3, 0.1f);
    player->AddComponent<portfolio::PlayerControllerComponent>();
    outPlayer = player.get();
    scene.Add(std::move(player));

    auto tr1 = std::make_unique<portfolio::GameObject>();
    tr1->SetLocalPosition(624.0f, 720.0f);
    tMain = tr1->AddComponent<portfolio::TriggerComponent>(124.0f, 48.0f);
    tMain->SetTarget(outPlayer, 88.0f, 120.0f);
    scene.Add(std::move(tr1));

    AddMuteIconsToScene(scene);
}

void LoadContactScene(portfolio::GameObject*& outPlayer, portfolio::TriggerComponent*& tMain)
{
    auto& scene = portfolio::SceneManager::GetInstance().GetScene(2);

    auto bg = std::make_unique<portfolio::GameObject>();
    bg->AddComponent<portfolio::RenderComponent>("ContactBackground.png");
    scene.Add(std::move(bg));

    auto player = std::make_unique<portfolio::GameObject>();
    player->AddComponent<portfolio::SpriteComponent>("PlayerSprite.png", 3, 3, 0.1f);
    player->AddComponent<portfolio::PlayerControllerComponent>();
    outPlayer = player.get();
    scene.Add(std::move(player));

    auto tr1 = std::make_unique<portfolio::GameObject>();
    tr1->SetLocalPosition(0, 388.0f);
    tMain = tr1->AddComponent<portfolio::TriggerComponent>(86.0f, 168.0f);
    tMain->SetTarget(outPlayer, 88.0f, 120.0f);
    scene.Add(std::move(tr1));

    AddMuteIconsToScene(scene);
}

// PROJECT SCENE GENERATOR
std::function<void()> CreateSingleProjectScene(portfolio::GameObject* projectsPlayerPtr, int targetSceneIndex, int projectNumber, const std::string& bgName)
{
    auto& scene = portfolio::SceneManager::GetInstance().GetScene(targetSceneIndex);

    std::vector<std::string> imageFiles;

    int numberOfImages = 0;
    if (projectNumber == 1) numberOfImages = 9;
    else if (projectNumber == 2) numberOfImages = 6;
    else if (projectNumber == 3) numberOfImages = 8;
    else if (projectNumber == 4) numberOfImages = 4;
    else if (projectNumber == 5) numberOfImages = 1;
    else if (projectNumber == 6) numberOfImages = 5;
    else if (projectNumber == 7) numberOfImages = 8;
    else if (projectNumber == 8) numberOfImages = 3;
    else if (projectNumber == 9) numberOfImages = 7;

    std::vector<std::string> ytLinks;
    ytLinks.resize(numberOfImages, "");

    for (int i = 1; i <= numberOfImages; ++i)
    {
        std::string numberPrefix = (i < 10) ? "0" + std::to_string(i) : std::to_string(i);
        std::string relativePath = "Proj" + std::to_string(projectNumber) + "/P" + std::to_string(projectNumber) + "_" + numberPrefix;
        
        std::string gifPath = relativePath + ".gif";
        std::string pngPath = relativePath + ".png";
        std::string txtPath = relativePath + ".txt";

        std::filesystem::path dataPath = portfolio::ResourceManager::GetInstance().GetDataPath();

        if (std::filesystem::exists(dataPath / txtPath)) 
        {
            std::ifstream file(dataPath / txtPath);
            std::string link;
            if (std::getline(file, link)) 
            {
                link.erase(std::remove(link.begin(), link.end(), '\r'), link.end());
                link.erase(std::remove(link.begin(), link.end(), '\n'), link.end());
                link.erase(std::remove(link.begin(), link.end(), ' '), link.end());
                
                size_t pos = link.find("youtu.be/");
                if (pos != std::string::npos) 
                {
                    link.replace(pos, 9, "www.youtube.com/embed/");
                    link += "?autoplay=1&mute=0";
                }
                else 
                {
                    pos = link.find("youtube.com/watch?v=");
                    if (pos != std::string::npos) 
                    {
                        link.replace(pos, 20, "www.youtube.com/embed/");
                        link += "?autoplay=1&mute=0";
                    }
                }
                ytLinks[i - 1] = link;
            }
        }

        if (std::filesystem::exists(dataPath / gifPath)) 
        {
            imageFiles.push_back(gifPath);
        } 
        else 
        {
            imageFiles.push_back(pngPath);
        }
    }

    auto slides = std::make_shared<std::vector<portfolio::GameObject*>>();
    auto currentIndex = std::make_shared<int>(0);
    auto lastSwitchTime = std::make_shared<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());

    glm::vec2 carouselPos = { 69.0f, 69.0f };

    for (size_t i = 0; i < imageFiles.size(); ++i)
    {
        auto slideObj = std::make_unique<portfolio::GameObject>();
        
        if (imageFiles[i].length() > 4 && imageFiles[i].substr(imageFiles[i].length() - 4) == ".gif") 
        {
            auto comp = slideObj->AddComponent<portfolio::GifComponent>(imageFiles[i]);
            comp->SetFillSize(875.0f, 515.0f);
        } 
        else 
        {
            auto comp = slideObj->AddComponent<portfolio::RenderComponent>(imageFiles[i]);
            comp->SetFillSize(875.0f, 515.0f);
        }

        if (i == 0)
        {
            slideObj->SetLocalPosition(carouselPos.x, carouselPos.y);
        }
        else
        {
            slideObj->SetLocalPosition(-2000.0f, -2000.0f);
        }

        slides->push_back(slideObj.get());
        scene.Add(std::move(slideObj));
    }

    auto bg = std::make_unique<portfolio::GameObject>();
    auto bgComp = bg->AddComponent<portfolio::RenderComponent>(bgName);
    bgComp->SetScale(1366.0f / 1920.0f); // Scale down the 1920x1080 image to match 1366x768 canvas
    scene.Add(std::move(bg));

    auto updateYtOverlay = [ytLinks, currentIndex]() 
    {
#ifdef __EMSCRIPTEN__
        std::string link = ytLinks[*currentIndex];
        if (!link.empty()) 
        {
            std::string js = "document.getElementById('video-container').style.display = 'block'; " "document.getElementById('yt-iframe').src = '" + link + "';";
            emscripten_run_script(js.c_str());
        } 
        else 
        {
            emscripten_run_script("document.getElementById('video-container').style.display = 'none'; " "document.getElementById('yt-iframe').src = '';");
        }
#endif
    };

    auto onEsc = [projectsPlayerPtr]()
    {
         std::cout << "Returning to Projects...\n";
#ifdef __EMSCRIPTEN__
         emscripten_run_script("document.getElementById('video-container').style.display = 'none'; " "document.getElementById('yt-iframe').src = '';");
#endif
         portfolio::SceneManager::GetInstance().TransitionToScene(3, [projectsPlayerPtr]()
         {
             std::vector<SDL_FRect> projectsWoodZones = { SDL_FRect{ 636.0f, 0.0f, 100.0f, 272.0f } };
             if (auto pc = projectsPlayerPtr->GetComponent<portfolio::PlayerControllerComponent>()) 
             {
                 pc->CancelAutoWalk();
                 pc->SetSpeed(150.0f);
             }
             BindPlayerInputs(projectsPlayerPtr, true);
        });
    };

    auto onQ = [slides, currentIndex, carouselPos, lastSwitchTime, updateYtOverlay]()
    {
         if (slides->empty()) return;

         auto now = std::chrono::steady_clock::now();
         if (std::chrono::duration_cast<std::chrono::milliseconds>(now - *lastSwitchTime).count() < 250) return;
         *lastSwitchTime = now;

         (*slides)[*currentIndex]->SetLocalPosition(-2000.0f, -2000.0f);
         int totalSlides = static_cast<int>(slides->size());
         *currentIndex = (*currentIndex - 1 + totalSlides) % totalSlides;
         (*slides)[*currentIndex]->SetLocalPosition(carouselPos.x, carouselPos.y);
            
         updateYtOverlay();
    };

    auto onE = [slides, currentIndex, carouselPos, lastSwitchTime, updateYtOverlay]()
    {
        if (slides->empty()) return;

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - *lastSwitchTime).count() < 250) return;
        *lastSwitchTime = now;

        (*slides)[*currentIndex]->SetLocalPosition(-2000.0f, -2000.0f);
        int totalSlides = static_cast<int>(slides->size());
        *currentIndex = (*currentIndex + 1) % totalSlides;
        (*slides)[*currentIndex]->SetLocalPosition(carouselPos.x, carouselPos.y);
            
        updateYtOverlay();
    };

    AddMuteIconsToScene(scene);
    return [targetSceneIndex, onEsc, onQ, onE, updateYtOverlay]()
    {
        std::cout << "Loading Project Screen...\n";

        portfolio::SceneManager::GetInstance().TransitionToScene(targetSceneIndex, [onEsc, onQ, onE, updateYtOverlay]()
        {
                BindProjectViewInputs(onEsc, onQ, onE);
                updateYtOverlay();
        });
    };
}

void LoadProjectsScene(portfolio::GameObject*& outPlayer, portfolio::TriggerComponent*& tMain)
{
    auto& scene = portfolio::SceneManager::GetInstance().GetScene(3);

    auto bg = std::make_unique<portfolio::GameObject>();
    bg->AddComponent<portfolio::RenderComponent>("ProjectsBackground.png");
    scene.Add(std::move(bg));

    auto player = std::make_unique<portfolio::GameObject>();
    player->AddComponent<portfolio::SpriteComponent>("PlayerSprite.png", 3, 3, 0.1f);
    player->AddComponent<portfolio::PlayerControllerComponent>();
    outPlayer = player.get();
    scene.Add(std::move(player));

    auto trMain = std::make_unique<portfolio::GameObject>();
    trMain->SetLocalPosition(624.0f, 0.0f);
    tMain = trMain->AddComponent<portfolio::TriggerComponent>(124.0f, 84.0f);
    tMain->SetTarget(outPlayer, 88.0f, 120.0f);
    scene.Add(std::move(trMain));

    struct ProjectInfo
    {
        glm::vec2 popupPos;
        std::string bgImageName;
    };

    std::vector<ProjectInfo> projects = 
    {
        { {1196.0f, 305.0f}, "Proj1_Bg.png" },
        { {1149.0f, 631.0f}, "Proj2_Bg.png" },
        { {522.0f, 457.0f}, "Proj3_Bg.png" },
        { {248.0f, 635.0f}, "Proj4_Bg.png" },
        { {195.0f, 293.0f}, "Proj5_Bg.png" },
        { {748.0f, 667.0f}, "Proj6_Bg.png" },
        { {895.0f, 423.0f}, "Proj7_Bg.png" },
        { {407.0f, 111.0f}, "Proj8_Bg.png" },
        { {1011.0f, 133.0f}, "Proj9_Bg.png" }
    };

    for (size_t i = 0; i < projects.size(); ++i)
    {
        auto flowerObj = std::make_unique<portfolio::GameObject>();
        flowerObj->SetLocalPosition(projects[i].popupPos.x - 120.0f, projects[i].popupPos.y - 50.0f);
        auto tComp = flowerObj->AddComponent<portfolio::TriggerComponent>(180.0f, 180.0f);
        tComp->SetTarget(outPlayer, 88.0f, 120.0f);

        glm::vec2 customPopupPos = projects[i].popupPos;
        auto popupObj = std::make_unique<portfolio::GameObject>();
        popupObj->AddComponent<portfolio::RenderComponent>("PressE.png");
        popupObj->SetLocalPosition(-2000.0f, -2000.0f);
        auto popupPtr = popupObj.get();
        scene.Add(std::move(popupObj));

        tComp->SetOnTriggerEnter([popupPtr, customPopupPos]()
        {
            popupPtr->SetLocalPosition(customPopupPos.x, customPopupPos.y);
        });

        tComp->SetOnTriggerExit([popupPtr]()
        {
            popupPtr->SetLocalPosition(-2000.0f, -2000.0f);
        });

        int targetSceneIndex = static_cast<int>(4 + i);
        int projectNumber = static_cast<int>(i + 1);
        std::string bgImageName = projects[i].bgImageName;
        
        g_ProjectInteractions.push_back({ tComp, [targetSceneIndex, projectNumber, bgImageName, outPlayer]()
        {
            static std::function<void()> transitionFuncs[10];
            
            if (!transitionFuncs[projectNumber]) 
            {
                std::cout << "Building Project Scene " << projectNumber << "...\n";
                transitionFuncs[projectNumber] = CreateSingleProjectScene(outPlayer, targetSceneIndex, projectNumber, bgImageName);
            }
            
            transitionFuncs[projectNumber]();
        } });
        scene.Add(std::move(flowerObj));
    }

    AddMuteIconsToScene(scene);
}

// LOAD GAME & MAIN
void load()
{
    std::cout << "Welcome to the Portfolio!\n";

    for (int i = 0; i < 13; ++i)
    {
        portfolio::SceneManager::GetInstance().CreateScene();
    }

    std::string dataPath = "";
#ifdef __EMSCRIPTEN__
    dataPath = "";
#else
    if (std::filesystem::exists("./Data/")) dataPath = "./Data/";
    else dataPath = "../Data/";
#endif

    // LOAD AUDIO SYSTEM
    auto audioSystem = std::make_unique<portfolio::MiniaudioSoundSystem>();
    portfolio::ServiceLocator::register_sound_system(std::make_unique<portfolio::LoggingSoundSystem>(std::move(audioSystem)));
    auto& ss = portfolio::ServiceLocator::get_sound_system();

    ss.loadSound(0, dataPath + "AnimalCrossingNewHorizonsMainTheme.mp3");
    ss.play(0, 0.25f);

    // Wood Footsteps (1-5)
    ss.loadSound(1, dataPath + "SoundEffects/Footstep_Wood_00_Ac.wav");
    ss.loadSound(2, dataPath + "SoundEffects/Footstep_Wood_01_Ac.wav");
    ss.loadSound(3, dataPath + "SoundEffects/Footstep_Wood_02_Ac.wav");
    ss.loadSound(4, dataPath + "SoundEffects/Footstep_Wood_03_Ac.wav");
    ss.loadSound(5, dataPath + "SoundEffects/Footstep_Wood_04_Ac.wav");
    // Grass Footsteps (6-10)
    ss.loadSound(6, dataPath + "SoundEffects/Footstep_Grass_00_Ac.wav");
    ss.loadSound(7, dataPath + "SoundEffects/Footstep_Grass_01_Ac.wav");
    ss.loadSound(8, dataPath + "SoundEffects/Footstep_Grass_02_Ac.wav");
    ss.loadSound(9, dataPath + "SoundEffects/Footstep_Grass_03_Ac.wav");
    ss.loadSound(10, dataPath + "SoundEffects/Footstep_Grass_04_Ac.wav");

    // Jump Transitions (I11-12)
    ss.loadSound(11, dataPath + "SoundEffects/Jump_Wood_00.wav");
    ss.loadSound(12, dataPath + "SoundEffects/Jump_Grass_00.wav");

    portfolio::GameObject* p1, * p2, * p3, * p4;
    portfolio::TriggerComponent* tMainToAbout, * tMainToContact, * tMainToProj;
    portfolio::TriggerComponent* tAboutToMain, * tContactToMain, * tProjToMain;

    LoadMainMenu(p1, tMainToAbout, tMainToContact, tMainToProj);
    LoadAboutScene(p2, tAboutToMain);
    LoadContactScene(p3, tContactToMain);
    LoadProjectsScene(p4, tProjToMain);

    std::vector<SDL_FRect> mainScenePlanks =
    {
        SDL_FRect{ 632.0f, 0.0f, 108.0f, 768.0f },
        SDL_FRect{ 740.0f, 448.0f, 626.0f, 92.0f }
    };
    std::vector<SDL_FRect> aboutScenePlanks = { SDL_FRect{ 632.0f, 484.0f, 108.0f, 284.0f } };
    std::vector<SDL_FRect> contactScenePlanks = { SDL_FRect{ 0.0f, 460.0f, 612.0f, 92.0f } };

    std::vector<SDL_FRect> projectsWoodZones = { SDL_FRect{ 636.0f, 0.0f, 100.0f, 272.0f } };

    tMainToAbout->SetOnTriggerEnter([p2, aboutScenePlanks]()
    {
        portfolio::InputManager::GetInstance().UnbindAll();
        portfolio::SceneManager::GetInstance().TransitionToScene(1, [p2, aboutScenePlanks]()
        {
            p2->SetLocalPosition(642.5f, 768.0f - 160.0f);
            if (auto pc = p2->GetComponent<portfolio::PlayerControllerComponent>()) 
            {
                pc->ConfigureZones(aboutScenePlanks, true, {});
                pc->CancelAutoWalk();
                pc->SetSpeed(150.0f);
            }
            BindPlayerInputs(p2, false);
        });
    });

    tMainToContact->SetOnTriggerEnter([p3, contactScenePlanks]()
    {
        portfolio::InputManager::GetInstance().UnbindAll();
        portfolio::SceneManager::GetInstance().TransitionToScene(2, [p3, contactScenePlanks]()
        {
            p3->SetLocalPosition(150.0f, 400.0f);
            if (auto pc = p3->GetComponent<portfolio::PlayerControllerComponent>()) 
            {
                pc->ConfigureZones(contactScenePlanks, true, {});
                pc->CancelAutoWalk();
                pc->SetSpeed(150.0f);
            }
            BindPlayerInputs(p3, false);
        });
    });

    tMainToProj->SetOnTriggerEnter([p4, projectsWoodZones]()
    {
        portfolio::InputManager::GetInstance().UnbindAll();
        portfolio::SceneManager::GetInstance().TransitionToScene(3, [p4, projectsWoodZones]()
        {
            p4->SetLocalPosition(642.5f, 95.0f);
            if (auto pc = p4->GetComponent<portfolio::PlayerControllerComponent>()) 
            {
                pc->ConfigureZones({}, false, projectsWoodZones);
                pc->CancelAutoWalk();
                pc->SetSpeed(150.0f);
            }
			BindPlayerInputs(p4, true);
        });
    });

    tAboutToMain->SetOnTriggerEnter([p1, mainScenePlanks]()
    {
        portfolio::InputManager::GetInstance().UnbindAll();
        portfolio::SceneManager::GetInstance().TransitionToScene(0, [p1, mainScenePlanks]()
        {
            p1->SetLocalPosition(642.5f, 95.0f);
            if (auto pc = p1->GetComponent<portfolio::PlayerControllerComponent>()) 
            {
                pc->ConfigureZones(mainScenePlanks, true, {});
                pc->CancelAutoWalk();
                pc->SetSpeed(150.0f);
            }
            BindPlayerInputs(p1, false);
        });
    });

    tContactToMain->SetOnTriggerEnter([p1, mainScenePlanks]()
    {
        portfolio::InputManager::GetInstance().UnbindAll();
        portfolio::SceneManager::GetInstance().TransitionToScene(0, [p1, mainScenePlanks]()
        {
            p1->SetLocalPosition(1366.0f - 250.0f, 400.0f);
            if (auto pc = p1->GetComponent<portfolio::PlayerControllerComponent>()) 
            {
                pc->ConfigureZones(mainScenePlanks, true, {});
                pc->CancelAutoWalk();
                pc->SetSpeed(150.0f);
            }
            BindPlayerInputs(p1, false);
        });
    });

    tProjToMain->SetOnTriggerEnter([p1, mainScenePlanks]()
    {
        portfolio::InputManager::GetInstance().UnbindAll();
        portfolio::SceneManager::GetInstance().TransitionToScene(0, [p1, mainScenePlanks]()
        {
            p1->SetLocalPosition(642.5f, 768.0f - 250.0f);
            if (auto pc = p1->GetComponent<portfolio::PlayerControllerComponent>()) 
            {
                pc->ConfigureZones(mainScenePlanks, true, {});
                pc->CancelAutoWalk();
                pc->SetSpeed(150.0f);
            }
            BindPlayerInputs(p1, false);
        });
    });

    // START GAME
    if (auto pc = p1->GetComponent<portfolio::PlayerControllerComponent>()) 
    {
        pc->ConfigureZones(mainScenePlanks, true, {});
    }
    BindPlayerInputs(p1, false);
    portfolio::SceneManager::GetInstance().SetActiveScene(0);
}

int main(int, char* [])
{
    std::cout << "Starting Portfolio Engine...\n";

#ifdef __EMSCRIPTEN__
    fs::path data_location = "";
#else
    fs::path data_location = "./Data/";
    if (!fs::exists(data_location))
    {
        data_location = "../Data/";
    }
#endif

    portfolio::Minigin engine(data_location);
    engine.Run(load);

    return 0;
}