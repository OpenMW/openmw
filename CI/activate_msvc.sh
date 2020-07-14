#!/bin/bash

set -euo pipefail

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "Error: Script not sourced."
    echo "You must source this script for it to work, i.e. "
    echo "source ./activate_msvc.sh"
    echo "or"
    echo ". ./activate_msvc.sh"
    exit 1
fi

command -v unixPathAsWindows >/dev/null 2>&1 || function unixPathAsWindows {
	if command -v cygpath >/dev/null 2>&1; then
		cygpath -w $1
	else
		echo "$1" | sed "s,^/\([^/]\)/,\\1:/," | sed "s,/,\\\\,g"
	fi
}

function windowsSystemPathAsUnix {
	if command -v cygpath >/dev/null 2>&1; then
		cygpath -u -p $1
	else
		IFS=';' read -r -a paths <<< "$1"
		declare -a convertedPaths
		for entry in paths; do
			convertedPaths+=(windowsPathAsUnix $entry)
		done
		convertedPath=printf ":%s" ${convertedPaths[@]}
		echo ${convertedPath:1}
	fi
}

# capture CMD environment so we know what's been changed
declare -A originalCmdEnv
originalIFS="$IFS"
IFS=$'\n\r'
for pair in $(cmd //c "set"); do
    IFS='=' read -r -a separatedPair <<< "${pair}"
    if [ ${#separatedPair[@]} -ne 2 ]; then
        echo "Parsed '$pair' as ${#separatedPair[@]} parts, expected 2."
        continue
    fi
    originalCmdEnv["${separatedPair[0]}"]="${separatedPair[1]}"
done

# capture CMD environment in a shell with MSVC activated
cmdEnv="$(cmd //c "$(unixPathAsWindows "$(dirname "${BASH_SOURCE[0]}")")\ActivateMSVC.bat" "&&" set)"

declare -A cmdEnvChanges
for pair in $cmdEnv; do
    if [ -n "$pair" ]; then
        IFS='=' read -r -a separatedPair <<< "${pair}"
        if [ ${#separatedPair[@]} -ne 2 ]; then
            echo "Parsed '$pair' as ${#separatedPair[@]} parts, expected 2."
            continue
        fi
        key="${separatedPair[0]}"
        value="${separatedPair[1]}"
        if ! [ ${originalCmdEnv[$key]+_} ] || [ "${originalCmdEnv[$key]}" != "$value" ]; then
            if [ $key != 'PATH' ] && [ $key != 'path' ] && [ $key != 'Path' ]; then
                export "$key=$value"
            else
                export PATH=$(windowsSystemPathAsUnix $value)
            fi
        fi
    fi
done

MISSINGTOOLS=0

command -v cl >/dev/null 2>&1 || { echo "Error: cl (MSVC Compiler) missing."; MISSINGTOOLS=1; }
command -v link >/dev/null 2>&1 || { echo "Error: link (MSVC Linker) missing."; MISSINGTOOLS=1; }
command -v rc >/dev/null 2>&1 || { echo "Error: rc (MS Windows Resource Compiler) missing."; MISSINGTOOLS=1; }
command -v mt >/dev/null 2>&1 || { echo "Error: mt (MS Windows Manifest Tool) missing."; MISSINGTOOLS=1; }

if [ $MISSINGTOOLS -ne 0 ]; then
    echo "Some build tools were unavailable after activating MSVC in the shell. It's likely that your Visual Studio $MSVC_DISPLAY_YEAR installation needs repairing."
    return 1
fi

IFS="$originalIFS"