cmake_minimum_required(VERSION 3.26)
project(Minigin)

if(EMSCRIPTEN)
  message("Using Emscripten!")
endif()

if(EMSCRIPTEN)
  set(TARGET_NAME AnaCarvalho-GamePortfolio)
  set(TARGET_PARAM )
else()
  set(TARGET_NAME Portfolio)
endif()

# ENGINE LIBRARY (Minigin)

add_library(Minigin STATIC
  Minigin/GameObject.cpp
  Minigin/Gamepad.cpp
  Minigin/InputManager.cpp
  Minigin/Minigin.cpp
  Minigin/RenderComponent.cpp
  Minigin/Renderer.cpp
  Minigin/ResourceManager.cpp
  Minigin/Scene.cpp
  Minigin/SceneManager.cpp
  Minigin/Texture2D.cpp
  Minigin/Transform.cpp
  Minigin/MiniaudioSoundSystem.cpp 
  Minigin/ServiceLocator.cpp 
  Minigin/SpriteComponent.cpp 
  Minigin/TriggerComponent.cpp
  Minigin/GifComponent.cpp)

target_compile_features(Minigin PUBLIC cxx_std_20)

target_include_directories(Minigin PUBLIC ${CMAKE_SOURCE_DIR}/Minigin)

# ---------------------------------------------------

# PORTFOLIO EXECUTABLE
add_executable(${TARGET_NAME} ${TARGET_PARAM}
  Portfolio/Main.cpp
)

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)

# ---------------------------------------------------

include(FetchContent)

# glm
find_package(glm CONFIG QUIET)
if(NOT glm_FOUND)
  FetchContent_Declare(
    glm
    URL https://github.com/g-truc/glm/releases/download/1.0.3/glm-1.0.3.zip
    DOWNLOAD_NO_PROGRESS ON
    DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
  )
  FetchContent_MakeAvailable(glm)
endif()

# SDL3
find_package(SDL3 3.4 CONFIG QUIET)
if(NOT SDL3_FOUND)
  if (WIN32)
    FetchContent_Declare(
      SDL3
      URL https://www.libsdl.org/release/SDL3-devel-3.4.0-VC.zip
      DOWNLOAD_NO_PROGRESS ON
      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
    )
    FetchContent_MakeAvailable(SDL3)
    list(PREPEND CMAKE_PREFIX_PATH "${sdl3_SOURCE_DIR}")
    find_package(SDL3 CONFIG REQUIRED)
  else()
    FetchContent_Declare(
      SDL3
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG release-3.4.0
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(SDL3)
  endif()
endif()

# ImGui and SDL3_ttf removed for efficiency

if (WIN32)
  find_package(VLD CONFIG)
endif()

# ---------------------------------------------------

# LINKING DEPENDENCIES
# ImGui removed

target_link_libraries(Minigin PUBLIC
  SDL3::SDL3
  glm::glm
)

target_include_directories(${TARGET_NAME} PUBLIC
  ${VLD_INCLUDE_DIR}
)

target_link_libraries(${TARGET_NAME} PRIVATE 
  Minigin
  ${VLD_LIBRARIES}
)

# ------------------------------------------------

if(MSVC)
  target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Werror)
endif()

if(EMSCRIPTEN)
  set(EMSCRIPTEN_COMPILE_FLAGS "${EMSCRIPTEN_LINK_FLAGS}")
  set(EMSCRIPTEN_LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS} -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sALLOW_MEMORY_GROWTH --preload-file \"${CMAKE_SOURCE_DIR}/Data@/\" --shell-file \"${CMAKE_SOURCE_DIR}/web/shell.html\"")

  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmscriptenToolchain.cmake)

  set_target_properties(${TARGET_NAME} PROPERTIES
      COMPILE_FLAGS "${EMSCRIPTEN_COMPILE_FLAGS}"
      LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS}"
  )

  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/web" "$<TARGET_FILE_DIR:${TARGET_NAME}>/"
  )

  set(CMAKE_EXECUTABLE_SUFFIX ".html")

elseif(WIN32)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TARGET_NAME})
  set_target_properties(${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>")

  set(SDL_RUNTIME_DLLS
    SDL3::SDL3
  )

  foreach(dllTarget IN LISTS SDL_RUNTIME_DLLS)
    add_custom_command(
      TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE:${dllTarget}>"
          "$<TARGET_FILE_DIR:${TARGET_NAME}>"
      VERBATIM
    )
  endforeach()

  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Data" "$<TARGET_FILE_DIR:${TARGET_NAME}>/Data"
  )
elseif(APPLE)
  set_target_properties(${TARGET_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    INSTALL_RPATH "@executable_path/../Frameworks"
    BUILD_WITH_INSTALL_RPATH TRUE
  )
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3-shared>" "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>/Contents/Frameworks/"
    COMMAND install_name_tool -change "@rpath/libSDL3.0.dylib" "@executable_path/../Frameworks/libSDL3.0.dylib" "$<TARGET_FILE:${TARGET_NAME}>"
  )

  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Data" "$<TARGET_FILE_DIR:${TARGET_NAME}>/Data"
  )
else()
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Data" "$<TARGET_FILE_DIR:${TARGET_NAME}>/Data"
  )
endif()