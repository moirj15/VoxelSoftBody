#include "common.h"
// #include "glad.h"
#include "tiny_obj_loader.h"

#include <cassert>
#include <cstdio>
#include <entt/entt.hpp>
#include <filesystem>
#include <focus.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <optional>
#include <sdl2/SDL.h>
#include <string>
#include <string_view>
#include <vector>

enum class FilePermissions {
    Read,
    Write,
    ReadWrite,
    BinaryRead,
    BinaryWrite,
    BinaryReadWrite,
};

FILE *OpenFile(const char *file, FilePermissions permissions)
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

std::string ReadEntireFileAsString(const char *file)
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

std::vector<uint8_t> ReadEntireFileAsVector(const char *file)
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

#if 0
void DBCallBack(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const *message,
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

    printf("%s\n", glGetString(GL_VERSION));

    return {
        .width = width,
        .height = height,
        .sdl_window = window,
    };
}

class ShaderProgram
{
  public:
    struct VertexAttribute {
        const char *name;
        GLuint index;
        GLint components;
        GLenum type;
        GLboolean normalized;
        GLsizei stride;
        GLintptr offset;
    };

  private:
    const std::string m_name;
    GLuint m_program;

    std::vector<VertexAttribute> m_vertex_attributes;

  public:
    ShaderProgram(const char *name, const std::vector<std::string> &sources, const std::vector<GLenum> &types,
        const std::vector<VertexAttribute> &vertex_attributes) :
            m_name(name),
            m_program(glCreateProgram()), m_vertex_attributes(vertex_attributes)
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

#endif
struct Mesh {
#pragma pack(push, 1)
    struct Vertex {
        glm::vec3 position = {};
        glm::vec3 normal = {};
    };
#pragma pack(pop)
    std::vector<Vertex> vertices; // xyz
    std::vector<u32> indices;
};

// Components
struct LoadMeshParams {
    std::string filename;
};

struct MeshBuffers {
    focus::VertexBuffer vertex_buffer;
    focus::IndexBuffer index_buffer;
};

namespace utils
{
static Mesh LoadMeshFromObjFile(const std::string &path)
{
    if (!std::filesystem::exists(path)) {
        printf("Not a valid path %s\n", path.c_str());
        assert(0);
    }

    Mesh mesh;
    tinyobj::ObjReader reader;
    reader.ParseFromFile(path);
    const auto &attrib = reader.GetAttrib();
    // assuming one shape for now
    const auto &shape = reader.GetShapes()[0];
    // TODO: calculate a better index buffer
    u32 index = 0;
    for (const auto &indices : shape.mesh.indices) {
        Mesh::Vertex vertex;
        vertex.position.x = attrib.vertices[indices.vertex_index * 3];
        vertex.position.y = attrib.vertices[indices.vertex_index * 3 + 1];
        vertex.position.z = attrib.vertices[indices.vertex_index * 3 + 2];

        const u32 normal_index = indices.normal_index != -1 ? indices.normal_index : indices.vertex_index;
        vertex.normal.x = attrib.normals[normal_index * 3];
        vertex.normal.y = attrib.normals[normal_index * 3 + 1];
        vertex.normal.z = attrib.normals[normal_index * 3 + 2];

        mesh.vertices.push_back(vertex);
        mesh.indices.push_back(index);
        index++;
    }
    return mesh;
}
} // namespace utils


struct System {
    entt::registry &m_registry;
    const std::string_view m_system_name;

    constexpr System(entt::registry &registry, const std::string_view &system_name) :
            m_registry(registry), m_system_name(system_name)
    {
    }

    virtual ~System() = default;
    virtual void Run() = 0;
};

struct TestSystem final : public System {
    explicit constexpr TestSystem(entt::registry &registry) : System(registry, "Test-System") {}

    bool already_ran = true;

