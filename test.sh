#!/bin/bash

# Test 1
echo Test 1: Testing with empty folder

if [ -d "test" ]; then
    rm -r test
    mkdir test
else
    mkdir test
fi

# Redirect messages to a separate file
bash ./lab1.sh /home/alena/test 2 >> text.txt

code=$?

if [ $code -eq 0 ]; then
    echo "Success!"
else
    echo "Error!"
fi

rm -r test

# Test 2
echo Test 2: Testing with non-existent folder

# Use "test" folder, deleted it earlier
bash ./lab1.sh /home/alena/test 2 >> text.txt
code=$?

# If the output file contains error message, script works correctly
if [ $code -eq 1 ] && grep -q "Error: folder does not exist" text.txt; then
    echo "Success!"
else
    echo "Error!"
fi

# Test 3
echo Test 3: Testing backup folder creation

mkdir test

bash ./lab1.sh /home/alena/test 2 >> text.txt


if [ -d "backup" ]; then
    echo "Success!"
else
    echo "Error!"
fi

rm -r test

# Test 4
echo Test 4: Testing if files are archived

mkdir test
cd test

# Fill with files
for (( a = 0; a < 10; a++ ))
do
    touch "$a.txt"
    truncate -s 5000000 "$a.txt"
done

# Get folder size before archiving (for test 4)
origin=$(du -sb ~/test | cut -f1)

cd ..
bash ./lab1.sh /home/alena/test 2 >> text.txt

code=$?

count=0
for file in test/*; do
    if [ -f "$file" ]; then
        ((count++))
    fi
done

# Check number of files decreased
if [ $code -eq 0 ] && [ $count -lt 10 ]; then
    echo "Success!"
else
    echo "Error!"
fi

# Test 5
echo Test 5: Testing size change

final=$(du -sb ~/test | cut -f1)

if [ $final -le $origin ]; then
    echo "Success!"
else
    echo "Error!"
fi

rm -r backup
rm -r test
rm text.txt
