#pragma once

#include <cstdint>

namespace engine {
    // used for dynamic rendering of various geometries
    // extends uint8_t to minimize memory usage
    enum class PrimitiveType : std::uint8_t{
        Points,
        Lines,
        LineStrip,
        LineLoop,
        Triangles,
        TriangleStrip,
        TriangleFan
    };
    
    enum class IndexType : std::uint8_t {
        UInt16,
        UInt32
    };

    struct GeometryData {
        const void* vertices = nullptr;
        int vertexByteSize = 0;
        int vertexCount = 0;
        int componentsPerVertex = 3;

        const void* indices = nullptr;
        int indexCount = 0;
        PrimitiveType primitive = PrimitiveType::Triangles;
        IndexType indexType = IndexType::UInt32;
    };
} 