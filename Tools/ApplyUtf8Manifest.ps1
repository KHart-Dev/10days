<#
  ApplyUtf8Manifest.ps1

  リリース版 exe に activeCodePage=UTF-8 のマニフェストを埋め込む。

  CalyxEngine は パスを GetModuleFileNameW でワイド文字取得したあと
  WideCharToMultiByte で narrow 化して CreateFileA でファイルを開く。
  既定の ANSI コードページ (日本語環境では CP932) と UTF-8 が食い違うため、
  exe の置き場所のパスに日本語が含まれるとアセットが一切読めなくなる
  (ウィンドウは出るが画面が真っ暗になる)。

  マニフェストでプロセスの ANSI コードページを UTF-8 に固定すると解消する。
  Windows 10 1903 以降で有効。

  使い方:
    powershell -ExecutionPolicy Bypass -File Tools\ApplyUtf8Manifest.ps1 <exeへのパス>

  リリースビルドをやり直すたびに exe は engine SDK の CalyxGame.exe で
  上書きされるので、そのたびに実行すること。
#>
param(
  [Parameter(Mandatory=$true)][string]$ExePath
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $ExePath)) { throw "exe が見つかりません: $ExePath" }

$manifest = Join-Path $PSScriptRoot 'utf8.manifest'
if (-not (Test-Path -LiteralPath $manifest)) { throw "utf8.manifest が見つかりません: $manifest" }

$mt = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin' -Recurse -Filter mt.exe -ErrorAction SilentlyContinue |
      Where-Object { $_.FullName -match '\x64\' } |
      Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
if (-not $mt) { throw 'mt.exe (Windows SDK) が見つかりません。' }

$backup = "$ExePath.orig_backup"
if (-not (Test-Path -LiteralPath $backup)) { Copy-Item -LiteralPath $ExePath -Destination $backup }

& $mt -nologo -manifest $manifest -outputresource:"$ExePath;#1"
if ($LASTEXITCODE -ne 0) { throw "mt.exe が失敗しました (exit $LASTEXITCODE)" }

$bytes = [System.IO.File]::ReadAllBytes($ExePath)
if ([System.Text.Encoding]::ASCII.GetString($bytes) -match 'activeCodePage') {
  Write-Host "OK: UTF-8 マニフェストを埋め込みました -> $ExePath"
} else {
  throw "埋め込みを確認できませんでした: $ExePath"
}
