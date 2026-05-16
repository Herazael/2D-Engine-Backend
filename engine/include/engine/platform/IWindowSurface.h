#pragma once

struct SDL_Window;

namespace engine
{
    class IWindowSurface
    {
    public:
        virtual ~IWindowSurface() = default;

        virtual SDL_Window* getWindowHandle() const = 0;
        virtual void getSize(int& width, int& height) const = 0;
    };
}
