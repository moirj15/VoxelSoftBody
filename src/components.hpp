#pragma once

#include <glm/vec3.hpp>
#include "common.h"
#include <string>
#include <focus.hpp>

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