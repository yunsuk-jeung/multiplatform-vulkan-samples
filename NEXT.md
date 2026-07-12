# ▶ 여기서부터 (NEXT)

> 이 파일은 **매 세션 덮어쓰기**된다 — 항상 "지금 시작점"만 담는다.
> 누적 히스토리는 [docs/progress.md](docs/progress.md), 개념은 [docs/concepts.md](docs/concepts.md).
> 컴퓨터 이동 시: 시작 전 `git pull`, 끝나고 `git push` 해야 동기화됨.

**마지막 갱신:** 2026-07-12

## 현재 위치
- ✅ sample 01 / 02 완료, Instance에 validation layer + debug messenger (spdlog)
- ✅ **sample 03 (`03_window_surface`) 완료** — window + surface + graphics/present queue,
  validation 0개. headless(02)/windowed(03)를 `const Surface*`+`optional`로 통합.

## 다음에 할 일
**sample 04 (`04_clear_screen`) 시작** — swapchain으로 화면을 특정 색으로 지우기.
가장 큰 덩어리(렌더 루프의 뼈대). 새 개념 多:
- swapchain 생성(surface capabilities/format/present mode 질의 → 선택), swapchain 이미지/뷰
- command pool / command buffer, 이미지 acquire → clear 기록 → submit → present
- 동기화: semaphore(GPU-GPU), fence(CPU-GPU), frames-in-flight
- 시작 규칙: 교육 문서 [docs/samples/04_clear_screen.md](docs/samples/04_clear_screen.md) 먼저(작성됨).
  첫 스텝은 **swapchain 지원 질의 + `mpvk::Swapchain` 생성**부터 작게.

> 협업 방식: 사용자가 구현, 나는 파일 스켈레톤/리뷰/가이드 + 문서 갱신.
> 미커밋 변경이 남아 있으면 먼저 커밋(스크립트·window·surface·physical/device·문서).

## 이어서 하려면 (환경 복구)
```bash
direnv allow            # SDK 환경 자동 로드 (최초 1회)
cmake --build --preset debug
./build/debug/samples/02_logical_device/02_logical_device
# validation 콜백 확인: device.cpp에서 setQueuePriorities 잠깐 지웠다 실행 → [vk] 로그 → 원복
```
