import React from 'react';

// ─────────────────────────────────────────────────────────────────────────────
//  ProjectionSelector.tsx
//
//  Renders a row of projection type buttons. Each button calls
//  engine_set_projection(id) via the WASM module when clicked.
// ─────────────────────────────────────────────────────────────────────────────

export type ProjectionId = 0 | 1 | 2 | 3;

export interface ProjectionDefinition {
  id: ProjectionId;
  label: string;
  description: string;
}

export const PROJECTIONS: ProjectionDefinition[] = [
  {
    id: 0,
    label: 'Equirectangular',
    description: 'Maps latitude and longitude directly to x and y. The simplest spherical projection — every grid square covers equal angle ranges.',
  },
  {
    id: 1,
    label: 'Stereographic',
    description: 'Projects from one pole through the sphere onto a tangent plane. Conformal: preserves local angles and shapes, but distorts area away from the center.',
  },
  {
    id: 2,
    label: 'Gnomonic',
    description: 'Projects from the sphere center onto a tangent plane. Unique property: all great circles (geodesics) appear as straight lines.',
  },
  {
    id: 3 as ProjectionId,
    label: 'Mercator',
    description: 'Cylindrical conformal projection. Preserves angles and compass bearings, making it ideal for navigation — but inflates areas near the poles.',
  },
];

interface ProjectionSelectorProps {
  selectedId: ProjectionId;
  onSelect: (id: ProjectionId) => void;
}

const ProjectionSelector: React.FC<ProjectionSelectorProps> = ({ selectedId, onSelect }) => {
  const selectedProjection = PROJECTIONS.find(p => p.id === selectedId)!;

  return (
    <div style={styles.container}>
      <div style={styles.label}>Projection</div>

      <div style={styles.buttonRow}>
        {PROJECTIONS.map((projection) => (
          <button
            key={projection.id}
            style={{
              ...styles.button,
              ...(projection.id === selectedId ? styles.buttonActive : styles.buttonInactive),
            }}
            onClick={() => onSelect(projection.id)}
          >
            {projection.label}
          </button>
        ))}
      </div>

      <p style={styles.description}>{selectedProjection.description}</p>
    </div>
  );
};

const styles: Record<string, React.CSSProperties> = {
  container: {
    display: 'flex',
    flexDirection: 'column',
    gap: '8px',
  },
  label: {
    fontSize: '11px',
    fontWeight: 600,
    letterSpacing: '0.08em',
    textTransform: 'uppercase',
    color: '#7a9fc4',
  },
  buttonRow: {
    display: 'flex',
    flexWrap: 'wrap',
    gap: '6px',
  },
  button: {
    padding: '6px 12px',
    borderRadius: '4px',
    border: '1px solid transparent',
    fontSize: '13px',
    fontFamily: 'inherit',
    cursor: 'pointer',
    transition: 'background 0.15s, border-color 0.15s',
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
  description: {
    fontSize: '12px',
    color: '#6a8aaa',
    lineHeight: 1.5,
    margin: 0,
    padding: '8px',
    background: '#111a25',
    borderRadius: '4px',
    borderLeft: '2px solid #2a5080',
  },
};

export default ProjectionSelector;
