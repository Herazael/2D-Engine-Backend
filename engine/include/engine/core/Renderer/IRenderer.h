#pragma once

namespace engine
{
    class IWindowSurface;

    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        virtual void configureContextAttributes() = 0;
        virtual bool init(IWindowSurface& surface) = 0;
        virtual void beginFrame() = 0;
        virtual void endFrame() = 0;
        virtual void resize(int width, int height) = 0;
        virtual void shutdown() = 0;
    };
}
