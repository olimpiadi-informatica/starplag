#!/bin/bash
set -e
if [[ $# != 1 ]]; then
    echo "Usage: $0 round_folder"
    echo "Creates fordels templates/task_name/file.*"
    exit 1
fi

if [[ ! -f "$1/contest.yaml" ]]; then
    echo "Argument should be a round folder (no contest.yaml found)"
    exit 1
fi

set -x

mkdir templates

for d in $1/*; do
    if [[ ! -d "$d" ]]; then
        continue;
    fi
    echo "Processing $d"
    if [[ ! -f "$d/task.yaml" ]]; then
        echo "  No task.yaml found, skipping"
        continue
    fi
    mkdir templates/$(basename $d)
    for f in $d/att/*; do
        if [[ "$f" != *".txt" ]] && [[ "$f" != *".zip" ]] && [[ "$f" != "grader"* ]]; then
            echo "      Processing $f "
            cp $f templates/$(basename $d)/$(basename $f)
        fi
    done
done