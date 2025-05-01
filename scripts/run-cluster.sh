#!/bin/bash

# Usage: run-command-n-times.sh <n>
# Runs the specified command n times.

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <n>"
    exit 1
fi

n=$1

go run colyseus/demo/locator/locator.go -num-servers "$n" &

for ((i=0; i<n; i++)); do
    go run colyseus/demo/server/server.go --sid "$i" --num-servers "$n" &
done

wait