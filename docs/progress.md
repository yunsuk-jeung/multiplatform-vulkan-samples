# Progress Log

작업할 때마다 위에서부터(최신순) 짧게 기록한다.
형식: 날짜 · 무엇을 했나 · 배운 것 / 막힌 것 · 다음 할 일.

---

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
