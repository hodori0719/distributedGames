#!/bin/bash

# Number of servers to start
NUM_SERVERS=8

# Base port for the servers
BASE_PORT=9000

# Path to the server binary (user must build and link)
SERVER_BINARY="UNDEF"

# Additional flags (optional)
LOG_LEVEL="fatal"
PROACTIVE_REPL="true"
FRAME_RATE=10

# Start each server
for ((i=0; i<NUM_SERVERS; i++)); do
    PORT=$((BASE_PORT + i))
    echo "Starting server $i on port $PORT..."
    $SERVER_BINARY \
        --sid=$i \
        --base-port=$BASE_PORT \
        --num-servers=$NUM_SERVERS \
        --log-level=$LOG_LEVEL \
        --proactive-repl=$PROACTIVE_REPL \
        --frame-rate=$FRAME_RATE \
        > "server_$i.log" 2>&1 & # Redirect output to a log file and run in the background
done

echo "All $NUM_SERVERS servers started."