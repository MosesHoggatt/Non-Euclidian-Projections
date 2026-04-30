import React, { useState, useCallback, useRef } from 'react';
import ViewerCanvas from './components/ViewerCanvas';
import ProjectionSelector, { ProjectionId } from './components/ProjectionSelector';
import ParameterPanel, { EngineParameters } from './components/ParameterPanel';
import type { ProjectionEngineModule } from './wasm/projection_engine';

// ─────────────────────────────────────────────────────────────────────────────
//  App.tsx
//
//  Root component. Owns the engine reference and all UI state.
//  Bridges user interactions in the panel to C++ engine calls.
//
//  Architecture:
//    App (state) → ProjectionSelector / ParameterPanel (events up)
//                → ViewerCanvas (engine ref down via onEngineReady)
// ─────────────────────────────────────────────────────────────────────────────

const App: React.FC = () => {
  const engineRef = useRef<ProjectionEngineModule | null>(null);
  const [engineReady, setEngineReady] = useState(false);

  const [selectedProjection, setSelectedProjection] = useState<ProjectionId>(0);
  const [parameters, setParameters] = useState<EngineParameters>({
    gridDensity: 32,
    objectType:  0,
  });

  // ── Engine ready callback ─────────────────────────────────────────────────
  const handleEngineReady = useCallback((engine: ProjectionEngineModule) => {
    engineRef.current = engine;
    setEngineReady(true);
  }, []);

  // ── Projection change ─────────────────────────────────────────────────────
  const handleProjectionSelect = useCallback((id: ProjectionId) => {
    setSelectedProjection(id);
    engineRef.current?.ccall('engine_set_projection', null, ['number'], [id]);
  }, []);

  // ── Grid density change ───────────────────────────────────────────────────
  const handleGridDensityChange = useCallback((density: number) => {
    setParameters(prev => ({ ...prev, gridDensity: density }));
    engineRef.current?.ccall('engine_set_grid_density', null, ['number'], [density]);
  }, []);

  // ── Object type change ────────────────────────────────────────────────────
  const handleObjectTypeChange = useCallback((objectId: number) => {
    setParameters(prev => ({ ...prev, objectType: objectId }));
    engineRef.current?.ccall('engine_set_object_type', null, ['number'], [objectId]);
  }, []);

  return (
    <div style={styles.appRoot}>

      {/* ── Side panel ──────────────────────────────────────────────────── */}
      <aside style={styles.sidePanel}>

        <header style={styles.header}>
          <h1 style={styles.title}>Projection<br />Visualizer</h1>
          <p style={styles.subtitle}>Non-Euclidean Geometry</p>
        </header>

        {!engineReady && (
          <div style={styles.loadingBadge}>
            Loading WASM engine…
          </div>
        )}

        <div style={styles.panelContent}>
          <ProjectionSelector
            selectedId={selectedProjection}
            onSelect={handleProjectionSelect}
          />

          <div style={styles.divider} />

          <ParameterPanel
            parameters={parameters}
            onGridDensityChange={handleGridDensityChange}
            onObjectTypeChange={handleObjectTypeChange}
          />
        </div>

        <footer style={styles.footer}>
          C++ · OpenGL ES 3.0 · WebAssembly
        </footer>
      </aside>

      {/* ── 3D viewport ──────────────────────────────────────────────────── */}
      <main style={styles.viewport}>
        <ViewerCanvas onEngineReady={handleEngineReady} />
      </main>

    </div>
  );
};

const styles: Record<string, React.CSSProperties> = {
  appRoot: {
    display: 'flex',
    flexDirection: 'row',
    width: '100vw',
    height: '100vh',
    overflow: 'hidden',
    background: '#080c12',
    fontFamily: "'Inter', 'Segoe UI', system-ui, sans-serif",
    color: '#c8dff0',
  },
  sidePanel: {
    width: '280px',
    minWidth: '280px',
    height: '100%',
    background: '#0d1520',
    borderRight: '1px solid #1a2a3a',
    display: 'flex',
    flexDirection: 'column',
    overflow: 'hidden',
  },
  header: {
    padding: '24px 20px 16px',
    borderBottom: '1px solid #1a2a3a',
  },
  title: {
    margin: 0,
    fontSize: '22px',
    fontWeight: 700,
    lineHeight: 1.2,
    color: '#d8eeff',
    letterSpacing: '-0.02em',
  },
  subtitle: {
    margin: '6px 0 0',
    fontSize: '11px',
    color: '#4a6a8a',
    letterSpacing: '0.1em',
    textTransform: 'uppercase',
  },
  loadingBadge: {
    margin: '12px 20px',
    padding: '8px 12px',
    background: '#1a2535',
    borderRadius: '4px',
    fontSize: '12px',
    color: '#7a9fc4',
    border: '1px solid #2a3f55',
  },
  panelContent: {
    flex: 1,
    overflowY: 'auto',
    padding: '20px',
    display: 'flex',
    flexDirection: 'column',
    gap: '20px',
  },
  divider: {
    height: '1px',
    background: '#1a2a3a',
    margin: '0 -20px',  // bleed to panel edges
  },
  footer: {
    padding: '12px 20px',
    fontSize: '11px',
    color: '#2a4a6a',
    borderTop: '1px solid #1a2a3a',
    letterSpacing: '0.05em',
  },
  viewport: {
    flex: 1,
    height: '100%',
    position: 'relative',
  },
};

export default App;
