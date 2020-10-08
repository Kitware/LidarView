# Contribution to LidarView and Co.

## Commit convention

Developers are encouraged to use the provided [gitmessage template](Documentation/.gitmessage) in their [git configuration](https://git-scm.com/book/en/v2/Customizing-Git-Git-Configuration#_commit_template), so every commit message follows the same convention.

```bash
$ git config --global commit.template <LVCore_path>/Documentation/.gitmessage
```

Adopting a commit convention, has multiple advantages:
* enforce coherence
* force to have atomic commits
* enable to parse automatically commit messages
* quickly see if a submodule upgrade could be done easily (no api break, no default value change, ...)
* and many more ...