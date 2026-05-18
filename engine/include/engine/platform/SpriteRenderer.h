#pragma once

#include <engine/core/Renderer/IRenderer.h>

#include <glad/gl.h>
#include <SDL3/SDL_video.h>

#include <vector>

struct SDL_Window;

namespace engine
{
    class SpriteRenderer final : public IRenderer
    {
    public:
        ~SpriteRenderer() override;

        bool init(const RenderSurface& surface) override;
        void beginFrame() override;
        void endFrame() override;
        void resize(int width, int height) override;
        void shutdown() override;
        void drawSprite(const SpriteData& sprite) override;
        TextureHandle loadTexture(const char* path) override;

    private:
        bool compileShader();
        bool initSpriteResources();
        SDL_Window* m_window = nullptr;
        SDL_GLContext m_context = nullptr;
        GLuint m_spriteProgram = 0;
        GLuint m_spriteVao = 0;
        GLuint m_spriteVbo = 0;
        GLuint m_spriteEbo = 0;
        std::vector<GLuint> m_ownedTextures;
        int m_viewportWidth = 1;
        int m_viewportHeight = 1;
        bool m_initialized = false;
    };
}