# Phase 1 테스트케이스 — 시료 관리 (등록/조회/검색)

범위: `docs/PLAN.md` Phase 1 = `SampleService` + `SampleFactory` + 시료 관리 메뉴(`MainController::handleSampleManagement`).
관련 요구사항: `docs/requirement.md` 5.2절(시료 관리), 5.1절(메인 메뉴 표), 3장(시스템 개요 - 시료 개념).
관련 설계: `docs/design.md` §3(패키지 구조 - `Domain/Factory/SampleFactory`, `Service/SampleService`), §4.4(Factory 패턴 상세), §11.1(유닛 테스트 대상).

## 0. Phase 0 이식 코드 확인 결과 (실제 클래스/필드명)

- `Model/Sample.h`: `struct Sample { std::string id; std::string name; double avgProductionTime = 0.0; double yield = 1.0; int stock = 0; };`
- `Repository/ISampleRepository.h`: `findById(id) -> optional<Sample>`, `findAll() -> vector<Sample>`, `save(sample)`(upsert, 즉시 영속화).
- `Repository/JsonSampleRepository`: Phase 0에서 이미 구현 완료(메모리 캐시 + `JsonFileStore` 통한 영속화). Phase 1에서는 이 위에 `SampleFactory`(생성 검증)와 `SampleService`(등록/조회/검색 오케스트레이션), `MainController::handleSampleManagement` 하위 메뉴만 추가하면 된다.
- `Domain/Factory/SampleFactory`, `Service/SampleService`는 아직 미구현(Phase 1에서 Develope가 신규 작성) — design.md §3, §4.4 기준으로 시그니처를 가정해 테스트케이스를 작성했다. 실제 시그니처가 다르면 회귀 단계에서 이 문서를 갱신한다.
- `MainController::handleSampleManagement()`는 현재 TODO 스텁(`"[시료 관리] 기능은 Phase 1에서 구현됩니다."` 출력)만 존재 — Phase 1에서 등록/조회/검색 하위 메뉴로 채워야 한다.

**주의**: 아래 케이스 중 시료 ID를 사용자가 직접 입력하는 것으로 가정한 부분과, 시스템이 자동 채번(`S{n}`)한다고 가정한 부분이 섞여 있다. 이는 requirement.md 5.2절과 design.md §8의 서술이 서로 모순되기 때문이며, 상세 내용은 [abnormal.md](abnormal.md) 1번 항목 참고. **각 케이스에 두 가지 해석을 모두 표기**했으니, Develope가 실제로 어느 쪽을 채택했는지에 따라 회귀 시 하나를 확정한다.

---

## A. 시료 등록 (`SampleFactory::create` + `SampleService::registerSample`)

### TC-P1-001: 정상 등록 (Happy Path)
- **대상 기능**: 신규 시료 등록
- **Given**: 저장소가 비어 있거나 임의의 시료들이 등록되어 있음. 사용자가 이름="WaferA", 평균생산시간=2.0, 수율=0.9를 입력.
  - (ID 자동 채번 해석) ID는 시스템이 자동 부여(`S1`).
  - (ID 사용자 입력 해석) 사용자가 ID="S1"도 함께 입력.
- **When**: 등록 메뉴에서 위 값을 입력해 등록을 실행한다.
- **Then**: `SampleService::registerSample`이 성공적으로 `Sample{id, name:"WaferA", avgProductionTime:2.0, yield:0.9, stock:0}`을 생성해 `ISampleRepository::save`를 호출한다. `stock`은 등록 입력값에 없으므로 0으로 초기화된다(abnormal.md 2번 참고). 등록 완료 메시지와 부여된 ID가 화면에 표시된다.
- **참조**: requirement.md 5.2절, design.md §4.4, §3(Model/Sample.h)

