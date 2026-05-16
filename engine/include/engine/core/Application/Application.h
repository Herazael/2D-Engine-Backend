#pragma once

#include <SDL3/SDL_video.h>

namespace engine
{
    class Application
    {
        public:
            ~Application();
            void init();
            void run();

        private:   
            SDL_Window* m_window = nullptr;
            SDL_GLContext m_context = nullptr;
            bool m_running = false;  
            void cleanUp();
    };
}