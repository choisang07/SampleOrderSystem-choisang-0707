# Phase 1 테스트케이스 — 시료 관리 (등록/조회/검색) [확정본]

범위: `docs/PLAN.md` Phase 1 = `SampleService` + `SampleFactory` + 시료 관리 메뉴(`MainController::handleSampleManagement`).
관련 요구사항: `docs/requirement.md` 5.2절(시료 관리), 5.1절(메인 메뉴 표), 3장(시스템 개요 - 시료 개념).
관련 설계: `docs/design.md` §3(패키지 구조 - `Domain/Factory/SampleFactory`, `Service/SampleService`), §4.4(Factory 패턴 상세), §11.1(유닛 테스트 대상).

> **개정 이력**: 최초 작성(커밋 90cbdfd) 시 abnormal.md에 기록된 6개 미확정 분기 중, 아래 3가지는 사용자가 직접 결정했고 나머지 3가지는 문서에 적혀 있던 "잠정 제안"을 그대로 채택해 확정했다. 이 문서는 그 결정을 반영한 **확정본**이다. 결정 근거/기록은 [abnormal.md](abnormal.md)에 유지한다.
>
> 1. **시료 ID**: 시스템 자동 채번(`S{n}`, 예: `S1`, `S2`, ...). 사용자는 이름/평균생산시간/수율만 입력.
> 2. **수율 유효범위**: `(0, 1]` — 0 초과, 1 이하. 0.0은 등록 거부.
> 3. **초기 재고(stock)**: 등록 메뉴에서 입력받지 않음. 항상 `stock=0`으로 시작하며, 이후 Phase 3(주문 승인/생산)을 통해서만 증가.
> 4. **검색 매칭 방식**: 부분 일치(contains) + 대소문자 무시 + 이름(name)만 검색 대상(ID 등 다른 속성은 제외).
> 5. **이름 유일성**: 이름 중복 등록 허용(서로 다른 ID를 가진 동명 시료가 여러 건 존재 가능).
> 6. **빈 검색어**: "검색어를 입력하세요" 안내 후 재입력 요구(검색 실행 안 함).
>
> **주의(ID 포맷 관련 문서 간 불일치)**: `docs/design.md` §5는 화면 예시(`docs/screens.md`) 근거로 `S-{3자리 순번}`(예: `S-001`) 포맷을 "현재 기준"으로 제시하지만, 사용자가 이번에 확정한 포맷은 `S{n}`(예: `S1`)이다. 이 문서(테스트케이스)는 사용자의 명시적 결정(`S{n}`)을 기준으로 작성했다. `design.md` 자체의 포맷 서술 갱신 여부는 Test 역할의 편집 권한 밖이므로, [abnormal.md](abnormal.md) 1번 항목에 메모로 남기고 Develope/사용자가 `design.md` 갱신 여부를 판단하도록 한다. 실제 구현 시 어느 포맷이든 **일관되게만** 적용되면 이 문서의 케이스들은 ID 리터럴만 치환해 그대로 유효하다.

## 0. Phase 0 이식 코드 확인 결과 (실제 클래스/필드명)

- `Model/Sample.h`: `struct Sample { std::string id; std::string name; double avgProductionTime = 0.0; double yield = 1.0; int stock = 0; };`
- `Repository/ISampleRepository.h`: `findById(id) -> optional<Sample>`, `findAll() -> vector<Sample>`, `save(sample)`(upsert, 즉시 영속화). **채번(next id) 전용 메서드는 없다** — `SampleFactory`(또는 `SampleService`)가 `findAll()` 결과에서 `S{n}` 접미사의 최댓값을 파싱해 `+1`로 다음 ID를 계산하는 책임을 진다고 가정한다(design.md §8 "Repository 구현체가 채번을 전담" 원칙과, 인터페이스에 채번 메서드가 없는 현재 코드 사이의 간극 — Develope가 `ISampleRepository`에 채번 메서드를 추가할지, `SampleFactory`가 `findAll()`로 직접 계산할지는 구현 선택 사항. 이 문서의 테스트는 "다음 ID가 올바르게 계산된다"는 결과만 검증하므로 어느 방식이든 무방하다).
- `Repository/JsonSampleRepository`: Phase 0에서 이미 구현 완료(메모리 캐시 + `JsonFileStore` 통한 영속화). Phase 1에서는 이 위에 `SampleFactory`(생성 검증)와 `SampleService`(등록/조회/검색 오케스트레이션), `MainController::handleSampleManagement` 하위 메뉴만 추가하면 된다.
- `Domain/Factory/SampleFactory`, `Service/SampleService`는 아직 미구현(Phase 1에서 Develope가 신규 작성) — design.md §3, §4.4 기준으로 시그니처를 가정해 테스트케이스/유닛테스트를 작성했다. 실제 시그니처가 다르면 회귀 단계에서 이 문서와 `Tests/*.cpp`를 갱신한다.
- `MainController::handleSampleManagement()`는 현재 TODO 스텁(`"[시료 관리] 기능은 Phase 1에서 구현됩니다."` 출력)만 존재 — Phase 1에서 등록/조회/검색 하위 메뉴로 채워야 한다.

