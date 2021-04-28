#!/bin/bash

oldSettings=$-
set -eu

function restoreOldSettings {
    if [[ $oldSettings != *e* ]]; then
        set +e
    fi
    if [[ $oldSettings != *u* ]]; then
        set +u
    fi
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "Error: Script not sourced."
    echo "You must source this script for it to work, i.e. "
    echo "source ./activate_msvc.sh"
    echo "or"
    echo ". ./activate_msvc.sh"
    restoreOldSettings
    exit 1
fi

command -v unixPathAsWindows >/dev/null 2>&1 || function unixPathAsWindows {
	if command -v cygpath >/dev/null 2>&1; then
		cygpath -w $1
	else
		echo "$1" | sed "s,^/\([^/]\)/,\\1:/," | sed "s,/,\\\\,g"
	fi
}


# capture CMD environment in a shell with MSVC activated
cmd //c "$(unixPathAsWindows "$(dirname "${BASH_SOURCE[0]}")")\ActivateMSVC.bat" "&&" "bash" "-c" "declare -px > declared_env.sh"
source ./declared_env.sh
rm declared_env.sh

MISSINGTOOLS=0

command -v cl >/dev/null 2>&1 || { echo "Error: cl (MSVC Compiler) missing."; MISSINGTOOLS=1; }
command -v link >/dev/null 2>&1 || { echo "Error: link (MSVC Linker) missing."; MISSINGTOOLS=1; }
command -v rc >/dev/null 2>&1 || { echo "Error: rc (MS Windows Resource Compiler) missing."; MISSINGTOOLS=1; }
command -v mt >/dev/null 2>&1 || { echo "Error: mt (MS Windows Manifest Tool) missing."; MISSINGTOOLS=1; }

if [ $MISSINGTOOLS -ne 0 ]; then
    echo "Some build tools were unavailable after activating MSVC in the shell. It's likely that your Visual Studio $MSVC_DISPLAY_YEAR installation needs repairing."
    restoreOldSettings
    return 1
fi

restoreOldSettings
