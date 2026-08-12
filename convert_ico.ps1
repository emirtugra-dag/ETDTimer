Add-Type -AssemblyName System.Drawing

$srcPath = "c:\Users\vboxuser\Desktop\ETDTimer\etdtimer.png"
$icoPath = "c:\Users\vboxuser\Desktop\ETDTimer\res\app.ico"

$srcImg = [System.Drawing.Image]::FromFile($srcPath)
$sizes = @(256, 128, 64, 48, 32, 16)

$pngStreams = @()
foreach ($size in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality

    $g.DrawImage($srcImg, 0, 0, $size, $size)
    $g.Dispose()

    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()

    $pngStreams += @{ Size = $size; Bytes = $ms.ToArray() }
    $ms.Dispose()
}
$srcImg.Dispose()

$fs = New-Object System.IO.FileStream($icoPath, [System.IO.FileMode]::Create)
$bw = New-Object System.IO.BinaryWriter($fs)

# ICONDIR Header
$bw.Write([uint16]0) # Reserved
$bw.Write([uint16]1) # Type (1 = Icon)
$bw.Write([uint16]$pngStreams.Count) # Count

$offset = 6 + ($pngStreams.Count * 16)

foreach ($item in $pngStreams) {
    $w = if ($item.Size -ge 256) { 0 } else { [byte]$item.Size }
    $h = if ($item.Size -ge 256) { 0 } else { [byte]$item.Size }
    $len = $item.Bytes.Length

    $bw.Write([byte]$w)
    $bw.Write([byte]$h)
    $bw.Write([byte]0) # Color count
    $bw.Write([byte]0) # Reserved
    $bw.Write([uint16]1) # Planes
    $bw.Write([uint16]32) # Bit count
    $bw.Write([uint32]$len)
    $bw.Write([uint32]$offset)

    $offset += $len
}

foreach ($item in $pngStreams) {
    $bw.Write($item.Bytes, 0, $item.Bytes.Length)
}

$bw.Close()
$fs.Close()

Write-Host "Multi-resolution PNG ICO successfully generated at $icoPath!"
