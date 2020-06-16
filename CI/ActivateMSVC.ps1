& "${env:COMSPEC}" /c ActivateMSVC.bat "&&" set | ForEach-Object {
    if ($_.Contains("=")) {
        $name, $value = $_ -split '=', 2
        Set-Content env:\"$name" $value
    }
}

$MissingTools = $false
$tools = "cl", "link", "rc", "mt"
$descriptions = "MSVC Compiler", "MSVC Linker", "MS Windows Resource Compiler", "MS Windows Manifest Tool"
for ($i = 0; $i -lt $tools.Length; $i++) {
    $present = $true
    try {
        Get-Command $tools[$i] *>&1 | Out-Null
        $present = $present -and $?
    } catch {
        $present = $false
    }
    if (!$present) {
        Write-Warning "$($tools[$i]) ($($descriptions[$i])) missing."
        $MissingTools = $true
    }
}

if ($MissingTools) {
    Write-Error "Some build tools were unavailable after activating MSVC in the shell. It's likely that your Visual Studio $MSVC_DISPLAY_YEAR installation needs repairing."
    exit 1
}