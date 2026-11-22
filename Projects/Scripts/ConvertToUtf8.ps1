# Source\Code 以下の .h/.hpp/.cpp を UTF-8（BOM 付き）にそろえる

param(
    [string]$Root
)

$codeDir = Join-Path $Root 'Source\Code'

$utf8Strict = New-Object System.Text.UTF8Encoding($false, $true) # UTF-8 として不正なバイトがあれば例外を出す
$utf8Bom    = New-Object System.Text.UTF8Encoding($true)         # 書き込み用（BOM 付き）
$sjis       = [System.Text.Encoding]::GetEncoding(932)           # Shift-JIS

Get-ChildItem $codeDir -Recurse -File -Include *.h, *.hpp, *.cpp | ForEach-Object {
    $bytes = [IO.File]::ReadAllBytes($_.FullName)

    # すでに BOM 付き UTF-8 なら何もしない
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) { return }

    # UTF-8 として正しく読めるか試し、だめなら Shift-JIS として読む
    try {
        $text = $utf8Strict.GetString($bytes)
        $from = 'UTF-8(BOMなし)'
    } catch {
        $text = $sjis.GetString($bytes)
        $from = 'Shift-JIS'
    }

    [IO.File]::WriteAllText($_.FullName, $text, $utf8Bom)
    Write-Output ("{0} -> UTF-8(BOM): {1}" -f $from, $_.FullName)
}