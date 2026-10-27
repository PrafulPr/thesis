#!/usr/bin/env python3
"""After simulation exit, merge the two Geant4 CSVs into an append-only dataset.

One row per event/sensor; event results repeat across its 100 sensor rows.
P1/P2 are read per event from the simulation summary (photons/MeV and cm).
"""
import argparse
import csv
import fcntl
import hashlib
import io
import math
import uuid
from datetime import datetime
from pathlib import Path

BASE = Path(__file__).resolve().parent
FIELDS = ['INDEX tag', 'experiment_id', 'experiment_time', 'input_sha256',
          'run_id', 'event_id', 'source_x_mm', 'source_y_mm', 'source_z_mm',
          'generated', 'absorbed', 'collected', 'sensor_id', 'sensor_x_mm',
          'sensor_y_mm', 'photon_count', 'energies_eV', 'sensor_energy_eV',
          'event_energy_eV', 'P1(N/MeV)', 'P2(Absorption Length)', 'P2_unit',
          'parameter_source', 'pitch_mm', 'analysis_version']
for cm in (3, 5):
    FIELDS += [f'N(CM = {cm})', f'x_wa_mm(CM = {cm})',
               f'y_wa_mm(CM = {cm})', f'bullseye_sensor(CM = {cm})',
               f'status(CM = {cm})']
    FIELDS += [f'E_fraction_N{n}(CM = {cm})' for n in range(1, 6)]


def positive(value):
    number = float(value)
    if not math.isfinite(number) or number <= 0:
        raise argparse.ArgumentTypeError('must be finite and positive')
    return number


