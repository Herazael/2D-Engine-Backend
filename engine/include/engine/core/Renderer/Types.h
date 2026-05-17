#pragma once

#include <glad/gl.h>

namespace engine {
    enum class PrimitiveType {
        Points,
        Lines,
        LineStrip,
        LineLoop,
        Triangles,
        TriangleStrip,
        TriangleFan
    };

    struct GeometryData {
        const float* vertices;
        int vertexByteSize;
        int vertexCount;
        PrimitiveType primitive;
    };

    inline GLenum toGLenum(PrimitiveType type) {
        switch (type) {
            case PrimitiveType::Points: return GL_POINTS;
            case PrimitiveType::Lines: return GL_LINES;
            case PrimitiveType::LineStrip: return GL_LINE_STRIP;
            case PrimitiveType::LineLoop: return GL_LINE_LOOP;
            case PrimitiveType::Triangles: return GL_TRIANGLES;
            case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
            case PrimitiveType::TriangleFan: return GL_TRIANGLE_FAN;
            default: return GL_TRIANGLES;
        }
    }

} 