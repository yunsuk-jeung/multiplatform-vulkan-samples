# ▶ 여기서부터 (NEXT)

> 이 파일은 **매 세션 덮어쓰기**된다 — 항상 "지금 시작점"만 담는다.
> 누적 히스토리는 [docs/progress.md](docs/progress.md), 개념은 [docs/concepts.md](docs/concepts.md).
> 컴퓨터 이동 시: 시작 전 `git pull`, 끝나고 `git push` 해야 동기화됨.

**마지막 갱신:** 2026-07-12

## 현재 위치
- ✅ sample 01 / 02 완료, Instance에 validation layer + debug messenger (spdlog)
- 🚧 **sample 03 (`03_window_surface`) 진행 중**:
  - ✅ GLFW 빌드 통합 (소스 빌드, `install_dependencies_mac.sh` + cpp CMake, 검증됨)
  - ✅ `mpvk::Window` (GLFW 창 RAII, `should_close`/`poll_events`) — 빈 창 확인
  - ⬜ `mpvk::Surface` (아직 스켈레톤)
  - ⬜ Instance에 surface 확장 병합, present family 탐색, Device present queue

## 다음에 할 일 (sample 03 이어서, 순서대로)
1. **`mpvk::Window`에 `required_instance_extensions()` 추가** — `glfwGetRequiredInstanceExtensions` 래핑.
2. **Instance에 surface 확장 병합** — 위 목록을 extensions에 합침(portability 확장은 유지).
   ⚠️ `glfwInit` 이후라야 유효 → main에서 **Window를 Instance보다 먼저** 생성.
3. **`mpvk::Surface` 구현** — `glfwCreateWindowSurface`, `vk::Instance` 보관해
   소멸자에서 `destroySurfaceKHR`(instance보다 먼저 파괴).
4. **PhysicalDevice에 present family 탐색** — `getSurfaceSupportKHR(family, surface)`.
   graphics와 다를 수 있음.
5. **Device에 present queue** — 두 family 같으면 큐 1개, 다르면 2개.
6. **surface 지원 질의 출력** — capabilities/formats/present modes 개수.
7. main에서 `Window → Instance → Surface → PhysicalDevice → Device` 연결 + validation 0개.

> 상세/함정은 [docs/samples/03_window_surface.md](docs/samples/03_window_surface.md) 참고.
> 사용자가 구현, 나는 파일 스켈레톤/리뷰/가이드 담당.

## 이어서 하려면 (환경 복구)
```bash
direnv allow            # SDK 환경 자동 로드 (최초 1회)
cmake --build --preset debug
./build/debug/samples/02_logical_device/02_logical_device
# validation 콜백 확인: device.cpp에서 setQueuePriorities 잠깐 지웠다 실행 → [vk] 로그 → 원복
```