---

## A. 시료 등록 (`SampleFactory::create` + `SampleService::registerSample`)

### TC-P1-001: 정상 등록 (Happy Path)
- **대상 기능**: 신규 시료 등록
- **Given**: 저장소가 비어 있거나 임의의 시료들이 등록되어 있음. 사용자가 이름="WaferA", 평균생산시간=2.0, 수율=0.9를 입력한다. **ID는 입력하지 않는다** — 시스템이 자동 채번한다.
- **When**: 등록 메뉴에서 위 값을 입력해 등록을 실행한다.
- **Then**: `SampleService::registerSample("WaferA", 2.0, 0.9)`가 성공적으로 `Sample{id:"S1", name:"WaferA", avgProductionTime:2.0, yield:0.9, stock:0}`을 생성해 `ISampleRepository::save`를 호출한다. `stock`은 등록 입력값에 없으므로 **항상 0으로 초기화**된다(확정). 등록 완료 메시지와 시스템이 부여한 ID(`S1`)가 화면에 표시된다.
- **참조**: requirement.md 5.2절, design.md §4.4, §3(Model/Sample.h)

### TC-P1-002: (삭제됨 — 자동 채번 채택으로 해당 없음)
- ID 자동 채번을 확정 채택했으므로, 사용자가 ID를 직접 입력해 중복을 일으키는 시나리오 자체가 발생하지 않는다. 이 케이스 번호는 결번으로 유지하고, 채번 로직 자체의 검증은 아래 TC-P1-002b로 대체한다.

### TC-P1-002b: ID 채번 로직 검증 (확정)
- **대상 기능**: 시스템의 `S{n}` 자동 채번
- **Given**: 저장소에 `S1`, `S2`, `S3`이 등록되어 있음.
- **When**: 신규 시료(이름="WaferD", 평균생산시간=1.0, 수율=0.8)를 등록한다.
- **Then**: 새 ID는 `S4`로 부여된다(기존 ID들의 숫자 접미사 최댓값 3에 +1).
- **경계 케이스**: 저장소가 완전히 비어 있는 상태에서 최초 등록 시 ID는 `S1`이어야 한다(TC-P1-001과 동일 케이스, 중복 확인용).
- **참조**: design.md §8("ID 채번 방식의 한계" 관련 서술, "Repository 구현체가 채번을 전담"), abnormal.md 1번 항목

### TC-P1-003: 이름 빈 문자열 등록 거부
- **대상 기능**: `SampleFactory` 검증 — 이름 필수
- **Given**: 이름 입력값이 빈 문자열("") 또는 공백만("   ")인 경우.
- **When**: 등록을 시도한다.
- **Then**: 등록이 거부되고 에러 메시지가 표시된다. 저장소에 아무 것도 추가되지 않는다(`save` 호출 안 됨).
- **참조**: requirement.md 5.2절("고유한 이름"), design.md §4.4

### TC-P1-004: 평균 생산시간 0 이하 등록 거부
- **대상 기능**: `SampleFactory` 검증 — 평균 생산시간 유효 범위(0 초과)
- **Given**: 평균 생산시간 입력값이 0, 또는 음수(-1.5)인 경우.
- **When**: 등록을 시도한다.
- **Then**: 두 경우 모두 등록이 거부된다("0 초과"가 조건이므로 0도 실패해야 함). 에러 메시지 표시, 저장소 변경 없음.
- **참조**: design.md §4.4("평균 생산시간... 0 초과")

