#include <engine/platform/SdlAppHost.h>

#include <SDL3/SDL.h>

namespace
{
    constexpr const char* kWindowTitle = "NeoLab2D";
    constexpr int kWindowWidth = 1280;
    constexpr int kWindowHeight = 720;
    constexpr int kWindowMinWidth = 800;
    constexpr int kWindowMinHeight = 600;
}

bool engine::SdlAppHost::initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    m_sdlInitialized = true;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_PropertiesID windowProps = SDL_CreateProperties();
    if (!windowProps) {
        SDL_Log("Failed to create window properties: %s", SDL_GetError());
        shutdown();
        return false;
    }

    SDL_SetStringProperty(windowProps, SDL_PROP_WINDOW_CREATE_TITLE_STRING, kWindowTitle);
    SDL_SetBooleanProperty(windowProps, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(windowProps, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetNumberProperty(windowProps, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, kWindowWidth);
    SDL_SetNumberProperty(windowProps, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, kWindowHeight);

    m_window = SDL_CreateWindowWithProperties(windowProps);
    SDL_DestroyProperties(windowProps);

    if (!m_window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        shutdown();
        return false;
    }

    SDL_SetWindowMinimumSize(m_window, kWindowMinWidth, kWindowMinHeight);
    return true;
}

bool engine::SdlAppHost::pollEvent(engine::AppEvent& outEvent)
{
    SDL_Event event;
    if (!SDL_PollEvent(&event)) {
        return false;
    }

    outEvent = {};

    if (event.type == SDL_EVENT_QUIT) {
        outEvent.type = AppEventType::Quit;
        return true;
    }

    if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        outEvent.type = AppEventType::WindowResized;
        outEvent.width = event.window.data1;
        outEvent.height = event.window.data2;
        return true;
    }

    return true;
}

engine::RenderSurface engine::SdlAppHost::getRenderSurface() const
{
    RenderSurface surface;
    surface.nativeWindowHandle = m_window;
    surface.width = 0;
    surface.height = 0;

    if (m_window) {
        SDL_GetWindowSize(m_window, &surface.width, &surface.height);
    }

    return surface;
}

void engine::SdlAppHost::shutdown()
{
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    if (m_sdlInitialized) {
        SDL_Quit();
        m_sdlInitialized = false;
    }
}