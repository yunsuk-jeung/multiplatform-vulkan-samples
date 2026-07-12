# Progress Log

작업할 때마다 위에서부터(최신순) 짧게 기록한다.
형식: 날짜 · 무엇을 했나 · 배운 것 / 막힌 것 · 다음 할 일.

---

## 2026-07-12 — sample 03 완료: window + surface + present queue

- `mpvk::Surface`(VkSurfaceKHR RAII, instance보다 먼저 파괴), Instance에 surface
  확장 주입(`extra_extensions`), `PhysicalDevice`에 present family 탐색
  (`getSurfaceSupportKHR`, `std::optional`), `Device`에 present queue 확보 완료.
- 실행: graphics/present family=0, graphics/present queue OK, validation 0개.
- 설계 결정:
  - present family는 **PhysicalDevice**에 둠(GPU 선택에 present 반영). Device는 읽기만.
  - **headless = `const Surface* = nullptr` + `optional<present_family_>`** 로 표현
    → windowed(03)/headless(02)를 한 타입으로. sample 02 그대로 동작.
  - surface 확장은 GLFW가 제공(필수라 지원 확인 없이 주입), portability는 Instance 내부 유지.
- 버그/교훈: `if (!res)`(VkResult 0=성공) → `if (res != VK_SUCCESS)` (cpp_notes 등재).
  중복 family index로 큐 2개 만들면 validation 에러 → 중복 제거.
- surface capabilities/formats/present modes 질의는 **04로 이월**(swapchain에서 사용).
- **다음**: sample 04 (`04_clear_screen`) — swapchain + 렌더 루프 + 동기화.

## 2026-07-12 — sample 03 착수: GLFW 통합 + Window

- **GLFW 빌드 통합** (소스 빌드로 결정): `install_dependencies_mac.sh`에
  spdlog와 동일한 `fetch_source`+`build_library` 패턴으로 GLFW 3.4 추가.
  - prebuilt zip 방식도 만들어봤으나 폐기 — 소스 빌드는 GLFW가 자기
    `glfw3Config.cmake`를 생성(프레임워크 링크 자동)하고 ubuntu와도 동일 경로.
  - `cpp/CMakeLists.txt`에 `find_package(glfw3 CONFIG)` + link, window/surface 소스 등록.
- **`mpvk::Window`** (GLFW 창 RAII) 구현: `glfwInit`→`GLFW_NO_API`→`glfwCreateWindow`,
  `should_close`/`poll_events`, 소멸자 destroy+terminate. 빈 창 뜨는 것 확인.
  - 배운 것: 헤더 전방선언은 `struct GLFWwindow;`(MSVC 대비). `unique_ptr<GLFWwindow>`는
    기본 deleter가 `delete`라 UB → 커스텀 deleter 필요, 이 단계선 raw 포인터 RAII 채택.
- 협업 방식 확정: **파일 스켈레톤/리뷰/가이드는 내가, 구현은 사용자가.**
- **다음**: Surface 구현 + Instance surface 확장 + present family (NEXT.md 참고).

## 2026-07-11 — validation layer + debug messenger (Instance)

- `mpvk::Instance`에 디버그 빌드 한정 validation 추가: `VK_LAYER_KHRONOS_validation`
  레이어 + `VK_EXT_debug_utils` 확장(둘 다 지원 확인 후 opt-in) + debug messenger.
- sample 02를 validation 켠 상태로 재검증: 정상 시 경고 0개, priority를 일부러
  제거하니 콜백이 `[vk] vkCreateDevice(): ... queueCount must be greater than 0`
  으로 잡힘 → **실제 검증 동작 확인**(02의 "validation 미검증" 숙제 해소).
- 막혔던 것 / 배운 것:
  - **순서:** 레이어/확장 검출은 createInstance 前, messenger 생성은 後
    (handle_ 필요). 섞으면 dispatcher init assert로 죽음.
  - **콜백 타입:** 이 SDK vulkan.hpp는 pfnUserCallback을 `vk::` 강타입으로 정의 →
    콜백도 `vk::` 타입이어야 함(C 타입은 컴파일 에러).
  - **EXT 함수:** `vk::detail::DispatchLoaderDynamic` 필요(생성/파괴 호출에 전달).
  - release 전용 스코프 버그(`layers`가 `#ifndef NDEBUG` 안에 선언)도 잡음.
- 스택 변화: 로깅에 **spdlog** 도입됨(로그 파일 출력). 콜백은 아직 fprintf라
  추후 spdlog로 통일 예정(폴리시).
- **다음**: Phase 2 — `03_window_surface`.

## 2026-07-08 — 02_logical_device 완료

- `mpvk`에 `PhysicalDevice`(GPU 선택 + graphics queue family 탐색),
  `Device`(logical device 소유 + graphics queue 확보, RAII) 추가.
- 실행 확인: `Selected GPU: Apple M4 Pro (graphics family = 0)` /
  `Logical device created. Graphics queue: OK`, validation 경고 0개.
- 막혔던 것 / 결정:
  - **portability_subset 매크로**가 안 보임 → `VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME`는
    `vulkan_beta.h`(`VK_ENABLE_BETA_EXTENSIONS` 게이트) 소속. **문자열 리터럴
    `"VK_KHR_portability_subset"` 직접 사용**으로 해결(이식성↑, beta 헤더 의존 제거).
  - 확장은 `#ifdef __APPLE__` 없이 **항상 쿼리 후 있으면 추가** — 쿼리가 플랫폼
    차이를 흡수하므로 매크로 게이트 불필요.
  - 의존 인자를 **포인터→참조**로 정정. "전방 선언하려면 포인터"는 오해였고,
    참조도 전방 선언으로 선언 가능(단 전방 선언 줄 자체는 필요).
  - `std::pmr::vector` 오타, `<cstring>` 누락, `int` 인덱스(signed/unsigned) 수정.
- ⚠️ **검증 미완**: 02 실행 시 validation layer가 실제로 켜져 있지 않았음
  (Instance에 레이어/​debug messenger 없음). "경고 0개"는 검증 통과가 아님.
- **다음 (우선순위 순)**:
  1. Instance에 **validation layer + debug messenger** 추가 (디버그 빌드 한정),
     그 후 02를 validation 켠 상태로 재검증. → concepts.md에 항목 추가 예정.
  2. Phase 2 — `03_window_surface` (GLFW 창 + `VkSurfaceKHR` + present queue).

## 2026-07-07 — 킥오프

- 프로젝트 구조 파악, 학습 방식 확정: Khronos식 대형 프레임워크 대신
  **샘플 주도 증분 구현**.
- [roadmap.md](roadmap.md) 작성 — Phase 0~8 학습 경로 확정.
- 현재 상태:
  - `mpvk`: `Instance` 클래스만 존재.
  - 샘플: `01_device_info` 완료 (instance 생성 + physical device 열거).
  - 빌드: CMake presets(debug/release/relwithdebinfo) + direnv로 SDK 로드. 정상.
- **다음**: Phase 1 — `02_logical_device`.
  `PhysicalDevice`(선택 헬퍼) + `Device`(logical device + queue) 를 `mpvk`에 추가.
