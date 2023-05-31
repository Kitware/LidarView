#!/bin/sh

set -e

target_repository=""

if [[ -z "${TRIGGER_MODULE_PATH}" ]]; then
    if [[ -z "${CI_COMMIT_REF_NAME}" ]]; then
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
modified_files=$(git diff --name-only  $ancestor_SHA $current_SHA *.{h,cxx,txx})

declare -a existing_files

for file in $modified_files; do
    test -e $file && echo "test $file" && existing_files+=("$file")
done

if [[ ${#existing_files[@]} -eq 0 ]]; then
    echo "No c++ files detected in changes for clang-format to run against"
else
    echo "Performing clang-format check on: ${existing_files[@]}"
    clang-format --dry-run -Werror ${existing_files[@]}
    exit_code=`echo $?`
fi

exit $exit_code
