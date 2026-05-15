#pragma once

struct SDL_Window;

namespace engine
{
    class Application
    {
        public:
            void init();
            void run();

        private:
            SDL_Window* m_window = nullptr;
            bool m_running = false;  
    };
}