### TC-P1-002: 중복 ID 등록 거부
- **대상 기능**: `SampleFactory` 생성 시점 검증 — ID 유일성
- **Given**: 이미 ID="S1"인 시료가 등록되어 있음(`ISampleRepository::findById("S1")`이 값 반환).
- **When**: (ID 사용자 입력을 채택한 경우) 다시 ID="S1"로 등록을 시도한다.
- **Then**: `SampleFactory::create`가 예외/실패를 반환하고, 기존 `S1` 레코드는 변경되지 않는다(`save` 호출 자체가 발생하지 않아야 함). 화면에는 "이미 존재하는 시료 ID입니다" 류의 에러 메시지가 표시된다.
- **비고**: ID 자동 채번을 채택하면 사용자가 ID 충돌을 일으킬 방법이 없으므로 이 케이스는 자연히 발생하지 않는다 — 대신 Repository의 채번 로직(예: 마지막 `S{n}`의 `n`을 파싱해 +1) 자체의 단위 테스트로 대체한다(TC-P1-002b).
- **참조**: requirement.md 5.2절("고유한 이름과 속성"), design.md §4.4

### TC-P1-002b: (자동 채번 채택 시) ID 채번 로직 검증
- **대상 기능**: Repository의 `S{n}` 채번
- **Given**: 저장소에 `S1`, `S2`, `S3`이 등록되어 있음.
- **When**: 신규 시료를 등록한다.
- **Then**: 새 ID는 `S4`로 부여된다. 이후 `S3`을 삭제(또는 존재하지 않는 상태로 시딩)해도 다음 채번은 `S4`(최대값+1) 기준이지 "빈 자리 재사용"이 아니어야 한다(design.md §8 "ID 채번 방식의 한계" 언급과 연결 — 삭제 기능이 없는 한 이 케이스는 이론적 경계 케이스로만 기록).
- **참조**: design.md §8

### TC-P1-003: 이름 빈 문자열 등록 거부
- **대상 기능**: `SampleFactory` 검증 — 이름 필수
- **Given**: 이름 입력값이 빈 문자열("") 또는 공백만("   ")인 경우.
- **When**: 등록을 시도한다.
- **Then**: 등록이 거부되고 에러 메시지가 표시된다. 저장소에 아무 것도 추가되지 않는다.
- **참조**: requirement.md 5.2절("고유한 이름"), design.md §4.4

### TC-P1-004: 평균 생산시간 0 이하 등록 거부
- **대상 기능**: `SampleFactory` 검증 — 평균 생산시간 유효 범위(0 초과)
- **Given**: 평균 생산시간 입력값이 0, 또는 음수(-1.5)인 경우.
- **When**: 등록을 시도한다.
- **Then**: 두 경우 모두 등록이 거부된다("0 초과"가 조건이므로 0도 실패해야 함). 에러 메시지 표시, 저장소 변경 없음.
- **참조**: design.md §4.4("평균 생산시간... 0 초과")

### TC-P1-005: 수율 경계값 0.0 (0%) 등록
- **대상 기능**: `SampleFactory` 검증 — 수율 범위(design.md §4.4상 "0~1")
- **Given**: 수율 입력값 = 0.0.
- **When**: 등록을 시도한다.
- **Then**: design.md §4.4 문구("수율 0~1")만 보면 0.0도 유효 범위 내라 등록이 **허용되어야** 한다. 다만 수율 0%는 Phase 3의 실 생산량 공식 `ceil(부족분/yield)`에서 0으로 나누는 상황을 만들어 향후 생산 단계에서 문제가 된다 — 등록 시점에 0을 막을지는 requirement.md에 명시가 없어 판단이 불명확하다.
  - **가정 A(그대로 허용)**: 등록 성공, `yield=0.0`으로 저장. Phase 3 진입 전까지는 문제 없음.
  - **가정 B(0 초과로 제한)**: 등록 거부, "수율은 0보다 커야 합니다" 에러.
- **비고**: 이 모순은 [abnormal.md](abnormal.md) 3번 항목에 기록. Develope 구현 시 가정 A/B 중 택1하고 회귀 단계에서 확정한다.
- **참조**: design.md §4.4, requirement.md 5.6절(실 생산량 공식)

### TC-P1-006: 수율 경계값 1.0 (100%) 등록
- **대상 기능**: `SampleFactory` 검증 — 수율 상한
- **Given**: 수율 입력값 = 1.0.
- **When**: 등록을 시도한다.
- **Then**: 등록 성공(`yield=1.0`으로 저장). 100%는 "정상적인 생산만 존재"하는 정상적인 경계값이므로 허용되어야 한다.
- **참조**: requirement.md 5.2절(수율 정의: 정상 시료 수/총 생산 시료 수, 이론상 1.0까지 가능)

