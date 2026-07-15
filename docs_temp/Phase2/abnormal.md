# Phase 2 특이사항 기록 (요구사항/설계 모순, 모호성)

Test 에이전트는 Edit/Bash로 프로덕션 코드나 PRD/요구사항 문서를 수정하지 않고 아래 항목을 기록만 한다.
[testcase.md](testcase.md)에서 참조하는 판단 근거를 여기 모았다. Phase 1(`docs_temp/Phase1/abnormal.md`)과 동일한 형식을 따른다.

## 1. `MainController`의 메뉴 번호 라우팅이 확정된 6개 메뉴 설계와 어긋남 (High)

- `docs/design.md` §7 "메뉴 구성" 표와 `docs/screens.md` "메인 메뉴" 예시는 이미 다음과 같이 6개 메뉴를 확정해 두었다.

  | 메뉴 번호 | 메뉴 | Phase |
  |---|---|---|
  | [1] | 시료 관리 | Phase 1 |
  | [2] | 시료 주문 | Phase 2 |
  | [3] | 주문 승인/거절 | Phase 3 |
  | [4] | 모니터링 | Phase 5 |
  | [5] | 생산라인 조회 | Phase 3 |
  | [6] | 출고 처리 | Phase 4 |

- 하지만 실제 `SampleOrderSystem/Controller/MainController.cpp`의 `run()` 분기(Phase 0에서 이식된 스캐폴딩)는 다음과 같다.

  ```cpp
  if (command == "1") { handleSampleManagement(); }
  else if (command == "2") { handleOrder(); }         // "[주문 (접수/승인/거절)] 기능은 Phase 2~3에서 구현됩니다."
  else if (command == "3") { handleMonitoring(); }     // 설계상 [3]은 "주문 승인/거절"이어야 함
  else if (command == "4") { handleRelease(); }        // 설계상 [4]는 "모니터링"이어야 함
  else if (command == "5") { handleProductionLine(); } // 이것만 설계와 일치([5] 생산라인 조회)
  // "6"(출고 처리)에 대응하는 분기 자체가 없음
  ```

- 즉 `"2"`가 "시료 주문(접수)"과 "주문 승인/거절"을 하나의 TODO(`handleOrder`)로 묶어놓았고, 이후 번호(`"3"`~`"5"`)가 한 칸씩 밀려 있으며 `"6"`(출고 처리) 분기 자체가 없다. `docs/design.md` §7 표 자체도 마지막에 "생산 라인 | ProductionService, MonitorService | Phase 3"이 중복 행으로 한 번 더 들어가 있어(표 서식 오류로 추정) 이 불일치를 문서 리뷰만으로는 바로 알아채기 어려웠다.
- 이 스캐폴딩은 design.md §7/screens.md의 6메뉴 구조가 최종 확정(커밋 `35c0cd9` "출고 처리 화면 추가, ID 포맷을 화면 예시 기준으로 갱신")되기 이전, Phase 0 시점에 임시로 작성된 것으로 추정된다.

### 확정 필요 사항 (Develope 판단 요청)
- Phase 2에서 `handleOrder()`를 "시료 주문(접수)" 전용으로 좁히고, 번호를 design.md §7/screens.md 기준(`[2] 시료 주문`)에 맞출지, 아니면 현재 `"2"` 분기를 그대로 유지하되 내부에서 접수/승인 서브메뉴로 나눌지 결정이 필요하다.
- 어느 쪽을 택하든 `"3"`(모니터링 자리에 있던 분기)~`"6"`(출고, 현재 없음) 전체 번호 재정렬이 함께 필요하므로, Phase 2 범위를 넘어 Phase 3~5까지 영향을 미치는 결정이다. Phase 2 시점에 먼저 정리해 두면 이후 Phase에서 다시 번호를 바꾸는 手戻り(rework)를 피할 수 있다.
- **우선순위: High** — Phase 2 콘솔 메뉴 구현(TC-P2-050~052) 자체가 이 결정에 의존한다.

## 2. `Order`의 상태 표현 방식 — design.md §4.1(State 패턴)과 이미 이식된 Model 구조의 불일치 (Medium)

- `docs/design.md` §4.1: "`Order`는 현재 상태를 non-owning 참조/포인터(`IOrderState*`)로만 가리키고, `Order::status()`는 `state_->statusCode()`를 그대로 리턴 — JSON 직렬화 시에는 `statusCode()`만 문자열로 저장."
- 그러나 Phase 0에서 이미 이식·확정된 `SampleOrderSystem/Model/Order.h`는 `IOrderState` 포인터가 전혀 없고, 순수 `enum class OrderStatus status = OrderStatus::RESERVED;` 필드 하나만 갖는다(`to_json`/`from_json`도 이 enum을 문자열로 직렬화). 즉 이미 존재하는 Model 코드는 design.md §4.1이 서술한 "Order가 IOrderState를 참조"하는 구조가 애초에 아니다.
- 이 자체는 오히려 design.md §3의 "Model = 데이터+JSON 매핑만, 로직 없음" 원칙에는 더 부합한다(포인터 기반 State 패턴을 Model에 두면 Model이 Domain 계층 타입을 알아야 해서 계층 의존 방향이 꼬일 수 있음). 다만 design.md §4.1 코드 스니펫과 실제 코드가 명백히 다르므로, Phase 3에서 `IOrderState::approve()`/`reject()`/`completeProduction()`/`release()`를 구현할 때 "이 메서드들이 `Order&`를 받아 **직접 `order.status = ...`를 대입**하는 방식"으로 재해석해 구현할 것으로 추정되지만, 이는 design.md §4.1을 문자 그대로 구현할 수 없다는 뜻이므로 확인이 필요하다.
- Phase 2 자체 구현에는 지장이 없다 — `OrderFactory::create`는 단순히 `order.status = OrderStatus::RESERVED`로 초기화하면 되고, "ReservedState 초기 상태"라는 Phase 2 산출물 요구는 `IOrderState`/`ReservedState` 클래스 골격을 실제로 만들지 않고도 enum 값 하나로 충족 가능하다.

