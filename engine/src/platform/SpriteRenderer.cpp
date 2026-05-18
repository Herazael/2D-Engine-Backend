#include <engine/platform/SpriteRenderer.h>
#include <engine/core/Renderer/IWindowSurface.h>

#include <stb_image.h>
#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace {
constexpr float kClearColorR = 0.16f;
constexpr float kClearColorG = 0.46f;
constexpr float kClearColorB = 0.68f;
constexpr float kClearColorA = 1.0f;

constexpr const char* kSpriteVertexShaderSource = R"glsl(
    #version 330 core
    layout(location = 0) in vec3 aPosition;
    layout(location = 1) in vec2 aUV;
    out vec2 vUV;
    uniform mat4 uProjection;
    void main()
    {
        vUV = aUV;
        gl_Position = uProjection * vec4(aPosition, 1.0);
    }
)glsl";
constexpr const char* kSpriteFragmentShaderSource = R"glsl(
    #version 330 core
    in vec2 vUV;
    out vec4 FragColor;
    uniform sampler2D uTexture;
    uniform vec4 uTint;
    void main()
    {
        vec4 texColor = texture(uTexture, vUV);
        FragColor = texColor * uTint;
    }
)glsl";

const engine::SpriteVertex kSpriteQuadVertices[] = {
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f, 1.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }
};

const std::uint32_t kSpriteQuadIndices[] = {
    0, 1, 2,
    2, 3, 0
};

inline GLenum toGLenum(engine::PrimitiveType type)
{
    switch (type) {
        case engine::PrimitiveType::Points: return GL_POINTS;
        case engine::PrimitiveType::Lines: return GL_LINES;
        case engine::PrimitiveType::LineStrip: return GL_LINE_STRIP;
        case engine::PrimitiveType::LineLoop: return GL_LINE_LOOP;
        case engine::PrimitiveType::Triangles: return GL_TRIANGLES;
        case engine::PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
        case engine::PrimitiveType::TriangleFan: return GL_TRIANGLE_FAN;
        default: return GL_TRIANGLES;
    }
}

inline bool hasVertexData(const engine::GeometryData& g)
{
    return g.vertexByteSize > 0 && g.vertexCount > 0 && g.vertices != nullptr && g.componentsPerVertex > 0;
}

inline bool hasIndexData(const engine::GeometryData& g)
{
    return g.indexCount > 0 && g.indices != nullptr;
}

inline GLenum getIndexType(const engine::GeometryData& g)
{
    switch (g.indexType) {
        case engine::IndexType::UInt16: return GL_UNSIGNED_SHORT;
        case engine::IndexType::UInt32: return GL_UNSIGNED_INT;
        default: return GL_UNSIGNED_INT;
    }
}

static void matrixMultiplyVec3(const float* mat4x4, float x, float y, float z, float& outX, float& outY, float& outZ)
{
    outX = mat4x4[0] * x + mat4x4[4] * y + mat4x4[8] * z + mat4x4[12];
    outY = mat4x4[1] * x + mat4x4[5] * y + mat4x4[9] * z + mat4x4[13];
    outZ = mat4x4[2] * x + mat4x4[6] * y + mat4x4[10] * z + mat4x4[14];
}

static bool checkShader(GLuint shader)
{
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 1) {
            std::vector<GLchar> infoLog(static_cast<size_t>(logLength), '\0');
            glGetShaderInfoLog(shader, logLength, nullptr, infoLog.data());
            SDL_Log("Shader compilation failed: %s", infoLog.data());
        } else {
            SDL_Log("Shader compilation failed with empty info log.");
        }

        return false;
    }
    return true;
}

static GLuint compileHelper(const char* source, GLenum shaderType)
{
    GLuint shader = glCreateShader(shaderType);
    if (shader == 0) {
        SDL_Log("Could not create shader");
        return 0;
    }

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    return shader;
}

static bool linkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint& outProgram)
{
    GLuint program = glCreateProgram();
    if (program == 0) {
        SDL_Log("Could not create shader program");
        return false;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 1) {
            std::vector<GLchar> infoLog(static_cast<size_t>(logLength), '\0');
            glGetProgramInfoLog(program, logLength, nullptr, infoLog.data());
            SDL_Log("Shader link failed: %s", infoLog.data());
        } else {
            SDL_Log("Shader link failed with empty info log.");
        }

        glDeleteProgram(program);
        return false;
    }

    outProgram = program;
    return true;
}

