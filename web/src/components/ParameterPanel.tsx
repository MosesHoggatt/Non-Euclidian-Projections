import React from 'react';

// ─────────────────────────────────────────────────────────────────────────────
//  ParameterPanel.tsx
//
//  Sliders and toggles that control the engine's rendering parameters.
//  Each control calls the corresponding exported C function via the engine ref.
// ─────────────────────────────────────────────────────────────────────────────

export interface EngineParameters {
  gridDensity: number;
  objectType: number;
  isLit: boolean;
}

interface ParameterPanelProps {
  parameters: EngineParameters;
  onGridDensityChange: (density: number) => void;
  onObjectTypeChange:  (objectId: number) => void;
  onLitChange:         (isLit: boolean) => void;
}

const OBJECT_TYPES = [
  { id: 0, label: 'Sphere' },
  { id: 1, label: 'Torus'  },
];

const ParameterPanel: React.FC<ParameterPanelProps> = ({
  parameters,
  onGridDensityChange,
  onObjectTypeChange,
  onLitChange,
}) => {
  return (
    <div style={styles.container}>

      {/* ── Object type ──────────────────────────────────────────────── */}
      <div style={styles.group}>
        <div style={styles.groupLabel}>Object</div>
        <div style={styles.buttonRow}>
          {OBJECT_TYPES.map((obj) => (
            <button
              key={obj.id}
              style={{
                ...styles.button,
                ...(parameters.objectType === obj.id ? styles.buttonActive : styles.buttonInactive),
              }}
              onClick={() => onObjectTypeChange(obj.id)}
            >
              {obj.label}
            </button>
          ))}
        </div>
      </div>

      {/* ── Shading ───────────────────────────────────────────────────── */}
      <div style={styles.group}>
        <div style={styles.groupLabel}>Shading</div>
        <div style={styles.buttonRow}>
          <button
            style={{ ...styles.button, ...(parameters.isLit ? styles.buttonActive : styles.buttonInactive) }}
            onClick={() => onLitChange(true)}
          >Lit</button>
          <button
            style={{ ...styles.button, ...(!parameters.isLit ? styles.buttonActive : styles.buttonInactive) }}
            onClick={() => onLitChange(false)}
          >Unlit</button>
        </div>
      </div>

      {/* ── Grid density ─────────────────────────────────────────────── */}
      <div style={styles.group}>
        <div style={styles.groupLabel}>
          Grid Density
          <span style={styles.valueLabel}>{parameters.gridDensity}</span>
        </div>
        <input
          type="range"
          min={8}
          max={256}
          step={4}
          value={parameters.gridDensity}
          onChange={(e) => onGridDensityChange(parseInt(e.target.value, 10))}
          style={styles.slider}
        />
        <div style={styles.sliderBounds}>
          <span>Low (fast)</span>
          <span>High (smooth)</span>
        </div>
      </div>

      {/* ── Controls hint ─────────────────────────────────────────────── */}
      <div style={styles.hint}>
        <span style={styles.hintRow}>⟳ Drag to orbit</span>
        <span style={styles.hintRow}>⊕ Scroll to zoom</span>
      </div>

    </div>
  );
};

const styles: Record<string, React.CSSProperties> = {
  container: {
    display: 'flex',
    flexDirection: 'column',
    gap: '20px',
  },
  group: {
    display: 'flex',
    flexDirection: 'column',
    gap: '8px',
  },
  groupLabel: {
    fontSize: '11px',
    fontWeight: 600,
    letterSpacing: '0.08em',
    textTransform: 'uppercase',
    color: '#7a9fc4',
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  valueLabel: {
    fontSize: '13px',
    fontWeight: 400,
    color: '#c0daf5',
    letterSpacing: 'normal',
    textTransform: 'none',
  },
  buttonRow: {
    display: 'flex',
    gap: '6px',
  },
  button: {
    flex: 1,
    padding: '6px 10px',
    borderRadius: '4px',
    border: '1px solid transparent',
    fontSize: '13px',
    fontFamily: 'inherit',
    cursor: 'pointer',
    transition: 'background 0.15s',
    fontWeight: 500,
  },
  buttonActive: {
    background: '#2a5080',
    borderColor: '#4a90d9',
    color: '#e8f4ff',
  },
  buttonInactive: {
    background: '#1a2535',
    borderColor: '#2d3f55',
    color: '#8aaccc',
  },
  slider: {
    width: '100%',
    accentColor: '#4a90d9',
    cursor: 'pointer',
  },
  sliderBounds: {
    display: 'flex',
    justifyContent: 'space-between',
    fontSize: '11px',
    color: '#4a6a8a',
  },
  hint: {
    display: 'flex',
    flexDirection: 'column',
    gap: '4px',
    paddingTop: '8px',
    borderTop: '1px solid #1e2e40',
  },
  hintRow: {
    fontSize: '12px',
    color: '#4a6a8a',
  },
};

export default ParameterPanel;
