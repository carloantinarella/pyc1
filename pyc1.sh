#!/bin/sh

file1="pyc1"
file2="main.py"
file3="pyc.kv"

# Check if files exist
if [ -f "$file1" ] && [ -f "$file2" ] && [ "$file3" ]
then
    echo "All files exist. Launching application..."
    ./$file1 &
    sleep 1
    python3 $file2 &
else
    echo "One or more file(s) are missing"
fi
