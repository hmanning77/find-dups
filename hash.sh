#!/usr/bin/env bash
for file in $(find .); do
    if [[ -f $file ]]; then
        sha1sum $file;
    fi
done
