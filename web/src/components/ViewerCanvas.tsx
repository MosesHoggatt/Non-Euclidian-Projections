import React, { useEffect, useRef, useCallback } from 'react';
import type { ProjectionEngineModule } from '../wasm/projection_engine';

// ─────────────────────────────────────────────────────────────────────────────
//  ViewerCanvas.tsx
//
//  Owns the <canvas> element and the WASM engine lifecycle.
//  Responsibilities:
//    - Load and initialize the Emscripten WASM module
//    - Forward mouse drag and scroll events to the C++ camera
//    - Forward resize events to the C++ viewport
//    - Expose an imperative handle for parent components to call engine functions
//
//  What this component does NOT do:
//    - Any rendering logic (all in C++)
//    - Any math (all in C++)
// ─────────────────────────────────────────────────────────────────────────────

interface ViewerCanvasProps {
  onEngineReady?: (engine: ProjectionEngineModule) => void;
}

const ViewerCanvas: React.FC<ViewerCanvasProps> = ({ onEngineReady }) => {
  const canvasRef     = useRef<HTMLCanvasElement>(null);
  const engineRef     = useRef<ProjectionEngineModule | null>(null);
  const isDraggingRef = useRef(false);
  const lastMouseRef  = useRef({ x: 0, y: 0 });
  const containerRef  = useRef<HTMLDivElement>(null);

  // ── Engine initialization ─────────────────────────────────────────────────
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    let isMounted = true;

    const loadEngine = async () => {
      // The WASM glue JS is placed in /src/wasm/ by the CMake build.
      // During development without a built WASM, this import will fail —
      // that's expected until after running `emcmake cmake && emmake make`.
      let createEngine: typeof import('../wasm/projection_engine').default;
      try {
        const wasmModule = await import('../wasm/projection_engine.js');
        createEngine = wasmModule.default;
      } catch {
        console.warn(
          '[ViewerCanvas] WASM module not found. ' +
          'Run the CMake/Emscripten build first: see /cpp/CMakeLists.txt'
        );
        return;
      }

      // Vite hashes WASM assets (e.g. projection_engine-D8anZNAq.wasm) and puts
      // them in /assets/. The Emscripten module would otherwise look for the
      // file at the document root using the un-hashed name and 404. Providing
      // locateFile via import.meta.url lets Vite statically analyse the path at
      // build time and rewrite it to the correct hashed URL automatically.
      const wasmUrl = new URL('../wasm/projection_engine.wasm', import.meta.url).href;

      const engine = await createEngine({
        locateFile: (path: string) => path.endsWith('.wasm') ? wasmUrl : path,
        onRuntimeInitialized() {
          if (!isMounted) return;

          const width  = canvas.clientWidth  || 800;
          const height = canvas.clientHeight || 600;

          // Set physical pixel dimensions to match the CSS layout size
          canvas.width  = width;
          canvas.height = height;

          const success = engine.ccall(
            'engine_initialize',
            'number',
            ['number', 'number'],
            [width, height]
          );

          if (success) {
            engineRef.current = engine;
            onEngineReady?.(engine);
            console.log('[ViewerCanvas] Engine initialized.');
          } else {
            console.error('[ViewerCanvas] engine_initialize returned failure.');
          }
        },
      });
    };

    loadEngine();

    return () => { isMounted = false; };
  }, [onEngineReady]);

  // ── Resize handling ───────────────────────────────────────────────────────
  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const resizeObserver = new ResizeObserver((entries) => {
      const entry  = entries[0];
      const width  = Math.floor(entry.contentRect.width);
      const height = Math.floor(entry.contentRect.height);

      if (!canvasRef.current || !engineRef.current) return;

      canvasRef.current.width  = width;
      canvasRef.current.height = height;

      engineRef.current.ccall(
        'engine_on_resize',
        null,
        ['number', 'number'],
        [width, height]
      );
    });

    resizeObserver.observe(container);
    return () => resizeObserver.disconnect();
  }, []);

  // ── Mouse drag ────────────────────────────────────────────────────────────
  const handleMouseDown = useCallback((event: React.MouseEvent) => {
    isDraggingRef.current = true;
    lastMouseRef.current  = { x: event.clientX, y: event.clientY };
  }, []);

  const handleMouseMove = useCallback((event: React.MouseEvent) => {
    if (!isDraggingRef.current || !engineRef.current) return;

    const deltaX = event.clientX - lastMouseRef.current.x;
    const deltaY = event.clientY - lastMouseRef.current.y;
    lastMouseRef.current = { x: event.clientX, y: event.clientY };

    engineRef.current.ccall(
      'engine_on_mouse_drag',
      null,
      ['number', 'number'],
      [deltaX, deltaY]
    );
  }, []);

  const handleMouseUp = useCallback(() => {
    isDraggingRef.current = false;
  }, []);

  // ── Scroll zoom ───────────────────────────────────────────────────────────
  const handleWheel = useCallback((event: React.WheelEvent) => {
    event.preventDefault();
    if (!engineRef.current) return;

    // Normalize scroll delta: trackpad gestures produce small values,
    // mouse wheels produce large ones. We cap at ±10 to prevent jumping.
    const normalizedDelta = Math.max(-10, Math.min(10, event.deltaY * 0.01));

    engineRef.current.ccall(
      'engine_on_mouse_scroll',
      null,
      ['number'],
      [normalizedDelta]
    );
  }, []);

  // ── Touch support ─────────────────────────────────────────────────────────
  const lastTouchRef = useRef({ x: 0, y: 0 });

  const handleTouchStart = useCallback((event: React.TouchEvent) => {
    const touch = event.touches[0];
    lastTouchRef.current  = { x: touch.clientX, y: touch.clientY };
    isDraggingRef.current = true;
  }, []);

  const handleTouchMove = useCallback((event: React.TouchEvent) => {
    event.preventDefault();
    if (!isDraggingRef.current || !engineRef.current) return;

    const touch  = event.touches[0];
    const deltaX = touch.clientX - lastTouchRef.current.x;
    const deltaY = touch.clientY - lastTouchRef.current.y;
    lastTouchRef.current = { x: touch.clientX, y: touch.clientY };

    engineRef.current.ccall(
      'engine_on_mouse_drag',
      null,
      ['number', 'number'],
      [deltaX, deltaY]
    );
  }, []);

  const handleTouchEnd = useCallback(() => {
    isDraggingRef.current = false;
  }, []);

  return (
    <div
      ref={containerRef}
      style={{ width: '100%', height: '100%', position: 'relative' }}
    >
      <canvas
        id="projection-canvas"
        ref={canvasRef}
        style={{
          display: 'block',
          width: '100%',
          height: '100%',
          cursor: isDraggingRef.current ? 'grabbing' : 'grab',
          touchAction: 'none', // prevent browser scroll interference
        }}
        onMouseDown={handleMouseDown}
        onMouseMove={handleMouseMove}
        onMouseUp={handleMouseUp}
        onMouseLeave={handleMouseUp}
        onWheel={handleWheel}
        onTouchStart={handleTouchStart}
        onTouchMove={handleTouchMove}
        onTouchEnd={handleTouchEnd}
      />
    </div>
  );
};

export default ViewerCanvas;
