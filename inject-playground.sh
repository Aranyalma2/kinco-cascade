#!/bin/bash

# Define source and destination directories
SRC_DIR="macros"
DEST_DIR="playground_project/HMI0/macro"

# Ensure the destination directory exists
mkdir -p "$DEST_DIR"

# Copy all .c files from source to destination
cp "$SRC_DIR"/*.c "$DEST_DIR"/

# Print completion message
echo "All .c files have been copied from $SRC_DIR to $DEST_DIR."
