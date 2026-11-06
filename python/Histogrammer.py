#!/usr/bin/env python3
"""Save x/y centre histograms for every configuration in MOTHER.csv.

Centres use collected sensor energy in a 3x3 matrix around the peak-energy
sensor (ties: lowest sensor ID). Edge matrices shift inward. Gaussian curves
use maximum-likelihood normal fits to individual centres, not histogram bins.
Replaces plot images in Output_Data/Histograms. MOTHER.csv is read-only.
Requires numpy and matplotlib.
"""
import csv
import math
from pathlib import Path

import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

DATA_FILE = Path(__file__).resolve().parent.parent / 'Output_Data' / 'MOTHER.csv'
OUTPUT_DIR = DATA_FILE.parent / 'Histograms'
CENTRE_MATRIX_SIZE = 9 # May be changed to 4 or 5 for matrix-size comparisons. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!HERE FOR NxN CHANGE!!!!!!!!!!!!!!!!!!!!!!!!!!!11


def configuration(row):
    values = (float(row['P1(N/MeV)']), float(row['P2(Absorption Length)']))
    if row['P2_unit'] != 'cm' or not all(math.isfinite(v) and v > 0 for v in values):
        raise ValueError('Expected finite positive P1 and P2, with P2 in cm.')
    return values


def event_key(row):
    return row['experiment_id'], row['run_id'], row['event_id']


