# ▶ 여기서부터 (NEXT)

> 이 파일은 **매 세션 덮어쓰기**된다 — 항상 "지금 시작점"만 담는다.
> 누적 히스토리는 [docs/progress.md](docs/progress.md), 개념은 [docs/concepts.md](docs/concepts.md).
> 컴퓨터 이동 시: 시작 전 `git pull`, 끝나고 `git push` 해야 동기화됨.

**마지막 갱신:** 2026-07-08

## 현재 위치
- ✅ sample 01 (instance + physical device 열거)
- ✅ sample 02 (`PhysicalDevice` 선택 + `Device` logical device/queue) — 실행 확인
- ⚠️ 단, sample 02는 **validation layer 없이** 실행만 확인함 (검증 통과 아님)

## 다음에 할 일 (우선순위 순)
1. **[우선] Instance에 validation layer + debug messenger 추가** (디버그 빌드 한정)
   - `VK_LAYER_KHRONOS_validation` 레이어 enable (지원 확인 후 opt-in)
   - `VK_EXT_debug_utils` 확장 + debug messenger 콜백으로 메시지 수신/출력
   - release 빌드에선 생략
   - 완료 후 **sample 02를 validation 켠 상태로 재실행** → 경고 0개 재확인
   - `docs/concepts.md`에 "validation layer / debug messenger" 항목 추가
2. **sample 03 (`03_window_surface`)** — GLFW 창 + `VkSurfaceKHR` + present queue family
   - 시작 시 규칙: 먼저 `docs/samples/03_window_surface.md` 교육 문서 작성

## 이어서 하려면 (환경 복구)
```bash
direnv allow            # SDK 환경 자동 로드 (최초 1회)
cmake --build --preset debug
./build/debug/samples/02_logical_device/02_logical_device
```
