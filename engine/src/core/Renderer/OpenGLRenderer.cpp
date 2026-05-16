#include <engine/core/Renderer/OpenGLRenderer.h>
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

namespace {
// ... existing constants ...
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

    compileShader();

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

void engine::OpenGLRenderer::drawLine()
{
    glDrawArrays(GL_LINES, 0, 2);
}

static GLuint compileHelper(const char* const *shaderSource, GLenum shaderType){
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, shaderSource, 0);
    glCompileShader(shader);
    return shader;
}

static bool checkShader(GLuint *shader){
    GLint isCompiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
    	GLint maxLength = 0;
    	glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &maxLength);

    	std::vector<GLchar> infoLog(maxLength);
    	glGetShaderInfoLog(*shader, maxLength, &maxLength, &infoLog[0]);
    	return false;
    }
    return true;
}

void engine::OpenGLRenderer::compileShader(){
    GLuint vertexShader = compileHelper(&kVertexShaderSource, GL_VERTEX_SHADER);
    if(!checkShader(&vertexShader)){
        glDeleteShader(vertexShader);
        return;
    }
    
    GLuint fragmentShader = compileHelper(&kFragmentShaderSource, GL_FRAGMENT_SHADER);
    if(!checkShader(&fragmentShader)){
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
        return;
    }

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int *)&isLinked);
    if (isLinked == GL_FALSE)
    {
    	GLint maxLength = 0;
    	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

    	std::vector<GLchar> infoLog(maxLength);
    	glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
    
    	glDeleteProgram(program);
    	glDeleteShader(vertexShader);
    	glDeleteShader(fragmentShader);
    	return;
    }
    m_program = program;
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
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

    m_window = nullptr;
    m_initialized = false;
}
