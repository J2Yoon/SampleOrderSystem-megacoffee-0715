# Phase 0 — 프로젝트 셋업

## 목표
빈 Visual Studio 스켈레톤(`SampleOrderSystem.slnx` / `SampleOrderSystem.vcxproj`만 존재)에
실제 개발을 시작할 수 있는 최소 기반을 마련한다. (docs/All_phase_goals.md Phase 0,
관련 PRD: 1.3 시스템 형태, 6장 비기능 요구사항 — 아키텍처/영속성)

## 확인한 현재 상태 (재작업 시점 기준)
- `SampleOrderSystem.slnx`: x86/x64 플랫폼, 프로젝트 1개(`SampleOrderSystem.vcxproj`)만 참조.
  (이전에 잘못 추가되었던 `SampleOrderSystem.Tests.vcxproj` 참조는 이미 제거됨)
- `SampleOrderSystem.vcxproj`: ConfigurationType=Application, PlatformToolset v145, C++20.
- `src/Model`, `src/View`, `src/Controller`, `src/Persistence`, `src/Json` 디렉터리와 `.gitkeep`,
  `src/main.cpp`(최소 진입점)는 이미 존재. `tests/`는 빈 디렉터리.
- `vcpkg.json`: `gtest` 의존성만 등록된 manifest 존재.
- vcpkg는 VS 내장 버전이 전역 통합(`vcpkg integrate install`) 되어 있어
  `%LOCALAPPDATA%\vcpkg\vcpkg.user.props`/`vcpkg.user.targets`가 자동으로 `Microsoft.Cpp.targets`에
  연결됨 → `VcpkgEnableManifest=true`만 vcxproj에 설정하면 별도 Import 없이 manifest 복원이 동작한다.

## 설계 변경 사항 (사용자 직접 지시 반영)
- 테스트를 별도 프로젝트(`SampleOrderSystem.Tests.vcxproj`)로 만들지 않는다.
- `SampleOrderSystem` 단일 프로젝트 안에서 `RUN_TESTS` 전처리기 매크로로 진입점을 조건부 분기한다:
  `src/main.cpp`가 `#ifdef RUN_TESTS`이면 GoogleTest 러너(`InitGoogleTest` + `RUN_ALL_TESTS()`)를,
  그렇지 않으면 실제 앱 진입점을 컴파일한다.
- `.vcxproj`는 `RunTests` MSBuild 프로퍼티(`/p:RunTests=true`)를 받아 `RUN_TESTS` 전처리기 정의를
  조건부로 추가하고, `RunTests=true`일 때만 `tests/*.cpp`를 컴파일 대상에 포함한다.
- `tests/` 디렉터리는 유지하되 별도 main 진입점 파일(예: `main_test.cpp`)은 두지 않는다 — 진입점은
  `src/main.cpp` 하나뿐이다.

## 이번 Phase 산출물 목록

1. **디렉터리 구조** (기존 유지)
   - `src/Model/`, `src/View/`, `src/Controller/`, `src/Persistence/`, `src/Json/`: 각 `.gitkeep`으로 빈 폴더 추적.
   - `tests/`: `.gitkeep`으로 빈 폴더 추적(테스트 소스 파일은 이후 Phase에서 unit-tester가 채움).

2. **`src/main.cpp`**
   - `#ifdef RUN_TESTS` 블록: `<gtest/gtest.h>` 포함, `::testing::InitGoogleTest(&argc, argv)` +
     `return RUN_ALL_TESTS();`.
   - `#else` 블록: 기존 최소 진입점(초기 메시지 출력 후 0 반환) 유지.
   - 파일을 UTF-8 BOM으로 저장하여 MSVC가 한글 리터럴을 코드페이지 949로 잘못 해석해 발생시키는
     `warning C4819`를 제거함(빌드 성공 여부에는 영향 없었으나 경고 없는 빌드를 위해 정리).

