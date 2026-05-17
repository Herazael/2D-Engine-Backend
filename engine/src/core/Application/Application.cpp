#include <engine/core/Application/Application.h>
#include <engine/core/Application/AppEvent.h>
#include <engine/core/Application/IAppHost.h>
#include <engine/core/Application/IScene.h>
#include <engine/core/Renderer/IRenderer.h>

namespace
{
    void clampSize(int& width, int& height)
    {
        if (width < 1) {
            width = 1;
        }
        if (height < 1) {
            height = 1;
        }
    }
}

engine::Application::~Application(){
    cleanUp();
}

engine::Application::Application(
    std::unique_ptr<engine::IRenderer> renderer,
    std::unique_ptr<engine::IAppHost> appHost,
    std::unique_ptr<engine::IScene> scene
)
    : m_renderer(std::move(renderer)),
      m_appHost(std::move(appHost)),
      m_scene(std::move(scene)) {}

void engine::Application::cleanUp()
{
    if (m_sceneInitialized && m_scene && m_renderer) {
        m_scene->shutdown(*m_renderer);
        m_sceneInitialized = false;
    }

    if (m_renderer) {
        m_renderer->shutdown();
    }

    if (m_appHost) {
        m_appHost->shutdown();
    }

    m_running = false;
}

void engine::Application::run() {
    while (m_running) {
        AppEvent event;
        while (m_appHost->pollEvent(event)) {
            if (event.type == AppEventType::Quit) {
                m_running = false;
            }

            if (event.type == AppEventType::WindowResized) {
                int width = event.width;
                int height = event.height;
                clampSize(width, height);
                m_renderer->resize(width, height);
            }
        }

        m_renderer->beginFrame();
        if (m_scene) {
            m_scene->render(*m_renderer);
        }
        m_renderer->endFrame();
    }
}

void engine::Application::init()
{
    if (!m_renderer || !m_appHost || !m_scene) {
        m_running = false;
        return;
    }

    if (!m_appHost->initialize()) {
        m_running = false;
        return;
    }

    const RenderSurface renderSurface = m_appHost->getRenderSurface();
    if (renderSurface.nativeWindowHandle == nullptr) {
        cleanUp();
        return;
    }

    if (!m_renderer->init(renderSurface)) {
        cleanUp();
        return;
    }

    if (!m_scene->initialize(*m_renderer)) {
        cleanUp();
        return;
    }

    m_sceneInitialized = true;

    int width = renderSurface.width;
    int height = renderSurface.height;
    clampSize(width, height);
    m_renderer->resize(width, height);

    m_running = true;
}