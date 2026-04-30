#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>

#include "../math/Matrix4.h"
#include "../math/MathConstants.h"
#include "../renderer/Shader.h"
#include "../renderer/VertexBuffer.h"
#include "../renderer/VertexArray.h"
#include "../scene/Camera.h"
#include "../geometry/Mesh.h"
#include "../geometry/SphereGenerator.h"
#include "../geometry/GridGenerator.h"
#include "../geometry/TorusGenerator.h"
#include "../projections/Projection.h"
#include "../projections/EquirectangularProjection.h"
#include "../projections/StereographicProjection.h"
#include "../projections/GnomonicProjection.h"
#include "../projections/MercatorProjection.h"
#include "../projections/ViewAdaptiveProjection.h"

// ─────────────────────────────────────────────────────────────────────────────
//  bindings/main.cpp
//
//  The Emscripten entry point. Responsibilities:
//    1. Create the WebGL2 context from the HTML canvas
//    2. Compile and link the GLSL shaders
//    3. Upload sphere mesh data to the GPU
//    4. Register emscripten_set_main_loop (replaces while(true) in WebGL)
//    5. Export C functions for React to call via Module.ccall()
//
//  The exported API surface (callable from JavaScript):
//    engine_initialize(width, height)
//    engine_render_frame()
//    engine_on_mouse_drag(deltaX, deltaY)
//    engine_on_mouse_scroll(deltaY)
//    engine_on_resize(width, height)
//    engine_set_projection(projectionId)
//    engine_set_grid_density(density)
//    engine_set_object_type(objectId)
// ─────────────────────────────────────────────────────────────────────────────

// ── GLSL ES 3.0 Shaders ───────────────────────────────────────────────────────
// These live in the C++ file so they compile with the rest of the engine.
// The vertex shader receives interleaved position/normal/uv data and forwards
// the world-space normal and UV to the fragment shader for lighting + projection display.

static const char* VERTEX_SHADER_SOURCE =
"#version 300 es\n"
"precision highp float;\n"
"layout(location = 0) in vec3 vertexPosition;\n"
"layout(location = 1) in vec3 vertexNormal;\n"
"layout(location = 2) in vec2 vertexUV;\n"
"uniform mat4 modelMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 projectionMatrix;\n"
"uniform mat3 normalMatrix;\n"
"out vec3 worldNormal;\n"
"out vec3 worldPosition;\n"
"out vec2 uv;\n"
"void main() {\n"
"    vec4 worldPos = modelMatrix * vec4(vertexPosition, 1.0);\n"
"    worldPosition = worldPos.xyz;\n"
"    worldNormal   = normalize(normalMatrix * vertexNormal);\n"
"    uv            = vertexUV;\n"
"    gl_Position   = projectionMatrix * viewMatrix * worldPos;\n"
"}";

