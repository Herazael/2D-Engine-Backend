#pragma once

namespace engine
{
    class IRenderer;

    class IScene
    {
    public:
        virtual ~IScene() = default;

        virtual bool initialize(IRenderer& renderer) = 0;
        virtual void render(IRenderer& renderer) = 0;
        virtual void shutdown(IRenderer& renderer) = 0;
        virtual void finalDraw(IRenderer& renderer) = 0;
    };
}