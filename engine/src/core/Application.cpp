#include <engine/core/Application.h>
#include <SDL3/SDL.h>

static SDL_PropertiesID createWindowProps(){
    SDL_PropertiesID props = SDL_CreateProperties();
    if(props == 0) {
        SDL_Log("Unable to create properties: %s", SDL_GetError());
        return 0;
    }

    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "NeoLab2D");
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1280);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 720);
    return props;
}

void engine::Application::init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        m_running = false;
        return;
    }    

    m_window = SDL_CreateWindowWithProperties(createWindowProps());
    if(m_window == 0){
        SDL_Quit();
        m_running = false;
        return;
    }
    SDL_SetWindowMinimumSize(m_window, 800, 600);
    m_running = true;
}

void engine::Application::run() {
    while (m_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_running = false;
            }
        }
    }

    if(m_window) {
        SDL_DestroyWindow(m_window);
    }

    SDL_Quit();
    return;
}