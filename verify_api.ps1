$baseUrl = "http://localhost:18080"

function Check-DashboardStats {
    Write-Host "--- [CHECK] Dashboard Stats ---" -ForegroundColor Cyan
    try {
        $stats = Invoke-RestMethod -Uri "$baseUrl/api/stats" -Method Get
        Write-Host "Status: OK" -ForegroundColor Green
        Write-Host "Data:"
        $stats | Format-List
    } catch {
        Write-Host "Status: ERROR - $_" -ForegroundColor Red
    }
    Write-Host ""
}

function Check-StudentsList {
    Write-Host "--- [CHECK] Students List ---" -ForegroundColor Cyan
    try {
        $students = Invoke-RestMethod -Uri "$baseUrl/api/students" -Method Get
        $count = $students.Count
        Write-Host "Status: OK" -ForegroundColor Green
        Write-Host "Total Students: $count"
        
        if ($count -gt 0) {
            Write-Host "Sample (First 5):"
            $students | Select-Object -First 5 | Format-Table
        }
    } catch {
        Write-Host "Status: ERROR - $_" -ForegroundColor Red
    }
    Write-Host ""
}

function Check-RecentRecords {
    Write-Host "--- [CHECK] Recent Records ---" -ForegroundColor Cyan
    try {
        $response = Invoke-RestMethod -Uri "$baseUrl/api/recent" -Method Get
        Write-Host "Status: OK" -ForegroundColor Green
        Write-Host "Total Recent: $($response.total)"
        
        if ($response.records.Count -gt 0) {
            Write-Host "Latest 5 Records:"
            $response.records | Select-Object -First 5 | Format-Table id, run, nombre_completo, tipo_racion, estado
        }
    } catch {
        Write-Host "Status: ERROR - $_" -ForegroundColor Red
    }
    Write-Host ""
}

# Menu
Write-Host "=== BLACK BOX VERIFICATION TOOLS ===" -ForegroundColor Yellow
Write-Host "1. Run All Checks"
Write-Host "2. Check Stats Only"
Write-Host "3. Check Students Only"
Write-Host "4. Check Recent Records Only"
Write-Host "===================================="

$selection = Read-Host "Select an option (1-4)"

switch ($selection) {
    '1' { Check-DashboardStats; Check-StudentsList; Check-RecentRecords }
    '2' { Check-DashboardStats }
    '3' { Check-StudentsList }
    '4' { Check-RecentRecords }
    default { Write-Host "Invalid selection." -ForegroundColor Red }
}
