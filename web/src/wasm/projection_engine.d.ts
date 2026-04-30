// Type declarations for the Emscripten-generated WASM glue module.
// The actual .js file is emitted into this directory by the CMake build.
// Until then, this declaration file lets TypeScript know the module's shape.

export interface ProjectionEngineModule {
  // Called by Emscripten when the WASM binary is fully loaded and ready.
  onRuntimeInitialized?: () => void;

  // Emscripten hook: called for every file the module needs to locate.
  // Return a URL string to override the default path resolution.
  // Used here to supply the Vite-hashed WASM asset URL via locateFile.
  locateFile?: (path: string, prefix: string) => string;

  // Type-safe wrappers around exported C functions.
  // All parameters must match the C function signatures in main.cpp.
  ccall(
    functionName: string,
    returnType: 'number' | 'string' | 'boolean' | null,
    argumentTypes: Array<'number' | 'string' | 'boolean'>,
    argumentValues: Array<number | string | boolean>
  ): number | string | boolean | null;

  cwrap(
    functionName: string,
    returnType: 'number' | 'string' | 'boolean' | null,
    argumentTypes: Array<'number' | 'string' | 'boolean'>
  ): (...args: Array<number | string | boolean>) => number | string | boolean | null;
}

// The factory function exported by Emscripten's MODULARIZE output.
declare function createProjectionEngine(
  options?: Partial<ProjectionEngineModule>
): Promise<ProjectionEngineModule>;

export default createProjectionEngine;
