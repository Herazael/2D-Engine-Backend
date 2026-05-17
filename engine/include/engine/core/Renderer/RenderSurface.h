#pragma once

namespace engine
{
    struct RenderSurface
    {
        void* nativeWindowHandle = nullptr;
        int width = 0;
        int height = 0;
    };
}