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
#include "system.hpp"
#include "components.hpp"
#include "render_system.hpp"

struct TestSystem final : public System {
    explicit constexpr TestSystem(entt::registry &registry) : System(registry, "Test-System") {}

    bool already_ran = true;

    void Run() override
    {
        if (already_ran) {
            return;
        }
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
        m_systems.emplace_back(CreateRenderSystem(registry));
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
    entt::registry registry;
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