### TC-P1-007: 수율 범위 밖 값(음수, 1 초과) 등록 거부
- **대상 기능**: `SampleFactory` 검증 — 수율 범위
- **Given**: 수율 입력값 = -0.1, 또는 1.5.
- **When**: 등록을 시도한다.
- **Then**: 두 경우 모두 등록 거부. "수율은 0~1 사이여야 합니다" 에러 메시지. 저장소 변경 없음.
- **참조**: design.md §4.4

### TC-P1-008: 평균 생산시간/수율에 숫자가 아닌 값 입력
- **대상 기능**: 입력 파싱(View/Controller 레벨) 예외 처리
- **Given**: 평균 생산시간 입력값 = "abc" (숫자 파싱 불가).
- **When**: 등록을 시도한다.
- **Then**: 프로그램이 크래시하지 않고 "숫자를 입력하세요" 류의 에러 메시지를 표시한 뒤 등록 메뉴로 복귀(또는 재입력 요구)한다. 저장소 변경 없음.
- **참조**: design.md §8(EOF/예외 상황 공통 처리 원칙 연장선), requirement.md 5.2절

### TC-P1-009: 이름 중복 등록 허용 여부
- **대상 기능**: `SampleFactory` 검증 — 이름 유일성 미명시
- **Given**: 이미 이름="WaferA"인 시료(`S1`)가 등록되어 있음.
- **When**: 이름="WaferA"로 두 번째 시료를 등록한다(ID는 다름, 예: 자동 채번 `S2` 또는 평균생산시간/수율이 다른 변형 시료).
- **Then**: requirement.md/design.md 어디에도 "이름 유일성" 요구가 없으므로, **등록이 허용되어야 한다**고 가정하고 케이스를 작성했다(서로 다른 ID를 가진 동명 시료 2건 존재). 이후 이름 검색(TC-P1-021)에서 두 건 모두 조회되는지도 함께 확인한다.
- **참조**: [abnormal.md](abnormal.md) 5번 항목(이름 유일성 미명시)

---

## B. 시료 조회 (`SampleService::listAll` / 목록 조회 메뉴)

### TC-P1-010: 등록된 시료가 없을 때 조회
- **대상 기능**: 시료 목록 조회
- **Given**: 저장소가 완전히 비어 있음(`findAll()`이 빈 벡터 반환).
- **When**: 시료 조회 메뉴를 실행한다.
- **Then**: "등록된 시료가 없습니다" 류의 안내 메시지가 표시되고, 프로그램은 정상적으로 메뉴로 복귀한다(크래시/빈 화면 없음).
- **참조**: requirement.md 5.2절("시료 조회: 등록된 모든 시료 목록 확인")

