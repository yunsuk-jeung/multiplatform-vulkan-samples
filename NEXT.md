# ▶ 여기서부터 (NEXT)

> 이 파일은 **매 세션 덮어쓰기**된다 — 항상 "지금 시작점"만 담는다.
> 누적 히스토리는 [docs/progress.md](docs/progress.md), 개념은 [docs/concepts.md](docs/concepts.md).
> 컴퓨터 이동 시: 시작 전 `git pull`, 끝나고 `git push` 해야 동기화됨.

**마지막 갱신:** 2026-07-11

## 현재 위치
- ✅ sample 01 (instance + physical device 열거)
- ✅ sample 02 (`PhysicalDevice` 선택 + `Device` logical device/queue)
- ✅ Instance에 **validation layer + debug messenger** (디버그 빌드 한정),
  콜백은 spdlog(`LogE/LogW/LogI`)로 severity 분기 + VUID 이름 출력. 검증 완료.
- (스택) 로깅에 spdlog 도입됨 ([logger.hpp](cpp/include/mpvk/logger.hpp), `LogI/LogW/LogE`)

## 다음에 할 일
**sample 03 (`03_window_surface`)** — GLFW 창 + `VkSurfaceKHR` + present queue family
- 시작 규칙: 먼저 `docs/samples/03_window_surface.md` 교육 문서 작성 ("start sample 03")
- 새 요소: GLFW를 `third_party`에 추가하는 **첫 외부 의존성** 작업 (빌드 통합)
- 새 개념(예정): surface, present queue family(graphics와 다를 수 있음),
  surface 지원 format/present mode 질의
- 설계 방향: `mpvk::Window`(GLFW 래퍼) + surface 생성. Instance/Device와 연결.

## 이어서 하려면 (환경 복구)
```bash
direnv allow            # SDK 환경 자동 로드 (최초 1회)
cmake --build --preset debug
./build/debug/samples/02_logical_device/02_logical_device
# validation 콜백 확인: device.cpp에서 setQueuePriorities 잠깐 지웠다 실행 → [vk] 로그 → 원복
```
