# test_task1.ps1 — Run Server1 in background, then Client, then compare files
Set-Location "C:\Users\sd407\OneDrive\Desktop\lab-02\Task1_File_Transfer"

# Start Server in background, redirect its output to a file
$srv = Start-Process -FilePath ".\Server1.exe" `
       -PassThru -WindowStyle Hidden `
       -RedirectStandardOutput "server_output.txt"

Write-Host "Server1 started (PID $($srv.Id)). Waiting 2 seconds for it to bind..."
Start-Sleep -Seconds 2

# Run Client and capture output
Write-Host ""
Write-Host "=== CLIENT OUTPUT ==="
.\Client.exe "..\test_files\alice29.txt"

# Give server a moment to finish writing
Start-Sleep -Seconds 1

# Stop server if still running
if (-not $srv.HasExited) {
    $srv.Kill()
}

Write-Host ""
Write-Host "=== SERVER OUTPUT ==="
Get-Content "server_output.txt"
