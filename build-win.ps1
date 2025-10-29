Function Info($msg) {
  Write-Host -ForegroundColor DarkGreen "`nINFO: $msg`n"
}

Function Error($msg) {
  Write-Host `n`n
  Write-Error $msg
  exit 1
}

Function CheckReturnCodeOfPreviousCommand($msg) {
  if(-Not $?) {
    Error "${msg}. Error code: $LastExitCode"
  }
}

Function BuildAndTest($buildType, $arch) {
  Info "Build and test $buildType-$arch"

  $thisBuildDir = "$buildDir/win-$buildType-$arch"

  Info "Cmake generate cache"
  cmake -S $root `
        -B $thisBuildDir `
        -G Ninja `
        -D CMAKE_BUILD_TYPE=$buildType
  CheckReturnCodeOfPreviousCommand "cmake cache failed"

  Info "Cmake build"
  cmake --build $thisBuildDir
  CheckReturnCodeOfPreviousCommand "cmake build failed"

  Info "Run tests"
  & "$thisBuildDir/arduino_dsmr_test.exe"
  CheckReturnCodeOfPreviousCommand "tests failed"
}

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$root = Resolve-Path $PSScriptRoot
$buildDir = "$root/build"

Info "Find Visual Studio installation path"
$vswhereCommand = Get-Command -Name "${Env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installationPath = & $vswhereCommand -prerelease -latest -property installationPath

Info "Open Visual Studio 2022 Developer PowerShell amd64"
& "$installationPath\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64

BuildAndTest -buildType Debug -arch amd64
BuildAndTest -buildType Release -arch amd64

Info "Open Visual Studio 2022 Developer PowerShell x86"
& "$installationPath\Common7\Tools\Launch-VsDevShell.ps1" -Arch x86

BuildAndTest -buildType Debug -arch x86
BuildAndTest -buildType Release -arch x86
