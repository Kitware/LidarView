# Contributing

## Contribution workflow

All new work must be committed on topic branches.
Name topics like you might name functions: concise but precise.
A reader should have a general idea of the feature or fix to be developed given
just the branch name.

1. To start a new topic branch:
```
$ git fetch origin
```

2. For new development, start the topic from origin/master:
```
$ git checkout -b my-topic origin/master
```

3. Edit files and create commits (repeat as needed):
```
$ edit file1 file2 file3
$ git add file1 file2 file3
$ git commit
```

## Commit convention

Commit messages must contain a brief summary as the first line and a more detailed description
about why the changes are necessary below:
```
[feat] Add a new feature in LidarView

This feature allows...
```

Best Practices for Commits:
- Try to keep the commits organized logically, with a medium level of granularity
- In general, commit files and features from the most depended on to the least
- Write short but meaningful commit messages
- Use imperative:
```
Fix issue with stuff : Good
Fixing issue with stuff: Bad
```
- You can reference gitlab issues, MRs and commits if needed.
- Developers could use the provided [gitmessage template](./.gitmessage) in their [git configuration](https://git-scm.com/book/en/v2/Customizing-Git-Git-Configuration#_commit_template):
```bash
$ git config commit.template Documentation/.gitmessage
```
- Each commit of a MR should ideally be compilable. You can use the commands `git rebase -i` and `git commit --fixup` to edit your commits and keep them clean during a review! 

## Clang-format & style

Please follow the [VTK coding style](https://docs.vtk.org/en/latest/developers_guide/coding_conventions.html) when contributing to LidarView.

All commits in LidarView must be formatted using [clang-format](https://clang.llvm.org/docs/ClangFormat.html).
This tool ensures consistent C++ code style across the project. It can be integrated into your IDE or run via the command line.

## Debugging LidarView

LidarView and custom app in general rely massively on ParaView.
So when a bug is detected the best practice is to try to reproduce it on:

1. [LidarView master](https://gitlab.kitware.com/LidarView/lidarview/-/tree/master)
2. The ParaView executable compiled with the superbuild. It can be found at `<lidarview_superbuild>/install/bin/paraview[.exe]`
3. The corresponding [ParaView release](https://www.paraview.org/download/)
4. [ParaView master](https://gitlab.kitware.com/paraview/paraview/-/tree/master)
