#pragma once

#include <engine/core/Renderer/IRenderer.h>

#include <glad/gl.h>
#include <SDL3/SDL_video.h>

#include <vector>
#include <map>

struct SDL_Window;

namespace engine
{
    class SpriteRenderer final : public IRenderer
    {
    public:
        ~SpriteRenderer() override;

        void configureContextAttributes() override;
        bool init(const IWindowSurface& surface) override;
        void beginFrame() override;
        void endFrame() override;
        void resize(int width, int height) override;
        void shutdown() override;
        void drawSprite(const SpriteData& sprite) override;
        void drawGeometry(const GeometryData& geometry) override;
        TextureHandle loadTexture(const char* path) override;
        bool buildAndFlushBatch() override;

    private:
        bool compileShader();
        bool initSpriteResources();
        bool initGeometryResources();
        SDL_Window* m_window = nullptr;
        SDL_GLContext m_context = nullptr;
        GLuint m_geometryProgram = 0;
        GLuint m_geometryVao = 0;
        GLuint m_geometryVbo = 0;
        GLuint m_geometryEbo = 0;
        GLuint m_spriteProgram = 0;
        GLuint m_spriteVao = 0;
        GLuint m_spriteVbo = 0;
        GLuint m_spriteEbo = 0;
        std::vector<SpriteData> m_spriteBatch;              
        std::vector<SpriteBatchEntry> m_batchEntries; 
        std::vector<GLuint> m_ownedTextures;
        std::map<TextureHandle, std::vector<int>> m_textureGroups;
        int m_viewportWidth = 1;
        int m_viewportHeight = 1;
        bool m_initialized = false;
    };
}