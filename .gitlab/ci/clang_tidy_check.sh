#!/bin/bash

set -e

target_repository=""

if [ -z "${TRIGGER_MODULE_PATH}" ]; then
    if [ -z "${CI_COMMIT_REF_NAME}" ]; then
        echo "Missing CI_COMMIT_REF_NAME env variable"
        exit 1
    fi
    target_repository="."
else
    target_repository="$TRIGGER_MODULE_PATH"
fi

cd $target_repository

exit_code=0

current_SHA=$(git rev-parse HEAD)
ancestor_SHA=$(git merge-base $current_SHA origin/master)
modified_files=$(git diff --name-only $ancestor_SHA $current_SHA *.{h,cxx,txx})

declare -a existing_files

for file in $modified_files; do
    test -e $file && existing_files+=("$file")
done

if [ ${#existing_files[@]} -eq 0 ]; then
    echo "No c++ files detected in changes for clang-tidy to run against"
else
    # Remove -fno-fat-lto-objects flags (unsupported by clang-tidy)
    sed -i 's/-fno-fat-lto-objects //g' build/compile_commands.json

    echo "Performing clang-format check on: ${existing_files[@]}"
    run-clang-tidy -p build/ ${existing_files[@]}
    exit_code=`echo $?`
fi

exit $exit_code
