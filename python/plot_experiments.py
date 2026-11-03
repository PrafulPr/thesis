#!/usr/bin/env python3
"""Plot the local experiment snapshot. Run: python3 plot_experiments.py
Heatmaps sum photon counts across events. N is recalculated for 1..5
from sensor_energy_eV using the saved CM-specific bullseye sensors. LEGO means/std use resolved events
only; unresolved events remain in the reported failure fractions.
"""
from pathlib import Path
import csv
import math
from collections import defaultdict, Counter
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize

import os
DATA_DIR = Path(__file__).resolve().parent.parent / 'Output_Data'
BASE = Path(os.environ.get('TRIGGER3_PLOT_DIR', DATA_DIR / 'plots'))

def main():
    configs = defaultdict(lambda: {'counts': Counter(), 'events': {}, 'energies': defaultdict(dict)})
    with (DATA_DIR / 'MOTHER.csv').open(newline='') as f:
        for row in csv.DictReader(f):
            if row['P2_unit'] != 'cm':
                raise ValueError('Expected P2 in cm')
            key = (float(row['P1(N/MeV)']), float(row['P2(Absorption Length)']))
            group = configs[key]
            xy = (float(row['sensor_x_mm']), float(row['sensor_y_mm']))
            group['counts'][xy] += int(row['photon_count'])
            event = (row['experiment_id'], row['run_id'], row['event_id'])
            values = tuple(row[f'bullseye_sensor(CM = {cm})'] for cm in (3, 5))
            sensor = int(row['sensor_id'])
            if sensor in group['energies'][event]:
                raise ValueError(f'Duplicate event/sensor: {event}, {sensor}')
            group['energies'][event][sensor] = float(row['sensor_energy_eV'])
            if event in group['events'] and group['events'][event] != values:
                raise ValueError(f'Inconsistent repeated event: {event}')
            group['events'][event] = values
    if not configs:
        raise ValueError('CSV contains no events')
    # Preserve each CM's energy-weighted centre/bullseye from the collector,
    # but recompute containment using the full available sensor-energy map.
    for group in configs.values():
        for event, bulls in list(group['events'].items()):
            energies = group['energies'][event]
            if set(energies) != set(range(100)):
                raise ValueError(f'Expected 100 distinct sensors: {event}')
            total = math.fsum(energies.values())
            results = []
            for bull in bulls:
                if total == 0:
                    results.append(('zero_collected_energy', ''))
                    continue
                r, c = divmod(int(bull), 10)
                found = None
                for n in range(1, 6):
                    r0 = max(0, min(10-n, r-(n-1)//2))
                    c0 = max(0, min(10-n, c-(n-1)//2))
                    fraction = math.fsum(energies[y*10+x] for y in range(r0,r0+n) for x in range(c0,c0+n))/total
                    if fraction > .90:
                        found = n
                        break
                if found is None:
                    results.append(('not_reached_within_5', ''))
                    continue
                results.append(('ok', found))
            group['events'][event] = tuple(results)
    out = BASE / 'Heatmaps'
    out.mkdir(parents=True, exist_ok=True)
    lego_out = BASE / 'Legos'
    lego_out.mkdir(parents=True, exist_ok=True)
    vmax = max(max(g['counts'].values()) for g in configs.values())
    norm = Normalize(0, max(1, vmax))
    summary = []
    for (p1, p2), group in sorted(configs.items()):
        xs = sorted({xy[0] for xy in group['counts']})
        ys = sorted({xy[1] for xy in group['counts']})
        if len(xs) != 10 or len(ys) != 10:
            raise ValueError('Expected 10x10 sensor coordinates')
        grid = np.array([[group['counts'][(x,y)] for x in xs] for y in ys])
        dx, dy = xs[1]-xs[0], ys[1]-ys[0]
        fig, ax = plt.subplots(figsize=(9,8))
        im = ax.imshow(grid, origin='lower', extent=(xs[0]-dx/2,xs[-1]+dx/2,ys[0]-dy/2,ys[-1]+dy/2), cmap='inferno', norm=norm)
        ax.invert_xaxis()
        for j,y in enumerate(ys):
            for i,x in enumerate(xs):
                ax.text(x,y,f'{grid[j,i]:,}',ha='center',va='center',fontsize=6,color='white' if norm(grid[j,i]) < .55 else 'black')
        ax.set(xlabel='Physical sensor x (mm), mirrored display',ylabel='Physical sensor y (mm)',title=f'P1 = {p1:g} photons/MeV; P2 = {p2:g} cm\n{len(group["events"])} events; {grid.sum():,} collected photons')
        fig.colorbar(im,ax=ax,label='Summed photon count (shared scale)')
        fig.tight_layout()
        fig.savefig(out/f'heatmap_P1_{p1:g}_P2_{p2:g}.png',dpi=180)
        plt.close(fig)
        for index, cm in enumerate((3,5)):
            records = [v[index] for v in group['events'].values()]
            statuses = Counter(s for s,n in records)
            unknown = set(statuses)-{'ok','not_reached_within_5','zero_collected_energy'}
            if unknown: raise ValueError(f'Unknown statuses: {unknown}')
            ns = np.array([float(n) for s,n in records if s=='ok'])
            summary.append(dict(p1_photons_per_MeV=p1,p2_cm=p2,CM=cm,events=len(records),resolved=len(ns),mean_N=float(ns.mean()) if len(ns) else '',std_N_sample=float(ns.std(ddof=1)) if len(ns)>1 else '',unresolved_fraction=1-len(ns)/len(records),not_reached_within_5=statuses['not_reached_within_5'],zero_collected_energy=statuses['zero_collected_energy']))
    with (BASE/'configuration_summary.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=list(summary[0]));writer.writeheader();writer.writerows(summary)
    p1s=sorted({k[0] for k in configs});p2s=sorted({k[1] for k in configs})
    for cm in (3,5):
        fig=plt.figure(figsize=(12,10))
        axes=[fig.add_subplot(111,projection='3d')]
        for r in summary:
            if r['CM']!=cm: continue
            x=p1s.index(r['p1_photons_per_MeV']);y=p2s.index(r['p2_cm'])
            mean=r['mean_N'];fail=r['unresolved_fraction']
            if mean!='':
                axes[0].bar3d(x-.3,y-.3,0,.6,.6,mean,color=plt.cm.viridis((mean-1)/9),alpha=.85,shade=True)
                sd=r['std_N_sample']
                if sd!='': axes[0].plot([x,x],[y,y],[max(0,mean-sd),mean+sd],color='black',linewidth=2)
                label=f'{mean:.2f}\n{r["resolved"]}/{r["events"]}'
                axes[0].text(x,y,mean+.15,label,ha='center',fontsize=7)
            else:
                axes[0].text(x,y,.12,'No resolved\nevents',ha='center',fontsize=7,color='crimson')
        for ax in axes:
            ax.set_xticks(range(len(p1s)),[f'{v:g}' for v in p1s]);ax.set_yticks(range(len(p2s)),[f'{v:g}' for v in p2s])
            ax.set_xlabel('P1 (photons/MeV)',labelpad=12);ax.set_ylabel('P2 (cm)',labelpad=12)
            ax.view_init(elev=25,azim=-55)
        axes[0].set(zlabel='Mean N (resolved events only)',zlim=(0,11),title='Mean N; black lines = ±1 sample SD\nLabels: mean and resolved/total events')
        fig.suptitle(f'Cluster containment LEGO plots — CM = {cm}\nN = smallest window size with collected energy fraction > 90%; search N = 1…5',fontsize=14)
        fig.text(.5,.025,'P1/P2 axes use categorical spacing. Missing N is never treated as zero. Physical coordinates are unchanged.',ha='center',fontsize=10)
        fig.subplots_adjust(left=.03,right=.90,bottom=.12,top=.84)
        fig.savefig(lego_out/f'LEGO_CM_{cm}.png',dpi=200)
        plt.close(fig)
    print(f'Created {len(configs)} heatmaps, 2 LEGO plots, and configuration_summary.csv')
    for cm in (3,5):
        rows=[r for r in summary if r['CM']==cm]
        print(f'CM={cm}: {sum(r["resolved"] for r in rows)}/{sum(r["events"] for r in rows)} resolved events')

if __name__=='__main__':
    print('Plot generation is disabled; results are saved in MOTHER.csv.')
