#include <engine/platform/SdlWindowSurface.h>
#include <SDL3/SDL_video.h>

engine::SdlWindowSurface::SdlWindowSurface(SDL_Window* window)
    : m_window(window){}

SDL_Window* engine::SdlWindowSurface::getWindowHandle() const
{
    return m_window;
}

void engine::SdlWindowSurface::getSize(int& width, int& height) const
{
    width = 0;
    height = 0;

    if (m_window) {
        SDL_GetWindowSize(m_window, &width, &height);
    }
}