static GLuint loadTextureFromFile(const char* path)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(false);
    unsigned char* pixels = stbi_load(path, &width, &height, &channels, 4);
    if (!pixels) {
        SDL_Log("Failed to load image: %s", path);
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);

    return texture;
}

static void buildOrthoProjection(float width, float height, float* outMatrix)
{
    float left = 0.0f;
    float right = width;
    float bottom = height;
    float top = 0.0f;
    float near = -1.0f;
    float far = 1.0f;

    for (int i = 0; i < 16; ++i) {
        outMatrix[i] = 0.0f;
    }

    outMatrix[0] = 2.0f / (right - left);
    outMatrix[5] = 2.0f / (top - bottom);
    outMatrix[10] = -2.0f / (far - near);
    outMatrix[12] = -(right + left) / (right - left);
    outMatrix[13] = -(top + bottom) / (top - bottom);
    outMatrix[14] = -(far + near) / (far - near);
    outMatrix[15] = 1.0f;
}

static void buildSpriteModel(const engine::SpriteData& sprite, float* outMatrix)
{
    float sinRot = std::sin(sprite.rotation);
    float cosRot = std::cos(sprite.rotation);

    for (int i = 0; i < 16; ++i) {
        outMatrix[i] = 0.0f;
    }

    outMatrix[0] = sprite.width * cosRot;
    outMatrix[1] = sprite.width * sinRot;
    outMatrix[4] = -sprite.height * sinRot;
    outMatrix[5] = sprite.height * cosRot;
    outMatrix[10] = 1.0f;
    outMatrix[12] = sprite.x;
    outMatrix[13] = sprite.y;
    outMatrix[15] = 1.0f;
}
} // namespace

engine::SpriteRenderer::~SpriteRenderer()
{
    shutdown();
}

bool engine::SpriteRenderer::init(const engine::IWindowSurface& surface)
{
    m_window = static_cast<SDL_Window*>(surface.getNativeWindowHandle());
    if (!m_window) {
        SDL_Log("Renderer init failed: native window handle is null");
        return false;
    }

    configureContextAttributes();

    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context) {
        SDL_Log("Failed to create OpenGL context: %s", SDL_GetError());
        shutdown();
        return false;
    }

    if (!SDL_GL_MakeCurrent(m_window, m_context)) {
        SDL_Log("Failed to make context current: %s", SDL_GetError());
        shutdown();
        return false;
    }

    const int glVersion = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    if (glVersion == 0) {
        SDL_Log("GLAD2 init failed: could not load OpenGL functions");
        shutdown();
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!compileShader()) {
        shutdown();
        return false;
    }

    if (!initGeometryResources()) {
        shutdown();
        return false;
    }

    if (!initSpriteResources()) {
        shutdown();
        return false;
    }

    resize(surface.getWidth(), surface.getHeight());

    m_initialized = true;
    return true;
}

void engine::SpriteRenderer::configureContextAttributes()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

