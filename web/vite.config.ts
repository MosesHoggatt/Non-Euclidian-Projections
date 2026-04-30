import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';

// Vite config: serves the compiled WASM binary with the correct MIME type
// and aliases the wasm/ output directory so React can import the glue script.
// VITE_BASE_PATH is set in CI to the GitHub Pages sub-path; defaults to '/'
// so local dev continues to work at http://localhost:5173/.
export default defineConfig({
  base: process.env.VITE_BASE_PATH ?? '/',
  plugins: [react()],
  resolve: {
    alias: {
      '@wasm': path.resolve(__dirname, 'src/wasm'),
    },
  },
  server: {
    headers: {
      // Required for SharedArrayBuffer (future threading support)
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
    },
  },
  // Prevent Vite from inlining or transforming the WASM binary
  assetsInclude: ['**/*.wasm'],
  optimizeDeps: {
    exclude: ['projection_engine'],
  },
});
