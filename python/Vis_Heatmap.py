#!/usr/bin/env python3
"""Display photon-count heatmaps from vis.mac's Collected_Data.csv.

Run with: python3 /home/praful/TRIGGER/python/Vis_Heatmap.py
Reads the CSV once; never modifies data or saves plots. Event selection is
1-based in CSV order; plot titles also show the original run/event IDs.
"""
import csv
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
import numpy as np

DATA_FILE = Path(__file__).resolve().parent.parent / "Output_Data" / "Collected_Data.csv"
GRID_SIZE = 10
SENSOR_PITCH_MM = 16  # Current detector geometry; raw CSV has no coordinates.


def read_events(path=DATA_FILE):
    csv.field_size_limit(100_000_000)
    events = {}
    with path.open(newline="") as source:
        reader = csv.DictReader(source)
        required = {"run_id", "event_id", "sensor_id", "photon_count"}
        if not required.issubset(reader.fieldnames or []):
            raise ValueError(f"CSV must contain columns: {', '.join(sorted(required))}")
        for row in reader:
            key = (int(row["run_id"]), int(row["event_id"]))
            sensor, count = int(row["sensor_id"]), int(row["photon_count"])
            if not 0 <= sensor < GRID_SIZE**2 or count < 0:
                raise ValueError(f"Invalid sensor ID or photon count in event {key}.")
            sensors = events.setdefault(key, {})
            if sensor in sensors:
                raise ValueError(f"Duplicate sensor {sensor} in event {key}.")
            sensors[sensor] = count
    if not events:
        raise ValueError("No events found. Run the simulation and close it before reading its output.")
    for key, sensors in events.items():
        if len(sensors) != GRID_SIZE**2:
            raise ValueError(f"Event {key} is incomplete: expected 100 sensor rows.")
    return [(key, np.array([sensors[s] for s in range(GRID_SIZE**2)],
                           dtype=np.int64).reshape(GRID_SIZE, GRID_SIZE))
            for key, sensors in events.items()]


def show_heatmap(grid, title):
    fig, ax = plt.subplots(figsize=(10, 9))
    half_width = GRID_SIZE * SENSOR_PITCH_MM / 2
    palette = LinearSegmentedColormap.from_list("black_yellow", ["black", "yellow"])
    image = ax.imshow(grid, origin="lower", cmap=palette, vmin=0,
                      vmax=max(1, int(grid.max())),
                      extent=(-half_width, half_width, -half_width, half_width))
    ax.invert_xaxis()
    for row in range(GRID_SIZE):
        for col in range(GRID_SIZE):
            count = int(grid[row, col])
            ax.text((col - 4.5) * SENSOR_PITCH_MM,
                    (row - 4.5) * SENSOR_PITCH_MM, f"{count:,}",
                    ha="center", va="center", fontsize=7,
                    color="black" if image.norm(count) > .55 else "white")
    ax.set(xlabel="Sensor x (mm), mirrored display", ylabel="Sensor y (mm)",
           title=f"{title}\n{int(grid.sum()):,} collected photons")
    fig.colorbar(image, ax=ax, label="Collected photon count per sensor")
    fig.tight_layout()
    quit_requested = False

    def on_key(event):
        nonlocal quit_requested
        if event.key and event.key.lower() == "q":
            quit_requested = True
            plt.close(fig)

    fig.canvas.mpl_connect("key_press_event", on_key)
    print("Close the plot to return; press q in the plot window to quit.")
    plt.show(block=True)
    plt.close(fig)
    return quit_requested


def main():
    print(f"Reading {DATA_FILE}", flush=True)
    events = read_events()
    total = np.sum([grid for key, grid in events], axis=0)
    print(f"Loaded {len(events):,} events. Enter q at any prompt to quit.")
    print("Event selection numbers start at 1 in CSV order, not raw event IDs.")
    while True:
        choice = input("\n1: Full set (summed heatmap)\n2: Per event\nq: Quit\nSelect: ").strip().lower()
        if choice == "q":
            return
        if choice == "1":
            if show_heatmap(total, f"All {len(events):,} gamma events (summed)"):
                return
        elif choice == "2":
            while True:
                answer = input(f"Event number (1–{len(events)}), b: back, q: quit: ").strip().lower()
                if answer == "q":
                    return
                if answer == "b":
                    break
                try:
                    number = int(answer)
                    if not 1 <= number <= len(events):
                        raise ValueError
                except ValueError:
                    print(f"Enter an event number from 1 to {len(events)}, b, or q.")
                    continue
                (run_id, event_id), grid = events[number - 1]
                if show_heatmap(grid, f"Event {number} of {len(events)} | Run {run_id}, raw event ID {event_id}"):
                    return
        else:
            print("Enter 1, 2, or q.")


if __name__ == "__main__":
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        print("\nViewer closed.")
    except (OSError, ValueError, csv.Error) as error:
        raise SystemExit(f"Error: {error}")
