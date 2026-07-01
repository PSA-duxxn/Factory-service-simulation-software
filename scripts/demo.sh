#!/usr/bin/env bash
# demo.sh — build then run sample simulations

set -e
BINARY="build/fsss"

if [ ! -f "$BINARY" ]; then
    echo "Binary not found. Running build first..."
    bash scripts/build.sh Release
fi

echo ""
echo "════════════════════════════════════════════════"
echo "  Demo 1: Single simulation (default factory)"
echo "════════════════════════════════════════════════"
$BINARY run config/factory_default.json --csv results_default.csv

echo ""
echo "════════════════════════════════════════════════"
echo "  Demo 2: Adjuster sensitivity sweep (3 → 20)"
echo "════════════════════════════════════════════════"
$BINARY sweep config/small_factory.json 1 10 --step 1 --csv sweep_results.csv

echo ""
echo "Outputs: results_default.csv, sweep_results.csv"
