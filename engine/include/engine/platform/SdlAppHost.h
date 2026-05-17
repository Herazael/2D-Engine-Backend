#pragma once

#include <engine/core/Application/IAppHost.h>

struct SDL_Window;

namespace engine
{
    class SdlAppHost final : public IAppHost
    {
    public:
        bool initialize() override;
        bool pollEvent(AppEvent& outEvent) override;
        RenderSurface getRenderSurface() const override;
        void shutdown() override;

    private:
        SDL_Window* m_window = nullptr;
        bool m_sdlInitialized = false;
    };
}