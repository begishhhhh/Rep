# Initialize test environment
$OutputFile = "test_output.txt"
$TestFolderPath = "C:\Users\User\Desktop\test"
$MainScriptPath = "C:\Users\User\Desktop\architecture\log-cleaner.ps1"
$BackupPath = "C:\Users\User\Desktop\architecture\backup"

# Clean up previous test artifacts
if (Test-Path $OutputFile) {
    Remove-Item $OutputFile -Force
}

Write-Host "=== FOLDER CLEANUP SCRIPT TESTING ===" -ForegroundColor Yellow

# Test 1: Empty Folder Handling
Write-Host "`nTest 1: Empty folder verification"

# Create empty folder
if (Test-Path $TestFolderPath) {
    Remove-Item $TestFolderPath -Recurse -Force
}
New-Item -ItemType Directory -Path $TestFolderPath -Force | Out-Null

# Run main script
& $MainScriptPath -FolderPath $TestFolderPath *>> $OutputFile
$code = $LASTEXITCODE 

if ($code -eq 0) {
    Write-Host "PASS" -ForegroundColor Green
} else {
    Write-Host "FAIL" -ForegroundColor Red
}

# Cleanup
Remove-Item $TestFolderPath -Recurse -Force -ErrorAction SilentlyContinue

# Test 2: Non-Existent Folder Handling 
Write-Host "`nTest 2: Non-existent folder error handling"

$nonExistentPath = "C:\Users\User\Desktop\architecture\non_existent_folder"

# Run script with non-existent folder 
& $MainScriptPath -FolderPath $nonExistentPath *>> $OutputFile
$code = $LASTEXITCODE

# Check error message
$output = Get-Content $OutputFile -Raw -ErrorAction SilentlyContinue
if ($code -eq 1 -and $output -match "Error: folder does not exist") {
    Write-Host "PASS" -ForegroundColor Green
} else {
    Write-Host "FAIL" -ForegroundColor Red
}

# Test 3: Backup Directory Creation
Write-Host "`nTest 3: Backup directory creation"

# Create test folder
New-Item -ItemType Directory -Path $TestFolderPath -Force | Out-Null

# Run script
& $MainScriptPath -FolderPath $TestFolderPath *>> $OutputFile

# Check if backup folder was created
if (Test-Path $BackupPath) {
    Write-Host "PASS" -ForegroundColor Green
} else {
    Write-Host "FAIL" -ForegroundColor Red
}

# Cleanup
Remove-Item $TestFolderPath -Recurse -Force -ErrorAction SilentlyContinue

# Test 4: File Archiving Verification
Write-Host "`nTest 4: File archiving functionality"

# Create folder with files
New-Item -ItemType Directory -Path $TestFolderPath -Force | Out-Null

# Create 10 test files (~3MB each)
for ($i = 0; $i -lt 10; $i++) {
    $filePath = Join-Path $TestFolderPath "$i.txt"
    $content = "x" * (3MB)
    Set-Content -Path $filePath -Value $content -NoNewline
}

# Count files before
$filesBefore = (Get-ChildItem $TestFolderPath -File).Count

# Run script
& $MainScriptPath -FolderPath $TestFolderPath *>> $OutputFile
$code = $LASTEXITCODE

# Count files after
$filesAfter = (Get-ChildItem $TestFolderPath -File).Count

if ($code -eq 0 -and $filesAfter -lt $filesBefore) {
    Write-Host "PASS" -ForegroundColor Green
} else {
    Write-Host "FAIL" -ForegroundColor Red
}

# Test 5: Size Reduction Verification
Write-Host "`nTest 5: Folder size reduction check" 

# Get size after archiving
$sizeAfter = (Get-ChildItem $TestFolderPath -Recurse -File | Measure-Object -Property Length -Sum).Sum

if ($sizeAfter -le 15MB) {
    Write-Host "PASS" -ForegroundColor Green
} else {
    Write-Host "FAIL" -ForegroundColor Red
}

# Final cleanup
Remove-Item $TestFolderPath -Recurse -Force -ErrorAction SilentlyContinue
if (Test-Path $BackupPath) {
    Remove-Item $BackupPath -Recurse -Force
}
if (Test-Path $OutputFile) {
    Remove-Item $OutputFile -Force
}

Write-Host "`n=== TESTING COMPLETED ===" -ForegroundColor Yellow