#include <engine/core/Application/Application.h>
#include <engine/core/Renderer/IRenderer.h>
#include <engine/core/Renderer/Types.h>
#include <engine/platform/SdlWindowSurface.h>
#include <SDL3/SDL.h>

namespace {
    constexpr const char* kWindowTitle = "NeoLab2D";
    constexpr int kWindowWidth = 1280;
    constexpr int kWindowHeight = 720;
    constexpr int kWindowMinWidth = 800;
    constexpr int kWindowMinHeight = 600;

    // Test geometry
    float triangleVertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
}

engine::Application::Application(std::unique_ptr<engine::IRenderer> renderer)
    : m_renderer(std::move(renderer))
{
}

engine::Application::~Application(){
    cleanUp();
}

void engine::Application::cleanUp(){
    if (m_renderer) {
        m_renderer->shutdown();
    }

    if(m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    if (m_sdlInitialized) {
        SDL_Quit();
        m_sdlInitialized = false;
    }

    m_running = false;
}

static SDL_PropertiesID createWindowProps(){
    SDL_PropertiesID props = SDL_CreateProperties();

    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, kWindowTitle);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, kWindowWidth);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, kWindowHeight);
    
    return props;
}

void engine::Application::run() {
    while (m_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_running = false;
            }

            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                m_renderer->resize(event.window.data1, event.window.data2);
            }
        }


        engine::GeometryData testGeom;
        testGeom.vertices = triangleVertices;
        testGeom.vertexByteSize = sizeof(triangleVertices);
        testGeom.vertexCount = _countof(triangleVertices) / 3;
        testGeom.primitive = engine::PrimitiveType::Triangles;



        m_renderer->beginFrame();
        m_renderer->drawGeometry(testGeom);
        m_renderer->endFrame();
    } 
    return;
}

void engine::Application::init() {
    if (!m_renderer) {
        SDL_Log("Application init failed: renderer dependency is null");
        m_running = false;
        return;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        m_running = false;
        return;
    }
    m_sdlInitialized = true;

    m_renderer->configureContextAttributes();

    SDL_PropertiesID windowProps = createWindowProps();
    if(!windowProps){
        SDL_Log("Failed to create window properties: %s", SDL_GetError());
        cleanUp();
        return;
    }

    m_window = SDL_CreateWindowWithProperties(windowProps);
    SDL_DestroyProperties(windowProps);

    if(!m_window){
        SDL_Log("Failed to create window: %s", SDL_GetError());
        cleanUp();
        return;
    }
    SDL_SetWindowMinimumSize(m_window, kWindowMinWidth, kWindowMinHeight);

    engine::SdlWindowSurface surface(m_window);
    if (!m_renderer->init(surface)) {
        cleanUp();
        return;
    }

    m_running = true;
}