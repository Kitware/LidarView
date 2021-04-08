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

## Debugging the User Interface

LidarView and custom app rely massively on ParaView for the UI/Reaction/Behavior. 

So when a UI bug is detected the best practice is to try to reproduce it on:

1. [LidarView master](https://gitlab.kitware.com/LidarView/lidarview/-/tree/master)
2. the ParaView executable compile with the superbuild. It can be found at `<lidarview_superbuild>/install/bin/paraview[.exe]`
3. the corresponding [ParaView release](https://www.paraview.org/download/)
4. [ParaView master](https://gitlab.kitware.com/paraview/paraview/-/tree/master)

Don't forget to check the version of Qt (in **Help** > **About**), as the bug could be on qt's side.
