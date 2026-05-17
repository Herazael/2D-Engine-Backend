#pragma once

#include <engine/core/Application/AppEvent.h>
#include <engine/core/Renderer/RenderSurface.h>

namespace engine
{
    class IAppHost
    {
    public:
        virtual ~IAppHost() = default;

        virtual bool initialize() = 0;
        virtual bool pollEvent(AppEvent& outEvent) = 0;
        virtual RenderSurface getRenderSurface() const = 0;
        virtual void shutdown() = 0;
    };
}