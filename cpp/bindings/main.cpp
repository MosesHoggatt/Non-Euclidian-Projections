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
// ViewAdaptiveProjection removed — adaptive is now a projection-agnostic toggle

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
"uniform vec3 objectColor;\n"
"uniform float shininess;\n"
"uniform vec3 cameraWorldPosition;\n"
"uniform float useLighting;\n"
"out vec4 fragmentColor;\n"
"void main() {\n"
"    vec3 normal   = normalize(worldNormal);\n"
"    vec3 toLight  = normalize(lightDirection);\n"
"    vec3 toCamera = normalize(cameraWorldPosition - worldPosition);\n"
"    float diff    = max(dot(normal, toLight), 0.0);\n"
"    vec3 hw       = normalize(toLight + toCamera);\n"
"    float spec    = pow(max(dot(normal, hw), 0.0), shininess) * 0.4;\n"
"    vec3 lighting = mix(vec3(1.0), ambientColor + diff*lightColor + spec*lightColor, useLighting);\n"
"    fragmentColor = vec4(objectColor * lighting, 1.0);\n"
"}";

// ── Cell-fill shaders: per-vertex terrain colour baked from C++ noise ────────
static const char* CELL_FILL_VERTEX_SHADER_SOURCE =
"#version 300 es\n"
"precision highp float;\n"
"layout(location = 0) in vec3 vertexPosition;\n"
"layout(location = 1) in vec3 vertexColor;\n"
"uniform mat4 modelMatrix;\n"
"uniform mat4 viewMatrix;\n"
"uniform mat4 projectionMatrix;\n"
"out vec3 fragColor;\n"
"out vec3 fragWorldPos;\n"
"void main() {\n"
"    vec4 wp     = modelMatrix * vec4(vertexPosition, 1.0);\n"
"    fragWorldPos = wp.xyz;\n"
"    fragColor    = vertexColor;\n"
"    gl_Position  = projectionMatrix * viewMatrix * wp;\n"
"}";

static const char* CELL_FILL_FRAGMENT_SHADER_SOURCE =
"#version 300 es\n"
"precision highp float;\n"
"in vec3 fragColor;\n"
"in vec3 fragWorldPos;\n"
"uniform vec3 lightDirection;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 ambientColor;\n"
"uniform float shininess;\n"
"uniform vec3 cameraWorldPosition;\n"
"uniform float useLighting;\n"
"out vec4 fragmentColor;\n"
"void main() {\n"
"    vec3 normal   = normalize(fragWorldPos);\n"
"    vec3 toLight  = normalize(lightDirection);\n"
"    vec3 toCamera = normalize(cameraWorldPosition - fragWorldPos);\n"
"    float diff    = max(dot(normal, toLight), 0.0);\n"
"    vec3 hw       = normalize(toLight + toCamera);\n"
"    float spec    = pow(max(dot(normal, hw), 0.0), shininess) * 0.18;\n"
"    vec3 lighting = mix(vec3(1.0), ambientColor + diff*lightColor + spec*lightColor, useLighting);\n"
"    fragmentColor = vec4(fragColor * lighting, 1.0);\n"
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

    // GPU resources for cell fill quads (terrain color per cell)
    std::unique_ptr<Shader>       cellFillShader;
    std::unique_ptr<VertexBuffer> cellFillVertexBuffer;
    std::unique_ptr<VertexArray>  cellFillVertexArray;
    int                           cellFillVertexCount = 0;

    // Current object type: 0=sphere, 1=torus
    int                           currentObjectType = 0;

    // ── Adaptive mode (camera-travel grid) ───────────────────────────────────
    bool  isAdaptive          = false;
    float adaptiveRawOffsetX  = 0.0f;   // accumulated flat-space offset (never wraps)
    float adaptiveRawOffsetY  = 0.0f;
    float adaptiveOffsetX     = 0.0f;   // wrapped to [-cellSpacing/2, +cellSpacing/2)
    float adaptiveOffsetY     = 0.0f;
    float adaptivePrevTheta   = 0.0f;
    float adaptivePrevPhi     = 0.0f;
    bool  adaptiveInitialized = false;
    Vector3 adaptiveCameraFacing{0.0f, 0.0f, -1.0f}; // camera-facing unit vector
};