def block(sensor, n):
    """Even N extends toward +x/+y; shift full square inward at edges.

    This rule produces nested blocks for N=1..10 around a fixed sensor.
    """
    r, c = divmod(sensor, 10)
    r0 = max(0, min(10-n, r-(n-1)//2))
    c0 = max(0, min(10-n, c-(n-1)//2))
    return [y*10+x for y in range(r0, r0+n) for x in range(c0, c0+n)]


def analyze(energies, cm, pitch):
    suffix = f'(CM = {cm})'
    total = math.fsum(energies)
    out = {f'N{suffix}': '', f'x_wa_mm{suffix}': '',
           f'y_wa_mm{suffix}': '', f'bullseye_sensor{suffix}': '',
           f'status{suffix}': 'zero_collected_energy'}
    # Requested N column has a space before the parenthesis.
    out[f'N(CM = {cm})'] = out.pop(f'N{suffix}')
    out.update({f'E_fraction_N{n}{suffix}': '' for n in range(1, 6)})
    if total == 0:
        return out
    peak = max(range(100), key=lambda s: energies[s])  # ties: lowest ID
    selected = block(peak, cm)
    weight = math.fsum(energies[s] for s in selected)
    x = math.fsum(energies[s]*((s % 10)-4.5)*pitch for s in selected)/weight
    y = math.fsum(energies[s]*((s // 10)-4.5)*pitch for s in selected)/weight
    # Half-open sensor cells; an internal boundary belongs to the + side.
    col = min(9, max(0, math.floor(x/pitch+5)))
    row = min(9, max(0, math.floor(y/pitch+5)))
    bull = row*10+col
    fractions = [math.fsum(energies[s] for s in block(bull, n))/total
                 for n in range(1, 6)]
    found = next((n for n, f in enumerate(fractions, 1) if f > 0.90), '')
    out.update({f'N(CM = {cm})': found, f'x_wa_mm{suffix}': x,
                f'y_wa_mm{suffix}': y, f'bullseye_sensor{suffix}': bull,
                f'status{suffix}': 'ok' if found else 'not_reached_within_5'})
    out.update({f'E_fraction_N{n}{suffix}': f for n, f in enumerate(fractions, 1)})
    return out


def read_inputs(photon_bytes, summary_bytes):
    csv.field_size_limit(100_000_000)
    summaries, events = {}, {}
    summary_reader = csv.DictReader(io.StringIO(summary_bytes.decode()))
    parameter_columns = {'p1_photons_per_MeV', 'p2_cm'}
    if not parameter_columns.issubset(summary_reader.fieldnames or []):
        raise ValueError(
            'Summary CSV is missing p1_photons_per_MeV and/or p2_cm. '
            'Rebuild the updated C++ simulation and finish a new run; '
            'old CSVs do not contain these settings.')
    for row in summary_reader:
        key = (int(row['run_id']), int(row['event_id']))
        if key in summaries:
            raise ValueError(f'Duplicate summary event {key}')
        for name in ('source_x_mm', 'source_y_mm', 'source_z_mm'):
            if not math.isfinite(float(row[name])):
                raise ValueError('Nonfinite source coordinate')
        for name in ('generated', 'absorbed', 'collected'):
            if int(row[name]) < 0:
                raise ValueError('Negative summary count')
        for name in ('p1_photons_per_MeV', 'p2_cm'):
            try:
                value = float(row[name])
            except (ValueError, TypeError) as error:
                raise ValueError(f'Invalid simulation parameter {name} for event {key}') from error
            if not math.isfinite(value) or value <= 0:
                raise ValueError(f'Invalid simulation parameter {name} for event {key}')
        summaries[key] = row
    for row in csv.DictReader(io.StringIO(photon_bytes.decode())):
        key = (int(row['run_id']), int(row['event_id']))
        sensor, count = int(row['sensor_id']), int(row['photon_count'])
        values = [float(v) for v in row['energies_eV'].split()]
        if not 0 <= sensor < 100 or count < 0 or len(values) != count:
            raise ValueError('Invalid sensor/count/energy list')
        if any(not math.isfinite(v) or v <= 0 for v in values):
            raise ValueError('Invalid photon energy')
        sensors = events.setdefault(key, {})
        if sensor in sensors:
            raise ValueError(f'Duplicate sensor for event {key}')
        sensors[sensor] = (count, row['energies_eV'], math.fsum(values))
    if not events or events.keys() != summaries.keys():
        raise ValueError('Input files empty or event sets differ')
    for key, sensors in events.items():
        if len(sensors) != 100:
            raise ValueError(f'Incomplete event {key}: expected 100 sensors')
        if sum(v[0] for v in sensors.values()) != int(summaries[key]['collected']):
            raise ValueError(f'Collected count mismatch for {key}')
    return summaries, events


def collect(args):
    paths = [args.photons.resolve(), args.summary.resolve()]
    if args.output.resolve() in paths:
        raise ValueError('Output must differ from both input files')
    def signature(p):
        s = p.stat()
        return s.st_ino, s.st_size, s.st_mtime_ns
    before = [signature(p) for p in paths]
    raw = [p.read_bytes() for p in paths]
    summaries, events = read_inputs(*raw)
    if before != [signature(p) for p in paths]:
        raise ValueError('Inputs changed: wait for simulation exit')
    digest = hashlib.sha256(raw[0]+b'\0'+raw[1]).hexdigest()
    now = datetime.now().astimezone()
    common = {'INDEX tag': now.strftime('%m%d%H%M%S'),
              'experiment_id': str(uuid.uuid4()), 'experiment_time': now.isoformat(),
              'input_sha256': digest, 'P2_unit': 'cm',
              'parameter_source': 'simulation_material_properties', 'pitch_mm': args.pitch_mm,
              'analysis_version': '1; E_fraction>0.90; N<=5; inward_edge_shift'}
    # Validate and calculate everything before opening the destination.
    buffer = io.StringIO(newline='')
    writer = csv.DictWriter(buffer, fieldnames=FIELDS)
    for key, sensors in events.items():
        energy = [sensors[s][2] for s in range(100)]
        event = dict(common)
        event['P1(N/MeV)'] = float(summaries[key]['p1_photons_per_MeV'])
        event['P2(Absorption Length)'] = float(summaries[key]['p2_cm'])
        event.update({name: summaries[key][name] for name in
                      ('run_id', 'event_id', 'source_x_mm', 'source_y_mm',
                       'source_z_mm', 'generated', 'absorbed', 'collected')})
        event['event_energy_eV'] = math.fsum(energy)
        for cm in (3, 5):
            event.update(analyze(energy, cm, args.pitch_mm))
        for s in range(100):
            count, values, total = sensors[s]
            writer.writerow(dict(event, sensor_id=s, photon_count=count,
                                 energies_eV=values, sensor_energy_eV=total,
                                 sensor_x_mm=(s%10-4.5)*args.pitch_mm,
                                 sensor_y_mm=(s//10-4.5)*args.pitch_mm))
    with args.output.open('a+', newline='') as destination:
        fcntl.flock(destination, fcntl.LOCK_EX)
        destination.seek(0)
        reader = csv.DictReader(destination)
        if reader.fieldnames is not None and reader.fieldnames != FIELDS:
            raise ValueError('Existing output has a different schema')
        for row in reader:
            if row['input_sha256'] == digest:
                raise ValueError('These exact input files were already imported; no rows added')
        destination.seek(0, 2)
        start = destination.tell()
        try:
            if start == 0:
                csv.DictWriter(destination, fieldnames=FIELDS).writeheader()
            destination.write(buffer.getvalue())
            destination.flush()
            import os
            os.fsync(destination.fileno())
        except BaseException:
            destination.seek(start)
            destination.truncate()
            raise
    print(f'Added {len(events)} events ({len(events)*100} sensor rows) to {args.output}')
    print(f'INDEX tag: {common["INDEX tag"]}; experiment_id: {common["experiment_id"]}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--photons', type=Path, default=BASE/'build/photons.csv')
    parser.add_argument('--summary', type=Path, default=BASE/'build/photon_summary.csv')
    parser.add_argument('--output', type=Path, default=BASE/'experiment_data.csv')
    parser.add_argument('--pitch-mm', type=positive, default=16, help='Sensor pitch; current geometry: 16 mm')
    args = parser.parse_args()
    try:
        collect(args)
    except (OSError, ValueError, KeyError, csv.Error) as error:
        parser.exit(1, f'Error: {error}\n')


if __name__ == '__main__':
    main()