3. **`SampleOrderSystem.vcxproj` 수정**
   - `<PropertyGroup Label="Vcpkg"><VcpkgEnableManifest>true</VcpkgEnableManifest></PropertyGroup>` 추가.
   - `<PropertyGroup><RunTests Condition="'$(RunTests)'==''">false</RunTests></PropertyGroup>` 추가
     (커맨드라인에서 지정하지 않으면 일반 앱 빌드로 처리).
   - `<ItemDefinitionGroup Condition="'$(RunTests)'=='true'">`를 4개 Configuration별
     `ItemDefinitionGroup` 뒤에 추가하여:
     - `ClCompile.PreprocessorDefinitions`에 `RUN_TESTS;%(PreprocessorDefinitions)` 추가.
     - `Link.AdditionalDependencies`에 `gtest.lib;gtest_main.lib;gmock.lib;gmock_main.lib;%(AdditionalDependencies)`
       추가(vcpkg gtest 포트는 Debug/Release 라이브러리 파일명에 `d` 접미사를 쓰지 않고 디렉터리로만
       구분하며, `AdditionalLibraryDirectories`는 vcpkg 전역 통합 targets가 `lib`와 `lib\manual-link`를
       구성별로 자동 추가하므로 별도 지정 불필요).
   - `<ItemGroup Condition="'$(RunTests)'=='true'"><ClCompile Include="tests\*.cpp" /></ItemGroup>` 추가
     — `tests/`가 비어 있어도 와일드카드가 매치되는 파일이 없으면 무해하게 통과.

4. **`vcpkg.json`** (기존 유지)
   - manifest 모드, `dependencies`에 `gtest`만 등록. gtest vcpkg 포트가 gmock/gmock_main도 함께
     제공하므로 별도 의존성 추가 불필요(사용자가 언급한 "vcpkg로 gmock을 이미 설치" 상황과 일치).

5. **`SampleOrderSystem.slnx`**: 별도 수정 없음(이미 단일 프로젝트만 참조하도록 정리되어 있음).

## 완료 조건(DoD) 검증 결과
- 앱 빌드: `msbuild SampleOrderSystem.slnx -p:Configuration=Debug -p:Platform=x64` → exit code 0,
  `x64\Debug\SampleOrderSystem.exe` 생성 및 정상 실행(초기 메시지 출력 후 0 반환) 확인.
- 테스트 빌드: `msbuild SampleOrderSystem.slnx -p:Configuration=Debug -p:Platform=x64 -p:RunTests=true`
  → exit code 0, 동일 경로의 `SampleOrderSystem.exe`가 GoogleTest 러너로 동작:
  ```
  [==========] Running 0 tests from 0 test suites.
  [==========] 0 tests from 0 test suites ran. (0 ms total)
  [  PASSED  ] 0 tests.
  ```
  종료 코드 0 확인.
- vcpkg manifest 최초 복원(gtest 1.17.0 및 의존 포트 vcpkg-cmake/vcpkg-cmake-config) 자동 실행 확인.
  빌드 로그 말미에 `'pwsh.exe'은(는) ... 아닙니다` 메시지가 출력되지만 이는 vcpkg 바이너리 캐시 제출
  단계(백그라운드, PowerShell 부재)에서 나는 무해한 경고이며 MSBuild 종료 코드에는 영향 없음(0으로 확인).

## 재작업 이력

### 3차 재작업: vcpkg gtest → NuGet gmock(1.11.0, packages.config)로 일원화
- **문제**: unit-tester 검증 중 `.slnx`에 `SampleOrderSystem.Tests.vcxproj` 참조가 다시 나타나 앱 빌드가
  `MSB3202`로 실패했고(Visual Studio가 열려 있는 상태에서 이전 세션의 캐시된 솔루션 상태를 재기록한 것으로
  추정), 이를 제거하고 재검증하는 과정에서 `RunTests`를 지정하지 않은 일반 앱 빌드에서도 `gtest-all.cc`/
  `gmock-all.cc`가 무조건 컴파일되는 것을 발견함.
- **원인**: 사용자가 이미 Visual Studio에서 클래식 NuGet 패키지 `gmock`(1.11.0, `packages.config` +
  `packages\gmock.1.11.0\build\native\gmock.targets`)을 `SampleOrderSystem.vcxproj`에 추가해둔 상태였고,
  이 `.targets`는 `RunTests` 조건 없이 무조건 `ImportGroup`으로 로드되어 `gtest-all.cc`/`gmock-all.cc`를
  `ClCompile` 대상에 추가하고 있었음. 이 상태에서 phase-developer가 구성한 vcpkg `gtest` 포트가 동시에
  존재해 GoogleTest/GoogleMock 통합 경로가 두 가지(vcpkg + NuGet)로 중복됨.
