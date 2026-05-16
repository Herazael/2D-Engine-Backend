#pragma once

#include <engine/platform/IWindowSurface.h>

struct SDL_Window;

namespace engine
{
    class SdlWindowSurface final : public IWindowSurface
    {
    public:
        explicit SdlWindowSurface(SDL_Window* window);

        SDL_Window* getWindowHandle() const override;
        void getSize(int& width, int& height) const override;

    private:
        SDL_Window* m_window = nullptr;
    };
}
