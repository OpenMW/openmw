if (-Not (Test-Path CMakeCache.txt))
{
    Write-Error "This script must be run from the build directory."
}

if (-Not (Test-Path .cmake\api\v1\reply))
{
    New-Item -Type File -Force .cmake\api\v1\query\codemodel-v2
    cmake .
}

try
{
    Push-Location .cmake\api\v1\reply

    $index = Get-Content -Raw index-*.json | ConvertFrom-Json

    $codemodel = Get-Content -Raw $index.reply."codemodel-v2".jsonFile | ConvertFrom-Json

    $targets = @()
    $codemodel.configurations | ForEach-Object {
        $_.targets | ForEach-Object {
            $target = Get-Content -Raw $_.jsonFile | ConvertFrom-Json
            if ($target.type -eq "EXECUTABLE" -or $target.type -eq "SHARED_LIBRARY")
            {
                $targets += $target
            }
        }
    }

    $artifacts = @()
    $targets | ForEach-Object {
        $_.artifacts | ForEach-Object {
            $artifacts += $_.path
        }
    }
}
finally
{
    Pop-Location
}

if (-not (Test-Path symstore-venv))
{
    python -m venv symstore-venv
}
if (-not (Test-Path symstore-venv\Scripts\symstore.exe))
{
    symstore-venv\Scripts\pip install symstore==0.3.3
}
$artifacts = $artifacts | Where-Object { Test-Path $_ }
symstore-venv\Scripts\symstore --compress .\SymStore @artifacts
