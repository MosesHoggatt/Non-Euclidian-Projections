cmake_minimum_required(VERSION 3.20)
project(ProjectionEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Source files ──────────────────────────────────────────────────────────────
file(GLOB_RECURSE ENGINE_SOURCES
    "math/*.cpp"
    "renderer/*.cpp"
    "scene/*.cpp"
    "geometry/*.cpp"
    "projections/*.cpp"
    "bindings/*.cpp"
)

add_executable(projection_engine ${ENGINE_SOURCES})

target_include_directories(projection_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ── Emscripten-specific flags ─────────────────────────────────────────────────
if (EMSCRIPTEN)
    # Export a set of C functions so JavaScript can call them via Module.ccall()
    set(EXPORTED_FUNCTIONS
        "_engine_initialize"
        "_engine_render_frame"
        "_engine_on_mouse_drag"
        "_engine_on_mouse_scroll"
        "_engine_on_resize"
        "_engine_set_projection"
        "_engine_set_grid_density"
        "_engine_set_object_type"
        "_engine_set_lit"
        "_engine_set_adaptive"
        "_main"
    )
    string(JOIN "," EXPORTED_FUNCTIONS_STR ${EXPORTED_FUNCTIONS})

    target_link_options(projection_engine PRIVATE
        # WebGL2 via OpenGL ES 3.0 emulation
        "-sUSE_WEBGL2=1"
        "-sFULL_ES3=1"
        # Keep the exported C API alive through dead-code elimination
        "-sEXPORTED_FUNCTIONS=${EXPORTED_FUNCTIONS_STR}"
        "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap"
        # Allow memory growth for large meshes
        "-sALLOW_MEMORY_GROWTH=1"
        # Enable C++ exception catching (disabled by default in Emscripten).
        # The Shader class throws std::runtime_error on GLSL compile failure;
        # without this the abort handler fires instead of the C++ catch block.
        "-sDISABLE_EXCEPTION_CATCHING=0"
        # Output a clean ES module glue file
        "-sMODULARIZE=1"
        "-sEXPORT_NAME=createProjectionEngine"
        # Optimization: enable in release builds
        # "-O3"
    )

    # Output the .js glue and .wasm file directly into the web src/wasm/ folder
    set(WASM_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../web/src/wasm")
    set_target_properties(projection_engine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${WASM_OUTPUT_DIR}"
        OUTPUT_NAME "projection_engine"
    )
endif()
