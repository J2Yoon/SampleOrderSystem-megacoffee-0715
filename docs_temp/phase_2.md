# Phase 2 — 데이터 영속성 (JSON 파일 + Repository)

## 관련 문서
- `docs/All_phase_goals.md` Phase 2 (데이터 영속성)
- `docs/PRD.md` 1.3(영속성 요구), 5.4(JSON 파일 스키마), 6(비기능 요구사항 — 데이터 영속성)
- `CLAUDE.md` 아키텍처 절(Persistence 계층), "PoC별 참고 포인트"(DataPersistence 행), "데이터 파일 스키마" 절

## doc-verifier가 발견한 문서 공백에 대한 결정 (이번 Phase에서 확정, 근거 포함)

### 결정 1 — 시각 필드(chrono) 직렬화 방식: epoch 밀리초 정수

`Order::createdAt_`, `ProductionQueueItem::productionStartTime_`/`expectedCompletionTime_`는 모두
`std::chrono::system_clock::time_point`이다. 이를 JSON에 저장할 때 아래 이유로 **epoch 이후 경과 밀리초
(정수, `long long`)** 를 선택한다.

- **정확한 왕복(round-trip) 보존**: `time_point.time_since_epoch()`를
  `std::chrono::duration_cast<std::chrono::milliseconds>`로 변환한 정수 값은 손실 없이 다시
  `std::chrono::system_clock::time_point(std::chrono::milliseconds(value))`로 복원 가능하다. 이는 Phase 6의
  "재시작 시점 현재 시각 기준 정산" 로직이 저장된 시각과 `std::chrono::system_clock::now()`를 직접 뺄셈
  비교해야 하므로 정확도가 핵심이다.
- **자체 구현 파서의 복잡도 최소화**: ISO-8601 문자열(`2026-07-15T10:23:00.123Z` 등)을 지원하려면 자체 JSON
  파서와 별개로 날짜/시간 파싱기(윤년, 타임존 오프셋, 밀리초 자릿수 가변 등)를 새로 작성해야 한다. 이
  저장소는 외부 라이브러리 없이 JSON조차 직접 구현하는 상황이므로, 불필요한 파싱 복잡도와 버그 위험을
  늘리는 ISO-8601 대신 정수 파싱만으로 끝나는 epoch 방식을 택한다.
- **PoC 스키마와 상충하지 않음**: `data/orders.json`의 PoC 호환 필드(`orderId`/`sampleId`/`customerName`/
  `quantity`/`status`)는 그대로 유지하고, 시각은 `createdAtEpochMillis`라는 **추가 필드**로만 얹는다
  (`CLAUDE.md`: "기존 필드는 유지한 채 추가만 한다"). `data/productionQueue.json`은 PoC 호환 대상이 아니므로
  전량 이 방식으로 필드를 설계한다(결정 2 참고).

### 결정 2 — `ProductionQueueItem` 저장 파일명/스키마: `data/productionQueue.json` (PoC 비호환, 자유 설계)

`docs/PRD.md`/`CLAUDE.md` 어디에도 생산 큐 저장 파일명이 정의되어 있지 않다. 5.4절은 "생산 큐 등 PRD 전용
데이터를 위한 추가 파일/필드가 필요하면 기존 필드는 유지한 채 확장한다"고만 명시하므로, 이 파일은
`DataPersistence`/`DataMonitor`/`DummyDataGenerator`가 공유하는 PoC 호환 스키마의 대상이 **아니며** 자유롭게
설계할 수 있다.

- **파일명**: `data/productionQueue.json` (배열, 각 원소가 하나의 생산 큐 항목)
- **필드**:
  | 필드명 | 대응 모델 필드 | 타입 |
  |---|---|---|
  | `orderId` | 연결 주문번호 | string (고유 키) |
  | `shortageQuantity` | 부족분 | number(정수) |
  | `actualProductionQuantity` | 실 생산량 | number(정수) |
  | `totalProductionMinutes` | 총 생산 시간(분) | number(실수) |
  | `productionStartEpochMillis` | 생산 시작 시각 | number(정수, epoch ms) |
  | `expectedCompletionEpochMillis` | 예상 완료 시각 | number(정수, epoch ms, **파생값·참고용**) |
- `expectedCompletionEpochMillis`는 `productionStartEpochMillis + totalProductionMinutes`로부터 항상
  재계산 가능한 파생값이다. `ProductionQueueItem` 생성자가 두 값으로부터 내부적으로 항상 다시 계산하므로
  로드(`FromJson`) 시에는 이 필드를 사용하지 않고 무시한다. 그럼에도 파일만 열어봐도 완료 예정 시점을 바로
  알 수 있도록 저장 시에는 함께 기록한다(가독성/운영 편의 목적, PRD 5.3의 "예상 완료 시각" 필드 요구를
  파일에도 반영).
- `orderId`를 고유 키로 사용하는 이유: PRD 4.4.2 "동일한 시료에 대한 주문이라도 각각 별도의 생산 작업으로
  처리한다(병합하지 않음)" 규칙에 따라 주문 1건당 생산 큐 항목이 최대 1건만 존재하므로, `orderId`만으로
  유일성이 보장된다.

## 산출물 목록

