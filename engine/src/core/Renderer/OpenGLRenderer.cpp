#define STB_IMAGE_IMPLEMENTATION
#include <engine/core/Renderer/OpenGLRenderer.h>
#include <engine/core/Renderer/Types.h>
#include <engine/platform/IWindowSurface.h>

#include <stb_image.h>
#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <cmath>
#include <vector>

namespace {
constexpr int kGLMajorVersion = 3;
constexpr int kGLMinorVersion = 3;
constexpr float kClearColorR = 0.16f;
constexpr float kClearColorG = 0.46f;
constexpr float kClearColorB = 0.68f;
constexpr float kClearColorA = 1.0f;

constexpr const char* kVertexShaderSource = R"glsl(
    #version 330 core

    layout(location = 0) in vec3 aPosition;
    layout(location = 1) in vec2 aUV;

    out vec2 vUV;

    uniform mat4 uModel;
    uniform mat4 uProjection;

    void main()
    {
        vUV = aUV;
        gl_Position = uProjection * uModel * vec4(aPosition, 1.0);
    }
)glsl";

constexpr const char* kFragmentShaderSource = R"glsl(
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

inline void buildOrthoProjection(float width, float height, float out[16])
{
    out[0] = 2.0f / width;
    out[1] = 0.0f;
    out[2] = 0.0f;
    out[3] = 0.0f;

    out[4] = 0.0f;
    out[5] = -2.0f / height;
    out[6] = 0.0f;
    out[7] = 0.0f;

    out[8] = 0.0f;
    out[9] = 0.0f;
    out[10] = -1.0f;
    out[11] = 0.0f;

    out[12] = -1.0f;
    out[13] = 1.0f;
    out[14] = 0.0f;
    out[15] = 1.0f;
}

inline void buildSpriteModel(const engine::SpriteData& sprite, float out[16])
{
    const float cosine = std::cos(sprite.rotation);
    const float sine = std::sin(sprite.rotation);

    out[0] = cosine * sprite.width;
    out[1] = sine * sprite.width;
    out[2] = 0.0f;
    out[3] = 0.0f;

    out[4] = -sine * sprite.height;
    out[5] = cosine * sprite.height;
    out[6] = 0.0f;
    out[7] = 0.0f;

    out[8] = 0.0f;
    out[9] = 0.0f;
    out[10] = 1.0f;
    out[11] = 0.0f;

    out[12] = sprite.x;
    out[13] = sprite.y;
    out[14] = 0.0f;
    out[15] = 1.0f;
}

static GLuint compileHelper(const char* shaderSource, GLenum shaderType)
{
    GLuint shader = glCreateShader(shaderType);
    if (shader == 0) {
        SDL_Log("glCreateShader failed for type: %u", static_cast<unsigned>(shaderType));
        return 0;
    }

    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);
    return shader;
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
            SDL_Log("Shader compile failed: %s", infoLog.data());
        } else {
            SDL_Log("Shader compile failed with empty info log.");
        }
        return false;
    }
    return true;
}

static GLuint loadTextureFromFile(const char* path)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(true);
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
} // namespace

engine::OpenGLRenderer::~OpenGLRenderer()
{
    shutdown();
}

void engine::OpenGLRenderer::configureContextAttributes()
{
    const auto setAttr = [](SDL_GLAttr attr, int value, const char* name) {
        if (!SDL_GL_SetAttribute(attr, value)) {
            SDL_Log("SDL_GL_SetAttribute failed for %s: %s", name, SDL_GetError());
        }
    };

    setAttr(SDL_GL_CONTEXT_MAJOR_VERSION, kGLMajorVersion, "SDL_GL_CONTEXT_MAJOR_VERSION");
    setAttr(SDL_GL_CONTEXT_MINOR_VERSION, kGLMinorVersion, "SDL_GL_CONTEXT_MINOR_VERSION");
    setAttr(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE, "SDL_GL_CONTEXT_PROFILE_MASK");
    setAttr(SDL_GL_RED_SIZE, 8, "SDL_GL_RED_SIZE");
    setAttr(SDL_GL_GREEN_SIZE, 8, "SDL_GL_GREEN_SIZE");
    setAttr(SDL_GL_BLUE_SIZE, 8, "SDL_GL_BLUE_SIZE");
    setAttr(SDL_GL_DEPTH_SIZE, 24, "SDL_GL_DEPTH_SIZE");
    setAttr(SDL_GL_DOUBLEBUFFER, 1, "SDL_GL_DOUBLEBUFFER");
}