static const char* FRAGMENT_SHADER_SOURCE =
"#version 300 es\n"
"precision highp float;\n"
"in vec3 worldNormal;\n"
"in vec3 worldPosition;\n"
"in vec2 uv;\n"
"uniform vec3 lightDirection;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 ambientColor;\n"
"uniform float shininess;\n"
"uniform vec3 cameraWorldPosition;\n"
"uniform float useLighting;\n"
"out vec4 fragmentColor;\n"
// ── value noise hash ────────────────────────────────────────────────────────
"float hash(vec3 p) {\n"
"    p = fract(p * vec3(0.1031, 0.1030, 0.0973));\n"
"    p += dot(p, p.yxz + 33.33);\n"
"    return fract((p.x + p.y) * p.z);\n"
"}\n"
// ── trilinear-interpolated value noise ─────────────────────────────────────
"float vnoise(vec3 p) {\n"
"    vec3 i = floor(p); vec3 f = fract(p);\n"
"    vec3 u = f*f*(3.0-2.0*f);\n"
"    return mix(\n"
"        mix(mix(hash(i),              hash(i+vec3(1,0,0)),u.x),\n"
"            mix(hash(i+vec3(0,1,0)),  hash(i+vec3(1,1,0)),u.x),u.y),\n"
"        mix(mix(hash(i+vec3(0,0,1)), hash(i+vec3(1,0,1)),u.x),\n"
"            mix(hash(i+vec3(0,1,1)), hash(i+vec3(1,1,1)),u.x),u.y),u.z);\n"
"}\n"
// ── fractal Brownian motion (7 octaves) ─────────────────────────────────────
"float fbm(vec3 p) {\n"
"    float v=0.0; float a=0.5;\n"
"    for(int i=0;i<7;i++){v+=a*vnoise(p);p=p*2.03+vec3(1.7,9.2,5.3);a*=0.5;}\n"
"    return v*2.0-1.0;\n"  // remap [0,1] sum → [-1,1]
"}\n"
// ── 6-stop terrain palette (t in [0,1]) ─────────────────────────────────────
"vec3 terrainColor(float t) {\n"
"    vec3 c;\n"
"    if     (t<0.28) c=mix(vec3(0.03,0.10,0.38),vec3(0.08,0.30,0.56), t/0.28);\n"
"    else if(t<0.36) c=mix(vec3(0.08,0.30,0.56),vec3(0.76,0.72,0.52),(t-0.28)/0.08);\n"
"    else if(t<0.52) c=mix(vec3(0.76,0.72,0.52),vec3(0.20,0.52,0.15),(t-0.36)/0.16);\n"
"    else if(t<0.67) c=mix(vec3(0.20,0.52,0.15),vec3(0.30,0.25,0.15),(t-0.52)/0.15);\n"
"    else if(t<0.82) c=mix(vec3(0.30,0.25,0.15),vec3(0.52,0.48,0.45),(t-0.67)/0.15);\n"
"    else            c=mix(vec3(0.52,0.48,0.45),vec3(0.93,0.95,0.98),(t-0.82)/0.18);\n"
"    return c;\n"
"}\n"
"void main() {\n"
// continent-scale FBM + fine detail layer
"    float h = fbm(worldPosition*3.0) + 0.28*fbm(worldPosition*9.5);\n"
"    float t = clamp(h*0.68+0.50, 0.0, 1.0);\n"
"    vec3 base = terrainColor(t);\n"
"    vec3 normal   = normalize(worldNormal);\n"
"    vec3 toLight  = normalize(lightDirection);\n"
"    vec3 toCamera = normalize(cameraWorldPosition - worldPosition);\n"
"    float diff = max(dot(normal, toLight), 0.0);\n"
"    vec3 hw = normalize(toLight + toCamera);\n"
// reduced specular so water doesn't over-shine
"    float spec = pow(max(dot(normal, hw), 0.0), shininess) * 0.18;\n"
"    vec3 lighting = mix(vec3(1.0), ambientColor + diff*lightColor + spec*lightColor, useLighting);\n"
"    fragmentColor = vec4(base * lighting, 1.0);\n"
"}";

// Wireframe overlay uses a simpler single-color shader
static const char* WIREFRAME_VERTEX_SHADER_SOURCE =
"#version 300 es\n"
"precision highp float;\n"
"layout(location = 0) in vec3 vertexPosition;\n"
"uniform mat4 modelMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 projectionMatrix;\n"
"void main() {\n"
"    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);\n"
"}";

static const char* WIREFRAME_FRAGMENT_SHADER_SOURCE =
"#version 300 es\n"
"precision highp float;\n"
"uniform vec4 wireframeColor;\n"
"out vec4 fragmentColor;\n"
"void main() { fragmentColor = wireframeColor; }";

// ─────────────────────────────────────────────────────────────────────────────
//  Engine state — kept in a plain struct so all callbacks can access it.
//  Using a global pointer to a heap-allocated struct gives us a clear
//  single ownership point and avoids static initialization order issues.
// ─────────────────────────────────────────────────────────────────────────────

struct EngineState {
    // Scene
    Camera camera;

    // GPU resources for the sphere
    std::unique_ptr<Shader>       surfaceShader;
    std::unique_ptr<Shader>       wireframeShader;
    std::unique_ptr<VertexBuffer> sphereVertexBuffer;
    std::unique_ptr<IndexBuffer>  sphereIndexBuffer;
    std::unique_ptr<VertexArray>  sphereVertexArray;

