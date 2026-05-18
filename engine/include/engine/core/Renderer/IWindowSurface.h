#pragma once

namespace engine
{
    class IWindowSurface
    {
    public:
        virtual ~IWindowSurface() = default;
        
        virtual void* getNativeWindowHandle() const = 0;
        virtual int getWidth() const = 0;
        virtual int getHeight() const = 0;
    };
}
