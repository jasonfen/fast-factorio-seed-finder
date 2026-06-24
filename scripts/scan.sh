#!/usr/bin/env bash
#
# Chunked, resumable seed scan for the fast-factorio-seed-finder.
#
# Splits the seed space into fixed-size chunks and runs the finder on each one,
# recording completion so that a crash or reboot resumes where it left off
# (already-finished chunks are skipped). After scanning it merges all chunk
# outputs into a single global top-N CSV.
#
# Designed to be wrapped by systemd (Restart=always) or launchd (KeepAlive) on
# an always-on machine; on restart it simply continues. See SCANNING.md.
#
# Usage:
#   scripts/scan.sh --bin <finder-binary> --out <dir> [options]
#
# Options:
#   --bin PATH         Finder executable (e.g. build/.../peninsula_resources_normal)  [required]
#   --out DIR          Output directory for chunks and the merged result            [required]
#   --first N          First seed (inclusive, must be even)         [default 0]
#   --last  N          Last seed (exclusive)                        [default 4294967295]
#   --chunk N          Seeds per chunk (must be even)               [default 16777216 = 2^24]
#   --threads N        Threads passed to the finder                 [default nproc]
#   --merge-top N      Size of the merged global top list           [default 1000]
#   --merge-only       Skip scanning; just (re)merge existing chunks
#
set -euo pipefail

BIN=""
OUT=""
FIRST=0
LAST=4294967295
CHUNK=16777216
THREADS=""
MERGE_TOP=1000
MERGE_ONLY=0

die() { echo "scan.sh: $*" >&2; exit 1; }

while [ $# -gt 0 ]; do
    case "$1" in
        --bin)        BIN="$2"; shift 2;;
        --out)        OUT="$2"; shift 2;;
        --first)      FIRST="$2"; shift 2;;
        --last)       LAST="$2"; shift 2;;
        --chunk)      CHUNK="$2"; shift 2;;
        --threads)    THREADS="$2"; shift 2;;
        --merge-top)  MERGE_TOP="$2"; shift 2;;
        --merge-only) MERGE_ONLY=1; shift;;
        -h|--help)    sed -n '2,30p' "$0"; exit 0;;
        *)            die "unknown argument: $1";;
    esac
done

[ -n "$BIN" ] || die "--bin is required"
[ -n "$OUT" ] || die "--out is required"
[ "$MERGE_ONLY" -eq 1 ] || [ -x "$BIN" ] || die "finder binary not found or not executable: $BIN"

# Default thread count to the number of available CPUs.
if [ -z "$THREADS" ]; then
    THREADS="$( (command -v nproc >/dev/null && nproc) || sysctl -n hw.ncpu 2>/dev/null || echo 1)"
fi

# The first stage only checks even seeds, and the finder requires an even
# --first-seed, so both the start and the chunk size must be even.
[ $((FIRST % 2)) -eq 0 ] || die "--first must be even (got $FIRST)"
[ $((CHUNK % 2)) -eq 0 ] || die "--chunk must be even (got $CHUNK)"
[ "$FIRST" -lt "$LAST" ] || die "--first must be less than --last"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CHUNK_DIR="$OUT/chunks"
mkdir -p "$CHUNK_DIR"

merge() {
    # Merge whatever chunk outputs currently exist into a global top list.
    shopt -s nullglob
    local csvs=("$CHUNK_DIR"/chunk_*.csv)
    shopt -u nullglob
    if [ ${#csvs[@]} -eq 0 ]; then
        echo "scan.sh: no chunk outputs to merge yet" >&2
        return 0
    fi
    python3 "$SCRIPT_DIR/merge_results.py" --top "$MERGE_TOP" --out "$OUT/top.csv" "${csvs[@]}"
    echo "scan.sh: merged ${#csvs[@]} chunk(s) -> $OUT/top.csv (top $MERGE_TOP)"
}

if [ "$MERGE_ONLY" -eq 1 ]; then
    merge
    exit 0
fi

total_chunks=0
done_chunks=0
start="$FIRST"
while [ "$start" -lt "$LAST" ]; do
    end=$((start + CHUNK))
    [ "$end" -gt "$LAST" ] && end="$LAST"

    name="chunk_${start}_${end}"
    csv="$CHUNK_DIR/${name}.csv"
    marker="$CHUNK_DIR/${name}.done"

    total_chunks=$((total_chunks + 1))

    if [ -f "$marker" ]; then
        done_chunks=$((done_chunks + 1))
        start="$end"
        continue
    fi

    echo "scan.sh: [$(date '+%Y-%m-%d %H:%M:%S')] chunk $name (threads=$THREADS)"
    tmp="$CHUNK_DIR/${name}.tmp.csv"
    # Write to a temp file and rename on success so a killed run never leaves a
    # partial .csv that looks complete.
    "$BIN" --output "$tmp" --threads "$THREADS" --first-seed "$start" --last-seed "$end"
    mv -f "$tmp" "$csv"
    : > "$marker"
    done_chunks=$((done_chunks + 1))

    # Refresh the merged top list after each chunk so partial progress is queryable.
    merge

    start="$end"
done

echo "scan.sh: all $total_chunks chunk(s) complete ($done_chunks done)."
merge