| 파일 | 내용 | 완료 조건 매핑 |
|---|---|---|
| `src/Json/JsonValue.h/.cpp` | `Json::Value` — Null/Boolean/Number/String/Array/Object를 표현하는 값 타입. `MakeXxx` 팩토리, `AsXxx`/`GetXxx`(object 키 조회, 기본값 지원) 접근자, `Add`/`Set`/`Find`, `Dump(indent)` 직렬화, `Parse(text, outValue)` 파싱(실패 시 예외 없이 `false`) | 외부 라이브러리 없이 자체 구현, PoC(DataPersistence)와 동등한 기능(값 표현/직렬화/파싱) |
| `src/Json/JsonIO.h/.cpp` | `Json::FileIO::ReadAllText`/`WriteAllText` — 파일 read/write, 쓰기 시 상위 디렉터리 자동 생성(`std::filesystem::create_directories`), 파일 없음은 `std::nullopt`로 표현(예외 없음) | 파일 없음/쓰기 시 폴더 없음 상황을 예외 없이 처리 |
| `src/Persistence/ISampleRepository.h` | `Create/GetAll/FindById/Update/Remove` 순수 가상 인터페이스 | CRUD 인터페이스 |
| `src/Persistence/JsonSampleRepository.h/.cpp` | 생성자 `Load()` + CUD마다 `Persist()`(전체 재기록), `ToJson`/`FromJson`은 private static, 필드 `id/name/avgProductionMinutesPerUnit/yield/stock`(PoC 호환, `docs/PRD.md` 5.4 그대로) | 저장 파일 `data/samples.json`, PoC 호환 스키마 |
| `src/Persistence/IOrderRepository.h` | `Create/GetAll/FindById/Update/Remove` 순수 가상 인터페이스 | CRUD 인터페이스 |
| `src/Persistence/JsonOrderRepository.h/.cpp` | 필드 `orderId/sampleId/customerName/quantity/status`(PoC 호환) + `createdAtEpochMillis`(추가). `OrderStatusToString`/`OrderStatusFromString`, `ToEpochMilliseconds`/`FromEpochMilliseconds`를 private static으로 배치 | 저장 파일 `data/orders.json`, 결정 1 반영 |
| `src/Persistence/IProductionQueueRepository.h` | `Create/GetAll/FindById/Update/Remove` 순수 가상 인터페이스(`orderId`가 고유 키) | CRUD 인터페이스 |
| `src/Persistence/JsonProductionQueueRepository.h/.cpp` | 결정 2의 스키마로 `data/productionQueue.json` 저장, `ToEpochMilliseconds`/`FromEpochMilliseconds`를 private static으로 배치 | 저장 파일 `data/productionQueue.json`, 결정 2 반영 |

## 완료 조건(DoD) 매핑
- 저장 → 재시작(재적재) 시 데이터 동일: 각 Repository의 `Load()`/`Persist()`가 대칭적으로 동일 필드를
  왕복하도록 구현(단위/통합 테스트는 unit-tester가 별도 작성).
- CRUD 각각 가능: 3개 Repository 모두 `Create/GetAll/FindById/Update/Remove` 구현.
- 파일 없을 때 예외 없이 빈 목록: `Json::FileIO::ReadAllText`가 `std::nullopt` 반환 시 `Load()`가 조기
  반환하여 빈 벡터 유지. `Json::Value::Parse` 실패 시에도 동일하게 빈 목록으로 폴백.
- PoC 실행파일(`DummyDataGenerator.exe`) 데이터 호환은 수동 검증 항목이므로 unit-tester/사용자 수동 확인
  대상으로 남겨둔다.

## 설계 메모
- Persistence 계층은 Phase 1에서 확립한 `Model` 네임스페이스와의 일관성을 위해 `Persistence` 네임스페이스로
  묶는다(PoC 자체는 네임스페이스를 쓰지 않지만, 이 저장소는 Phase 1부터 네임스페이스를 쓰는 것으로 이미
  결정되어 있었으므로 그 컨벤션을 유지).
- `Json::Value`의 파서는 `.cpp` 내부 익명 네임스페이스의 `JsonParser` 클래스로 캡슐화해 `Value` 헤더를
  가볍게 유지한다(SRP — 파싱 책임과 값 표현 책임 분리).
- `ToJson`/`FromJson`, enum↔문자열 변환, epoch 변환은 모두 각 Repository 구현체 내부 `private static`
  메서드로 배치(`CLAUDE.md` 지시 그대로, Model 클래스나 공용 유틸로 빼지 않음 — Persistence 계층만의 관심사).
- `data/` 폴더에 초기 파일을 미리 만들어두지 않는다. `Json::FileIO::WriteAllText`가 상위 디렉터리를 자동
  생성하므로 최초 `Create` 호출 시점에 자연스럽게 `data/` 폴더와 파일이 생성된다(파일 없음 폴백 동작을
  자연스럽게 검증할 수 있도록).

## 빌드 반영
- `SampleOrderSystem.vcxproj` / `.vcxproj.filters`에 신규 소스 10개(`.h` 5 + `.cpp` 5: JsonValue, JsonIO,
  JsonSampleRepository, JsonOrderRepository, JsonProductionQueueRepository) 및 인터페이스 헤더 3개
  (ISampleRepository, IOrderRepository, IProductionQueueRepository) 추가.
- Release/Debug(x64) 양쪽 모두 빌드 성공 확인.

## 재작업 이력
(현재까지 없음)
