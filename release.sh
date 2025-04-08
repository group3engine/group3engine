#!/bin/bash
set -e

# Define output zip file name
OUTPUT="project_archive.zip"

# Remove existing zip if it exists
if [ -f "$OUTPUT" ]; then
    rm "$OUTPUT"
fi

echo "Zipping up directories into $OUTPUT..."

# Zip the specified directories. Note that directories with spaces are quoted.
zip -r "$OUTPUT" "./assets/shaders" "./assets/Sample obby"
cd build/src/src && zip -r "../../$OUTPUT" src && cd ../../..

echo "Archive created: $OUTPUT"