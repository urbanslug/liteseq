#!/bin/bash

if [ $# -ne 3 ]; then
    echo "Usage: $0 <input.fasta> <output.fasta> <wrap_length>"
    exit 1
fi

input_file=$1
output_file=$2
wrap_length=$3

# Validate that the wrap length is a positive integer
if ! [[ $wrap_length =~ ^[0-9]+$ ]] || [ "$wrap_length" -le 0 ]; then
    echo "Error: <wrap_length> must be a positive integer."
    exit 1
fi

# Process the FASTA file and wrap sequence lines
awk '/^>/ {print; next} {printf "%s", $0} END {print ""}' "$input_file" | fold -w "$wrap_length" > "$output_file"

echo "Formatted FASTA saved to $output_file with line width $wrap_length."