### TC-P1-011: 다수 시료 등록 후 전체 목록 조회 (재고 포함)
- **대상 기능**: 시료 목록 조회 — 현재 재고 수량 포함 여부
- **Given**: `S1`(WaferA, stock=5), `S2`(WaferB, stock=0)가 등록되어 있음.
- **When**: 시료 조회 메뉴를 실행한다.
- **Then**: 두 시료 모두 목록에 표시되며, 각 행에 ID/이름/평균생산시간/수율/**현재 재고 수량**이 함께 표시된다(요구사항 5.2절 "현재 재고 수량 포함" 명시).
- **참조**: requirement.md 5.2절

### TC-P1-012: 재고 0인 시료도 목록에 포함되는지 확인
- **대상 기능**: 시료 목록 조회 — 재고 0 경계
- **Given**: `S2`(stock=0)만 등록되어 있음.
- **When**: 시료 조회를 실행한다.
- **Then**: `S2`가 "재고 0"으로 정상 표시된다(목록에서 제외되지 않음). 5.5절의 "고갈" 판정(모니터링 전용, Phase 5)과 혼동하지 않는다 — Phase 1 조회에서는 단순히 수치 0만 표시하면 된다.
- **참조**: requirement.md 5.2절, 5.5절(참고 구분용)

---

## C. 시료 검색 (`SampleService::search` / 이름 검색 메뉴)

### TC-P1-020: 이름으로 정확히 일치하는 시료 검색
- **대상 기능**: 이름 검색 — 존재하는 이름
- **Given**: `S1`(WaferA)가 등록되어 있음.
- **When**: 검색어 "WaferA"로 검색을 실행한다.
- **Then**: `S1`이 검색 결과에 포함되어 표시된다.
- **참조**: requirement.md 5.2절("이름 등 속성으로 특정 시료 검색")

### TC-P1-021: 이름 부분 일치 검색 (가정)
- **대상 기능**: 이름 검색 — 부분 문자열 매칭 여부(요구사항 미명시)
- **Given**: `S1`(WaferA), `S2`(WaferB)가 등록되어 있음.
- **When**: 검색어 "Wafer"(부분 문자열)로 검색을 실행한다.
- **Then**: requirement.md는 "이름 등 속성으로 특정 시료 검색"이라고만 되어 있어 정확 일치/부분 일치 여부가 불명확하다.
  - **가정 A(부분 일치, contains)**: `S1`, `S2` 모두 결과에 포함.
  - **가정 B(정확 일치)**: 결과 없음(빈 목록).
- **비고**: 이 모호성은 [abnormal.md](abnormal.md) 4번 항목에 기록. 사용자 경험상 부분 일치(가정 A)가 더 합리적이라고 판단되나, 임의로 확정하지 않고 이슈로 남긴다.
- **참조**: requirement.md 5.2절

### TC-P1-022: 대소문자 구분 여부
- **대상 기능**: 이름 검색 — 대소문자 민감도
- **Given**: `S1`(이름="WaferA")가 등록되어 있음.
- **When**: 검색어 "wafera"(소문자)로 검색한다.
- **Then**: 명세에 대소문자 처리 방침이 없다. 가정: 사용자 편의상 **대소문자 구분 없이** 매칭(가정 A) — 결과에 `S1` 포함. 대소문자 구분 채택(가정 B) 시 결과 없음. Develope 구현 확정 후 이 케이스를 갱신한다.
- **참조**: [abnormal.md](abnormal.md) 4번 항목

### TC-P1-023: 빈 검색어 입력
- **대상 기능**: 이름 검색 — 빈 입력 경계
- **Given**: 임의의 시료가 등록되어 있음.
- **When**: 검색어로 빈 문자열("")을 입력한다.
- **Then**: 요구사항에 명시 없음. 두 가지 합리적 동작 중 하나를 기대:
  - **가정 A**: 빈 검색어는 "검색어를 입력하세요" 안내 후 재입력 요구(검색 실행 안 함).
  - **가정 B**: 빈 문자열은 모든 이름에 포함되므로 전체 목록을 반환.
- **비고**: 크래시 없이 둘 중 하나로 일관되게 동작하기만 하면 통과로 간주. 어느 쪽인지는 [abnormal.md](abnormal.md)에 기록.
- **참조**: requirement.md 5.2절

### TC-P1-024: 존재하지 않는 이름으로 검색
- **대상 기능**: 이름 검색 — 결과 없음
- **Given**: `S1`(WaferA)만 등록되어 있음.
- **When**: 검색어 "NotExist"로 검색한다.
- **Then**: "검색 결과가 없습니다" 류의 메시지가 표시되고 빈 목록으로 처리된다(크래시 없음).
- **참조**: requirement.md 5.2절

### TC-P1-025: ID로 검색 가능한지 여부("이름 등 속성")
- **대상 기능**: 이름 검색 — "등 속성"의 범위(ID 포함 여부)
- **Given**: `S1`(WaferA)가 등록되어 있음.
- **When**: 검색어로 "S1"(ID 값)을 입력한다.
- **Then**: 요구사항 문구가 "이름 **등** 속성으로 검색"이라 이름 외 속성(ID 등)도 검색 대상에 포함될 수 있음을 시사하지만, Phase 1 메뉴 명세(5.1절 표)는 "이름 검색"으로만 한정한다.
  - **가정 A(이름만 검색 대상)**: ID "S1"로 검색 시 이름이 "S1"인 시료가 없으므로 결과 없음.
  - **가정 B(ID도 검색 대상에 포함)**: `S1` 레코드가 결과에 포함.
- **비고**: [abnormal.md](abnormal.md) 4번 항목에 기록. 최소 스코프인 가정 A로 우선 구현하고, 필요 시 확장하는 방향을 제안.
- **참조**: requirement.md 5.1절(메뉴 표: "이름 검색 기능"), 5.2절("이름 등 속성으로 특정 시료 검색")

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
- **When**: (1) 이름="WaferA", 평균생산시간=2.0, 수율=0.9로 등록 → (2) 조회 메뉴 실행 → (3) 검색어 "Wafer"로 검색.
- **Then**: (2)에서 방금 등록한 시료가 목록에 나타나고, (3)에서도 동일 시료가 검색 결과에 포함된다. 재시작 없이 등록 직후 즉시 조회/검색에 반영되어야 한다(Repository가 메모리 캐시 + 즉시 영속화 방식이므로 별도 새로고침 불필요, `JsonSampleRepository::save` 계약과 일치).
- **참조**: requirement.md 5.2절, design.md §4.2(Repository는 변경 시 즉시 영속화)

### TC-P1-033: 재시작 후 데이터 영속성 확인 (Phase 0 계승 회귀)
- **대상 기능**: 시료 등록 데이터의 파일 영속성
- **Given**: 시료를 하나 이상 등록한 뒤 프로그램을 종료한다.
- **When**: 프로그램을 재시작한다.
- **Then**: `data/store.json`에 저장된 시료가 재시작 후에도 조회 메뉴에서 그대로 나타난다(Phase 0에서 이식된 `JsonFileStore`/`JsonSampleRepository`의 영속성 계약을 Phase 1 메뉴 레벨에서 재확인하는 회귀 케이스).
- **참조**: requirement.md 7장(데이터 영속성 정의), design.md §3~5

---

## E. Repository/Factory 계층 유닛 테스트 커버리지 매핑 (참고용, 코드는 후속 단계에서 작성)

이번 단계는 문서화에 집중하되, 추후 `Tests/SampleServiceTest.cpp`(gmock, `MockSampleRepository : public ISampleRepository`)로 옮길 때 아래처럼 매핑할 것을 제안한다.

| TestCase | 유닛 테스트 대상 | Mock 설정 포인트 |
|---|---|---|
| TC-P1-001 | `SampleFactory::create` 성공 경로 | `MockSampleRepository::findById` → `nullopt` 기대 |
| TC-P1-002 | `SampleFactory::create` 중복 ID 실패 | `findById` → 기존 `Sample` 반환하도록 스텁, `save` 호출 안 됨(`EXPECT_CALL(..., save(_)).Times(0)`) 검증 |
| TC-P1-003~007 | `SampleFactory` 필드 검증(이름/시간/수율) | Repository 호출 자체가 필요 없는 순수 값 검증이면 Mock 불필요, Factory 단위 테스트로 충분 |
| TC-P1-010~012 | `SampleService::listAll` | `findAll` 반환값을 빈 벡터/N개 벡터로 스텁 |
| TC-P1-020~025 | `SampleService::search` | `findAll` 스텁 후 필터링 로직만 검증(Repository는 필터링 책임 없음 가정) |

시스템 테스트(`/system-test`) 이관 후보: TC-P1-001(정상 등록), TC-P1-010(빈 목록 조회), TC-P1-020(정상 검색), TC-P1-032(통합 플로우) — 콘솔 입출력으로 그대로 재현 가능. 단, Develope가 실제 메뉴 문구/입력 순서를 확정한 뒤 `run.ps1`의 `$cases`에 반영해야 정확한 `Expect` 문자열을 쓸 수 있으므로, 이번 "Raw" 단계에서는 `run.ps1` 수정은 보류한다.

---

## 다음 단계(사용자 검토 후)

1. 이 문서의 "가정 A/B" 분기들 중 어느 쪽을 채택할지 사용자 확인.
2. 확인된 사항을 반영해 이 문서를 확정본으로 갱신.
3. `Tests/SampleFactoryTest.cpp`, `Tests/SampleServiceTest.cpp`(gmock) 작성.
4. `.claude/skills/system-test/run.ps1`의 `$cases`에 콘솔 재현 케이스 추가.
