#pragma once

#include <memory>

namespace engine
{
    class IAppHost;
    class IRenderer;
    class IScene;

    class Application
    {
    public:
        Application(
            std::unique_ptr<IRenderer> renderer,
            std::unique_ptr<IAppHost> appHost,
            std::unique_ptr<IScene> scene
        );
        ~Application();
        void init();
        void run();

    private:
        std::unique_ptr<IRenderer> m_renderer;
        std::unique_ptr<IAppHost> m_appHost;
        std::unique_ptr<IScene> m_scene;
        bool m_sceneInitialized = false;
        bool m_running = false;
        void cleanUp();
    };
}