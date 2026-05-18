#pragma once

#include <engine/core/Renderer/IWindowSurface.h>

namespace engine
{
    struct RenderSurface : public IWindowSurface
    {
        void* nativeWindowHandle = nullptr;
        int width = 0;
        int height = 0;

        void* getNativeWindowHandle() const override { return nativeWindowHandle; }
        int getWidth() const override { return width; }
        int getHeight() const override { return height; }
    };
}