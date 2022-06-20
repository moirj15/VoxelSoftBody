#include "render_system.hpp"

#include "components.hpp"
#include "include/SDL2/SDL_video.h"
#include "sdl2/SDL.h"
#include "system.hpp"
#include "utils.hpp"
#include "glad.h"

#include <focus.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

struct Window {
    s32 width;
    s32 height;
    SDL_Window *sdl_window;
};

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

    GLuint m_vao;

    explicit RenderSystem(entt::registry &registry) : System(registry, "render-System")
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

        SDL_Window *sdl_window = SDL_CreateWindow(
            "OpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, SDL_WINDOW_OPENGL);

        assert(sdl_window != nullptr);

        // Don't resize window so it doesn't mess with tiling window managers
        SDL_SetWindowResizable(sdl_window, SDL_FALSE);

        auto *context = SDL_GL_CreateContext(sdl_window);
        assert(context);
        assert(gladLoadGL());

#ifdef _DEBUG
        // glEnable(GL_DEBUG_OUTPUT);
        // glDebugMessageCallback(DBCallBack, nullptr);
        printf("%s\n", glGetString(GL_VERSION));
#endif

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        auto window = Window {
            .width = 1920,
            .height = 1080,
            .sdl_window = sdl_window,
        };

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
            .shader = device->CreateShaderFromSource("Phong", utils::ReadEntireFileAsString("shaders/phong.vert"),
                utils::ReadEntireFileAsString("shaders/phong.frag")),
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
            // TODO (focus): Would be nice to use a single handle for multiple buffers, instead of constantly allocating
            // a new vector each time or I could just save off the scene state
            device->BindSceneState({.vb_handles = {buffers.vertex_buffer}, .ib_handle = buffers.index_buffer});
            device->Draw(focus::Primitive::Triangles, 0, mesh.indices.size());
        }

        device->EndPass();
    }
};

System *CreateRenderSystem(entt::registry &registry)
{
    return new RenderSystem(registry);
}