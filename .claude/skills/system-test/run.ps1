<#
.SYNOPSIS
  SampleOrderSystem(Project17) 콘솔 앱을 Release|x64로 1회 빌드하고,
  $cases에 누적된 콘솔 입출력 회귀 케이스를 그 빌드에 대해 일괄 실행/비교한다.

.PARAMETER SkipBuild
  이미 최신 Release 빌드가 있을 때 빌드를 건너뛰고 케이스 실행만 반복한다.
#>
param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$solution = Join-Path $repoRoot "Project17.slnx"
$exePath  = Join-Path $repoRoot "x64\Release\Project17.exe"

# ---------------------------------------------------------------------------
# 회귀 케이스 목록 — test 에이전트가 Phase별 TestCase 작성/회귀 시 이 배열에
# 케이스를 추가한다. develope/메인 에이전트가 이 스크립트로 실제 빌드+실행
# 검증을 수행한다 (test 에이전트는 Bash가 없어 직접 실행할 수 없다).
#
# 케이스 형식:
#   @{ Category = "<분류명>"; InputLines = @('<콘솔에 한 줄씩 입력할 값>', ...); Expect = "<기대 출력(줄바꿈으로 구분)>" }
#   구문/런타임 오류처럼 "정확한 메시지"가 아니라 오류 발생 여부만 확인하면 Expect 대신 ExpectError = $true
# ---------------------------------------------------------------------------
$cases = @(
    # Phase 0(스켈레톤/영속성 이식)이 끝나고 실제 메뉴가 생기면 여기에 케이스를 추가한다.
    # 예시:
    # @{ Category = "시료 등록"; InputLines = @('1', 'S1', 'WaferA', '2.0', '0.9', '0'); Expect = "시료가 등록되었습니다." }
)

if (-not $SkipBuild) {
    Write-Host "Building $solution (Release|x64)..." -ForegroundColor Cyan

    $msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
    if (-not $msbuild) {
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vswhere) {
            $vsInstall = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
            if ($vsInstall) {
                $msbuildPath = Join-Path $vsInstall "MSBuild\Current\Bin\MSBuild.exe"
                if (Test-Path $msbuildPath) {
                    $msbuild = @{ Source = $msbuildPath }
                }
            }
        }
    }
    if (-not $msbuild) {
        throw "msbuild.exe를 찾을 수 없습니다. Developer PowerShell for VS에서 실행하거나 PATH에 msbuild를 추가하세요."
    }

    & $msbuild.Source $solution /p:Configuration=Release /p:Platform=x64 /nologo /verbosity:minimal
    if ($LASTEXITCODE -ne 0) {
        throw "빌드 실패 (exit code $LASTEXITCODE). 케이스를 실행하지 않고 중단합니다."
    }
}

if (-not (Test-Path $exePath)) {
    throw "실행 파일을 찾을 수 없습니다: $exePath (빌드가 정상적으로 끝났는지 확인하세요.)"
}

if ($cases.Count -eq 0) {
    Write-Host "등록된 회귀 케이스가 없습니다. 빌드 성공만 확인했습니다." -ForegroundColor Yellow
    exit 0
}

$results = @()
foreach ($case in $cases) {
    $input = ($case.InputLines -join "`n") + "`n"
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exePath
    $psi.RedirectStandardInput = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.UseShellExecute = $false

    $proc = [System.Diagnostics.Process]::Start($psi)
    $proc.StandardInput.Write($input)
    $proc.StandardInput.Close()
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    $actualLines = ($stdout -split "`r?`n") | Where-Object { $_ -ne "" }
    $actual = $actualLines -join "`n"

    if ($case.ContainsKey("ExpectError") -and $case.ExpectError) {
        $pass = -not [string]::IsNullOrWhiteSpace($stderr)
        $expectDisplay = "(에러 발생)"
        $actualDisplay = if ($pass) { "(에러 발생함)" } else { "(에러 없음)" }
    } else {
        $pass = ($actual -eq $case.Expect)
        $expectDisplay = $case.Expect
        $actualDisplay = $actual
    }

    $results += [PSCustomObject]@{
        Category = $case.Category
        Input    = ($case.InputLines -join " / ")
        Expect   = $expectDisplay
        Actual   = $actualDisplay
        Result   = if ($pass) { "PASS" } else { "FAIL" }
    }
}

$results | Format-Table -AutoSize -Wrap

$failCount = ($results | Where-Object { $_.Result -eq "FAIL" }).Count
Write-Host "`n총 $($results.Count)건 중 실패 $failCount 건" -ForegroundColor $(if ($failCount -eq 0) { "Green" } else { "Red" })
exit $failCount
