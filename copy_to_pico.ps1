$binaryPath = ".\build\src\main.uf2"
try {
    $file = Get-Item $binaryPath -ErrorAction Stop
} catch {
    throw "Binary not found at path: " + $binaryPath
}

$picoLabel = "RPI-RP2"
try {

    $picoPath = (Get-Volume -FileSystemLabel $picoLabel -ErrorAction Stop).DriveLetter
    $picoPath += ":\"
} catch {
    throw "Pico not found at label: " + $picoLabel
}

Copy-Item $file -Destination $picoPath