bool engine::OpenGLRenderer::initSpriteResources()
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

bool engine::OpenGLRenderer::init(engine::IWindowSurface& surface)
{
    m_window = surface.getWindowHandle();
    if (!m_window) {
        SDL_Log("Renderer init failed: native window handle is null");
        return false;
    }

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

    SDL_Log("GLAD2 init OK (OpenGL version code: %d)", glVersion);

    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVer = glGetString(GL_SHADING_LANGUAGE_VERSION);

    SDL_Log("OpenGL Vendor: %s", vendor ? (const char*)vendor : "unknown");
    SDL_Log("OpenGL Renderer: %s", renderer ? (const char*)renderer : "unknown");
    SDL_Log("OpenGL Version: %s", version ? (const char*)version : "unknown");
    SDL_Log("GLSL Version: %s", glslVer ? (const char*)glslVer : "unknown");

    if (!compileShader()) {
        shutdown();
        return false;
    }

    if (!initSpriteResources()) {
        shutdown();
        return false;
    }

    int width = 0;
    int height = 0;
    surface.getSize(width, height);
    resize(width, height);

    m_initialized = true;
    return true;
}

void engine::OpenGLRenderer::beginFrame()
{
    if (!m_initialized) {
        return;
    }

    glClearColor(kClearColorR, kClearColorG, kClearColorB, kClearColorA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void engine::OpenGLRenderer::endFrame()
{
    if (!m_initialized || !m_window) {
        return;
    }

    SDL_GL_SwapWindow(m_window);
}

void engine::OpenGLRenderer::drawSprite(const SpriteData& sprite)
{
    if (!m_initialized || !m_spriteProgram) {
        SDL_Log("Renderer or Program not initialized.");
        return;
    }

    if (sprite.texture == 0) {
        SDL_Log("Sprite draw skipped: invalid texture.");
        return;
    }

    float model[16];
    float projection[16];
    buildSpriteModel(sprite, model);
    buildOrthoProjection(
        static_cast<float>(m_viewportWidth),
        static_cast<float>(m_viewportHeight),
        projection
    );

    glUseProgram(m_spriteProgram);
    glBindVertexArray(m_spriteVao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprite.texture);

    glUniform1i(glGetUniformLocation(m_spriteProgram, "uTexture"), 0);
    glUniform4f(
        glGetUniformLocation(m_spriteProgram, "uTint"),
        sprite.tintR,
        sprite.tintG,
        sprite.tintB,
        sprite.tintA
    );

    glUniformMatrix4fv(
        glGetUniformLocation(m_spriteProgram, "uModel"),
        1,
        GL_FALSE,
        model
    );

    glUniformMatrix4fv(
        glGetUniformLocation(m_spriteProgram, "uProjection"),
        1,
        GL_FALSE,
        projection
    );

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void engine::OpenGLRenderer::drawGeometry(const GeometryData& geometry)
{
    (void)geometry;
    SDL_Log("drawGeometry is temporarily disabled while sprite pipeline is active.");
}

bool engine::OpenGLRenderer::compileShader()
{
    GLuint vertexShader = compileHelper(kVertexShaderSource, GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        return false;
    }

    if (!checkShader(vertexShader)) {
        glDeleteShader(vertexShader);
        return false;
    }

    GLuint fragmentShader = compileHelper(kFragmentShaderSource, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    if (!checkShader(fragmentShader)) {
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
        return false;
    }

    GLuint program = glCreateProgram();
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
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    m_program = program;
    m_spriteProgram = program;

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

void engine::OpenGLRenderer::resize(int width, int height)
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

void engine::OpenGLRenderer::shutdown()
{
    for (const GLuint texture : m_ownedTextures) {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }
    m_ownedTextures.clear();

    if (m_spriteProgram) {
        glDeleteProgram(m_spriteProgram);
    }

    if (m_context) {
        SDL_GL_DestroyContext(m_context);
        m_context = nullptr;
    }

    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_ebo) {
        glDeleteBuffers(1, &m_ebo);
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

    m_program = 0;
    m_spriteProgram = 0;
    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;
    m_spriteVao = 0;
    m_spriteVbo = 0;
    m_spriteEbo = 0;
    m_window = nullptr;
    m_initialized = false;
}

engine::TextureHandle engine::OpenGLRenderer::loadTexture(const char* path)
{
    const GLuint texture = loadTextureFromFile(path);
    if (texture != 0) {
        m_ownedTextures.push_back(texture);
    }
    return texture;
}
