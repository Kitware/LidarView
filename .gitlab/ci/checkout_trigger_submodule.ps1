if (Test-Path env:TRIGGER_MODULE_PATH) {
    git -C "$env:TRIGGER_MODULE_PATH" fetch
    git -C "$env:TRIGGER_MODULE_PATH" checkout "$env:TRIGGER_MODULE_BRANCH"
    git -C "$env:TRIGGER_MODULE_PATH" submodule sync --recursive
    git -C "$env:TRIGGER_MODULE_PATH" submodule update --init --recursive
    git -C "$env:TRIGGER_MODULE_PATH" log --oneline -n1 --no-abbrev
} else {
    Write-Output "Using default submodules SHA"
}