bool engine::SpriteRenderer::initGeometryResources()
{
    glGenVertexArrays(1, &m_geometryVao);
    glGenBuffers(1, &m_geometryVbo);
    glGenBuffers(1, &m_geometryEbo);

    if (m_geometryVao == 0 || m_geometryVbo == 0 || m_geometryEbo == 0) {
        SDL_Log("Failed to create geometry buffers.");
        return false;
    }

    glBindVertexArray(m_geometryVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_geometryVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_geometryEbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

bool engine::SpriteRenderer::initSpriteResources()
{
    glGenVertexArrays(1, &m_spriteVao);
    glGenBuffers(1, &m_spriteVbo);
    glGenBuffers(1, &m_spriteEbo);

    if (m_spriteVao == 0 || m_spriteVbo == 0 || m_spriteEbo == 0) {
        SDL_Log("Failed to create sprite buffers.");
        return false;
    }

    glBindVertexArray(m_spriteVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(kSpriteQuadVertices),
        kSpriteQuadVertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_spriteEbo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(kSpriteQuadIndices),
        kSpriteQuadIndices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(engine::SpriteVertex),
        reinterpret_cast<void*>(0)
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(engine::SpriteVertex),
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return true;
}

bool engine::SpriteRenderer::compileShader()
{
    GLuint geometryVs = compileHelper(kSpriteVertexShaderSource, GL_VERTEX_SHADER);
    if (geometryVs == 0) {
        return false;
    }
    if (!checkShader(geometryVs)) {
        glDeleteShader(geometryVs);
        return false;
    }

    GLuint geometryFs = compileHelper(kSpriteFragmentShaderSource, GL_FRAGMENT_SHADER);
    if (geometryFs == 0) {
        glDeleteShader(geometryVs);
        return false;
    }
    if (!checkShader(geometryFs)) {
        glDeleteShader(geometryFs);
        glDeleteShader(geometryVs);
        return false;
    }

    if (!linkProgram(geometryVs, geometryFs, m_geometryProgram)) {
        glDeleteShader(geometryFs);
        glDeleteShader(geometryVs);
        return false;
    }

    glDetachShader(m_geometryProgram, geometryVs);
    glDetachShader(m_geometryProgram, geometryFs);
    glDeleteShader(geometryVs);
    glDeleteShader(geometryFs);

    GLuint spriteVs = compileHelper(kSpriteVertexShaderSource, GL_VERTEX_SHADER);
    if (spriteVs == 0) {
        return false;
    }
    if (!checkShader(spriteVs)) {
        glDeleteShader(spriteVs);
        return false;
    }

    GLuint spriteFs = compileHelper(kSpriteFragmentShaderSource, GL_FRAGMENT_SHADER);
    if (spriteFs == 0) {
        glDeleteShader(spriteVs);
        return false;
    }
    if (!checkShader(spriteFs)) {
        glDeleteShader(spriteFs);
        glDeleteShader(spriteVs);
        return false;
    }

    if (!linkProgram(spriteVs, spriteFs, m_spriteProgram)) {
        glDeleteShader(spriteFs);
        glDeleteShader(spriteVs);
        return false;
    }

    glDetachShader(m_spriteProgram, spriteVs);
    glDetachShader(m_spriteProgram, spriteFs);
    glDeleteShader(spriteVs);
    glDeleteShader(spriteFs);

    return true;
}

engine::TextureHandle engine::SpriteRenderer::loadTexture(const char* path)
{
    const GLuint texture = loadTextureFromFile(path);
    if (texture != 0) {
        m_ownedTextures.push_back(texture);
    }
    return texture;
}

void engine::SpriteRenderer::beginFrame()
{
    if (!m_initialized) {
        return;
    }

    glClearColor(kClearColorR, kClearColorG, kClearColorB, kClearColorA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void engine::SpriteRenderer::endFrame()
{
    if (!m_initialized || !m_window) {
        return;
    }

    buildAndFlushBatch();
    SDL_GL_SwapWindow(m_window);
}

void engine::SpriteRenderer::drawSprite(const SpriteData& sprite)
{
    if (!m_initialized) {
        return;
    }

    if (sprite.texture == 0) {
        return;
    }

    m_spriteBatch.push_back(sprite);
}

void engine::SpriteRenderer::drawGeometry(const GeometryData& geometry)
{
    const bool hasIndex = hasIndexData(geometry);
    const bool hasVertex = hasVertexData(geometry);

    if (!m_initialized || !m_geometryProgram) {
        SDL_Log("Geometry renderer is not initialized.");
        return;
    }

    if (!hasVertex) {
        SDL_Log("Invalid geometry passed, no vertex data found.");
        return;
    }

    glUseProgram(m_geometryProgram);
    glBindVertexArray(m_geometryVao);

    if (hasIndex) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_geometryEbo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            geometry.indexCount * (geometry.indexType == IndexType::UInt16 ? sizeof(std::uint16_t) : sizeof(std::uint32_t)),
            geometry.indices,
            GL_STATIC_DRAW
        );

        glBindBuffer(GL_ARRAY_BUFFER, m_geometryVbo);
        glBufferData(GL_ARRAY_BUFFER, geometry.vertexByteSize, geometry.vertices, GL_STATIC_DRAW);

        glDrawElements(
            toGLenum(geometry.primitive),
            geometry.indexCount,
            getIndexType(geometry),
            nullptr
        );

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, m_geometryVbo);
        glBufferData(GL_ARRAY_BUFFER, geometry.vertexByteSize, geometry.vertices, GL_STATIC_DRAW);

        glDrawArrays(
            toGLenum(geometry.primitive),
            0,
            geometry.vertexCount
        );

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

bool engine::SpriteRenderer::buildAndFlushBatch()
{
    if (m_spriteBatch.empty()) {
        return true;
    }

    std::vector<SpriteVertex> allVertices;
    std::vector<std::uint32_t> allIndices;
    m_batchEntries.clear();
    m_textureGroups.clear();

    // Build combined VBO/EBO
    for (size_t i = 0; i < m_spriteBatch.size(); ++i)
    {
        const SpriteData& sprite = m_spriteBatch[i];
        
        // Compute model matrix
        float model[16];
        buildSpriteModel(sprite, model);

        // Record batch entry
        int vertexOffset = static_cast<int>(allVertices.size() * sizeof(SpriteVertex));
        int indexOffset = static_cast<int>(allIndices.size() * sizeof(std::uint32_t));
        std::uint32_t baseIndex = static_cast<std::uint32_t>(allVertices.size());

        m_batchEntries.push_back({&sprite, vertexOffset, indexOffset, 6});
        m_textureGroups[sprite.texture].push_back(static_cast<int>(m_batchEntries.size()) - 1);

        // Transform and add 4 vertices
        for (int v = 0; v < 4; ++v)
        {
            float tx, ty, tz;
            matrixMultiplyVec3(model, 
                kSpriteQuadVertices[v].x, 
                kSpriteQuadVertices[v].y, 
                kSpriteQuadVertices[v].z, 
                tx, ty, tz);
            
            allVertices.push_back({tx, ty, tz, 
                                 kSpriteQuadVertices[v].u, 
                                 kSpriteQuadVertices[v].v});
        }

        // Add 6 indices for quad
        allIndices.push_back(baseIndex + 0);
        allIndices.push_back(baseIndex + 1);
        allIndices.push_back(baseIndex + 2);
        allIndices.push_back(baseIndex + 2);
        allIndices.push_back(baseIndex + 3);
        allIndices.push_back(baseIndex + 0);
    }

    // Upload to GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVbo);
    glBufferData(GL_ARRAY_BUFFER, 
                 static_cast<GLsizeiptr>(allVertices.size() * sizeof(SpriteVertex)), 
                 allVertices.data(), 
                 GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_spriteEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                 static_cast<GLsizeiptr>(allIndices.size() * sizeof(std::uint32_t)), 
                 allIndices.data(), 
                 GL_DYNAMIC_DRAW);

    // Render batched by texture
    float projection[16];
    buildOrthoProjection(
        static_cast<float>(m_viewportWidth),
        static_cast<float>(m_viewportHeight),
        projection
    );

    glUseProgram(m_spriteProgram);
    glBindVertexArray(m_spriteVao);

    glUniformMatrix4fv(
        glGetUniformLocation(m_spriteProgram, "uProjection"),
        1,
        GL_FALSE,
        projection
    );

    glUniform1i(glGetUniformLocation(m_spriteProgram, "uTexture"), 0);
    const GLint tintLocation = glGetUniformLocation(m_spriteProgram, "uTint");

    // Render each texture group
    for (const auto& [texture, entryIndices] : m_textureGroups)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        for (int entryIdx : entryIndices)
        {
            const SpriteBatchEntry& entry = m_batchEntries[entryIdx];
            if (tintLocation >= 0) {
                glUniform4f(
                    tintLocation,
                    entry.sprite->tintR,
                    entry.sprite->tintG,
                    entry.sprite->tintB,
                    entry.sprite->tintA
                );
            }
            glDrawElements(GL_TRIANGLES, entry.indexCount, GL_UNSIGNED_INT, 
                          reinterpret_cast<const void*>(entry.indexOffset));
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    m_spriteBatch.clear();
    return true;
}

void engine::SpriteRenderer::resize(int width, int height)
{
    if (width < 1) {
        width = 1;
    }
    if (height < 1) {
        height = 1;
    }

    m_viewportWidth = width;
    m_viewportHeight = height;
    glViewport(0, 0, width, height);
}

void engine::SpriteRenderer::shutdown()
{
    for (const GLuint texture : m_ownedTextures) {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }
    m_ownedTextures.clear();

    if (m_geometryProgram) {
        glDeleteProgram(m_geometryProgram);
    }
    if (m_spriteProgram) {
        glDeleteProgram(m_spriteProgram);
    }

    if (m_context) {
        SDL_GL_DestroyContext(m_context);
        m_context = nullptr;
    }

    if (m_geometryVao) {
        glDeleteVertexArrays(1, &m_geometryVao);
    }
    if (m_geometryVbo) {
        glDeleteBuffers(1, &m_geometryVbo);
    }
    if (m_geometryEbo) {
        glDeleteBuffers(1, &m_geometryEbo);
    }

    if (m_spriteVao) {
        glDeleteVertexArrays(1, &m_spriteVao);
    }
    if (m_spriteVbo) {
        glDeleteBuffers(1, &m_spriteVbo);
    }
    if (m_spriteEbo) {
        glDeleteBuffers(1, &m_spriteEbo);
    }

    m_initialized = false;
}
