#!/usr/bin/env python3
"""Merge per-chunk finder CSVs into a single global top-N list.

Each finder output is a ';'-separated file with the header:
    rank;seed;score;water scale;water coverage;elevation type

This reads every input file, drops the per-chunk ranks, keeps the highest-scoring
rows across all chunks, re-ranks them from 1, and writes the merged result.
"""
import argparse
import csv
import sys

HEADER = ["rank", "seed", "score", "water scale", "water coverage", "elevation type"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--top", type=int, default=1000, help="size of the merged top list")
    parser.add_argument("--out", required=True, help="output CSV path")
    parser.add_argument("inputs", nargs="+", help="chunk CSV files to merge")
    args = parser.parse_args()

    rows = []
    for path in args.inputs:
        try:
            with open(path, newline="") as fh:
                reader = csv.reader(fh, delimiter=";")
                next(reader, None)  # skip header
                for row in reader:
                    if len(row) < 6:
                        continue
                    try:
                        score = float(row[2])
                    except ValueError:
                        continue
                    # (score, seed, water scale, water coverage, elevation type)
                    rows.append((score, row[1], row[3], row[4], row[5]))
        except FileNotFoundError:
            print(f"merge_results.py: skipping missing file {path}", file=sys.stderr)

    # Highest score first; numeric tie-break by seed for deterministic output.
    def seed_key(r):
        try:
            return int(r[1])
        except ValueError:
            return 0
    rows.sort(key=lambda r: (-r[0], seed_key(r)))
    top = rows[: args.top]

    with open(args.out, "w", newline="") as fh:
        writer = csv.writer(fh, delimiter=";")
        writer.writerow(HEADER)
        for rank, (score, seed, w_scale, w_cov, elev) in enumerate(top, start=1):
            writer.writerow([rank, seed, score, w_scale, w_cov, elev])

    print(f"merge_results.py: {len(rows)} rows from {len(args.inputs)} file(s) "
          f"-> top {len(top)} written to {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
