# Phase 7 — 출고 처리

## 관련 문서
- `docs/All_phase_goals.md` Phase 7
- `docs/PRD.md` 4.7(출고 처리), 4.6.1(재고 확인/갱신 시점)

## 목표
`CONFIRMED` 상태 주문에 대해 전량 출고를 처리한다. 출고 시점에 재고를 출고 수량만큼 실제로 감소시키고
주문 상태를 `CONFIRMED -> RELEASED`로 전환한다. 부분 출고는 지원하지 않는다(항상 주문 수량 전체).

## 설계 결정

1. **재고 차감 방법**: `Controller/SampleController`에 기존 `IncreaseStock`(Phase 6, 생산 완료 시 사용)과
   대칭되는 `DecreaseStock(sampleId, decreaseAmount)`를 추가한다. 내부적으로 `sampleRepository_.FindById`로
   조회 후 `SetStock(현재재고 - decreaseAmount)`로 갱신하고 `Update`로 write-through 한다. 시료가 없으면
   false를 반환한다(방어적 처리, OrderApprovalController의 SampleNotFound 처리와 동일한 패턴).
   - 재고가 이미 출고 수량 이상임은 CONFIRMED 상태가 되는 시점(승인 시 충분 판정 또는 생산 완료 시 증가)에
     보장되므로 별도의 "재고 부족으로 출고 불가" 분기는 PRD상 요구되지 않는다. 다만 방어적으로 음수 재고가
     되지 않도록 구현 시 재고 부족 케이스도 고려한다(도메인상 정상 흐름에서는 발생하지 않지만, 데이터가
     외부에서 조작된 경우까지 대비).

2. **ShipmentController**: `OrderApprovalController`와 동일한 구조로 구성한다.
   - `GetShippableOrders() const`: `CONFIRMED` 상태 주문만 필터링해 반환(PRD 4.7 — 출고 가능 목록은 CONFIRMED로 제한).
   - `ShipOrder(orderId)`: 주문 조회 → `CONFIRMED` 상태 검증(`TryTransitionTo(Released)`로 상태 전이 테이블에
     위임) → 성공 시 `SampleController::DecreaseStock(sampleId, quantity)` 호출(quantity는 항상 주문 수량 전체,
     별도 입력 없음 — 부분 출고 미지원) → `orderRepository_.Update(order)`.
   - 결과 enum: `ShipmentResult { Success, OrderNotFound, InvalidOrderState, SampleNotFound }`.
     `SampleNotFound`는 주문이 참조하는 시료가 더 이상 존재하지 않는 방어적 케이스(OrderApprovalController와
     동일한 방어 패턴).
   - 상태 검증 순서: 주문 존재 확인 → 시료 존재 확인 → 상태 전이(TryTransitionTo) 순으로 하여, 상태가
     CONFIRMED가 아니면 재고 차감 이전에 걸러지도록 한다(재고 차감 후 상태 전이 실패로 롤백해야 하는 상황을
     피하기 위해 전이 검증을 먼저 수행).

3. **ShipmentView**: `OrderApprovalView`와 동일한 메뉴 구조(목록 조회 / 처리 / 뒤로)를 따른다.
   - 메뉴: `[1] 출고 가능 목록   [2] 출고 처리   [0] 뒤로`
   - 목록/처리 결과 출력은 `ConsoleView::PrintOrderTableHeader/PrintOrderRow`, `PrintLine/PrintError`를 재사용.

4. **main.cpp 배선**: SampleController, orderRepository 조립 이후에 ShipmentController/View를 생성해 메뉴에
   추가한다(임시 진입점이며 Phase 9에서 MainMenuController로 대체 예정, 기존 주석 패턴 유지).

## 산출물 파일 목록
- `src/Controller/ShipmentController.h` / `.cpp` (신규)
- `src/View/ShipmentView.h` / `.cpp` (신규)
- `src/Controller/SampleController.h` / `.cpp` (수정 — `DecreaseStock` 추가)
- `src/main.cpp` (수정 — ShipmentController/View 배선)
- `SampleOrderSystem.vcxproj` (수정 — 신규 파일 ClCompile/ClInclude 등록)

## 완료 조건과의 매핑
- "출고 처리 시 재고가 출고 수량만큼 감소하고 주문이 RELEASED로 전환" -> `ShipOrder`가 `DecreaseStock` +
  `TryTransitionTo(Released)` + `orderRepository_.Update`를 원자적 순서로 수행하도록 구현.
- "CONFIRMED가 아닌 주문은 출고 목록/처리 대상에서 제외" -> `GetShippableOrders`가 CONFIRMED만 필터링하고,
  `ShipOrder`는 `TryTransitionTo(Released)`가 CONFIRMED 이외 상태에서 false를 반환하는 상태 전이 테이블
  (`Model::IsValidOrderStatusTransition`)에 의해 자동으로 차단됨.

## 재작업 이력
(초기 구현, 아직 unit-tester 피드백 없음)