    void Run() override
    {
        if (already_ran) {
            return;
        }
        const auto entity = m_registry.create();
        m_registry.ctx().emplace<LoadMeshParams>("objects/block.obj");
        already_ran = true;
    }
};

struct InputSystem final : public System {
    explicit constexpr InputSystem(entt::registry &registry) : System(registry, "Input-System") {}

    void Run() override {}
};

struct MeshManagementSystem final : public System {
    // TODO: I could capture references to the fields each system needs access to
    explicit constexpr MeshManagementSystem(entt::registry &registry) : System(registry, "Mesh-Management-System") {}

    void Run() override
    {
        std::string &filename = m_registry.ctx().at<LoadMeshParams>().filename;
        if (filename.empty()) {
            return;
        }

        // TODO something more complex to actually manage the meshes
        const auto entity = m_registry.create();
        m_registry.emplace<Mesh>(entity, utils::LoadMeshFromObjFile(filename));
        filename.clear();
    }
};

struct RenderBufferManagementSystem final : public System {
    explicit constexpr RenderBufferManagementSystem(entt::registry &registry) :
            System(registry, "Render-Buffer-Management-System")
    {
    }

    void Run() override {}
};

struct RenderSystem final : public System {
    focus::Device *m_device;
    focus::Window window;
    focus::VertexBufferLayout m_phong_layout;

    struct PhongVertexConstantLayout {
        glm::mat4 camera;
        glm::mat4 mvp;
        glm::mat4 normal_mat;
        glm::vec4 light_position;
    };

    struct PhongFragConstantLayout {
        glm::vec4 light_color;
        glm::vec4 ambient_light;
        glm::vec4 ambient_color;
        glm::vec4 diffuse_color;
        glm::vec4 specular_color;
        glm::vec4 coefficients;
    };

    focus::ConstantBuffer m_phong_vertex_constant_buffer;
    focus::ConstantBuffer m_phong_frag_constant_buffer;

    explicit RenderSystem(entt::registry &registry) :
            System(registry, "render-System"), m_device(focus::Device::Init(focus::RendererAPI::OpenGL)),
            window(m_device->MakeWindow(1920, 1080)), m_phong_layout(0, focus::BufferUsage::Default, "INPUT")

    {
        m_phong_layout.Add("vPosition", focus::VarType::Float3).Add("vNormal", focus::VarType::Float3);
        focus::ConstantBufferLayout phong_vertex_constant_layout(0, focus::BufferUsage::Default, "vertexConstants");
        focus::ConstantBufferLayout phong_frag_constant_layout(1, focus::BufferUsage::Default, "fragConstants");
        m_phong_vertex_constant_buffer =
            m_device->CreateConstantBuffer(phong_vertex_constant_layout, nullptr, sizeof(PhongVertexConstantLayout));
        m_phong_frag_constant_buffer =
            m_device->CreateConstantBuffer(phong_frag_constant_layout, nullptr, sizeof(PhongFragConstantLayout));
    }

    void Run() override {}
};

struct UISystem final : public System {
    explicit constexpr UISystem(entt::registry &registry) : System(registry, "UI-System") {}

    void Run() override {}
};

struct HeadSystem final : public System {
    std::vector<std::unique_ptr<System>> m_systems;

    explicit constexpr HeadSystem(entt::registry &registry) : System(registry, "Head-System")
    {
        m_systems.emplace_back(new InputSystem(registry));
        // m_systems.emplace_back(new MeshManagementSystem(entity_list));
        m_systems.emplace_back(new RenderBufferManagementSystem(registry));
        m_systems.emplace_back(new RenderSystem(registry));
        m_systems.emplace_back(new UISystem(registry));
    }

    void Run() override
    {
        for (const auto &system : m_systems) {
            system->Run();
        }
    }
};

int main(int argc, char **argv)
{
    // auto window = init_gfx(1920, 1080, "voxel-soft-bodies");
    entt::registry registry;
    // Renderer renderer(entity_list);
    HeadSystem head_system(registry);
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
