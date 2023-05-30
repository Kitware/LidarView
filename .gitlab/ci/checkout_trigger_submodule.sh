#!/bin/sh

set -e

if [[ -z "${TRIGGER_MODULE_PATH}" ]]; then
    echo "Using default submodules SHA"
else
    git -C $TRIGGER_MODULE_PATH fetch
    git -C $TRIGGER_MODULE_PATH checkout $TRIGGER_MODULE_BRANCH
    git -C $TRIGGER_MODULE_PATH submodule sync --recursive
    git -C $TRIGGER_MODULE_PATH submodule update --init --recursive
    git -C $TRIGGER_MODULE_PATH log --oneline -n1 --no-abbrev
fi
