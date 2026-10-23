#!/usr/bin/env python3
"""Plot the SiPM array with photon counts and collected energies."""

import argparse
import csv
import math
from pathlib import Path


ROWS = 10
COLUMNS = 10
SENSOR_COUNT = ROWS * COLUMNS


def load_data(path, run_id=None, event_id=None):
    counts = [0] * SENSOR_COUNT
    energy_totals = [0.0] * SENSOR_COUNT
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
                if not 0 <= sensor < SENSOR_COUNT or count < 0 or count != len(values):
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
    if len(seen) != SENSOR_COUNT * len(events):
        raise ValueError("Incomplete event: expected all 100 sensor rows. Wait for the event to finish.")
    return counts, energy_totals, events


def load_summary(path, events):
    selected = {}
    with path.open(newline="") as source:
        for row in csv.DictReader(source):
            key = (int(row["run_id"]), int(row["event_id"]))
            if key not in events:
                continue
            if key in selected:
                raise ValueError("Duplicate event in photon summary")
            values = tuple(int(row[name]) for name in ("generated", "absorbed", "collected"))
            if min(values) < 0 or values[2] != events[key]:
                raise ValueError("Photon summary does not match sensor data")
            selected[key] = values
    if selected.keys() != events.keys():
        raise ValueError("Missing summary for selected events; rerun the updated simulation")
    return tuple(sum(values[i] for values in selected.values()) for i in range(3))


def main():
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", nargs="?", type=Path, default=root / "build" / "photons.csv")
    parser.add_argument("--summary", type=Path, help="Summary CSV (default: beside photon CSV)")
    parser.add_argument("--run", type=int, help="Plot only this run ID")
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument("--event", type=int, help="Plot only this event ID (within selected runs)")
    selection.add_argument("--recent", type=int, metavar="N",
                           help="Plot the Nth most recent saved event: 0 = latest, 1 = previous (optionally within --run)")
    parser.add_argument("--output", type=Path, help="Output PNG path")
    parser.add_argument("--no-show", action="store_true", help="Save without opening a window")
    args = parser.parse_args()

    if args.recent is not None and args.recent < 0:
        parser.error("--recent must be 0 or greater")

    try:
        if args.recent is not None:
            _, _, available = load_data(args.csv, args.run)
            # Events are inserted in CSV order, including events with no hits.
            newest_first = list(reversed(available))
            if args.recent >= len(newest_first):
                raise ValueError(
                    f"Only {len(newest_first)} saved events available; "
                    f"--recent must be between 0 and {len(newest_first) - 1}")
            args.run, args.event = newest_first[args.recent]
        counts, energy_totals, events = load_data(args.csv, args.run, args.event)
        generated, absorbed, collected = load_summary(
            args.summary or args.csv.with_name("photon_summary.csv"), events)
    except (OSError, ValueError, KeyError, csv.Error) as error:
        parser.exit(1, f"Error: {error}\n")

    import matplotlib
    if args.no_show:
        matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(figsize=(15, 14))
    fig.subplots_adjust(bottom=0.18, top=0.83)
    fig.text(0.5, 0.035,
             f"Generated: {generated:,}   |   Absorbed: {absorbed:,}   |   SiPMs total: {collected:,}\n"
             "Optical photons; absorption excludes SiPM arrival scoring",
             ha="center", va="bottom", fontsize=10)
    total = sum(counts)
    event_label = f"{len(events)} events"
    if len(events) == 1:
        run, event = next(iter(events))
        event_label = f"run {run}, event {event}"
    fig.suptitle(
        f"SiPM collection: {event_label}, {total} optical photons\n"
        f"Total collected photon energy: {math.fsum(energy_totals):.2f} eV"
    )
    grid = [counts[row * COLUMNS:(row + 1) * COLUMNS] for row in range(ROWS)]
    im = ax.imshow(grid, origin="lower", cmap="Blues", vmin=0, vmax=max(1, max(counts)))
    for row in range(ROWS):
        for column in range(COLUMNS):
            sensor = row * COLUMNS + column
            color = "white" if counts[sensor] > max(counts) * 0.6 else "black"
            ax.text(column, row,
                    f"SiPM {sensor}\n{counts[sensor]} photons\n{energy_totals[sensor]:.2f} eV",
                    ha="center", va="center", color=color, fontsize=7)
    ax.set(title="10×10 SiPM array — photon counts and collected energies",
           xlabel="Column (← +x)", ylabel="Row (+y ↑)")
    ax.set_xticks(range(COLUMNS))
    ax.set_yticks(range(ROWS))
    # Mirror the viewing direction while keeping sensor IDs and values together.
    ax.invert_xaxis()
    fig.colorbar(im, ax=ax, label="Photon count")

    output = args.output or root / "photon_plots.png"
    fig.savefig(output, dpi=160)
    print(f"Read: {args.csv.resolve()}")
    print(f"Selection: {event_label} | Collected photons: {total}")
    print(f"Saved: {output.resolve()}")
    if not args.no_show:
        plt.show()
    plt.close(fig)


if __name__ == "__main__":
    main()
