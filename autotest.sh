#!/bin/bash

CONFIG_FILE="settings.cfg"
LOG_DIR="parameter_tests"
RESULTS_FILE="test_results.csv"

# Create directories and header
mkdir -p "$LOG_DIR"
echo "winsize,pyrlevel,lost_frame,time,fps" > "$RESULTS_FILE"

# Test ranges (adjust as needed)
WINSIZE_START=3
WINSIZE_END=20
WINSIZE_STEP=3

PYRLEVEL_START=1
PYRLEVEL_END=3

# Test different parameter combinations
for winsize in $(seq $WINSIZE_START $WINSIZE_STEP $WINSIZE_END); do
    for pyrlevel in $(seq $PYRLEVEL_START 1 $PYRLEVEL_END); do
        # Modify config file
        sed -i "s/oft_winsize = .*/oft_winsize = $winsize;/" "$CONFIG_FILE"
        sed -i "s/oft_pyrlevels = .*/oft_pyrlevels = $pyrlevel;/" "$CONFIG_FILE"
        
        # Run program and capture output
        LOG_FILE="$LOG_DIR/w${winsize}_p${pyrlevel}.log"
        echo "Testing winsize=$winsize pyrlevels=$pyrlevel..."
        
        # Run with timeout (adjust 30s as needed)
        timeout 30s ./tracker --config="$CONFIG_FILE" | tee "$LOG_FILE"
        

        # Extract metrics
        LOST_FRAME=$(grep -oP 'Track lost at \K\d+' "$LOG_FILE" || echo "9999")
        TIME_MS=$(grep -oP 'Time: \K\d+' "$LOG_FILE" || echo "0")
        FPS=$(grep -oP 'FPS: \K\d+\.\d+' "$LOG_FILE" || echo "0.0")
        
        # Record results
        echo "$winsize,$pyrlevel,$LOST_FRAME,$TIME_MS,$FPS" >> "$RESULTS_FILE"
    done
done

# Show best combinations
echo -e "\nBest performing combinations:"
echo "1. By tracking duration:"
sort -t, -k3 -nr "$RESULTS_FILE" | head -n 5
echo -e "\n2. By FPS:"
sort -t, -k5 -nr "$RESULTS_FILE" | head -n 5