### 확정 필요 사항 (Develope/Phase 3 착수 전 판단 요청)
- (a) `IOrderState` 계층을 실제로 도입하되 `Order`에 `IOrderState*` 필드를 추가하도록 `Model/Order.h`를 확장(design.md §4.1 원안 그대로), (b) `IOrderState`는 "행동만 담은 정적 유틸(싱글턴)"로 두고 `Order.status`(enum)는 그대로 유지한 채 각 State의 메서드가 `Order&`의 enum 필드를 직접 갱신하는 방식(design.md 코드는 개념적 설명으로 취급) 중 하나를 Phase 3 착수 전 확정해야 한다.
- **우선순위: Medium** — Phase 2 구현을 막지는 않지만, Phase 3(`docs/PLAN.md` 표 기준 "IOrderState 전체 구현")의 설계 방향을 좌우하므로 Phase 2 완료 시점에는 확정돼 있는 것이 좋다.

## 3. 주문 ID(`ORD-{YYYYMMDD}-{HHMMSS}`)의 초 단위 정밀도와 유일성 보장 부재 (Medium)

- `docs/design.md` §5는 주문 ID 포맷을 `ORD-{YYYYMMDD}-{HHMMSS}`(초 단위까지만)로 확정했다. 시료 ID(`S-{3자리 순번}`)와 달리 순번이 아니라 시각 그 자체를 키로 쓰므로, **같은 초 안에 두 번째 주문이 접수되면 두 주문의 ID가 완전히 동일해진다.**
- `Repository/JsonOrderRepository::save`는 `findById`로 동일 `id`를 찾아 있으면 **덮어쓰기(upsert)** 방식이므로, ID가 충돌하면 먼저 접수된 주문이 통째로 사라지고 나중 주문으로 대체될 위험이 있다(요구사항 어디에도 "동일 초 내 중복 접수 금지" 같은 제약은 없어 이 상황 자체를 막을 방법이 없다).
- 콘솔에서 사람이 직접 입력하는 정상 사용 흐름에서는 1초 내에 두 번의 예약을 완료하기 어려워 실무 발생 확률은 낮지만, (a) 자동화된 시스템 테스트(`/system-test`가 입력을 매우 빠르게 흘려보내는 경우)나 (b) 미래에 배치/스크립트성 주문 접수가 추가되는 경우 실제로 재현될 수 있다.

### 확정 필요 사항
- ID에 밀리초 또는 순번 접미사를 추가해 유일성을 강제할지, 아니면 "동일 초 내 재접수는 발생 확률이 낮으므로 감수한다"고 명시적으로 결정할지 사용자/Develope 판단이 필요하다.
- **우선순위: Medium** — 정상 콘솔 사용 흐름에서는 드물지만, 데이터 유실(주문 통째로 사라짐)이라는 결과가 심각하므로 High에 준하는 주의가 필요하다고 판단해 기록한다. 이 문서(testcase.md TC-P2-041)에서는 이 케이스를 "회귀 실패로 자동 처리하지 않는 정보성 테스트"로만 남겨두었다.

## 4. 빈 시료 ID 입력 시 에러 메시지 구분 여부 (Low)

- 시료 ID가 빈 문자열일 때 "시료 ID를 입력하세요"(빈 입력 자체를 지적)와 "등록되지 않은 시료입니다"(존재 검증 실패로 취급) 중 어느 메시지를 보여줄지 요구사항에 명시가 없다.
- 동작 자체(예약 거부)는 어느 쪽이든 동일하므로 회귀에 영향을 주지 않는다. 문구 선택은 Develope 재량으로 남긴다.
- **우선순위: Low**

## 5. 예약 확인 화면에서 `[N]`(취소) 선택 시 동작 (Low)

- `docs/screens.md` "[2] 시료 주문" 예시는 `[Y] 예약 접수` 선택 흐름만 보여주고 `[N]`(취소) 선택 시 어떤 화면/메시지가 나오는지는 예시에 없다.
- 상식적으로 "저장 없이 메뉴로 복귀"가 맞을 것으로 추정하고 TC-P2-051에 반영했으나, 별도 확인 메시지 유무 등 세부 문구는 확정된 바 없다.
- **우선순위: Low**

---

## 우선순위 요약

1. **(High) 1번** — `MainController` 메뉴 번호 라우팅이 확정된 6메뉴 설계(design.md §7/screens.md)와 어긋남. Phase 2 콘솔 메뉴 구현 자체가 이 결정에 의존하므로 Develope 착수 전 우선 확인 필요.
2. **(Medium) 3번** — 주문 ID 초 단위 충돌 시 데이터 유실 위험. 발생 확률은 낮지만 결과가 심각.
3. **(Medium) 2번** — `Order` 상태 표현 방식(design.md §4.1 vs 실제 Model 코드) 불일치. Phase 2 구현은 막지 않으나 Phase 3 착수 전 확정 필요.
4. **(Low) 4번, 5번** — 에러 메시지 문구/취소 동작 세부사항. 회귀에 영향 없음, Develope 재량.
