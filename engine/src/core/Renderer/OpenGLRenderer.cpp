#include <engine/core/Renderer/OpenGLRenderer.h>
#include <engine/core/Renderer/Types.h>
#include <engine/platform/IWindowSurface.h>

#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <vector>

namespace {
constexpr int kGLMajorVersion = 3;
constexpr int kGLMinorVersion = 3;
constexpr float kClearColorR = 0.16f;
constexpr float kClearColorG = 0.46f;
constexpr float kClearColorB = 0.68f;
constexpr float kClearColorA = 1.0f;
}

//test shader
namespace {
constexpr const char* kVertexShaderSource = R"glsl(
    #version 330 core
    layout(location = 0) in vec3 position;
    void main() {
        gl_Position = vec4(position, 1.0);
    }
)glsl";

constexpr const char* kFragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
)glsl";
}

//translate custom type to GL, so types.h stays backend agnostic
inline GLenum toGLenum(engine::PrimitiveType type) {
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

//check for vertex data in passed geometry
inline bool hasVertexData(const engine::GeometryData& g){
    if(!(g.vertexByteSize && g.vertexCount && g.vertices && g.componentsPerVertex)){
        return false;
    }
    return true;
}

//check for index data in passed geometry
inline bool hasIndexData(const engine::GeometryData& g){
    if(!(g.indexCount && g.indices)){
        return false;
    }
    return true;
}

//checks for 16 or 32 bit indices
inline GLenum getIndexType(engine::GeometryData g){
    GLenum glIndexType;
    switch (g.indexType) {
        case engine::IndexType::UInt16:
            glIndexType = GL_UNSIGNED_SHORT;
            break;
        case engine::IndexType::UInt32:
            glIndexType = GL_UNSIGNED_INT;
            break;
        default:
            return 0;
    }
    return glIndexType;
}

engine::OpenGLRenderer::~OpenGLRenderer()
{
    shutdown();
}

//configure GL context
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

bool engine::OpenGLRenderer::init(engine::IWindowSurface& surface)
{
    //window and context validation before proceeding
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

    //set context to window
    if (!SDL_GL_MakeCurrent(m_window, m_context)) {
        SDL_Log("Failed to make context current: %s", SDL_GetError());
        shutdown();
        return false;
    }

    //initialize glad
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

    //compile shader
    if(!compileShader()){
        shutdown();
        return false;
    }

    //initialize vao, vbo and ebo
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //set initial size
    int width = 0;
    int height = 0;
    surface.getSize(width, height);
    resize(width, height);

    m_initialized = true;
    return true;
}

//clear buffer
void engine::OpenGLRenderer::beginFrame()
{
    if (!m_initialized) {
        return;
    }

    glClearColor(kClearColorR, kClearColorG, kClearColorB, kClearColorA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//swap buffers
void engine::OpenGLRenderer::endFrame()
{
    if (!m_initialized || !m_window) {
        return;
    }
    SDL_GL_SwapWindow(m_window);
}

//draw primitve geometries
void engine::OpenGLRenderer::drawGeometry(const GeometryData& geometry)
{
    bool hasIndex = hasIndexData(geometry);
    bool hasVertex = hasVertexData(geometry);

    if(!m_initialized || !m_program){
        SDL_Log("Renderer or Program not initialized.");
        return;
    }
    
    if(!hasVertex){
        SDL_Log("Invalid geometry passed, no vertex data found.");
        return;
    }

    glUseProgram(m_program);
    glBindVertexArray(m_vao);

    if(hasIndex){
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);   
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER, 
            geometry.indexCount * (geometry.indexType == IndexType::UInt16 ? sizeof(std::uint16_t) : sizeof(uint32_t)), 
            geometry.indices, GL_STATIC_DRAW
        );
        glDrawElements(
            toGLenum(geometry.primitive), 
            geometry.indexCount,         
            getIndexType(geometry),
            nullptr 
        );
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(
            GL_ARRAY_BUFFER, 
            geometry.vertexByteSize, 
            geometry.vertices, 
            GL_STATIC_DRAW
        );
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

//compiles vertex and fragment shader depending on parameter passed
static GLuint compileHelper(const char* shaderSource, GLenum shaderType){
    GLuint shader = glCreateShader(shaderType);
    if (shader == 0) {
        SDL_Log("glCreateShader failed for type: %u", static_cast<unsigned>(shaderType));
        return 0;
    }
    glShaderSource(shader, 1, &shaderSource, 0);
    glCompileShader(shader);
    return shader;
}

//validates shader compilation
static bool checkShader(GLuint shader){
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

//main compile shader function
bool engine::OpenGLRenderer::compileShader(){
    GLuint vertexShader = compileHelper(kVertexShaderSource, GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        return false;
    }
    if(!checkShader(vertexShader)){
        glDeleteShader(vertexShader);
        return false;
    }

    GLuint fragmentShader = compileHelper(kFragmentShaderSource, GL_FRAGMENT_SHADER);
    
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }
    if(!checkShader(fragmentShader)){
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
            SDL_Log("Shader compile failed: %s", infoLog.data());
        } else {
            SDL_Log("Shader compile failed with empty info log.");
        }
    	glDeleteProgram(program);
    	glDeleteShader(vertexShader);
    	glDeleteShader(fragmentShader);
    	return false;
    }
    m_program = program;
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
    glViewport(0, 0, width, height);
}

void engine::OpenGLRenderer::shutdown()
{
    if (m_context) {
        SDL_GL_DestroyContext(m_context);
        m_context = nullptr;
    }
    if (m_vao){
        glDeleteVertexArrays(1, &m_vao);
    } 
    if (m_vbo){
        glDeleteBuffers(1, &m_vbo);
    } 
    if (m_ebo){
        glDeleteBuffers(1, &m_ebo);
    } 
    m_vao = 0; 
    m_vbo = 0;
    m_ebo = 0;
    m_window = nullptr;
    m_initialized = false;
}