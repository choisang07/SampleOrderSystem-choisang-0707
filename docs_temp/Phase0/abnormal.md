# Phase 0 작업 기록 (준비 단계)

Phase 0은 requirement.md 기반 TestCase가 아니라 스캐폴딩(PoC 이식, 패키지 구조 셋업) 중심이라 test 에이전트의 통상적인 TestCase 문서 대신, 작업 중 발견된 특이사항을 이 파일에 기록한다.

## 진행 개요

- 브랜치: `phase/0-setup`
- 1차 스캐폴딩 커밋: `f0910dd` — Model/Persistence/Repository/Controller/View 골격 이식, main.cpp DI 와이어링, `/system-test` Release 빌드 성공 확인.
- Domain/(State/Factory), Service/, Tests/(gmock 유닛 테스트)는 설계대로 이번 Phase에 만들지 않음(Phase 1~3 몫).

## Review 결과 (1차) — 이상점 2건

1. **(중간) `MainController`가 `JsonFileStore`(Persistence 구체 클래스)에 직접 의존** — design.md §3(Controller는 인터페이스/원시 타입만 알아야 함, DIP)과 §8(Controller가 로드 실패를 받아 배너 출력) 서술이 서로 모순되어 발생한 문제.
   - **해결 결정**: `main.cpp`가 `JsonFileStore::isCorrupted()` 결과를 확인해 `bool` 플래그 하나만 `MainController`에 전달하는 방식으로 확정(design.md §8 갱신 완료). 별도 인터페이스(`IDataLoadStatus` 등)는 도입하지 않음 — 구현체가 영원히 하나뿐이라 Strategy 패턴 제거 때와 같은 이유로 과설계라고 판단.
   - 조치: Develope가 `MainController`/`main.cpp`를 이 방식으로 수정 예정(진행 중).
2. **(경미) `JsonFileStore::isCorrupted()`가 `loadRoot()`와 파일 열기/파싱 로직을 중복** — CleanCode 관점 경미한 이슈. `tryLoadRoot()`류 헬퍼로 통합 권장.

## 확인된 양호 사항 (Review에서 문제없음으로 확인)

- Repository+DIP 원칙(인터페이스만 노출, `JsonFileStore&`에만 의존) 준수
- `JsonFileStore`가 컬렉션별 저장 시 다른 컬렉션 데이터를 보존(유실 없음)
- Model 클래스 순수성(로직 없음, JSON 매핑만)
- `main.cpp` DI 와이어링이 한 곳에 집중(위 이상점 1 제외)
- EOF 처리, 파일 손상 배너 계약 자체는 반영됨(의존 방향만 문제)
- vcxproj/filters에 신규 파일 전부 등록됨
- `data/store.json` 스키마가 design.md §5와 일치

## Review 이상점 반영 (Develope, `phase/0-setup`)

1. **`MainController`의 `JsonFileStore` 직접 의존 제거**
   - `MainController.h/.cpp`에서 `JsonFileStore` include/멤버/전방 선언을 모두 제거하고, 생성자 첫 인자를 `bool dataLoadFailed`로 변경(멤버 `dataLoadFailed_`로 보관).
   - `run()`은 `store_.isCorrupted()` 대신 `dataLoadFailed_`를 확인해 기존과 동일하게 로드 실패 배너를 출력한다(로직 변경 없음, 의존 대상만 교체).
   - `main.cpp`가 `JsonFileStore store(...)` 생성 직후 `store.isCorrupted()`를 호출해 `bool dataLoadFailed`를 만들고, 이 값만 `MainController` 생성자에 전달하도록 와이어링 변경. 구체 클래스를 아는 곳은 여전히 `main.cpp` 한 곳뿐(DIP 유지).
   - 별도 인터페이스(`IDataLoadStatus` 등)는 도입하지 않음(과설계 판단, 확정된 방침 그대로).
2. **`JsonFileStore::isCorrupted()`/`loadRoot()` 파일 열기·파싱 중복 제거**
   - 내부 헬퍼 `nlohmann::json tryLoadRoot(bool& fileExists, bool& parsedOk) const`를 추가해 파일 열기+파싱을 한 곳에 통합.
   - `loadRoot()`는 `tryLoadRoot()` 결과(json)만 그대로 반환(파일 없음/파싱 실패 시 빈 객체는 헬퍼가 이미 처리).
   - `isCorrupted()`는 `tryLoadRoot()`를 호출해 `fileExists && !parsedOk`로 판단(파일이 없으면 손상 아님, 있는데 파싱 실패면 손상 — 기존 의미 그대로 유지). 파일을 두 번 여는 코드 경로 제거.
   - `/system-test` 스킬 기준 Release|x64 빌드 재확인(msbuild 직접 호출, 성공, 등록된 회귀 케이스 없음).
