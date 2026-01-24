#!/usr/bin/env python3
"""
Merge multiple .sco files (delta-tick format) into one multitrack .sco.

- Removes comment lines starting with '#'
- Replaces instrument ID (3rd column) according to a mapping per input file
- Interleaves events by absolute tick time computed from per-file deltas

Example:
  ./merge_sco_multitrack.py -o out.sco \
    --track Chord-piano.sco 0 \
    --track Drums.sco 1 \
    --track Solo1.sco 2 \
    --track Solo2.sco 3 \
    --track Violin_track.sco 4
"""

from __future__ import annotations
import argparse
import re
from pathlib import Path
from typing import List, Tuple, Iterable

LINE_RE = re.compile(r"^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s*$")

def read_events(path: Path, new_id: int, file_order: int):
    t = 0
    seq = 0
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            s = raw.strip()
            if not s or s.startswith("#"):
                continue
            m = LINE_RE.match(s)
            if not m:
                continue
            delta, action, _oldid, pitch, vel = map(int, m.groups())
            t += delta
            yield (t, file_order, seq, action, new_id, pitch, vel)
            seq += 1

def merge(files: List[Tuple[Path, int]]) -> List[str]:
    events = []
    for file_order, (path, new_id) in enumerate(files):
        events.extend(list(read_events(path, new_id, file_order)))

    # Sort by absolute time; keep deterministic ordering for simultaneous events
    events.sort(key=lambda x: (x[0], x[1], x[2]))

    out_lines = []
    last_t = 0
    for (t, _fo, _seq, action, inst_id, pitch, vel) in events:
        out_lines.append(f"{t - last_t}\t{action}\t{inst_id}\t{pitch}\t{vel}\n")
        last_t = t
    return out_lines

def parse_args():
    p = argparse.ArgumentParser(description="Merge .sco tracks into one multitrack .sco")
    p.add_argument("-o", "--output", required=True, help="Output .sco path")
    p.add_argument("--track", action="append", nargs=2, metavar=("FILE", "ID"),
                   required=True, help="Input track as: --track path/to/file.sco <instrument_id>")
    return p.parse_args()

def main():
    args = parse_args()
    files = [(Path(fp), int(i)) for fp, i in args.track]
    out_lines = merge(files)
    Path(args.output).write_text("".join(out_lines), encoding="utf-8")
    print(f"Wrote {len(out_lines)} events to {args.output}")

if __name__ == "__main__":
    main()
