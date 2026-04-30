# Non-Euclidean Projections

An interactive 3D visualizer for map projections built with C++, WebAssembly, and React.

The engine is written entirely in C++ (compiled to WebAssembly via Emscripten) and handles all math and rendering with raw OpenGL ES 3.0 / WebGL2. React is a thin UI shell with zero graphics logic.

**[Live Demo](https://moseshoggatt.github.io/Non-Euclidian-Projections/)**

---

## Projections

| Projection | Property | Grid Color |
|---|---|---|
| Equirectangular | Simple lat/lon mapping | Gold |
| Stereographic | Conformal (angle-preserving), projects from south pole | Cyan |
| Gnomonic | Great circles become straight lines, projects from center | Green |
| Mercator | Cylindrical conformal, navigation standard | Orange |

## Architecture

```
cpp/              C++ engine (compiled → WebAssembly)
├── math/         Vector2, Vector3, Matrix4, transforms
├── renderer/     Shader, VertexBuffer, VertexArray (RAII)
├── scene/        Orbit camera
├── geometry/     SphereGenerator, TorusGenerator, GridGenerator
├── projections/  Abstract Projection + 4 implementations
└── bindings/     Emscripten entry point, exported C API

web/              React + TypeScript + Vite (UI shell only)
└── src/
    ├── components/  ViewerCanvas, ProjectionSelector, ParameterPanel
    └── wasm/        Compiled engine (projection_engine.js + .wasm)
```

## Building Locally

**Prerequisites:** [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html), Node.js 18+, CMake 3.20+

```bash
# Install emsdk (one time)
./setup_emsdk.sh

# Compile C++ → WebAssembly
./build.sh

# Start dev server
cd web && npm install && npm run dev
```

Open `http://localhost:5173/`.

## Tech Stack

- **C++17** — all geometry, projection math, and rendering logic
- **OpenGL ES 3.0 / WebGL2** — Blinn-Phong shading, wireframe overlay, projected grid lines
- **Emscripten 3.x** — C++ → WASM, maps OpenGL calls to WebGL2
- **React 18 + TypeScript + Vite** — UI shell, calls engine via `Module.ccall()`
