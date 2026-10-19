#!/usr/bin/env python3
"""Plot the SiPM array with photon counts and collected energies."""

import argparse
import csv
import math
from pathlib import Path


def load_data(path, run_id=None, event_id=None):
    counts = [0] * 16
    energy_totals = [0.0] * 16
    events = {}
    seen = set()
    # Energy lists can exceed csv's default field-size limit.
    csv.field_size_limit(100_000_000)
    with path.open(newline="") as source:
        reader = csv.DictReader(source)
        required = {"run_id", "event_id", "sensor_id", "photon_count", "energies_eV"}
        if not required.issubset(reader.fieldnames or []):
            raise ValueError("CSV does not contain the expected photon columns.")
        for row in reader:
            try:
                run, event, sensor, count = (
                    int(row[name]) for name in
                    ("run_id", "event_id", "sensor_id", "photon_count")
                )
                if run_id is not None and run != run_id:
                    continue
                if event_id is not None and event != event_id:
                    continue
                values = [float(value) for value in row["energies_eV"].split()]
                if not 0 <= sensor < 16 or count < 0 or count != len(values):
                    raise ValueError("Invalid sensor ID or photon count/energy-list mismatch")
                if any(not math.isfinite(value) or value <= 0 for value in values):
                    raise ValueError("Photon energies must be finite and positive")
                key = (run, event, sensor)
                if key in seen:
                    raise ValueError("Duplicate run/event/sensor row")
                seen.add(key)
                counts[sensor] += count
                energy_totals[sensor] += math.fsum(values)
                events[(run, event)] = events.get((run, event), 0) + count
            except (ValueError, TypeError, AttributeError) as error:
                raise ValueError(f"CSV line {reader.line_num}: {error}") from error
    if not events:
        raise ValueError("No events found for the selected run/event.")
    if len(seen) != 16 * len(events):
        raise ValueError("Incomplete event: expected all 16 sensor rows. Wait for the event to finish.")
    return counts, energy_totals, events


def main():
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", nargs="?", type=Path, default=root / "build" / "photons.csv")
    parser.add_argument("--run", type=int, help="Plot only this run ID")
    parser.add_argument("--event", type=int, help="Plot only this event ID (within selected runs)")
    parser.add_argument("--output", type=Path, help="Output PNG path")
    parser.add_argument("--no-show", action="store_true", help="Save without opening a window")
    args = parser.parse_args()

    try:
        counts, energy_totals, events = load_data(args.csv, args.run, args.event)
    except (OSError, ValueError, csv.Error) as error:
        parser.exit(1, f"Error: {error}\n")

    import matplotlib
    if args.no_show:
        matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(figsize=(9, 8), constrained_layout=True)
    total = sum(counts)
    fig.suptitle(
        f"SiPM collection: {len(events)} events, {total} optical photons\n"
        f"Total collected photon energy: {math.fsum(energy_totals):.2f} eV"
    )
    grid = [counts[row * 4:(row + 1) * 4] for row in range(4)]
    im = ax.imshow(grid, origin="lower", cmap="Blues", vmin=0, vmax=max(1, max(counts)))
    for row in range(4):
        for column in range(4):
            sensor = row * 4 + column
            color = "white" if counts[sensor] > max(counts) * 0.6 else "black"
            ax.text(column, row,
                    f"SiPM {sensor}\n{counts[sensor]} photons\n{energy_totals[sensor]:.2f} eV",
                    ha="center", va="center", color=color)
    ax.set(title="Array layout — summed collected photon energy per SiPM",
           xlabel="Column (+x →)", ylabel="Row (+y ↑)")
    ax.set_xticks(range(4))
    ax.set_yticks(range(4))
    fig.colorbar(im, ax=ax, label="Photon count")

    output = args.output or root / "photon_plots.png"
    fig.savefig(output, dpi=160)
    print(f"Read: {args.csv.resolve()}")
    print(f"Events: {len(events)} | Collected photons: {total}")
    print(f"Saved: {output.resolve()}")
    if not args.no_show:
        plt.show()
    plt.close(fig)


if __name__ == "__main__":
    main()
