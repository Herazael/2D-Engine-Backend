#include <engine/core/Application/Application.h>
#include <engine/core/Renderer/OpenGLRenderer.h>

#include <memory>

int main() {
    engine::Application app(std::make_unique<engine::OpenGLRenderer>());
    app.init();
    app.run();
    return 0;
}