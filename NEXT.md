# ▶ 여기서부터 (NEXT)

> 이 파일은 **매 세션 덮어쓰기**된다 — 항상 "지금 시작점"만 담는다.
> 누적 히스토리는 [docs/progress.md](docs/progress.md), 개념은 [docs/concepts.md](docs/concepts.md).
> 컴퓨터 이동 시: 시작 전 `git pull`, 끝나고 `git push` 해야 동기화됨.

**마지막 갱신:** 2026-07-19

## 현재 위치
- ✅ sample 01 (instance + physical device 열거)
- ✅ sample 02 (physical/logical device + queue) — headless
- ✅ Instance validation layer + debug messenger (spdlog)
- ✅ sample 03 (window + surface + present queue)
- ✅ **sample 04 (clear screen)** — swapchain + command pool + 동기화 + 렌더 루프
  + frames-in-flight(2) + 리사이즈 재생성. 짙은 파랑, validation 0개.
- mpvk: Instance, PhysicalDevice, Device, Window, Surface, Swapchain, CommandPool

## 다음에 할 일
**Phase 3 — sample 05 (`05_hello_triangle`) 시작** — 04 렌더 루프 위에 "진짜 그리기".
- 새 개념: render pass, framebuffer, graphics pipeline(고정+프로그래머블), shader module(SPIR-V),
  viewport/scissor. 정점은 셰이더에 하드코딩.
- 새 빌드 요소: GLSL → SPIR-V 컴파일(SDK `glslc`)을 CMake에 통합.
- mpvk 추가(예정): `RenderPass`, `Framebuffer`, `ShaderModule`, `GraphicsPipeline`.
- 시작 규칙: 먼저 교육 문서 `docs/samples/05_hello_triangle.md` 작성("start sample 05").
- 참고: 04는 render pass 없이 `vkCmdClearColorImage`로 지웠음. 05에서 **render pass 도입**
  (로드맵: 고전 render pass 먼저, dynamic rendering은 이후 별도 샘플).

## 협업 방식 (고정)
- 사용자가 구현, 나는 파일 스켈레톤/리뷰/가이드 + **문서 자동 갱신**(시키지 않아도).
- 코드 주석은 영어, 문서 산문은 한국어.
- 마일스톤마다 sample doc/concepts/cpp_notes/progress/NEXT 갱신.

## 이어서 하려면 (환경 복구)
```bash
direnv allow
cmake --build --preset debug
./build/debug/samples/04_clear_screen/04_clear_screen   # 짙은 파란 창
```