### TC-P1-005: 수율 경계값 0.0 (0%) 등록 거부 [확정: 가정 B 채택]
- **대상 기능**: `SampleFactory` 검증 — 수율 유효 범위 하한
- **Given**: 수율 입력값 = 0.0.
- **When**: 등록을 시도한다.
- **Then**: **등록 거부**. "수율은 0보다 커야 합니다" 류의 에러 메시지가 표시되고 저장소 변경 없음(`save` 호출 안 됨).
- **확정 근거**: requirement.md 5.6절의 실 생산량 공식 `ceil(부족분/yield)`이 `yield=0`이면 0으로 나누기가 되어 Phase 3에서 미정의 동작을 유발한다. 이를 등록 시점에 원천 차단하기로 확정(abnormal.md 3번 항목 확정 결과). **유효 수율 범위는 `(0, 1]`**로 통일한다.
- **참조**: design.md §4.4, requirement.md 5.6절(실 생산량 공식)

### TC-P1-006: 수율 경계값 1.0 (100%) 등록
- **대상 기능**: `SampleFactory` 검증 — 수율 상한
- **Given**: 수율 입력값 = 1.0.
- **When**: 등록을 시도한다.
- **Then**: 등록 성공(`yield=1.0`으로 저장). 100%는 유효 범위 `(0, 1]`의 상한 경계값이므로 허용되어야 한다.
- **참조**: requirement.md 5.2절(수율 정의: 정상 시료 수/총 생산 시료 수, 이론상 1.0까지 가능)

### TC-P1-007: 수율 범위 밖 값(0 포함 이하, 1 초과) 등록 거부 [확정]
- **대상 기능**: `SampleFactory` 검증 — 수율 범위
- **Given**: 수율 입력값 = -0.1, 0.0(TC-P1-005와 중복 확인용), 또는 1.5.
- **When**: 등록을 시도한다.
- **Then**: 세 경우 모두 등록 거부. "수율은 0보다 크고 1 이하여야 합니다" 에러 메시지. 저장소 변경 없음.
- **유효 범위 확정**: 수율은 `(0, 1]` — 0 초과, 1 이하.
- **참조**: design.md §4.4, abnormal.md 3번 항목

### TC-P1-008: 평균 생산시간/수율에 숫자가 아닌 값 입력
- **대상 기능**: 입력 파싱(View/Controller 레벨) 예외 처리
- **Given**: 평균 생산시간 입력값 = "abc" (숫자 파싱 불가).
- **When**: 등록을 시도한다.
- **Then**: 프로그램이 크래시하지 않고 "숫자를 입력하세요" 류의 에러 메시지를 표시한 뒤 등록 메뉴로 복귀(또는 재입력 요구)한다. 저장소 변경 없음.
- **참조**: design.md §8(EOF/예외 상황 공통 처리 원칙 연장선), requirement.md 5.2절

### TC-P1-009: 이름 중복 등록 허용 [확정]
- **대상 기능**: `SampleFactory` 검증 — 이름 유일성 없음
- **Given**: 이미 이름="WaferA"인 시료(`S1`)가 등록되어 있음.
- **When**: 이름="WaferA", 평균생산시간=3.0, 수율=0.7로 두 번째 시료를 등록한다(ID는 자동 채번되어 `S2`).
- **Then**: **등록이 허용된다**(확정: 이름 유일성 요구 없음). 서로 다른 ID(`S1`, `S2`)를 가진 동명 시료 2건이 저장소에 존재하게 된다. 이후 이름 검색(TC-P1-021)에서 두 건 모두 조회됨을 함께 확인한다.
- **참조**: abnormal.md 5번 항목(확정)

---

## B. 시료 조회 (`SampleService::listAll` / 목록 조회 메뉴)

### TC-P1-010: 등록된 시료가 없을 때 조회
- **대상 기능**: 시료 목록 조회
- **Given**: 저장소가 완전히 비어 있음(`findAll()`이 빈 벡터 반환).
- **When**: 시료 조회 메뉴를 실행한다.
- **Then**: "등록된 시료가 없습니다" 류의 안내 메시지가 표시되고, 프로그램은 정상적으로 메뉴로 복귀한다(크래시/빈 화면 없음).
- **참조**: requirement.md 5.2절("시료 조회: 등록된 모든 시료 목록 확인")

