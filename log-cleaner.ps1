param(
    [string]$FolderPath = "C:\Users\User\Desktop\architecture\log",
    [int64]$SizeLimit = 15MB
)

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$path = $FolderPath
$limitBytes = $SizeLimit

# Check if the target folder exists
if (-not (Test-Path $path -PathType Container)) {
    Write-Host "Error: folder does not exist"
    exit 1
}

# Create a backup folder if it does not exist
$scriptFolder = "C:\Users\User\Desktop\architecture"
$backup = Join-Path $scriptFolder "backup"
New-Item -ItemType Directory -Force -Path $backup | Out-Null

# Calculate the total size of the folder (in bytes)
$size = (Get-ChildItem -Path $path -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum
if (-not $size) { $size = 0 }

# Display the configured limit and the current folder size
Write-Host ("Folder size limit: {0} MB" -f ($SizeLimit / 1MB))
Write-Host ("Current folder size: {0} MB" -f ([math]::Round($size / 1MB, 2)))

# Check if the folder size exceeds the limit
if ($size -le $limitBytes) {
    Write-Host "Folder size is within limit."
    exit 0
}

# Calculate how much space needs to be freed
$to_free = $size - $limitBytes
Write-Host ("Need to free: {0} MB" -f ([math]::Round($to_free / 1MB, 2)))

# Get all files and sort them by last modification date (oldest first)
$files = Get-ChildItem -Path $path -Recurse -File | Sort-Object LastWriteTime

$freed = 0
$files_to_archive = @()

# Select files to remove until the required space is freed
foreach ($file in $files) {
    $files_to_archive += $file.FullName
    $freed += $file.Length
    if ($freed -ge $to_free) { break }
}

# Check if there are files to archive
if ($files_to_archive.Count -eq 0) {
    Write-Host "No files to archive."
    exit 0
}

# Create a ZIP archive with a timestamped name
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$archive_name = "backup_$timestamp.zip"
$archive_path = Join-Path $backup $archive_name

# Compress selected files into the backup archive
try {
    Compress-Archive -Path $files_to_archive -DestinationPath $archive_path -Force
    Write-Host ("Archive created: {0}" -f $archive_path)
} catch {
    Write-Host "Error: failed to create archive"
    exit 1
}

# Delete archived files from the original folder
foreach ($f in $files_to_archive) {
    Remove-Item -Path $f -Force -ErrorAction SilentlyContinue

    # Remove empty directories up to the root folder
    $dir = Split-Path $f
    while ($dir -ne $path -and (Test-Path $dir) -and -not (Get-ChildItem $dir)) {
        Remove-Item $dir -Force
        $dir = Split-Path $dir
    }
}

# Final message when cleanup is complete
Write-Host "Old files have been archived and removed."
exit 0