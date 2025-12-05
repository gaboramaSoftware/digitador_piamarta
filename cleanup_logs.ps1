# Force close all cmd processes running build scripts
Get-Process cmd -ErrorAction SilentlyContinue | ForEach-Object {
    try {
        $_.Kill()
    } catch {}
}

Start-Sleep -Seconds 2

# Delete the build log files
Remove-Item -Path ".\src\build_log_delete*.txt" -Force -ErrorAction SilentlyContinue

Write-Host "Cleanup complete!" -ForegroundColor Green
