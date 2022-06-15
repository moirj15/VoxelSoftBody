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
struct MeshBuffers {
    focus::VertexBuffer vertex_buffer;
    focus::IndexBuffer index_buffer;
};

// this is a little weird
struct Position {
    glm::vec3 position;
};


// Deferred Actions
struct LoadMeshParams {
    std::string filename;
};

// Singleton Components
struct BufferLayouts {
    focus::VertexBufferLayout phong_vertex_layout;
    focus::IndexBufferLayout phong_index_layout;
    focus::ConstantBufferLayout phong_vertex_constant_layout;
    focus::ConstantBufferLayout phong_frag_constant_layout;
};

namespace utils
{
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
        Mesh mesh = LoadMeshFromObjFile(filename);
        MeshBuffers mesh_buffers = CreateMeshBuffers(mesh);
        glm::vec3 position = {0.0, 0.0, 5.0};

        m_registry.emplace<Mesh>(entity, mesh);
        m_registry.emplace<MeshBuffers>(entity, mesh_buffers);
        m_registry.emplace<Position>(entity, position);

        filename.clear();
    }

    Mesh LoadMeshFromObjFile(const std::string &path)
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

    MeshBuffers CreateMeshBuffers(const Mesh &mesh)
    {
        auto *device = m_registry.ctx().at<focus::Device *>();
        const auto &layouts = m_registry.ctx().at<BufferLayouts>();
        // clang-format off
        return {
            .vertex_buffer = device->CreateVertexBuffer(layouts.phong_vertex_layout, (void *)mesh.vertices.data(),
                mesh.vertices.size() * sizeof(Mesh::Vertex)),
            .index_buffer = device->CreateIndexBuffer(
                layouts.phong_index_layout, (void *)mesh.indices.data(), mesh.indices.size() * sizeof(u32))
        };
        // clang-format on
    }
};

/*
struct RenderBufferManagementSystem final : public System {
    explicit constexpr RenderBufferManagementSystem(entt::registry &registry) :
            System(registry, "Render-Buffer-Management-System")
    {
    }

    void Run() override {}
};*/

struct RenderSystem final : public System {
    struct PhongVertexConstantLayout {
        glm::mat4 camera;
        glm::mat4 mvp;
        glm::mat4 normal_mat;
        glm::vec4 light_position;
    };

    struct PhongFragConstantLayout {
        glm::vec4 light_color{1.0, 1.0, 1.0, 1.0};
        glm::vec4 ambient_light{0.3, 0.3, 0.3, 1.0};
        glm::vec4 ambient_color{0.3, 0.3, 0.3, 1.0};
        glm::vec4 diffuse_color{1.0, 0.3, 0.3, 1.0};
        glm::vec4 specular_color{0.0, 0.3, 0.3, 1.0};
        glm::vec4 coefficients{10.0, 10.0, 10.0, 10.0};
    };

    focus::ConstantBuffer m_phong_vertex_constant_buffer;
    focus::ConstantBuffer m_phong_frag_constant_buffer;

    focus::Pipeline m_phong_pipeline;

    explicit RenderSystem(entt::registry &registry) : System(registry, "render-System")
    {
        auto &context = m_registry.ctx();
        auto *device = focus::Device::Init(focus::RendererAPI::OpenGL);
        auto window = device->MakeWindow(1920, 1080);
        context.emplace<focus::Device *>(device);
        context.emplace<focus::Window>(window);

        // Populate the Singleton Component for the buffer layouts
        focus::VertexBufferLayout phong_vertex_layout(0, focus::BufferUsage::Default, "INPUT");
        phong_vertex_layout.Add("vPosition", focus::VarType::Float3).Add("vNormal", focus::VarType::Float3);

        focus::IndexBufferLayout phong_index_layout(focus::IndexBufferType::U32);

        focus::ConstantBufferLayout phong_vertex_constant_layout(0, focus::BufferUsage::Default, "vertexConstants");
        focus::ConstantBufferLayout phong_frag_constant_layout(1, focus::BufferUsage::Default, "fragConstants");

        // TODO: Need to figure out if I'm just going to throw these into the registry or if I'll do some management
        // thing A mix of the two is probably a good approach
        context.emplace<BufferLayouts>(
            phong_vertex_layout, phong_index_layout, phong_vertex_constant_layout, phong_frag_constant_layout);
        m_phong_vertex_constant_buffer =
            device->CreateConstantBuffer(phong_vertex_constant_layout, nullptr, sizeof(PhongVertexConstantLayout));
        m_phong_frag_constant_buffer =
            device->CreateConstantBuffer(phong_frag_constant_layout, nullptr, sizeof(PhongFragConstantLayout));

        focus::PipelineState phong_pipeline_state = {
            .shader = device->CreateShaderFromSource(
                "Phong", ReadEntireFileAsString("shaders/phong.vert"), ReadEntireFileAsString("shaders/phong.frag")),
        };
        m_phong_pipeline = device->CreatePipeline(phong_pipeline_state);
    }

    void Run() override
    {
        // TODO: for now I'm going to do the buffer updates in a lazy way. In the future I'd like to do this properly
        // with
        //       some actual resource sorting and render state sorting.
        auto *device = m_registry.ctx().at<focus::Device *>();
        device->BeginPass("Phong pass");
        device->BindPipeline(m_phong_pipeline);
        for (const auto &[entity, buffers, mesh] : m_registry.view<const MeshBuffers, const Mesh>().each()) {
            // TODO (focus): Would be nice to use a single handle for multiple buffers, instead of constantly allocating a new vector each time
            // or I could just save off the scene state
            device->BindSceneState({.vb_handles = {buffers.vertex_buffer}, .ib_handle = buffers.index_buffer});
            device->Draw(focus::Primitive::Triangles, 0, mesh.indices.size());
        }

        device->EndPass();
    }
};

struct UISystem final : public System {
    explicit constexpr UISystem(entt::registry &registry) : System(registry, "UI-System") {}

    void Run() override {}
};

class HeadSystem final : public System
{
    std::vector<std::unique_ptr<System>> m_systems;

  public:
    explicit HeadSystem(entt::registry &registry) : System(registry, "Head-System")
    {
        m_systems.emplace_back(new InputSystem(registry));
        // m_systems.emplace_back(new MeshManagementSystem(entity_list));
        //        m_systems.emplace_back(new RenderBufferManagementSystem(registry));
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

#include <windows.h>

int main(int argc, char **argv)
{
    // auto window = init_gfx(1920, 1080, "voxel-soft-bodies");
    entt::registry registry;
    TCHAR buffer[MAX_PATH] = { 0 };
    GetModuleFileName( NULL, buffer, MAX_PATH );
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