static EngineState* gEngine = nullptr;

// ─────────────────────────────────────────────────────────────────────────────
//  Rotate vector v so that Y-axis (0,1,0) maps to `target` (Rodrigues).
//  Applied after flat→sphere projection to keep coverage centred on camera.
// ─────────────────────────────────────────────────────────────────────────────
static Vector3 rotateNorthToDir(const Vector3& v, const Vector3& target) {
    const Vector3 north(0.0f, 1.0f, 0.0f);
    float d = north.dot(target);
    if (d >  0.9999f) return v;
    if (d < -0.9999f) return Vector3(v.x, -v.y, v.z);
    Vector3 axis  = north.cross(target).normalized();
    float   angle = std::acos(std::max(-1.0f, std::min(1.0f, d)));
    Matrix4 rot   = Transforms::rotationAroundAxis(axis, angle);
    return rot.transformDirection(v);
}

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
//  C++ terrain noise  (matches the cell-fill palette)
// ─────────────────────────────────────────────────────────────────────────────
static float cpuHash(float x, float y) {
    // Integer-based hash avoids float precision issues across runs
    int ix = static_cast<int>(std::floor(x * 73.1f + 1234.5f));
    int iy = static_cast<int>(std::floor(y * 47.3f + 5678.9f));
    unsigned int h = static_cast<unsigned int>(ix * 1664525 + iy * 1013904223 + 22695477u);
    h = h * 1664525u + 1013904223u;
    return static_cast<float>(h & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
}
static float cpuVnoise(float x, float y) {
    float ix = std::floor(x), iy = std::floor(y);
    float fx = x - ix,        fy = y - iy;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    float v00 = cpuHash(ix,   iy  ), v10 = cpuHash(ix+1, iy  );
    float v01 = cpuHash(ix,   iy+1), v11 = cpuHash(ix+1, iy+1);
    return v00 + (v10-v00)*fx + (v01-v00)*fy + (v00-v10-v01+v11)*fx*fy;
}
static float cpuFbm(float x, float y) {
    float v = 0.0f, a = 0.5f, ox = 0.0f, oy = 0.0f;
    for (int i = 0; i < 5; ++i) {
        float s = std::pow(2.1f, static_cast<float>(i));
        v += a * cpuVnoise(x * s + ox, y * s + oy);
        ox += 1.7f; oy += 9.2f; a *= 0.5f;
    }
    return v * 2.0f - 1.0f;
}
static Vector3 cpuTerrainColor(float cx, float cy) {
    float h = cpuFbm(cx * 3.0f, cy * 3.0f) + 0.28f * cpuFbm(cx * 9.5f, cy * 9.5f);
    float t = std::max(0.0f, std::min(1.0f, h * 0.68f + 0.50f));
    auto mix3 = [](Vector3 a, Vector3 b, float tt) {
        return Vector3(a.x + (b.x-a.x)*tt, a.y + (b.y-a.y)*tt, a.z + (b.z-a.z)*tt);
    };
    if      (t < 0.28f) return mix3({0.03f,0.10f,0.38f},{0.08f,0.30f,0.56f}, t/0.28f);
    else if (t < 0.36f) return mix3({0.08f,0.30f,0.56f},{0.76f,0.72f,0.52f}, (t-0.28f)/0.08f);
    else if (t < 0.52f) return mix3({0.76f,0.72f,0.52f},{0.20f,0.52f,0.15f}, (t-0.36f)/0.16f);
    else if (t < 0.67f) return mix3({0.20f,0.52f,0.15f},{0.30f,0.25f,0.15f}, (t-0.52f)/0.15f);
    else if (t < 0.82f) return mix3({0.30f,0.25f,0.15f},{0.52f,0.48f,0.45f}, (t-0.67f)/0.15f);
    else                return mix3({0.52f,0.48f,0.45f},{0.93f,0.95f,0.98f}, (t-0.82f)/0.18f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: upload cell fill quad mesh to GPU
//  Each grid cell becomes 2 triangles (6 verts) with:
//    - position: projected sphere position (updated each frame in adaptive mode)
//    - color: terrain noise at the cell's FIXED flat-space center (stable across offset changes)
//  Only generated for the sphere (object 0) — disabled for torus.
// ─────────────────────────────────────────────────────────────────────────────
static void uploadCellFillToGPU(EngineState* engine, float offsetX, float offsetY) {
    if (!engine->currentProjection || engine->currentObjectType != 0) {
        engine->cellFillVertexCount = 0;
        return;
    }
    const int   lineCount    = std::max(4, engine->gridDensity / 4);
    const float cellSpacing  = 2.0f / static_cast<float>(lineCount + 1);
    const float OFFSET       = 1.002f;  // slightly above sphere, below grid lines

    // Format: x,y,z, r,g,b — 6 floats per vertex, 6 verts per cell
    const int cells = (lineCount + 1) * (lineCount + 1);
    std::vector<float> verts;
    verts.reserve(cells * 6 * 6);

    for (int jj = 0; jj <= lineCount; ++jj) {
        for (int ii = 0; ii <= lineCount; ++ii) {
            float x0 = -1.0f + ii * cellSpacing;
            float x1 = x0 + cellSpacing;
            float y0 = -1.0f + jj * cellSpacing;
            float y1 = y0 + cellSpacing;

            // Color from noise at the FIXED cell center (independent of adaptive offset)
            float cx = (x0 + x1) * 0.5f;
            float cy = (y0 + y1) * 0.5f;
            Vector3 col = cpuTerrainColor(cx, cy);

            // Project the 4 corners with adaptive offset + camera-facing rotation
            auto proj = [&](float x, float y) -> Vector3 {
                Vector3 sp = engine->currentProjection->mapFlatToSphere(x + offsetX, y + offsetY);
                if (engine->isAdaptive)
                    sp = rotateNorthToDir(sp, engine->adaptiveCameraFacing);
                return sp * OFFSET;
            };
            Vector3 p00 = proj(x0, y0), p10 = proj(x1, y0);
            Vector3 p01 = proj(x0, y1), p11 = proj(x1, y1);

            auto push = [&](const Vector3& p) {
                verts.push_back(p.x); verts.push_back(p.y); verts.push_back(p.z);
                verts.push_back(col.x); verts.push_back(col.y); verts.push_back(col.z);
            };
            // Two triangles (CCW)
            push(p00); push(p10); push(p11);
            push(p00); push(p11); push(p01);
        }
    }

    engine->cellFillVertexCount = static_cast<int>(verts.size() / 6);
    GLsizeiptr byteSize = static_cast<GLsizeiptr>(verts.size() * sizeof(float));

    if (engine->cellFillVertexBuffer &&
        static_cast<int>(verts.size()) == engine->cellFillVertexCount * 6) {
        // Same size: update in-place (fast path)
        engine->cellFillVertexBuffer->bind();
        engine->cellFillVertexBuffer->updateData(verts.data(), byteSize);
        engine->cellFillVertexBuffer->unbind();
    } else {
        // Recreate (density or projection changed)
        engine->cellFillVertexBuffer = std::make_unique<VertexBuffer>(verts.data(), byteSize);
        engine->cellFillVertexArray  = std::make_unique<VertexArray>();
        engine->cellFillVertexArray->bind();
        engine->cellFillVertexBuffer->bind();
        engine->cellFillVertexArray->addAttributeDescriptor({0, 3, static_cast<GLsizei>(6*sizeof(float)), 0});
        engine->cellFillVertexArray->addAttributeDescriptor({1, 3, static_cast<GLsizei>(6*sizeof(float)), static_cast<int>(3*sizeof(float))});
        engine->cellFillVertexArray->unbind();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: update adaptive offset using delta-accumulation for speed matching.
//  Integrates dTheta/dPhi frame-by-frame so the grid scrolls at exactly the
//  same apparent speed as the sphere surface.
// ─────────────────────────────────────────────────────────────────────────────
static void updateAdaptiveOffset(EngineState* engine, const Vector3& cameraPos) {
    Vector3 center = cameraPos.normalized();
    float theta = std::atan2(center.x, center.z);
    float phi   = std::asin(std::max(-1.0f, std::min(1.0f, center.y)));

    if (engine->adaptiveInitialized) {
        float dTheta = theta - engine->adaptivePrevTheta;
        // Wrap delta to [-π, π] to handle the ±π discontinuity
        while (dTheta >  MathConstants::PI) dTheta -= MathConstants::TWO_PI;
        while (dTheta < -MathConstants::PI) dTheta += MathConstants::TWO_PI;
        float dPhi = phi - engine->adaptivePrevPhi;

        // Scale by 1/scale so the flat-space scroll rate matches the inverse
        // stereographic: moving one sphere-surface cell length ≈ one flat cell.
        const float scale = 2.5f;
        engine->adaptiveRawOffsetX += std::cos(phi) * dTheta / scale;
        engine->adaptiveRawOffsetY += dPhi / scale;
    }
    engine->adaptivePrevTheta     = theta;
    engine->adaptivePrevPhi       = phi;
    engine->adaptiveInitialized   = true;
    engine->adaptiveCameraFacing  = Vector3(
        std::cos(phi) * std::sin(theta),
        std::sin(phi),
        std::cos(phi) * std::cos(theta)).normalized();

    // Wrap to one cell for seamless tiling
    const int   lineCount   = std::max(4, engine->gridDensity / 4);
    const float cellSpacing = 2.0f / static_cast<float>(lineCount + 1);
    const float bias        = 1000.0f * cellSpacing;
    engine->adaptiveOffsetX = std::fmod(engine->adaptiveRawOffsetX + bias, cellSpacing) - cellSpacing * 0.5f;
    engine->adaptiveOffsetY = std::fmod(engine->adaptiveRawOffsetY + bias, cellSpacing) - cellSpacing * 0.5f;
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

    const float GRID_SURFACE_OFFSET = 1.003f;
    float ox = engine->isAdaptive ? engine->adaptiveOffsetX : 0.0f;
    float oy = engine->isAdaptive ? engine->adaptiveOffsetY : 0.0f;

    std::vector<float> projectedPositions;
    projectedPositions.reserve(engine->flatGridPoints.size() * 3);

    for (const Vector2& flatPoint : engine->flatGridPoints) {
        Vector3 sp = engine->currentProjection->mapFlatToSphere(flatPoint.x + ox, flatPoint.y + oy);
        if (engine->isAdaptive)
            sp = rotateNorthToDir(sp, engine->adaptiveCameraFacing);
        projectedPositions.push_back(sp.x * GRID_SURFACE_OFFSET);
        projectedPositions.push_back(sp.y * GRID_SURFACE_OFFSET);
        projectedPositions.push_back(sp.z * GRID_SURFACE_OFFSET);
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
    engine->sphereVertexArray->unbind();
    engine->surfaceShader->unbind();

    // ── Adaptive: update offset + facing, rebuild grid + cell fill every frame ─
    if (engine->isAdaptive && engine->currentProjection) {
        updateAdaptiveOffset(engine, cameraPosition);
        float ox = engine->adaptiveOffsetX;
        float oy = engine->adaptiveOffsetY;
        const Vector3& facing = engine->adaptiveCameraFacing;
        // Update grid line positions
        std::vector<float> gridPos;
        gridPos.reserve(engine->flatGridPoints.size() * 3);
        const float GRID_OFFSET = 1.003f;
        for (const Vector2& fp : engine->flatGridPoints) {
            Vector3 sp = engine->currentProjection->mapFlatToSphere(fp.x + ox, fp.y + oy);
            sp = rotateNorthToDir(sp, facing);
            gridPos.push_back(sp.x * GRID_OFFSET);
            gridPos.push_back(sp.y * GRID_OFFSET);
            gridPos.push_back(sp.z * GRID_OFFSET);
        }
        engine->projectedGridVertexBuffer->bind();
        engine->projectedGridVertexBuffer->updateData(gridPos.data(),
            static_cast<GLsizeiptr>(gridPos.size() * sizeof(float)));
        engine->projectedGridVertexBuffer->unbind();
        // Update cell fill positions (colors are stable — no recompute)
        uploadCellFillToGPU(engine, ox, oy);
    }

    // ── Draw cell fills (terrain coloured quads between grid lines) ───────────
    if (engine->currentObjectType == 0 && engine->cellFillVertexBuffer &&
        engine->cellFillVertexCount > 0) {
        engine->cellFillShader->bind();
        engine->cellFillShader->setUniformMatrix4("modelMatrix",      Matrix4::identity());
        engine->cellFillShader->setUniformMatrix4("viewMatrix",       viewMatrix);
        engine->cellFillShader->setUniformMatrix4("projectionMatrix", projectionMatrix);
        engine->cellFillShader->setUniformVector3("lightDirection",   Vector3(0.6f, 1.0f, 0.8f).normalized());
        engine->cellFillShader->setUniformVector3("lightColor",       Vector3(1.0f, 1.0f, 1.0f));
        engine->cellFillShader->setUniformVector3("ambientColor",     Vector3(0.15f, 0.15f, 0.2f));
        engine->cellFillShader->setUniformFloat(  "shininess",        48.0f);
        engine->cellFillShader->setUniformVector3("cameraWorldPosition", cameraPosition);
        engine->cellFillShader->setUniformFloat(  "useLighting",      engine->useLighting);
        engine->cellFillVertexArray->bind();
        glDrawArrays(GL_TRIANGLES, 0, engine->cellFillVertexCount);
        engine->cellFillVertexArray->unbind();
        engine->cellFillShader->unbind();
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
    gEngine->cellFillShader  = std::make_unique<Shader>(CELL_FILL_VERTEX_SHADER_SOURCE, CELL_FILL_FRAGMENT_SHADER_SOURCE);

    if (gEngine->surfaceShader->programId() == 0 || gEngine->wireframeShader->programId() == 0
        || gEngine->cellFillShader->programId() == 0) {
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
    uploadCellFillToGPU(gEngine, 0.0f, 0.0f);

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
        default:
            printf("[Engine] Unknown projection ID: %d\n", projectionId);
            return;
    }

    reprojectGridToGPU(gEngine);
    uploadCellFillToGPU(gEngine,
        gEngine->isAdaptive ? gEngine->adaptiveOffsetX : 0.0f,
        gEngine->isAdaptive ? gEngine->adaptiveOffsetY : 0.0f);
    printf("[Engine] Projection set to: %s\n", gEngine->currentProjection->name());
}

EMSCRIPTEN_KEEPALIVE
void engine_set_grid_density(int density) {
    if (!gEngine) return;
    gEngine->gridDensity = density;
    // Reset adaptive raw offset so cell count change doesn't cause a jump
    gEngine->adaptiveInitialized = false;
    uploadObjectToGPU(gEngine);
    regenerateFlatGrid(gEngine);
    reprojectGridToGPU(gEngine);
    uploadCellFillToGPU(gEngine,
        gEngine->isAdaptive ? gEngine->adaptiveOffsetX : 0.0f,
        gEngine->isAdaptive ? gEngine->adaptiveOffsetY : 0.0f);
}

EMSCRIPTEN_KEEPALIVE
void engine_set_object_type(int objectId) {
    if (!gEngine) return;
    gEngine->currentObjectType = objectId;
    uploadObjectToGPU(gEngine);
    uploadCellFillToGPU(gEngine,
        gEngine->isAdaptive ? gEngine->adaptiveOffsetX : 0.0f,
        gEngine->isAdaptive ? gEngine->adaptiveOffsetY : 0.0f);
    printf("[Engine] Object type set to: %d\n", objectId);
}

EMSCRIPTEN_KEEPALIVE
void engine_set_lit(int isLit) {
    if (gEngine) gEngine->useLighting = isLit ? 1.0f : 0.0f;
}

EMSCRIPTEN_KEEPALIVE
void engine_set_adaptive(int on) {
    if (!gEngine) return;
    gEngine->isAdaptive = (on != 0);
    if (!gEngine->isAdaptive) {
        // Reset offset so next activation starts fresh
        gEngine->adaptiveRawOffsetX  = 0.0f;
        gEngine->adaptiveRawOffsetY  = 0.0f;
        gEngine->adaptiveOffsetX     = 0.0f;
        gEngine->adaptiveOffsetY     = 0.0f;
        gEngine->adaptiveInitialized = false;
        reprojectGridToGPU(gEngine);
        uploadCellFillToGPU(gEngine, 0.0f, 0.0f);
    }
}

} // extern "C"

// main() is required by Emscripten even if empty.
int main() { return 0; }
