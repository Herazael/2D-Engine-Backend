#pragma once

namespace engine
{
    enum class AppEventType
    {
        None,
        Quit,
        WindowResized
    };

    struct AppEvent
    {
        AppEventType type = AppEventType::None;
        int width = 0;
        int height = 0;
    };
}