    // Wireframe (line) index buffer (edges, not triangles)
    std::unique_ptr<IndexBuffer>  sphereWireframeIndexBuffer;
    int                           wireframeIndexCount = 0;

    // Current mesh info
    int sphereIndexCount = 0;

    // Viewport
    int viewportWidth  = 800;
    int viewportHeight = 600;

    // Mouse drag tracking
    bool  isMouseDragging = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;

    // Sphere generation parameters
    int   gridDensity = 32;

    // Rendering options
    float useLighting = 1.0f;  // 1=lit, 0=unlit flat shading

    // ── Projection + grid line system ─────────────────────────────────────────
    // The active projection maps flat [-1,1]^2 grid points to 3D sphere points.
    std::unique_ptr<Projection>   currentProjection;

    // Source grid in flat normalized space — regenerated when density changes.
    std::vector<Vector2>          flatGridPoints;

    // GPU resources for the projected 3D grid lines
    std::unique_ptr<VertexBuffer> projectedGridVertexBuffer;
    std::unique_ptr<VertexArray>  projectedGridVertexArray;
    int                           projectedGridVertexCount = 0;

    // Current object type: 0=sphere, 1=torus
    int                           currentObjectType = 0;
};

static EngineState* gEngine = nullptr;

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: build wireframe indices from triangle indices
//  For each triangle (a, b, c) emit edges (a-b), (b-c), (c-a).
//  We deduplicate by only emitting an edge when the lower index comes first.
// ─────────────────────────────────────────────────────────────────────────────
static std::vector<unsigned int> buildWireframeIndices(const std::vector<unsigned int>& triangleIndices) {
    std::vector<unsigned int> wireframeIndices;
    wireframeIndices.reserve(triangleIndices.size() * 2); // rough upper bound

    auto emitEdge = [&](unsigned int a, unsigned int b) {
        // Only emit each edge once (smaller index first)
        if (a < b) {
            wireframeIndices.push_back(a);
            wireframeIndices.push_back(b);
        }
    };

    for (size_t i = 0; i < triangleIndices.size(); i += 3) {
        unsigned int a = triangleIndices[i];
        unsigned int b = triangleIndices[i + 1];
        unsigned int c = triangleIndices[i + 2];
        emitEdge(a, b);
        emitEdge(b, c);
        emitEdge(a, c);
    }

    return wireframeIndices;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: regenerate the flat source grid
//  Called when grid density changes. The flat grid is independent of the
//  active projection — it is always a regular 2D grid in [-1, 1]^2.
// ─────────────────────────────────────────────────────────────────────────────
static void regenerateFlatGrid(EngineState* engine) {
    // Grid line count scales loosely with sphere density for visual coherence.
    // Clamped: too few lines looks bare; too many looks noisy.
    const int gridLineCount      = std::max(4, engine->gridDensity / 4);
    const int segmentsPerLine    = 64;  // fixed high-quality subdivision for smooth curves

    engine->flatGridPoints = GridGenerator::generateGrid(gridLineCount, segmentsPerLine);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: project the flat grid onto the sphere and upload the result to the GPU
//  Called after any projection change or density change.
// ─────────────────────────────────────────────────────────────────────────────
static void reprojectGridToGPU(EngineState* engine) {
    if (!engine->currentProjection || engine->flatGridPoints.empty()) return;

    // Small offset places the grid slightly above the sphere surface
    // to prevent z-fighting with the sphere triangles.
    const float GRID_SURFACE_OFFSET = 1.003f;

    // Transform each flat 2D grid point to a 3D sphere position.
    std::vector<float> projectedPositions;
    projectedPositions.reserve(engine->flatGridPoints.size() * 3);

    for (const Vector2& flatPoint : engine->flatGridPoints) {
        Vector3 spherePoint = engine->currentProjection->mapFlatToSphere(flatPoint.x, flatPoint.y);
        projectedPositions.push_back(spherePoint.x * GRID_SURFACE_OFFSET);
        projectedPositions.push_back(spherePoint.y * GRID_SURFACE_OFFSET);
        projectedPositions.push_back(spherePoint.z * GRID_SURFACE_OFFSET);
    }

    engine->projectedGridVertexCount = static_cast<int>(engine->flatGridPoints.size());
    GLsizeiptr bufferSize = static_cast<GLsizeiptr>(projectedPositions.size() * sizeof(float));

    // Recreate the VBO and VAO. The old unique_ptrs are automatically deleted.
    // This handles both first-time creation and re-uploads after parameter changes.
    engine->projectedGridVertexBuffer = std::make_unique<VertexBuffer>(
        projectedPositions.data(), bufferSize
    );

    engine->projectedGridVertexArray = std::make_unique<VertexArray>();
    engine->projectedGridVertexArray->bind();
    engine->projectedGridVertexBuffer->bind();

    // Only position (vec3) at location 0 — the wireframe shader is reused for grid lines
    engine->projectedGridVertexArray->addAttributeDescriptor({
        0,                              // shader location 0: vertexPosition
        3,                              // 3 components: x, y, z
        static_cast<GLsizei>(3 * sizeof(float)),  // stride: 3 floats per vertex
        0                               // offset: starts at byte 0
    });

    engine->projectedGridVertexArray->unbind();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: upload sphere mesh to GPU
// ─────────────────────────────────────────────────────────────────────────────
static void uploadSphereToGPU(EngineState* engine) {
    Mesh sphere = SphereGenerator::generate(engine->gridDensity, engine->gridDensity, 1.0f);

    engine->sphereIndexCount = static_cast<int>(sphere.indices.size());

    // Vertex buffer: all interleaved data
    engine->sphereVertexBuffer = std::make_unique<VertexBuffer>(
        sphere.vertexData.data(),
        static_cast<GLsizeiptr>(sphere.vertexData.size() * sizeof(float))
    );

    // Triangle index buffer
    engine->sphereIndexBuffer = std::make_unique<IndexBuffer>(
        sphere.indices.data(),
        static_cast<GLsizei>(sphere.indices.size())
    );

    // Wireframe index buffer
    auto wireframeIndices = buildWireframeIndices(sphere.indices);
    engine->wireframeIndexCount = static_cast<int>(wireframeIndices.size());
    engine->sphereWireframeIndexBuffer = std::make_unique<IndexBuffer>(
        wireframeIndices.data(),
        static_cast<GLsizei>(wireframeIndices.size())
    );

    // Vertex array: bind the layout (position, normal, UV)
    engine->sphereVertexArray = std::make_unique<VertexArray>();
    engine->sphereVertexArray->bind();
    engine->sphereVertexBuffer->bind();

    // location 0: position (3 floats, offset 0)
    engine->sphereVertexArray->addAttributeDescriptor({
        0, 3, Mesh::STRIDE_IN_BYTES, Mesh::POSITION_OFFSET
    });
    // location 1: normal (3 floats, offset 12)
    engine->sphereVertexArray->addAttributeDescriptor({
        1, 3, Mesh::STRIDE_IN_BYTES, Mesh::NORMAL_OFFSET
    });
    // location 2: UV (2 floats, offset 24)
    engine->sphereVertexArray->addAttributeDescriptor({
        2, 2, Mesh::STRIDE_IN_BYTES, Mesh::UV_OFFSET
    });

    engine->sphereVertexArray->unbind();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: upload torus mesh to GPU (reuses the same GPU buffer fields)
// ─────────────────────────────────────────────────────────────────────────────
static void uploadTorusToGPU(EngineState* engine) {
    int circleDivisions = engine->gridDensity;
    int tubeDivisions   = std::max(8, engine->gridDensity / 2);

    Mesh torus = TorusGenerator::generate(circleDivisions, tubeDivisions);

    engine->sphereIndexCount = static_cast<int>(torus.indices.size());

    engine->sphereVertexBuffer = std::make_unique<VertexBuffer>(
        torus.vertexData.data(),
        static_cast<GLsizeiptr>(torus.vertexData.size() * sizeof(float))
    );
    engine->sphereIndexBuffer = std::make_unique<IndexBuffer>(
        torus.indices.data(),
        static_cast<GLsizei>(torus.indices.size())
    );

    auto wireframeIndices = buildWireframeIndices(torus.indices);
    engine->wireframeIndexCount = static_cast<int>(wireframeIndices.size());
    engine->sphereWireframeIndexBuffer = std::make_unique<IndexBuffer>(
        wireframeIndices.data(),
        static_cast<GLsizei>(wireframeIndices.size())
    );

    engine->sphereVertexArray = std::make_unique<VertexArray>();
    engine->sphereVertexArray->bind();
    engine->sphereVertexBuffer->bind();
    engine->sphereVertexArray->addAttributeDescriptor({ 0, 3, Mesh::STRIDE_IN_BYTES, Mesh::POSITION_OFFSET });
    engine->sphereVertexArray->addAttributeDescriptor({ 1, 3, Mesh::STRIDE_IN_BYTES, Mesh::NORMAL_OFFSET });
    engine->sphereVertexArray->addAttributeDescriptor({ 2, 2, Mesh::STRIDE_IN_BYTES, Mesh::UV_OFFSET });
    engine->sphereVertexArray->unbind();
}

// Dispatches to the correct mesh generator based on currentObjectType.
static void uploadObjectToGPU(EngineState* engine) {
    if (engine->currentObjectType == 1) {
        uploadTorusToGPU(engine);
    } else {
        uploadSphereToGPU(engine);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Render loop — called every frame by emscripten_set_main_loop
// ─────────────────────────────────────────────────────────────────────────────
static void renderFrame() {
    EngineState* engine = gEngine;
    if (!engine) return;

    // ── Clear ─────────────────────────────────────────────────────────────────
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);  // dark navy background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Matrix4 modelMatrix      = Matrix4::identity();
    Matrix4 viewMatrix       = engine->camera.viewMatrix();
    Matrix4 projectionMatrix = engine->camera.projectionMatrix();
    Vector3 cameraPosition   = engine->camera.worldPosition();

    // Normal matrix = transpose of inverse of upper-left 3x3 of model matrix.
    // For a uniform-scale model matrix (identity here), this equals the identity.
    // We pass it as a mat3 uniform (9 floats in column-major).
    // For now, model = identity so normal matrix = identity too.
    float normalMatrixData[9] = {
        1,0,0,
        0,1,0,
        0,0,1
    };

    // ── Draw sphere surface ───────────────────────────────────────────────────
    engine->surfaceShader->bind();
    engine->surfaceShader->setUniformMatrix4("modelMatrix",      modelMatrix);
    engine->surfaceShader->setUniformMatrix4("viewMatrix",       viewMatrix);
    engine->surfaceShader->setUniformMatrix4("projectionMatrix", projectionMatrix);

    // Set normal matrix
    GLint normalMatrixLoc = glGetUniformLocation(engine->surfaceShader->programId(), "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, normalMatrixData);

    // Lighting
    engine->surfaceShader->setUniformVector3("lightDirection",    Vector3(0.6f, 1.0f, 0.8f).normalized());
    engine->surfaceShader->setUniformVector3("lightColor",        Vector3(1.0f, 1.0f, 1.0f));
    engine->surfaceShader->setUniformVector3("ambientColor",      Vector3(0.15f, 0.15f, 0.2f));
    engine->surfaceShader->setUniformVector3("objectColor",       Vector3(0.3f, 0.5f, 0.8f));
    engine->surfaceShader->setUniformFloat(  "shininess",         48.0f);
    engine->surfaceShader->setUniformVector3("cameraWorldPosition", cameraPosition);
    engine->surfaceShader->setUniformFloat(  "useLighting",       engine->useLighting);

    engine->sphereVertexArray->bind();
    engine->sphereIndexBuffer->bind();
    glDrawElements(GL_TRIANGLES, engine->sphereIndexCount, GL_UNSIGNED_INT, nullptr);
    engine->sphereIndexBuffer->unbind();

    // ── Update view-adaptive projection every frame ───────────────────────────
    if (engine->currentProjection &&
        engine->currentProjection->type() == ProjectionType::ViewAdaptive) {
        // Camera faces toward the origin; the sphere point it looks at is
        // in the direction from origin toward the camera.
        Vector3 facing = cameraPosition.normalized();
        const int   gridLineCount = std::max(4, engine->gridDensity / 4);
        const float cellSpacing   = 2.0f / static_cast<float>(gridLineCount + 1);
        static_cast<ViewAdaptiveProjection*>(engine->currentProjection.get())
            ->setCenter(facing, cellSpacing);
        // Update the VBO in-place — same size, just new positions.
        std::vector<float> projectedPositions;
        projectedPositions.reserve(engine->flatGridPoints.size() * 3);
        const float GRID_SURFACE_OFFSET = 1.003f;
        for (const Vector2& fp : engine->flatGridPoints) {
            Vector3 sp = engine->currentProjection->mapFlatToSphere(fp.x, fp.y);
            projectedPositions.push_back(sp.x * GRID_SURFACE_OFFSET);
            projectedPositions.push_back(sp.y * GRID_SURFACE_OFFSET);
            projectedPositions.push_back(sp.z * GRID_SURFACE_OFFSET);
        }
        engine->projectedGridVertexBuffer->bind();
        engine->projectedGridVertexBuffer->updateData(
            projectedPositions.data(),
            static_cast<GLsizeiptr>(projectedPositions.size() * sizeof(float)));
        engine->projectedGridVertexBuffer->unbind();
    }

    // ── Draw projected grid lines ─────────────────────────────────────────────
    if (engine->currentObjectType == 0 && engine->projectedGridVertexBuffer && engine->projectedGridVertexCount > 0) {
        Vector3 gridColorRGB = engine->currentProjection->gridColor();

        engine->wireframeShader->bind();
        engine->wireframeShader->setUniformMatrix4("viewMatrix",       viewMatrix);
        engine->wireframeShader->setUniformMatrix4("projectionMatrix", projectionMatrix);
        engine->wireframeShader->setUniformMatrix4("modelMatrix",      Matrix4::identity());
        GLint wireColorLoc = glGetUniformLocation(engine->wireframeShader->programId(), "wireframeColor");
        glUniform4f(wireColorLoc, gridColorRGB.x, gridColorRGB.y, gridColorRGB.z, 1.0f);

        engine->projectedGridVertexArray->bind();
        glDrawArrays(GL_LINES, 0, engine->projectedGridVertexCount);
        engine->projectedGridVertexArray->unbind();
        engine->wireframeShader->unbind();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Exported C API — callable from JavaScript via Module.ccall()
// ─────────────────────────────────────────────────────────────────────────────

extern "C" {

// Called once by React after the canvas element is ready.
EMSCRIPTEN_KEEPALIVE
int engine_initialize(int canvasWidth, int canvasHeight) {
    // ── Create WebGL2 context ─────────────────────────────────────────────────
    EmscriptenWebGLContextAttributes contextAttributes;
    emscripten_webgl_init_context_attributes(&contextAttributes);
    contextAttributes.majorVersion  = 2;   // WebGL2 = OpenGL ES 3.0
    contextAttributes.minorVersion  = 0;
    contextAttributes.antialias     = EM_TRUE;
    contextAttributes.depth         = EM_TRUE;
    contextAttributes.stencil       = EM_FALSE;
    contextAttributes.alpha         = EM_FALSE;
    contextAttributes.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context =
        emscripten_webgl_create_context("#projection-canvas", &contextAttributes);

    if (context <= 0) {
        printf("[Engine] ERROR: Failed to create WebGL2 context.\n");
        return 0;
    }

    emscripten_webgl_make_context_current(context);

    // ── Allocate engine state ─────────────────────────────────────────────────
    gEngine = new EngineState();
    gEngine->viewportWidth  = canvasWidth;
    gEngine->viewportHeight = canvasHeight;
    gEngine->camera.setViewportSize(canvasWidth, canvasHeight);

    glViewport(0, 0, canvasWidth, canvasHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ── Compile shaders ───────────────────────────────────────────────────────
    gEngine->surfaceShader   = std::make_unique<Shader>(VERTEX_SHADER_SOURCE,   FRAGMENT_SHADER_SOURCE);
    gEngine->wireframeShader = std::make_unique<Shader>(WIREFRAME_VERTEX_SHADER_SOURCE, WIREFRAME_FRAGMENT_SHADER_SOURCE);

    if (gEngine->surfaceShader->programId() == 0 || gEngine->wireframeShader->programId() == 0) {
        printf("[Engine] ERROR: Shader compilation failed (see messages above).\n");
        return 0;
    }

    // ── Upload geometry ───────────────────────────────────────────────────────
    uploadObjectToGPU(gEngine);

    // ── Initialize projection system ──────────────────────────────────────────
    // Default to equirectangular — the most intuitive starting projection.
    gEngine->currentProjection = std::make_unique<EquirectangularProjection>();
    regenerateFlatGrid(gEngine);
    reprojectGridToGPU(gEngine);

    // ── Start render loop ─────────────────────────────────────────────────────
    // 0 fps = run as fast as possible (requestAnimationFrame rate)
    emscripten_set_main_loop(renderFrame, 0, 0);

    printf("[Engine] Initialized. WebGL2 context active. Canvas: %dx%d\n",
           canvasWidth, canvasHeight);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
void engine_render_frame() {
    renderFrame();
}

EMSCRIPTEN_KEEPALIVE
void engine_on_mouse_drag(float deltaX, float deltaY) {
    if (gEngine) gEngine->camera.orbitByPixelDelta(deltaX, deltaY);
}

EMSCRIPTEN_KEEPALIVE
void engine_on_mouse_scroll(float deltaY) {
    if (gEngine) gEngine->camera.zoomByScrollDelta(deltaY);
}

EMSCRIPTEN_KEEPALIVE
void engine_on_resize(int newWidth, int newHeight) {
    if (!gEngine) return;
    gEngine->viewportWidth  = newWidth;
    gEngine->viewportHeight = newHeight;
    gEngine->camera.setViewportSize(newWidth, newHeight);
    glViewport(0, 0, newWidth, newHeight);
}

EMSCRIPTEN_KEEPALIVE
void engine_set_projection(int projectionId) {
    if (!gEngine) return;

    // Instantiate the requested projection and re-project the existing flat grid.
    switch (static_cast<ProjectionType>(projectionId)) {
        case ProjectionType::Equirectangular:
            gEngine->currentProjection = std::make_unique<EquirectangularProjection>();
            break;
        case ProjectionType::Stereographic:
            gEngine->currentProjection = std::make_unique<StereographicProjection>();
            break;
        case ProjectionType::Gnomonic:
            gEngine->currentProjection = std::make_unique<GnomonicProjection>();
            break;
        case ProjectionType::Mercator:
            gEngine->currentProjection = std::make_unique<MercatorProjection>();
            break;
        case ProjectionType::ViewAdaptive:
            gEngine->currentProjection = std::make_unique<ViewAdaptiveProjection>();
            break;
        default:
            printf("[Engine] Unknown projection ID: %d\n", projectionId);
            return;
    }

    // Re-project the same flat grid through the new projection.
    reprojectGridToGPU(gEngine);
    printf("[Engine] Projection set to: %s\n", gEngine->currentProjection->name());
}

EMSCRIPTEN_KEEPALIVE
void engine_set_grid_density(int density) {
    if (!gEngine) return;
    gEngine->gridDensity = density;
    // Rebuild mesh AND regenerate the projected grid at the new density
    uploadObjectToGPU(gEngine);
    regenerateFlatGrid(gEngine);
    reprojectGridToGPU(gEngine);
}

EMSCRIPTEN_KEEPALIVE
void engine_set_object_type(int objectId) {
    if (!gEngine) return;
    gEngine->currentObjectType = objectId;
    // Rebuild the mesh for the new object type.
    // For sphere, also re-project the grid; for torus, grid projection is N/A.
    uploadObjectToGPU(gEngine);
    printf("[Engine] Object type set to: %d\n", objectId);
}

EMSCRIPTEN_KEEPALIVE
void engine_set_lit(int isLit) {
    if (gEngine) gEngine->useLighting = isLit ? 1.0f : 0.0f;
}

} // extern "C"

// main() is required by Emscripten even if empty.
int main() { return 0; }
