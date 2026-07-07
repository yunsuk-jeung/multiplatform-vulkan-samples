# Progress Log

작업할 때마다 위에서부터(최신순) 짧게 기록한다.
형식: 날짜 · 무엇을 했나 · 배운 것 / 막힌 것 · 다음 할 일.

---

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
