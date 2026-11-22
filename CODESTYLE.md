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

The first step in addressing the formatting issues would be installing clang-format 14. The simplest way to do so is with `pipx`:

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

clang-format 14 is also included in the [LLVM 14.0.6 release](https://github.com/llvm/llvm-project/releases/tag/llvmorg-14.0.6), which provides an installer for Windows. Make sure to use exactly this version, since other LLVM versions come with a clang-format that formats some code differently from CI. Bear in mind that the full LLVM installation takes several hundred megabytes and may conflict with another LLVM version installed on the same system, which is why `pipx` is the recommended way.

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

One item all three CI checks skip is the `extern/` directory, which holds third-party libraries that OpenMW keeps its own copy of, because they were abandoned upstream, never packaged properly, or needed patching. The directory is still checked by clang-tidy, but with a relaxed configuration (`extern/.clang-tidy`) that disables two of its checks.

In case any of these files have to be edited, keep the existing style and do not reformat them; the code follows its original authors' style, and small diffs make it easier to pull in upstream changes later. Should any problems occur with these, report them upstream where possible (see [CONTRIBUTING.md](./CONTRIBUTING.md#code-review)).

Only adopted third-party code belongs in `extern/`; do not put new OpenMW code there.

Pushing a branch to a forked repo will also start a CI pipeline; one does not need to open an MR first. The pipeline includes the checks this section covers:

- `Clang_Format`, which runs all the scripts listed above, and it does so on every push and on MRs, and
- the `Ubuntu_Clang_Tidy_*` jobs, which run on pushes to forks and on MRs.

Pushing to your fork is a fallback for anyone who can't run the checks locally, e.g. clang-tidy, which requires a full build. It is a final check before opening an MR. The [MR template](./.gitlab/merge_request_templates/Default.md) also asks for it, making it a requirement instead of merely a convenience (see [CONTRIBUTING.md](./CONTRIBUTING.md#fork-ci)).

## Formatting

- The code is indented with four spaces rather than tabs.
- The lines of code are limited to 120 characters.
- Braces follow the Allman style throughout, with the sole exception of lambda bodies.
- All content inside a namespace is indented (`NamespaceIndentation: All`), while access specifiers align with the class keyword (`AccessModifierOffset: -4`).
- The `case` labels are indented.
- References and pointers have the ampersand or asterisk against the type name, not the variable name. Example: `int* var`, not `int *var`.
- Lines break before binary operators and ternaries, and constructor initializer lists break before the comma.
- One-line functions are permitted only inside the class body; one-line `if` statements, loops and blocks are not allowed.

```cpp
// Allowed
if (condition)
    return 0;

while (var != 0)
    var--;

// Not allowed
if (condition) return 0;

while (var != 0) var--;
```

- A `template <...>` declaration is always followed by a line break, and no more than one consecutive blank line is permitted.
- Closing namespace comments such as `} // namespace X` are not used (`FixNamespaceComments: false`).
- Single-statement bodies without braces are acceptable.
- A space is expected after `if`, `for`, `while` and `switch`, but not between a function name and the opening parenthesis.
- There should be no spaces inside parentheses, square brackets or angle brackets.
- One space is expected before a trailing `//` comment.

## Files and Layout

- File names contain only lowercase letters and digits, followed by a `.cpp`, `.hpp` or `.h` extension. This is enforced by CI.
- Headers are protected with `#ifndef` include guards rather than `#pragma once`. New guards follow the pattern `APP[_ROOTLEVELDIRECTORY][_SUBDIRECTORY]_HEADERNAME_HPP`, built from the path of the header. Headers in `components/` use `OPENMW_COMPONENTS` as the start of the guard. Existing guards are not consistent, so they should not be used as a model.

```cpp
// apps/openmw/mwmechanics/spellcasting.hpp
#ifndef OPENMW_MWMECHANICS_SPELLCASTING_HPP
#define OPENMW_MWMECHANICS_SPELLCASTING_HPP
...
#endif

// apps/opencs/model/world/data.hpp
#ifndef OPENCS_MODEL_WORLD_DATA_HPP

// apps/launcher/maindialog.hpp
#ifndef LAUNCHER_MAINDIALOG_HPP

// components/misc/strings/format.hpp
#ifndef OPENMW_COMPONENTS_MISC_STRINGS_FORMAT_HPP
```

- Function bodies are placed in the `.cpp` file, with the exception of one-line getters and setters. Templates, as well as `constexpr` and `inline` functions, must be defined in the header, since the compiler needs the full definition wherever they are used.

```cpp
// somefile.hpp
namespace SomeSpace
{
    class SomeClass
    {
    public:
        float someFunction(float p1, float p2) const;

        int getSomeVar() const { return mSomeVar; } // One-line getters and setters may stay in the header.
        void setSomeVar(int v) { mSomeVar = v; }

    protected:
        void onSomeVarChanged();

    private:
        int mSomeVar = 0;
    };

    template <class T>
    T twice(const T& value) // Templates must be defined in the header.
    {
        return value + value;
    }
}

// somefile.cpp
namespace SomeSpace
{
    float SomeClass::someFunction(float p1, float p2) const
    {
        return p1 + p2;
    }
}
```

- Dependencies flow in one direction only: `apps/` may depend on `components/` or `extern/`, and `components/` on `extern/`, but never the reverse (not yet enforced by the build; see [#20](https://gitlab.com/OpenMW/openmw/-/issues/20)).

## Naming

- Namespaces use CamelCase, which is checked by clang-tidy. Namespaces in `apps/openmw` start with `MW`, those in OpenMW-CS start with `CSM` (model) or `CSV` (view), other applications use their own name as the namespace (e.g. `Launcher`, `Wizard`, `EsmTool`), and namespaces in `components/` use plain names without a prefix.
- Classes use CamelCase, while functions, local variables and parameters use camelBack, i.e. they start with a lowercase letter. The naming of local variables and parameters is checked by clang-tidy.
- Enum types and concepts use CamelCase, which is also checked by clang-tidy.
- New enumerations are declared as `enum class` with CamelCase enumerators, rather than as a plain `enum` with `TypePrefix_Name` enumerators. The latter remain in existing code but should not be copied. An exception is made for bit flags combined with `|` and `&`, for which a plain `enum` is acceptable, since an `enum class` would need a `static_cast` for every such operation. clang-tidy checks the name of the enum type, not its enumerators.

```cpp
// New code
enum class GetNextRefMode
{
    LoadAll,
    LoadOnlyMoved,
    LoadOnlyNotMoved,
};

// Bit flags
enum Flag
{
    Flag_ForceRun = 1,
    Flag_ForceSneak = 2,
};
```

- Member variables are prefixed with `m` and static members with `s`, as in `mName` and `sName`. A trailing underscore is not used. These prefixes are a convention only and are not checked by any tool.
- `constexpr` constants at namespace scope use CamelCase rather than camelBack or all caps. `static constexpr` members are static members, so the `s` prefix takes precedence, as in `sMaxSkillLevel`. Existing code also uses other forms, which should not be copied. Function-local `constexpr` variables are ordinary local variables and use camelBack.

```cpp
namespace MWMechanics
{
    class NpcStats
    {
    public:
        static constexpr int sMaxSkillLevel = 100;

        int getSkillLevel(int skillIndex) const;

    private:
        int mFatigue = 0;
        static int sInstanceCount;
    };

    int NpcStats::getSkillLevel(int skillIndex) const
    {
        const int baseLevel = skillIndex * 2;
        return baseLevel + mFatigue;
    }
}
```

- Global variables are not used.
- Macros should generally be avoided in favour of templates, `constexpr` variables and inline functions. Besides include guards and conditional compilation, they are acceptable where they are notably simpler than the equivalent template code. Macros are written in all caps.
- Acronyms are written as ordinary words, e.g. `Npc` rather than `NPC`. Established exceptions are the `ESM`, `ESM4` and `VFS` namespaces (and classes such as `ESMReader`), as well as names that mirror types from Bethesda's file formats, such as `BSKeyframeController` in `components/nif/`. Cryptic abbreviations should be avoided.

## Includes

- A `.cpp` file includes its own header first.
- The remaining includes are arranged in groups separated by blank lines, usually in the following order: the standard library, external libraries (one group per library), `<components/...>`, other subsystems of the same application (one group per subsystem), and finally local headers in quotes.

```cpp
// apps/openmw/mwworld/actionharvest.cpp
#include "actionharvest.hpp"

#include <sstream>

#include <MyGUI_LanguageManager.h>

#include <components/misc/strings/format.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/animation.hpp"

#include "class.hpp"
#include "containerstore.hpp"
```

- Headers from `components/` are included with angle brackets, as in `<components/...>`, rather than with quotes.
- In `apps/openmw`, headers from other subsystems are included with a relative path, as in `"../mwbase/..."`. This allows clangd to detect unused includes (see [!3727](https://gitlab.com/OpenMW/openmw/-/merge_requests/3727)).
- clang-format sorts the includes within each group but does not create the groups (`IncludeBlocks: Preserve`), so the grouping has to be done by hand.

## Namespaces and `using`

- `using namespace` directives are not allowed in headers. In `.cpp` files they are rare and appear mostly in tests.
- `using` declarations (e.g. `using std::swap;`) are kept to the narrowest possible scope, and fully qualified names are generally preferred over them. Type aliases (`using Name = Type;`) are a different construct and are used freely.
- OpenMW's own code is not placed in the global namespace; the only exception is the file containing `main()`. Forward declarations of third-party types, such as `class QWidget;`, stay in the global namespace because that is where those types live.
- Helpers that are used only within a single file are placed in an anonymous namespace rather than declared `static`.

```cpp
// somefile.cpp
#include "somefile.hpp"

#include <string>
#include <vector>

namespace SomeSpace
{
    namespace
    {
        bool isEmptyName(const std::string& name) // Visible only in this file.
        {
            return name.empty();
        }
    }

    std::size_t countEmptyNames(const std::vector<std::string>& names)
    {
        std::size_t result = 0;
        for (const std::string& name : names)
            if (isEmptyName(name))
                ++result;
        return result;
    }
}
```

## Language and Idioms

- The code is written in C++20, as set in `CMakeLists.txt`.
- Null pointers are written as `nullptr` rather than `NULL`, and overriding virtual functions are marked with `override`.
- OSG objects are created with `new` and held in an `osg::ref_ptr`. Other heap-allocated objects are created with `std::make_unique` or `std::make_shared`.
- Standard containers are preferred over raw arrays, and objects are given automatic storage wherever possible.
- Classes follow the Rule of Zero: resources are held in members that manage themselves, such as `std::unique_ptr` or `std::vector`, so that no special member functions have to be written. A class that does need a custom destructor, copy operation or move operation follows the Rule of Five and declares all five, using `= default` or `= delete` where appropriate.
- OSG's own utilities are used where they exist: `osg::PI` and `osg::PIf` rather than `M_PI`, and `osg::Vec3f` for vectors. Values are clamped with `std::clamp`.
- Engine code logs through `Log` with the appropriate level: `Debug::Error`, `Debug::Warning`, `Debug::Info`, `Debug::Verbose` or `Debug::Debug`. `std::cout` is used only in command-line tools, in `esm4` and in the logging implementation itself.

```cpp
if (count < 0)
    Log(Debug::Warning) << "Negative count " << count << ", ignoring it";
```

- Strings are formatted with `std::format`. `std::string_view` is commonly used for passing strings.

## Comments

- Classes and functions are documented where their purpose is not obvious; trivial ones such as getters and setters are left undocumented. Doxygen commands such as `\brief` and `\param` are not required.

## Tests

- Tests are written with GoogleTest and GoogleMock and are located in `apps/components_tests`, `apps/openmw_tests` and `apps/opencs_tests`.
- Test suite names use CamelCase, usually start with the name of the subsystem under test and end with the suffix `Test`.
- Test names use camelBack and start with `should`, followed by the expected behaviour. Existing tests also use the `for_x_should_y` form, which should not be copied.

```cpp
TEST(VFSPathNormalizedTest, shouldSupportConstructorFromStringView)
```

## Other File Types

- CMake files are indented with four spaces rather than tabs; CI rejects tabs in them. Shaders are also indented with four spaces, as set in `.editorconfig`.
- The built-in Lua scripts in `files/data/scripts/omw/` are indented with four spaces, and local functions use camelBack. There is no linter configuration for Lua, so these conventions are not checked by any tool.
