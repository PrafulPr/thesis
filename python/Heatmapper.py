#!/usr/bin/env python3
"""Save one summed photon-count heatmap per P1/P2 configuration in MOTHER.csv.

Replaces plot images in Output_Data/Heatmaps. All events, including zero-light
ones, are counted. Physical coordinates are preserved; the displayed x-axis is
mirrored. Requires numpy and matplotlib. MOTHER.csv is never modified.
"""
import csv
import math
from pathlib import Path

import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap, Normalize

DATA_FILE = Path(__file__).resolve().parent.parent / 'Output_Data' / 'MOTHER.csv'
OUTPUT_DIR = DATA_FILE.parent / 'Heatmaps'
COLOURS = LinearSegmentedColormap.from_list(
    'black_to_yellow', ['#000000', '#6b101b', '#df410c', '#ffae00', '#ffff00'])


def collect_heatmaps():
    """Accumulate counts in one pass and validate each event's 100 sensor rows."""
    csv.field_size_limit(100_000_000)
    groups = {}
    with DATA_FILE.open(newline='') as source:
        for row in csv.DictReader(source):
            config = (float(row['P1(N/MeV)']), float(row['P2(Absorption Length)']))
            if row['P2_unit'] != 'cm' or not all(math.isfinite(v) and v > 0 for v in config):
                raise ValueError('Expected finite positive P1/P2 and P2_unit=cm.')
            sensor = int(row['sensor_id'])
            count = int(row['photon_count'])
            xy = (float(row['sensor_x_mm']), float(row['sensor_y_mm']))
            if not 0 <= sensor < 100 or count < 0 or not all(math.isfinite(v) for v in xy):
                raise ValueError('Invalid sensor ID, photon count, or coordinates.')
            group = groups.setdefault(config, dict(counts=[0]*100, positions={}, events={}))
            if sensor in group['positions'] and group['positions'][sensor] != xy:
                raise ValueError(f'Coordinates changed for sensor {sensor}, configuration {config}.')
            group['positions'][sensor] = xy
            key = (row['experiment_id'], row['run_id'], row['event_id'])
            mask = group['events'].get(key, 0)
            if mask & (1 << sensor):
                raise ValueError(f'Duplicate sensor {sensor} for event {key}.')
            group['events'][key] = mask | (1 << sensor)
            group['counts'][sensor] += count
    if not groups:
        raise ValueError('MOTHER.csv contains no events.')
    for config, group in groups.items():
        if any(mask != (1 << 100)-1 for mask in group['events'].values()):
            raise ValueError(f'Incomplete event in configuration {config}; expected 100 sensors.')
        positions = group['positions']
        xs = sorted({x for x, y in positions.values()})
        ys = sorted({y for x, y in positions.values()})
        if len(xs) != 10 or len(ys) != 10 or len(set(positions.values())) != 100:
            raise ValueError(f'Expected a complete 10x10 sensor grid for {config}.')
        if not np.allclose(np.diff(xs), xs[1]-xs[0]) or not np.allclose(np.diff(ys), ys[1]-ys[0]):
            raise ValueError('Expected evenly spaced sensor coordinates.')
    return groups


def save_heatmap(config, group, norm, output):
    xs = sorted({x for x, y in group['positions'].values()})
    ys = sorted({y for x, y in group['positions'].values()})
    counts = {xy: group['counts'][sensor] for sensor, xy in group['positions'].items()}
    grid = np.array([[counts[x, y] for x in xs] for y in ys])
    dx, dy = xs[1]-xs[0], ys[1]-ys[0]
    fig, ax = plt.subplots(figsize=(10, 9))
    image = ax.imshow(grid, origin='lower', cmap=COLOURS, norm=norm,
                      extent=(xs[0]-dx/2, xs[-1]+dx/2, ys[0]-dy/2, ys[-1]+dy/2))
    ax.invert_xaxis()
    for j, y in enumerate(ys):
        for i, x in enumerate(xs):
            count = int(grid[j, i])
            ax.text(x, y, f'{count:,}', ha='center', va='center', fontsize=7,
                    color='black' if norm(count) > .65 else 'white')
    p1, p2 = config
    ax.set(xlabel='Physical sensor x (mm), mirrored display', ylabel='Physical sensor y (mm)',
           title=f'P1 = {p1:g} photons/MeV; P2 = {p2:g} cm\n'
                 f"{len(group['events']):,} events; {sum(group['counts']):,} collected photons")
    fig.colorbar(image, ax=ax, label='Summed photon count per SiPM (shared scale)')
    fig.tight_layout()
    fig.savefig(output, dpi=180)
    plt.close(fig)
    print(f'Saved: {output}', flush=True)


def main():
    print(f'Reading all configurations from {DATA_FILE} ...', flush=True)
    groups = collect_heatmaps()
    # Validate input before clearing prior plots; never delete dataset files.
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    image_extensions = {'.png', '.jpg', '.jpeg', '.svg', '.pdf', '.eps', '.webp', '.tif', '.tiff'}
    for path in OUTPUT_DIR.iterdir():
        if path.is_file() and path.suffix.lower() in image_extensions:
            path.unlink()
    norm = Normalize(0, max(1, max(max(g['counts']) for g in groups.values())))
    for (p1, p2), group in sorted(groups.items()):
        output = OUTPUT_DIR / f"Heatmap_Events_{len(group['events'])}__P1_{p1:g}_P2_{p2:g}.png"
        save_heatmap((p1, p2), group, norm, output)
    print(f'Finished: {len(groups)} PNGs in {OUTPUT_DIR}')


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print('\nHeatmap generation stopped.')
    except (OSError, ValueError, KeyError, csv.Error) as error:
        raise SystemExit(f'Error: {error}')
