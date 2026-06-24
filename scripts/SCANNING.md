# Always-on scanning

Tooling for running a finder over the full seed space on an always-on machine,
with crash/reboot resume and a merged global top-N result.

- `scan.sh` — splits the seed space into fixed chunks, runs the finder on each,
  and records completion so reboots/crashes resume where they left off.
- `merge_results.py` — merges per-chunk CSVs into one re-ranked global top list.

## Quick start

Build first (release):

```sh
./build_release.sh
```

Then scan. Pick the binary for the mode you want:

```sh
scripts/scan.sh \
  --bin build/seed_finders/peninsula_resources/peninsula_resources_normal \
  --out runs/normal \
  --threads 12
```

(The exact binary path depends on your CMake generator; check under `build/`.)

This scans seeds `0 .. 4294967295` in chunks of `2^24`, writing:

```
runs/normal/chunks/chunk_<start>_<end>.csv    # per-chunk top list
runs/normal/chunks/chunk_<start>_<end>.done   # completion marker
runs/normal/top.csv                           # merged global top-N (refreshed per chunk)
```

Re-running the same command after a stop skips finished chunks. To re-merge
without scanning (e.g. after changing `--merge-top`):

```sh
scripts/scan.sh --bin <any> --out runs/normal --merge-only
```

## Options

| Option | Default | Notes |
|---|---|---|
| `--first N` | `0` | inclusive, must be even (stage 1 checks even seeds only) |
| `--last N` | `4294967295` | exclusive |
| `--chunk N` | `16777216` (2^24) | seeds per chunk, must be even |
| `--threads N` | `nproc` | passed to the finder |
| `--merge-top N` | `1000` | size of the merged top list |

Smaller chunks = finer resume granularity (less lost on a crash) but more
process restarts. `2^24` ≈ 16.7M seeds is a reasonable balance; on a slow run
you may prefer `2^22` so each chunk finishes in minutes.

## Benchmark before committing to a full scan

A pure run touches billions of seeds. Measure real throughput on the target box
first by timing one chunk, then extrapolate:

```sh
time build/.../peninsula_resources_normal \
  --output /tmp/bench.csv --threads 12 --first-seed 0 --last-seed 16777216
```

`(4.29e9 / seeds_per_sec) / 3600` ≈ hours for a full even-seed pass through
stage 1; stage 2 cost depends on how many seeds survive the resource gate.

## Run it unattended (auto-resume across reboots)

### Linux (systemd)

`/etc/systemd/system/factorio-seedscan.service`:

```ini
[Unit]
Description=Factorio seed scan (normal)
After=network.target

[Service]
Type=simple
User=YOURUSER
WorkingDirectory=/path/to/fast-factorio-seed-finder
ExecStart=/path/to/fast-factorio-seed-finder/scripts/scan.sh \
  --bin /path/to/fast-factorio-seed-finder/build/seed_finders/peninsula_resources/peninsula_resources_normal \
  --out /path/to/fast-factorio-seed-finder/runs/normal --threads 12
Restart=always
RestartSec=10
Nice=10

[Install]
WantedBy=multi-user.target
```

```sh
sudo systemctl enable --now factorio-seedscan.service
journalctl -u factorio-seedscan -f          # watch progress
```

When the scan finishes, `scan.sh` exits 0; `Restart=always` will relaunch it,
but every chunk is already `.done` so it merges and exits again immediately
(cheap). Disable the unit once you have your results.

### macOS (launchd)

Create a `~/Library/LaunchAgents/com.local.factorio-seedscan.plist` with
`KeepAlive` true and `ProgramArguments` invoking `scan.sh` with the same flags.