- **사용자 결정**: vcpkg gtest 포트를 제거하고 NuGet `gmock` 패키지만 사용.
- **조치**:
  - `vcpkg.json`, `vcpkg_installed/` 삭제(이 저장소는 더 이상 vcpkg를 사용하지 않음).
  - `SampleOrderSystem.vcxproj`에서 `VcpkgEnableManifest` PropertyGroup과 `RunTests=='true'` 시
    `gtest.lib;gtest_main.lib;gmock.lib;gmock_main.lib`를 링크하던 `Link.AdditionalDependencies` 설정을
    제거(NuGet 패키지가 `.cc` 소스를 직접 컴파일해 넣는 방식이라 라이브러리 링크가 불필요).
  - `gmock.targets`를 가져오는 `<Import>`와 `EnsureNuGetPackageBuildImports` 타겟에
    `Condition="'$(RunTests)'=='true' ..."`를 추가해, 일반 앱 빌드에는 테스트 프레임워크 소스가 전혀
    섞이지 않도록 함.
  - `.gitignore`의 `vcpkg_installed/` 항목을 `packages/`(NuGet 클래식 복원 폴더, 커밋 대상 아님)로 교체.
  - `CLAUDE.md`, `docs/All_phase_goals.md` Phase 0 절의 "vcpkg 매니페스트/GoogleTest" 서술을 "NuGet
    `gmock`(1.11.0) + `packages.config`" 기준으로 갱신.
- **재검증 결과**: 앱 빌드(`RunTests` 미지정) 시 `main.cpp`만 컴파일됨을 확인. 테스트 빌드
  (`RunTests=true`) 시 `main.cpp`+`gtest-all.cc`+`gmock-all.cc`가 컴파일되고 실행 결과가
  `[PASSED] 0 tests`로 동일하게 나옴을 재확인. 앱 빌드로 다시 전환 시에도 정상적으로 재링크됨을 확인.

### 1차 → 2차 재작업: 테스트 프로젝트 분리 → 단일 프로젝트 RUN_TESTS 매크로 방식으로 변경
- **문제**: 최초 계획은 `SampleOrderSystem.Tests.vcxproj`를 별도 프로젝트로 만들어 `.slnx`에 추가하는
  방식이었으나, 사용자가 이를 원치 않는다고 판단하여 잘못 생성된 `.Tests.vcxproj`/`.filters`/`.user`
  파일과 `.slnx`의 참조를 이미 제거한 상태였음.
- **원인**: 테스트를 위해 별도 실행 파일/프로젝트를 두는 것보다, 동일 실행 파일이 빌드 매크로에 따라
  앱 진입점과 테스트 러너 진입점 중 하나로 컴파일되는 단일 프로젝트 구조가 이 저장소의 요구사항(단일
  `.vcxproj`, MSBuild 프로퍼티로 제어)에 더 부합한다는 지시.
- **조치**: `src/main.cpp`에 `#ifdef RUN_TESTS` 분기를 추가하고, `SampleOrderSystem.vcxproj`에
  `RunTests` MSBuild 프로퍼티 기반 조건부 `PreprocessorDefinitions`/`AdditionalDependencies`/
  `ClCompile`(tests/*.cpp) 설정을 추가함. `docs/All_phase_goals.md`, `CLAUDE.md`는 이미 이 방식으로
  갱신되어 있었으므로 이번 구현이 최신 문서와 일치하도록 작업함.
- **부수 이슈 및 해결**: 최초 `Link.AdditionalDependencies`에 `gtestd.lib` 등 `d` 접미사를 붙였다가
  `LNK1104(파일을 열 수 없음)` 에러 발생. vcpkg gtest 포트의 실제 산출물(`vcpkg_installed/x64-windows/**/lib/*.lib`)을
  확인한 결과 Debug/Release 모두 동일한 파일명(`gtest.lib`, `gmock.lib`)을 쓰고 `lib/manual-link`
  하위에 `gtest_main.lib`/`gmock_main.lib`가 있으며, 이 디렉터리들은 vcpkg 전역 통합 targets
  (`vcpkg.targets`)가 구성별로 자동으로 `AdditionalLibraryDirectories`에 추가함을 확인. 접미사 없는
  파일명으로 수정하여 해결.
