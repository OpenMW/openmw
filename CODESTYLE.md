# OpenMW Code Style

[[_TOC_]]

## Introduction

### Aims

This guide is compiled for the developers whose goal is to contribute to the OpenMW codebase. The document provides detail on how to style and format the code for this project properly. At the same time it offers suggestions and best practice advice on particular functions and other elements that may be used in code.

Adhering to the rules listed in this guide and formatting the code as specified will result in a merge request going through far fewer rounds during review, provided the code itself is error-free. In addition, the `Clang_Format` job is part of the CI pipeline; no MR gets completed without it turning green.

### Checking and Fixing Formatting Issues

OpenMW uses clang-format 14 to check for common style issues. The list of sources of truth is given in the table below.

| File            | What it governs                                                        | Enforced by                                   |
| --------------- | ---------------------------------------------------------------------- | --------------------------------------------- |
| `.clang-format` | C++ layout: indentation, braces, spacing, line length, include sorting | CI `Clang_Format` job (clang-format 14)       |
| `.editorconfig` | Indent style and size, final newline for `.cpp`/`.hpp`, shaders, CMake | Your editor; CI checks CMake files for tabs   |
| `.clang-tidy`   | Naming of namespaces, enums, concepts, local variables, parameters     | CI `Ubuntu_Clang_Tidy_*` jobs (clang-tidy 19) |

Two points to note:

- **Precedence**: these files win if it so happens that the guide deviates from them.
- **Editor setup**: readers are expected to enable their editor's support for these files. This includes VS Code, CLion, Visual Studio and Qt Creator, all of which support `.editorconfig` and `.clang-format`, natively or via a plugin. VS Code and CLion ship their own clang-format, so users need to point the editor at the clang-format 14 binary installed below; in VS Code that's the `C_Cpp.clang_format_path` setting.

The first step in addressing the formatting issues would be installing clang-format 14. This requires `pipx`:

```shell
# macOS
brew install pipx

# Debian/Ubuntu
sudo apt install pipx

# Windows
py -m pip install --user pipx

# Any other platform with Python
python3 -m pip install --user pipx
```

Then add `pipx`'s install directory to your `PATH` and install clang-format 14 (open a new terminal after `ensurepath`):

```shell
pipx ensurepath
pipx install clang-format==14.0.6
```

On Apple Silicon Macs, this clang-format build runs under Rosetta 2, which macOS offers to install on first launch if it is missing. On Windows, `pipx` can be skipped altogether by running `pip install clang-format==14.0.6` inside a virtual environment.

Ubuntu 22.04/24.04 and Debian 12 users can install clang-format 14 directly:

```shell
sudo apt install clang-format-14
```

Note that the binary is then called `clang-format-14` rather than `clang-format`. Newer Debian releases no longer ship clang-format 14, so use `pipx` there.

With clang-format installed, checking a file without changing anything is done by the following command:

```shell
clang-format --dry-run -Werror path/to/file.cpp
```

The file is then fixed as follows (this reformats the whole file, so use it only for new files):

```shell
clang-format -i path/to/file.cpp
```

For existing files, reformat only the lines you have changed. Without a commit, `git clang-format` covers uncommitted changes only; passing the commit your branch started from covers the whole branch, and `--diff` previews the result without applying it:

```shell
# clang-format installed via pipx
git clang-format --diff upstream/master
git clang-format upstream/master

# clang-format installed via apt
git clang-format-14 --diff upstream/master
git clang-format-14 upstream/master
```

Here `upstream` is the remote pointing to the main OpenMW repository; replace it if your remote has a different name.

The full CI check can be run locally as follows:

```shell
# clang-format installed via pipx
CI/check_clang_format.sh

# clang-format installed via apt
CLANG_FORMAT=clang-format-14 CI/check_clang_format.sh
```

These calls also need `nproc`; on macOS it is provided by the `brew install coreutils` command. On Windows, run the scripts from Git Bash or WSL; Git Bash already includes `nproc`. The scripts must be run from the repository root.

Additional CI checks available are:

- `CI/check_file_names.sh`: checks whether the file names contain only lowercase letters and digits, and fails if any don't, and
- `CI/check_cmake_format.sh`: ensures there are no tabs in CMake files.

One item all three CI checks skip is the `extern/` directory, which holds third-party libraries that OpenMW keeps its own copy of, because they were abandoned upstream, never packaged properly, or needed patching. Among those are `sol3`, `oics`, `osgQt`, `osg-ffmpeg-videoplayer`, `smhasher` and `Base64`. The directory is still checked by clang-tidy, but with a relaxed configuration (`extern/.clang-tidy`) that disables two of its checks.

In case any of these files have to be edited, keep the existing style and do not reformat them; the code follows its original authors' style, and small diffs make it easier to pull in upstream changes later. Should any problems occur with these, report them upstream where possible (see [CONTRIBUTING.md](./CONTRIBUTING.md#code-review)).

Only adopted third-party code belongs in `extern/`; do not put new OpenMW code there.

Pushing a branch to a forked repo will also start a CI pipeline; one does not need to open an MR first. The pipeline includes the checks this section covers:

- `Clang_Format`, which runs all the scripts listed above, and it does so on every push and on MRs, and
- the `Ubuntu_Clang_Tidy_*` jobs, which run on pushes to forks and on MRs.

Pushing to your fork is a fallback for anyone who can't run the checks locally, e.g. clang-tidy, which requires a full build. It is a final check before opening an MR. The [MR template](./.gitlab/merge_request_templates/Default.md) also asks for it, making it a requirement instead of merely a convenience (see [CONTRIBUTING.md](./CONTRIBUTING.md#fork-ci)).
