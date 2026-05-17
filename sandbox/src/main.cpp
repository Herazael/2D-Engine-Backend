#include <engine/core/Application/Application.h>
#include <engine/core/Application/IScene.h>
#include <engine/core/Renderer/IRenderer.h>
#include <engine/core/Renderer/Types.h>
#include <engine/platform/SdlAppHost.h>
#include <engine/platform/SpriteRenderer.h>

#include <memory>

namespace
{
    class DemoScene final : public engine::IScene
    {
    public:
        bool initialize(engine::IRenderer& renderer) override
        {
            m_sprite.texture = renderer.loadTexture("assets/sprite.png");
            m_sprite.x = 100.0f;
            m_sprite.y = 100.0f;
            m_sprite.width = 128.0f;
            m_sprite.height = 128.0f;
            return true;
        }

        void render(engine::IRenderer& renderer) override
        {
            renderer.drawSprite(m_sprite);
        }

        void shutdown(engine::IRenderer& renderer) override
        {
            (void)renderer;
        }

    private:
        engine::SpriteData m_sprite;
    };
}

int main() {
    engine::Application app(
        std::make_unique<engine::SpriteRenderer>(),
        std::make_unique<engine::SdlAppHost>(),
        std::make_unique<DemoScene>()
    );
    app.init();
    app.run();
    return 0;
}