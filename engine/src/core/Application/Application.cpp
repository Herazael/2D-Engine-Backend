#include <engine/core/Application/Application.h>
#include <SDL3/SDL.h>
#include <glad/gl.h>

engine::Application::~Application(){
    cleanUp();
}

void engine::Application::cleanUp(){ 
    if(m_context){
        SDL_GL_DestroyContext(m_context);
        m_context = nullptr;
    }
    if(m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
    m_running = false;
}

static SDL_PropertiesID createWindowProps(){
    SDL_PropertiesID props = SDL_CreateProperties();
    if(props == 0) {
        SDL_Log("Unable to create properties: %s", SDL_GetError());
        return 0;
    }
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "NeoLab2D");
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1280);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 720);
    return props;
}

static SDL_GLContext createOpenGLContext(SDL_Window* window) {
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr) {
        SDL_Log("Failed to create OpenGL context: %s", SDL_GetError());
        return nullptr;
    }
    return glContext;
}

static void setGLAttributes(){
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

void engine::Application::run() {
    while (m_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_running = false;
            }
        }
    } 
    return;
}

void engine::Application::init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        m_running = false;
        return;
    }    

    setGLAttributes();

    SDL_PropertiesID windowProps;
    if(!(windowProps = createWindowProps())){
        SDL_Log("Failed to create window properties: %s", SDL_GetError());
        cleanUp();
        return;
    };

    if(!(m_window = SDL_CreateWindowWithProperties(windowProps))){
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_DestroyProperties(windowProps);
        cleanUp();
        return;
    }
    SDL_SetWindowMinimumSize(m_window, 800, 600);
    SDL_DestroyProperties(windowProps);

    if(!(m_context = createOpenGLContext(m_window))){
        SDL_Log("Failed to create context: %s", SDL_GetError());
        cleanUp();
        return;
    }

    if(!SDL_GL_MakeCurrent(m_window, m_context)){
        SDL_Log("Failed to make context current: %s", SDL_GetError());
        cleanUp();
        return;
    }

    const int glVersion = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    if (glVersion == 0) {
        SDL_Log("GLAD2 init failed: could not load OpenGL functions");
        cleanUp();
        return;
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

    m_running = true;
}