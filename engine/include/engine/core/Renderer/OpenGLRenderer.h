#pragma once


#include <glad/gl.h>
#include <engine/core/Renderer/IRenderer.h>
#include <SDL3/SDL_video.h>

struct SDL_Window;

namespace engine
{
    class OpenGLRenderer final : public IRenderer
    {
    public:
        ~OpenGLRenderer() override;

        void configureContextAttributes() override;
        bool init(IWindowSurface& surface) override;
        void beginFrame() override;
        void endFrame() override;
        void resize(int width, int height) override;
        void shutdown() override;
        void drawGeometry(GeometryData& geometry) override;

    private:
        bool compileShader();
        SDL_Window* m_window = nullptr;
        SDL_GLContext m_context = nullptr;
        GLuint m_program = 0;
        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        bool m_initialized = false;
        
    };
}
