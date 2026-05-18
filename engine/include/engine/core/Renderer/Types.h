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

    using TextureHandle = std::uint32_t;
    struct SpriteData {
        TextureHandle texture = 0;
        float x = 0.0f;
        float y = 0.0f;
        float width = 1.0f;
        float height = 1.0f;
        float rotation = 0.0f;
        float tintR = 1.0f;
        float tintG = 1.0f;
        float tintB = 1.0f;
        float tintA = 1.0f;
    };

    struct SpriteVertex{
        float x, y, z;
        float u, v;
    };

    struct SpriteBatchEntry {
        const SpriteData* sprite;     
        int vertexOffset;               
        int indexOffset;            
        int indexCount;                
    };
} 