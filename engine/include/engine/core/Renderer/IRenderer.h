#pragma once

#include <engine/core/Renderer/Types.h>
#include <engine/core/Renderer/IWindowSurface.h>

namespace engine
{
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        virtual void configureContextAttributes() = 0;
        virtual bool init(const IWindowSurface& surface) = 0;
        virtual void beginFrame() = 0;
        virtual void endFrame() = 0;
        virtual void resize(int width, int height) = 0;
        virtual void shutdown() = 0;
        virtual void drawSprite(const SpriteData& sprite) = 0;
        virtual void drawGeometry(const GeometryData& geometry) = 0;
        virtual TextureHandle loadTexture(const char* path) = 0;
        virtual bool buildAndFlushBatch() = 0;
    };
}
