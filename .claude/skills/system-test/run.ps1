<#
.SYNOPSIS
  SampleOrderSystem 콘솔 앱을 Release|x64로 1회 빌드하고,
  $cases에 누적된 콘솔 입출력 회귀 케이스를 그 빌드에 대해 일괄 실행/비교한다.

.PARAMETER SkipBuild
  이미 최신 Release 빌드가 있을 때 빌드를 건너뛰고 케이스 실행만 반복한다.
#>
param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$solution = Join-Path $repoRoot "SampleOrderSystem.slnx"
$exePath  = Join-Path $repoRoot "x64\Release\SampleOrderSystem.exe"

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
    # Phase 1(시료 관리) 회귀 케이스 — docs_temp/Phase1/testcase.md TC-P1-001/010/020/032 대응.
    # 실제 메뉴 문구는 SampleOrderSystem/View/ConsoleView.cpp, Controller/MainController.cpp 기준
    # (2026-07-15 tester 회귀 검증 시 실제 Release 빌드 실행 결과로 채취).

    # TC-P1-001: 시료 등록(정상 등록, Happy Path). 메인메뉴[1]->등록[1]->이름/평균생산시간/수율 입력
    # -> 등록 완료 메시지(자동 채번 ID: S-001) -> 뒤로[0] -> 종료[0].
    @{
        Category   = "TC-P1-001 시료 등록 (정상 등록)"
        InputLines = @('1', '1', 'WaferA', '2.0', '0.9', '0', '0')
        Expect     = "==============================`n 반도체 시료 생산주문관리 시스템`n==============================`n[1] 시료 관리`n[2] 주문 (접수/승인/거절)`n[3] 모니터링`n[4] 출고 처리`n[5] 생산 라인`n[0] 종료`n선택 > __________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > 이름 > 평균 생산시간 > 수율 > 등록이 완료되었습니다. (ID: S-001)`n__________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > ==============================`n 반도체 시료 생산주문관리 시스템`n==============================`n[1] 시료 관리`n[2] 주문 (접수/승인/거절)`n[3] 모니터링`n[4] 출고 처리`n[5] 생산 라인`n[0] 종료`n선택 > "
    }

    # TC-P1-010: 저장소가 비어 있을 때 시료 목록 조회. 메인메뉴[1]->조회[2]
    # -> "등록된 시료가 없습니다." -> 뒤로[0] -> 종료[0].
    @{
        Category   = "TC-P1-010 시료 조회 (빈 목록)"
        InputLines = @('1', '2', '0', '0')
        Expect     = "==============================`n 반도체 시료 생산주문관리 시스템`n==============================`n[1] 시료 관리`n[2] 주문 (접수/승인/거절)`n[3] 모니터링`n[4] 출고 처리`n[5] 생산 라인`n[0] 종료`n선택 > __________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > 등록된 시료가 없습니다.`n__________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > ==============================`n 반도체 시료 생산주문관리 시스템`n==============================`n[1] 시료 관리`n[2] 주문 (접수/승인/거절)`n[3] 모니터링`n[4] 출고 처리`n[5] 생산 라인`n[0] 종료`n선택 > "
    }

    # TC-P1-020: 시료 등록 후 이름으로 정확 일치 검색. 메인메뉴[1]->등록[1]->WaferA/2.0/0.9
    # -> 검색[3]->"WaferA" -> 검색 결과에 S-001 표시 -> 뒤로[0] -> 종료[0].
    @{
        Category   = "TC-P1-020 시료 검색 (정상 검색)"
        InputLines = @('1', '1', 'WaferA', '2.0', '0.9', '3', 'WaferA', '0', '0')
        Expect     = "==============================`n 반도체 시료 생산주문관리 시스템`n==============================`n[1] 시료 관리`n[2] 주문 (접수/승인/거절)`n[3] 모니터링`n[4] 출고 처리`n[5] 생산 라인`n[0] 종료`n선택 > __________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > 이름 > 평균 생산시간 > 수율 > 등록이 완료되었습니다. (ID: S-001)`n__________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > 검색어 > 등록 시료 목록 (총 1 종)`nID`t시료명`t평균 생산시간`t수율`t현재 재고`nS-001`tWaferA`t2`t0.9`t0`n__________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > ==============================`n 반도체 시료 생산주문관리 시스템`n==============================`n[1] 시료 관리`n[2] 주문 (접수/승인/거절)`n[3] 모니터링`n[4] 출고 처리`n[5] 생산 라인`n[0] 종료`n선택 > "
    }

    # TC-P1-032: 등록 -> 조회 -> 검색 통합 플로우. 메인메뉴[1]->등록[1]->WaferA/2.0/0.9
    # -> 조회[2](방금 등록한 시료 표시) -> 검색[3]->"Wafer"(부분 일치, 동일 시료 표시)
    # -> 뒤로[0] -> 종료[0].
    @{
        Category   = "TC-P1-032 등록->조회->검색 통합 플로우"
        InputLines = @('1', '1', 'WaferA', '2.0', '0.9', '2', '3', 'Wafer', '0', '0')
        Expect     = "==============================`n 반도체 시료 생산주문관리 시스템`n==============================`n[1] 시료 관리`n[2] 주문 (접수/승인/거절)`n[3] 모니터링`n[4] 출고 처리`n[5] 생산 라인`n[0] 종료`n선택 > __________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > 이름 > 평균 생산시간 > 수율 > 등록이 완료되었습니다. (ID: S-001)`n__________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > 등록 시료 목록 (총 1 종)`nID`t시료명`t평균 생산시간`t수율`t현재 재고`nS-001`tWaferA`t2`t0.9`t0`n__________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > 검색어 > 등록 시료 목록 (총 1 종)`nID`t시료명`t평균 생산시간`t수율`t현재 재고`nS-001`tWaferA`t2`t0.9`t0`n__________________________________________________`n[1] 시료 관리`n__________________________________________________`n[1] 시료 등록    [2] 시료 목록    [3] 시료 검색    [0] 뒤로`n선택 > ==============================`n 반도체 시료 생산주문관리 시스템`n==============================`n[1] 시료 관리`n[2] 주문 (접수/승인/거절)`n[3] 모니터링`n[4] 출고 처리`n[5] 생산 라인`n[0] 종료`n선택 > "
    }
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
