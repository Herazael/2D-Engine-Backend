#pragma once

#include <memory>

struct SDL_Window;

namespace engine
{
    class IRenderer;

    class Application
    {
        public:
            explicit Application(std::unique_ptr<IRenderer> renderer);
            ~Application();
            void init();
            void run();

        private:   
            SDL_Window* m_window = nullptr;
            std::unique_ptr<IRenderer> m_renderer;
            bool m_running = false;
            bool m_sdlInitialized = false;
            void cleanUp();
    };
}