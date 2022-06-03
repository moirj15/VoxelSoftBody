#include "common.h"
#include "glad.h"

#include <cassert>
#include <cstdio>
#include <sdl2/SDL.h>
#include <string>
#include <vector>

enum class FilePermissions {
    Read,
    Write,
    ReadWrite,
    BinaryRead,
    BinaryWrite,
    BinaryReadWrite,
};

inline FILE *OpenFile(const char *file, FilePermissions permissions)
{
    const char *cPermissions[] = {"r", "w", "w+", "rb", "wb", "wb+"};
    FILE *ret = nullptr;
    ret = fopen(file, cPermissions[(uint32_t)permissions]);
    if (!ret) {
        // TODO: better error handling
        printf("FAILED TO OPEN FILE: %s\n", file);
        exit(EXIT_FAILURE);
    }
    return ret;
}

inline std::string ReadEntireFileAsString(const char *file)
{
    auto *fp = OpenFile(file, FilePermissions::Read);
    fseek(fp, 0, SEEK_END);
    uint64_t length = ftell(fp);
    rewind(fp);
    if (length == 0) {
        printf("Failed to read file size\n");
    }
    std::string data(length, 0);
    fread(data.data(), sizeof(uint8_t), length, fp);
    fclose(fp);
    return data;
}

inline std::vector<uint8_t> ReadEntireFileAsVector(const char *file)
{
    auto *fp = OpenFile(file, FilePermissions::BinaryRead);
    fseek(fp, 0, SEEK_END);
    uint64_t length = ftell(fp);
    rewind(fp);
    if (length == 0) {
        printf("Failed to read file size\n");
    }
    std::vector<uint8_t> data(length, 0);
    fread(data.data(), sizeof(uint8_t), data.size(), fp);
    fclose(fp);
    return data;
}

static void DBCallBack(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const *message,
    void const *userParam)
{
    printf("DBG: %s\n", message);
}

struct Window {
    s32 width;
    s32 height;
    SDL_Window *sdl_window;
};

Window init_gfx(s32 width, s32 height, const char *title)
{
    if (SDL_Init(SDL_INIT_VIDEO)) {
        assert(0 && "Failed to Init video");
    }
    SDL_GL_LoadLibrary(nullptr);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    SDL_Window *window =
        SDL_CreateWindow("OpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);

    assert(window != nullptr);

    // Don't resize window so it doesn't mess with tiling window managers
    SDL_SetWindowResizable(window, SDL_FALSE);

    auto *context = SDL_GL_CreateContext(window);
    assert(context);
    assert(gladLoadGL());

#ifdef GL_DEBUG
    // TODO: add debug flag
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(DBCallBack, nullptr);
#endif
    glEnable(GL_PROGRAM_POINT_SIZE);

    printf("%s", glGetString(GL_VERSION));

    return {
        .width = width,
        .height = height,
        .sdl_window = window,
    };
}

class ShaderProgram
{
    const std::string m_name;
    GLuint m_program;

  public:
    ShaderProgram(const char *name, const std::vector<std::string> &sources, const std::vector<GLenum> &types) :
            m_name(name), m_program(glCreateProgram())
    {
        assert(sources.size() == types.size());
        std::vector<GLuint> stageHandles;
        for (uint32_t i = 0; i < sources.size(); i++) {
            stageHandles.emplace_back(glCreateShader(types[i]));
            CompileShader(stageHandles[i], sources[i], ShaderTypeToString(types[i]));
        }
        GLuint program = glCreateProgram();
        LinkShaderProgram(stageHandles, program);
    }
    [[nodiscard]] GLuint handle() const { return m_program; }

  private:
    static void CompileShader(GLuint handle, const std::string &source, const std::string &type)
    {
        const int32_t sourceLen = source.size();
        auto shaderSource = source.c_str();
        glShaderSource(handle, 1, &shaderSource, &sourceLen);
        glCompileShader(handle);
        GLint success = 0;
        char compileLog[512] = {};
        glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(handle, 512, nullptr, compileLog);
            printf("%s Shader Compile Error: %s\n", type.c_str(), compileLog);
            assert(0);
        }
    }

    static void LinkShaderProgram(std::vector<GLuint> shaderHandles, GLuint programHandle)
    {
        for (GLuint sHandle : shaderHandles) {
            glAttachShader(programHandle, sHandle);
        }
        glLinkProgram(programHandle);

        GLint success = 0;
        char compileLog[512] = {};
        glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programHandle, sizeof(compileLog), nullptr, compileLog);
            printf("Shader Link Error: %s\n", compileLog);
            assert(0);
        }
        for (GLuint sHandle : shaderHandles) {
            glDeleteShader(sHandle);
        }
    }

    static const char *ShaderTypeToString(GLenum type)
    {
        switch (type) {
        case GL_VERTEX_SHADER:
            return "Vertex";
        case GL_TESS_CONTROL_SHADER:
            return "Tessellation Control";
        case GL_TESS_EVALUATION_SHADER:
            return "Tessellation Evaluation";
        case GL_GEOMETRY_SHADER:
            return "Geometry";
        case GL_FRAGMENT_SHADER:
            return "Fragment";
        case GL_COMPUTE_SHADER:
            return "Compute";
        default:
            assert(0 && "Invalid shader type");
            return "";
        }
    }
};

class Renderer
{
    GLuint m_default_vao = GL_NONE;
    ShaderProgram m_phong_program;

  public:
    Renderer() :
            m_phong_program("Phong",
                {ReadEntireFileAsString("shaders/phongLight.vert"), ReadEntireFileAsString("shaders/phongLight.frag")},
                {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER})
    {
        glGenVertexArrays(1, &m_default_vao);
        glBindVertexArray(m_default_vao);
    }
    void render_frame() {}
  private:
};

int main(int argc, char **argv)
{
    auto window = init_gfx(1920, 1080, "voxel-soft-bodies");
    Renderer renderer;
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e) > 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }
    }
    return 0;
}
