#!/usr/bin/env python3
"""Choose P1, P2 and one event, then open its photon-count heatmap.

Run: python3 /home/praful/TRIGGER/python/Singular_Selective_Heatmap.py
Event numbers in the menu start at 1 and follow CSV order. Each event is
identified by experiment_id, run_id and event_id, so repeated event IDs
from separate simulations remain separate.
"""
from pathlib import Path
import csv
import math

import matplotlib.pyplot as plt
import numpy as np

BASE = Path(__file__).resolve().parent.parent
DATA_FILE = BASE / "Output_Data" / "MOTHER.csv"


def read_events():
    """Group sensor rows by parameters and by the full event identity."""
    csv.field_size_limit(100_000_000)
    configurations = {}
    with DATA_FILE.open(newline="") as file:
        for row in csv.DictReader(file):
            p1 = float(row["P1(N/MeV)"])
            p2 = float(row["P2(Absorption Length)"])
            if not all(math.isfinite(value) and value > 0 for value in (p1, p2)):
                raise ValueError("P1 and P2 must be finite and positive.")
            if row["P2_unit"] != "cm":
                raise ValueError("Expected P2 in cm.")
            event_key = (row["experiment_id"], row["run_id"], row["event_id"])
            events = configurations.setdefault((p1, p2), {})
            sensors = events.setdefault(event_key, {})
            sensor_id = int(row["sensor_id"])
            if sensor_id in sensors:
                raise ValueError(f"Repeated sensor {sensor_id} in event {event_key}.")
            x = float(row["sensor_x_mm"])
            y = float(row["sensor_y_mm"])
            count = int(row["photon_count"])
            if not math.isfinite(x) or not math.isfinite(y) or count < 0:
                raise ValueError("Invalid sensor coordinates or photon count.")
            sensors[sensor_id] = (x, y, count)
    if not configurations:
        raise ValueError("The CSV contains no events.")
    return configurations


def ask_number(prompt, maximum):
    """Return an event number, or None when the user goes back."""
    while True:
        try:
            answer = input(prompt).strip().lower()
            if answer == "b":
                return None
            number = int(answer)
            if 1 <= number <= maximum:
                return number
        except ValueError:
            pass
        print(f"Enter a number from 1 to {maximum}, or b to go back.")


def choose_parameter(label, values, unit, allow_back=False):
    print(f"\nAvailable {label} values ({unit}):")
    print(", ".join(f"{value:g}" for value in values))
    while True:
        try:
            hint = " (b to go back)" if allow_back else ""
            answer = input(f"Enter the {label} value{hint}: ").strip().lower()
            if allow_back and answer == "b":
                return None
            value = float(answer)
            if value in values:
                return value
        except ValueError:
            pass
        print("Please enter one of the listed values.")


def show_heatmap(p1, p2, event_key, sensors, event_number):
    """Use physical coordinates for placement, then mirror the displayed x-axis."""
    if set(sensors) != set(range(100)):
        raise ValueError("The selected event must contain all 100 SiPM sensors.")
    xs = sorted({x for x, y, count in sensors.values()})
    ys = sorted({y for x, y, count in sensors.values()})
    counts = {(x, y): count for x, y, count in sensors.values()}
    if len(xs) != 10 or len(ys) != 10 or len(counts) != 100:
        raise ValueError("Expected a complete 10 by 10 sensor grid.")
    dx, dy = xs[1] - xs[0], ys[1] - ys[0]
    if not np.allclose(np.diff(xs), dx) or not np.allclose(np.diff(ys), dy):
        raise ValueError("Expected evenly spaced sensor coordinates.")
    grid = np.array([[counts[(x, y)] for x in xs] for y in ys])
    experiment_id, run_id, event_id = event_key

    fig, ax = plt.subplots(figsize=(10, 9))
    image = ax.imshow(
        grid, origin="lower", cmap="inferno", vmin=0,
        vmax=max(1, int(grid.max())),
        extent=(xs[0]-dx/2, xs[-1]+dx/2, ys[0]-dy/2, ys[-1]+dy/2),
    )
    ax.invert_xaxis()
    for y_index, y in enumerate(ys):
        for x_index, x in enumerate(xs):
            count = grid[y_index, x_index]
            color = "black" if image.norm(count) > 0.55 else "white"
            ax.text(x, y, f"{count:,}", ha="center", va="center",
                    fontsize=7, color=color)
    ax.set(
        xlabel="Physical sensor x (mm), mirrored display",
        ylabel="Physical sensor y (mm)",
        title=(f"P1 = {p1:g} photons/MeV; P2 = {p2:g} cm\n"
               f"Selected event {event_number}: run {run_id}, event ID {event_id}\n"
               f"Experiment: {experiment_id}\n"
               f"{grid.sum():,} collected photons"),
    )
    fig.colorbar(image, ax=ax, label="Photon count per SiPM")
    fig.tight_layout()
    print("Opening heatmap. Close its window to select another event.")
    plt.show(block=True)
    plt.close(fig)


def main():
    print(f"Reading {DATA_FILE} ...")
    configurations = read_events()
    print("Press Ctrl+C to quit at any selection prompt.")
    while True:
        p1 = choose_parameter(
            "P1", sorted({p1 for p1, p2 in configurations}), "photons/MeV"
        )
        while True:
            p2 = choose_parameter(
                "P2", sorted(p2 for a, p2 in configurations if a == p1),
                "cm", allow_back=True,
            )
            if p2 is None:
                break  # Return to P1 selection.
            events = configurations[(p1, p2)]
            event_keys = list(events)
            print(f"\nThere are {len(events)} events for P1={p1:g}, P2={p2:g}.")
            print("Event numbers follow CSV order, starting at 1.")
            while True:
                number = ask_number("Event number (b to select P2): ", len(events))
                if number is None:
                    break  # Return to P2 selection.
                event_key = event_keys[number - 1]
                show_heatmap(p1, p2, event_key, events[event_key], number)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, csv.Error) as error:
        raise SystemExit(f"Error: {error}")
    except (KeyboardInterrupt, EOFError):
        print("\nSelection cancelled.")