### TC-P1-011: 다수 시료 등록 후 전체 목록 조회 (재고 포함, stock=0 시작 확정 반영)
- **대상 기능**: 시료 목록 조회 — 현재 재고 수량 포함 여부
- **Given**: `S1`(WaferA)와 `S2`(WaferB)가 각각 등록 직후 상태(둘 다 `stock=0`, 확정)로 있음. 이후 Phase 3 승인 처리를 통해 `S1.stock`이 5로 증가했다고 가정(재고가 0이 아닌 경우도 표시되는지 확인하기 위한 전제 — Phase 1 시점에는 등록 메뉴만으로는 재고를 바꿀 수 없으므로, 이 케이스는 테스트 픽스처로 저장소에 `stock=5`인 `S1`을 직접 시딩해 검증한다).
- **When**: 시료 조회 메뉴를 실행한다.
- **Then**: 두 시료 모두 목록에 표시되며, 각 행에 ID/이름/평균생산시간/수율/**현재 재고 수량**이 함께 표시된다(요구사항 5.2절 "현재 재고 수량 포함" 명시). `S2`는 등록 직후 상태이므로 재고 0으로 표시된다.
- **참조**: requirement.md 5.2절

### TC-P1-012: 재고 0인 시료도 목록에 포함되는지 확인
- **대상 기능**: 시료 목록 조회 — 재고 0 경계
- **Given**: `S2`(등록 직후, stock=0)만 등록되어 있음. **확정 사항에 따라 등록 직후 재고는 항상 0이므로, 이 케이스는 신규 등록 후 즉시 조회하는 것만으로 자연히 재현된다**(별도 시딩 불필요).
- **When**: 시료 조회를 실행한다.
- **Then**: `S2`가 "재고 0"으로 정상 표시된다(목록에서 제외되지 않음). 5.5절의 "고갈" 판정(모니터링 전용, Phase 5)과 혼동하지 않는다 — Phase 1 조회에서는 단순히 수치 0만 표시하면 된다.
- **참조**: requirement.md 5.2절, 5.5절(참고 구분용)

---

## C. 시료 검색 (`SampleService::search` / 이름 검색 메뉴)

검색 방식 확정: **부분 일치(contains) + 대소문자 무시(case-insensitive) + 이름(name)만 검색 대상**(ID 등 다른 속성 제외).

### TC-P1-020: 이름으로 정확히 일치하는 시료 검색
- **대상 기능**: 이름 검색 — 존재하는 이름
- **Given**: `S1`(WaferA)가 등록되어 있음.
- **When**: 검색어 "WaferA"로 검색을 실행한다.
- **Then**: `S1`이 검색 결과에 포함되어 표시된다.
- **참조**: requirement.md 5.2절("이름 등 속성으로 특정 시료 검색")

### TC-P1-021: 이름 부분 일치 검색 [확정: 부분 일치 채택]
- **대상 기능**: 이름 검색 — 부분 문자열 매칭
- **Given**: `S1`(WaferA), `S2`(WaferB)가 등록되어 있음.
- **When**: 검색어 "Wafer"(부분 문자열)로 검색을 실행한다.
- **Then**: **`S1`, `S2` 모두 결과에 포함된다**(부분 일치/contains 확정).
- **참조**: abnormal.md 4번 항목(확정)

### TC-P1-022: 대소문자 구분 없음 [확정]
- **대상 기능**: 이름 검색 — 대소문자 민감도
- **Given**: `S1`(이름="WaferA")가 등록되어 있음.
- **When**: 검색어 "wafera"(소문자)로 검색한다.
- **Then**: **대소문자 구분 없이 매칭**되어 결과에 `S1`이 포함된다(확정).
- **참조**: abnormal.md 4번 항목(확정)

### TC-P1-023: 빈 검색어 입력 [확정: 재입력 요구]
- **대상 기능**: 이름 검색 — 빈 입력 경계
- **Given**: 임의의 시료가 등록되어 있음.
- **When**: 검색어로 빈 문자열("")을 입력한다.
- **Then**: **"검색어를 입력하세요" 류의 안내 메시지 표시 후 재입력을 요구한다**(검색을 실행하지 않고 전체 목록을 반환하지도 않는다). 크래시 없이 재입력 루프로 복귀해야 한다.
- **참조**: abnormal.md 6번 항목(확정), requirement.md 5.2절

### TC-P1-024: 존재하지 않는 이름으로 검색
- **대상 기능**: 이름 검색 — 결과 없음
- **Given**: `S1`(WaferA)만 등록되어 있음.
- **When**: 검색어 "NotExist"로 검색한다.
- **Then**: "검색 결과가 없습니다" 류의 메시지가 표시되고 빈 목록으로 처리된다(크래시 없음).
- **참조**: requirement.md 5.2절

### TC-P1-025: ID로는 검색되지 않음 [확정: 이름만 검색 대상]
- **대상 기능**: 이름 검색 — "등 속성"의 범위(ID 제외 확정)
- **Given**: `S1`(WaferA)가 등록되어 있음.
- **When**: 검색어로 "S1"(ID 값)을 입력한다.
- **Then**: **이름만 검색 대상이므로**, 이름에 "S1"을 포함한 시료가 없는 한 결과가 없다(빈 목록). `S1`은 결과에 포함되지 않는다.
- **참조**: abnormal.md 4번 항목(확정), requirement.md 5.1절(메뉴 표: "이름 검색 기능")

---

## D. 콘솔 메뉴 통합 (`MainController::handleSampleManagement`)

### TC-P1-030: 시료 관리 하위 메뉴 진입/복귀
- **대상 기능**: 메인 메뉴 [1] 시료 관리 진입 시 하위 메뉴(등록/조회/검색/뒤로가기) 표시
- **Given**: 프로그램이 정상 기동된 상태(메인 메뉴 표시 중).
- **When**: "1"을 입력해 시료 관리 메뉴로 진입한 뒤, 하위 메뉴에서 "뒤로가기"(또는 "0")를 선택한다.
- **Then**: 등록/조회/검색을 선택할 수 있는 하위 메뉴가 표시되고, 뒤로가기 선택 시 크래시 없이 메인 메뉴로 복귀한다.
- **참조**: requirement.md 5.1절, 5.2절

### TC-P1-031: 하위 메뉴에서 잘못된 번호 입력
- **대상 기능**: 시료 관리 하위 메뉴 — 잘못된 입력 처리
- **Given**: 시료 관리 하위 메뉴가 표시된 상태.
- **When**: 존재하지 않는 메뉴 번호(예: "9")를 입력한다.
- **Then**: "잘못된 입력입니다" 류의 메시지 표시 후 하위 메뉴 재표시(크래시 없음). design.md §8의 EOF 처리 공통 원칙과 마찬가지로 잘못된 입력에 대한 일관된 처리가 필요.
- **참조**: design.md §8, `MainController.cpp`의 기존 최상위 메뉴 잘못된 입력 처리 패턴과 동일하게 구현되어야 함(회귀 시 코드 스타일 일치 여부 확인)

### TC-P1-032: 등록 → 조회 → 검색 전체 플로우 (통합 Happy Path)
- **대상 기능**: 시료 관리 3개 기능 연쇄 확인
- **Given**: 저장소가 비어 있음.
- **When**: (1) 이름="WaferA", 평균생산시간=2.0, 수율=0.9로 등록(ID 자동 채번 `S1`) → (2) 조회 메뉴 실행 → (3) 검색어 "Wafer"로 검색.
- **Then**: (2)에서 방금 등록한 시료(`S1`, stock=0)가 목록에 나타나고, (3)에서도 동일 시료가 검색 결과에 포함된다. 재시작 없이 등록 직후 즉시 조회/검색에 반영되어야 한다(Repository가 메모리 캐시 + 즉시 영속화 방식이므로 별도 새로고침 불필요, `JsonSampleRepository::save` 계약과 일치).
- **참조**: requirement.md 5.2절, design.md §4.2(Repository는 변경 시 즉시 영속화)

### TC-P1-033: 재시작 후 데이터 영속성 확인 (Phase 0 계승 회귀)
- **대상 기능**: 시료 등록 데이터의 파일 영속성
- **Given**: 시료를 하나 이상 등록한 뒤 프로그램을 종료한다.
- **When**: 프로그램을 재시작한다.
- **Then**: `data/store.json`에 저장된 시료가 재시작 후에도 조회 메뉴에서 그대로 나타난다(Phase 0에서 이식된 `JsonFileStore`/`JsonSampleRepository`의 영속성 계약을 Phase 1 메뉴 레벨에서 재확인하는 회귀 케이스).
- **참조**: requirement.md 7장(데이터 영속성 정의), design.md §3~5

---

## E. Repository/Factory 계층 유닛 테스트 커버리지 매핑

`Tests/SampleFactoryTest.cpp`, `Tests/SampleServiceTest.cpp`(gmock, `MockSampleRepository : public ISampleRepository`)로 아래와 같이 작성했다(실제 파일: `SampleOrderSystem/Tests/Mocks/MockSampleRepository.h`, `SampleOrderSystem/Tests/SampleFactoryTest.cpp`, `SampleOrderSystem/Tests/SampleServiceTest.cpp`).

| TestCase | 유닛 테스트 대상 | Mock 설정 포인트 |
|---|---|---|
| TC-P1-001, TC-P1-002b | `SampleFactory::create` 성공 경로 + ID 자동 채번 | `MockSampleRepository::findAll` → 기존 시료 목록(빈 벡터 / `S1~S3`)을 스텁해 다음 ID(`S1`/`S4`) 계산 검증 |
| TC-P1-003 | `SampleFactory` 이름 필수 검증 | Repository 호출 불필요, 순수 값 검증 |
| TC-P1-004 | `SampleFactory` 평균 생산시간 `>0` 검증 | 순수 값 검증 |
| TC-P1-005, TC-P1-006, TC-P1-007 | `SampleFactory` 수율 범위 `(0, 1]` 검증 | 순수 값 검증(경계값 0.0/1.0/-0.1/1.5) |
| TC-P1-009 | `SampleService::registerSample` 이름 중복 허용 | `findAll` 두 번째 호출 시 이미 `WaferA`(S1)가 있어도 `save` 호출됨(`EXPECT_CALL(..., save(_)).Times(1)`)을 검증 |
| TC-P1-010~012 | `SampleService::listAll` | `findAll` 반환값을 빈 벡터/N개 벡터(재고 포함)로 스텁 |
| TC-P1-020~025 | `SampleService::search` | `findAll` 스텁 후 필터링 로직(부분 일치/대소문자 무시/이름만 대상/ID 미포함/빈 검색어 예외)만 검증 |

시스템 테스트(`/system-test`) 이관 후보는 TC-P1-001(정상 등록), TC-P1-010(빈 목록 조회), TC-P1-020(정상 검색), TC-P1-032(통합 플로우)다. 다만 **실제 메뉴 문구/입력 순서는 아직 Develope 구현 전이라 정확한 `Expect` 문자열을 특정할 수 없어, 이번 단계에서도 `.claude/skills/system-test/run.ps1`의 `$cases` 반영은 보류한다**(이전 단계에서 이미 이렇게 판단했고, 이번 확정 작업에서도 동일하게 유지). Develope가 Phase 1 메뉴 구현을 완료한 뒤, tester가 실제 출력 문구를 확인해 `$cases`에 추가하는 후속 작업이 필요하다.

---

## 다음 단계 (Develope 인계)

1. `SampleFactory::create`, `SampleService::registerSample`/`listAll`/`search`의 실제 시그니처를 구현하고, `Tests/*.cpp`가 그 시그니처와 맞는지 확인한다(다르면 tester가 회귀 단계에서 갱신).
2. `MainController::handleSampleManagement()`를 등록/조회/검색 하위 메뉴로 구현한다(TC-P1-030~033).
3. Debug 구성으로 `Tests/SampleFactoryTest.cpp`, `Tests/SampleServiceTest.cpp`를 빌드/실행해 통과 여부를 확인한다. 새 `Tests/*.cpp`, `Tests/Mocks/*.h` 파일을 `SampleOrderSystem.vcxproj`/`.vcxproj.filters`에 포함해야 컴파일 대상에 잡힌다(현재 vcxproj에는 미포함 상태 — Test 역할은 vcxproj를 직접 수정하지 않았다).
4. 구현 완료 후 tester가 실제 메뉴 문구를 확인해 `.claude/skills/system-test/run.ps1`의 `$cases`에 TC-P1-001/010/020/032 대응 콘솔 재현 케이스를 추가한다.