def weighted_centre(sensors, matrix_size=CENTRE_MATRIX_SIZE):
    """Return (x_wa, y_wa), or None for an event with zero collected energy."""
    if not 1 <= matrix_size <= 10:
        raise ValueError('Centre matrix size must be between 1 and 10.')
    if set(sensors) != set(range(100)):
        raise ValueError('Every event must contain all 100 distinct sensors.')
    xs = sorted({x for energy, x, y in sensors.values()})
    ys = sorted({y for energy, x, y in sensors.values()})
    if len(xs) != 10 or len(ys) != 10:
        raise ValueError('Expected a 10x10 sensor grid.')
    if any((x, y) != (xs[s % 10], ys[s // 10])
           for s, (energy, x, y) in sensors.items()):
        raise ValueError('Sensor IDs do not match the expected row/column coordinates.')
    peak = max(sorted(sensors), key=lambda s: sensors[s][0])
    if sensors[peak][0] == 0:
        return None
    r, c = divmod(peak, 10)
    r0 = max(0, min(10-matrix_size, r-(matrix_size-1)//2))
    c0 = max(0, min(10-matrix_size, c-(matrix_size-1)//2))
    selected = [sensors[y*10+x] for y in range(r0, r0+matrix_size)
                for x in range(c0, c0+matrix_size)]
    weight = math.fsum(e for e, x, y in selected)
    WeightedAverageCentre_x = math.fsum(e*x for e, x, y in selected) / weight
    WeightedAverageCentre_y = math.fsum(e*y for e, x, y in selected) / weight
    return WeightedAverageCentre_x, WeightedAverageCentre_y


def collect_all_centres():
    """Read once; retain compact centres instead of all sensor maps."""
    csv.field_size_limit(100_000_000)
    groups, pending, completed = {}, {}, set()
    with DATA_FILE.open(newline='') as source:
        for row in csv.DictReader(source):
            config = configuration(row)
            key = (config, event_key(row))
            if key in completed:
                raise ValueError(f'Duplicate rows for completed event {key}.')
            sensors = pending.setdefault(key, {})
            sensor = int(row['sensor_id'])
            if not 0 <= sensor < 100 or sensor in sensors:
                raise ValueError(f'Invalid or duplicate sensor {sensor} for event {key}.')
            values = tuple(float(row[name]) for name in
                           ('sensor_energy_eV', 'sensor_x_mm', 'sensor_y_mm'))
            if not all(math.isfinite(v) for v in values) or values[0] < 0:
                raise ValueError(f'Invalid sensor energy/coordinates for event {key}.')
            sensors[sensor] = values
            if len(sensors) == 100:
                centre = weighted_centre(sensors)
                group = groups.setdefault(config, dict(centres=[], total=0, zero=0))
                group['total'] += 1
                if centre is None:
                    group['zero'] += 1
                else:
                    group['centres'].append(centre)
                completed.add(key)
                del pending[key]
    if pending:
        raise ValueError(f'{len(pending)} incomplete events; expected 100 sensors each.')
    if not groups:
        raise ValueError('MOTHER.csv contains no events.')
    return groups


def plot_histograms(selected, centres, total_events, zero_light, output):
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    for column, (axis, name) in enumerate(zip(axes, ('x', 'y'))):
        values = centres[:, column]
        if not len(values):
            axis.text(.5, .5, 'No defined centres: all events have zero collected energy',
                      transform=axis.transAxes, ha='center', va='center', wrap=True)
            axis.set(xlabel=f'WeightedAverageCentre_{name} (mm)', ylabel='Number of events',
                     title=f'{name.upper()} centre: 0 event entries')
            continue
        counts, edges, _ = axis.hist(values, bins='auto', color='steelblue',
                                    edgecolor='white', alpha=.75, label='Event centres')
        mean = float(np.mean(values))
        sigma = float(np.std(values, ddof=0))  # Normal maximum-likelihood estimate.
        if len(values) >= 2 and sigma > 0:
            x = np.linspace(min(edges[0], mean-4*sigma), max(edges[-1], mean+4*sigma), 600)
            width = edges[1]-edges[0]
            curve = len(values)*width/(sigma*math.sqrt(2*math.pi))*np.exp(-.5*((x-mean)/sigma)**2)
            axis.plot(x, curve, color='crimson', linewidth=2, label='Gaussian fit (MLE)')
            axis.axvspan(mean-sigma, mean+sigma, color='crimson', alpha=.1, label='Mean ± sigma')
            text = f'Mean = {mean:.4g} mm\nSigma = {sigma:.4g} mm'
        else:
            text = f'Mean = {mean:.4g} mm\nNo nonzero-width Gaussian fit available'
        axis.axvline(mean, color='black', linestyle='--')
        axis.text(.97, .96, text, transform=axis.transAxes, ha='right', va='top',
                  bbox=dict(facecolor='white', alpha=.85, edgecolor='none'))
        axis.set(xlabel=f'WeightedAverageCentre_{name} (mm)', ylabel='Number of events',
                 title=f'{name.upper()} centre: {len(values):,} event entries')
        axis.legend(loc='upper left', fontsize=8)
        print(f'{name}: mean={mean:.6g} mm; sigma (Gaussian MLE)={sigma:.6g} mm')
    p1, p2 = selected
    fig.suptitle(f'P1 = {p1:g} photons/MeV; P2 = {p2:g} cm | '
                 f'{CENTRE_MATRIX_SIZE}x{CENTRE_MATRIX_SIZE} energy-weighted centres\n'
                 f'{total_events:,} events; {len(centres):,} plotted; {zero_light:,} zero-light events excluded')
    fig.text(.5, .01, 'Physical coordinates. Gaussian curves describe the centre distribution; inspect departures from the fit.',
             ha='center', fontsize=9)
    fig.tight_layout(rect=(0, .04, 1, .9))
    fig.savefig(output, dpi=180)
    plt.close(fig)
    print(f'Saved: {output}')


def main():
    print(f'Reading all configurations from {DATA_FILE} ...', flush=True)
    groups = collect_all_centres()
    # Validate and calculate the input before deleting previous plots.
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    image_extensions = {'.png', '.jpg', '.jpeg', '.svg', '.pdf', '.eps', '.webp', '.tif', '.tiff'}
    for path in OUTPUT_DIR.iterdir():
        if path.is_file() and path.suffix.lower() in image_extensions:
            path.unlink()
    print('Keep the true source position fixed to interpret sigma as position repeatability.')
    for (p1, p2), group in sorted(groups.items()):
        centres = np.asarray(group['centres'], dtype=float).reshape(-1, 2)
        output = OUTPUT_DIR / f"Histogram_Events_{group['total']}__P1_{p1:g}_P2_{p2:g}.png"
        print(f"P1={p1:g}, P2={p2:g}: {group['total']:,} events; "
              f"{group['zero']:,} zero-light events excluded.", flush=True)
        plot_histograms((p1, p2), centres, group['total'], group['zero'], output)
    print(f'Finished: {len(groups)} PNGs in {OUTPUT_DIR}')


if __name__ == '__main__':
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        print('\nHistogram generation stopped.')
    except (OSError, ValueError, KeyError, csv.Error) as error:
        raise SystemExit(f'Error: {error}')
