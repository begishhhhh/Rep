#!/bin/bash

# check two arguments
if [ $# -ne 2 ]; then
    echo "Enter two arguments: path to folder and limit in percent"
    exit 1
fi

path="$1"
limit="$2"

# check path exists
if [ ! -d "$path" ]; then
    echo "Error: folder does not exist"
    exit 1
fi

# check limit is number
if ! [[ "$limit" =~ ^[0-9]+$ ]]; then
    echo "Error: limit must be a positive integer"
    exit 1
fi

# create backup
backup="./backup"
mkdir -p "$backup"

# get folder size in bytes
size=$(du -sb "$path" | cut -f1)
size_percent=$(df "$path" | awk 'NR==2 {print $5}')
echo "Folder takes $size_percent"

# calculate limited size
max=$(( size * limit / 100 ))
if [ "$size" -le "$max" ]; then
    echo "Folder size is good"
    exit 0
fi

# calculate how much to free
to_free=$((size - max))
# sort files by modification time (oldest -> new)
mapfile -t files < <(find "$path" -type f -printf "%T@ %s %p\n" | sort -n)

freed=0
files_to_archive=()

for line in "${files[@]}"; do
    file_size=$(echo "$line" | cut -d' ' -f2)
    file_path=$(echo "$line" | cut -d' ' -f3-)

    files_to_archive+=("$file_path")
    freed=$((freed + file_size))

    if [ "$freed" -ge "$to_free" ]; then
        break
    fi
done

# create archive name with timestamp
archive_name="backup_$(date +%Y%m%d_%H%M%S).tar.gz"
archive_path="$backup/$archive_name"

tar -czf "$archive_path" -C "$(dirname "${files_to_archive[0]}")" "${files_to_archive[@]#$path/}"

if [ $? -eq 0 ]; then
    # remove archived files
    for f in "${files_to_archive[@]}"; do
        rm -f "$f"
    done
    echo "Old files have been archived and removed."
else
    echo "Error: failed to create archive"
    exit 1
fi
