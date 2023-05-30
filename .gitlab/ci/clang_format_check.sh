#!/bin/sh

branch_origin=""

if [[ -z "${TRIGGER_MODULE_BRANCH}" ]]; then
    if [[ -z "${CI_COMMIT_REF_NAME}" ]]; then
        echo "Missing CI_COMMIT_REF_NAME env variable"
        exit 1
    fi
    branch_origin="origin/$CI_COMMIT_REF_NAME"
else
    branch_origin="origin/$TRIGGER_MODULE_BRANCH"
fi

exit_code=0
changed_files=`git diff --name-only $branch_origin $(git merge-base $branch_origin origin/master)| egrep "\.txx$|\.cxx$|\.h$"`

if [[ -z "${changed_files}" ]]; then
    echo "No c++ files detected in changes for clang-format to run against"
else
    echo "Performing clang-format check on: $changed_files"
    clang-format --dry-run -Werror "$changed_files"
    exit_code=`echo $?`
fi

exit $exit_code
