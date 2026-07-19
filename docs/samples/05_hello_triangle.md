# 05_hello_triangle

> Phase 3 · 상태: 🚧 진행 중
> (셰이더+SPIR-V 빌드 ✅ / ShaderModule ⬜ / RenderPass ⬜ / Framebuffer ⬜ / Pipeline ⬜ / draw ⬜)

## Goal

04의 렌더 루프 위에 **첫 삼각형**을 그린다. 정점은 셰이더에 하드코딩(버퍼 아직 없음).
render pass + framebuffer + graphics pipeline + shader(SPIR-V)를 도입한다.

## What this sample teaches

- **shader module**: GLSL → SPIR-V 컴파일(SDK `glslc`, CMake 통합) → `VkShaderModule`.
- **render pass**: attachment(색 버퍼) + load/store op + subpass + 레이아웃 전환을 선언.
  04의 수동 barrier/clear가 render pass의 **load op clear**로 대체된다.
- **framebuffer**: render pass의 attachment ↔ 실제 swapchain image view 바인딩. 이미지마다 1개.
- **graphics pipeline**: shader stage + 고정 기능(input assembly, viewport/scissor, rasterizer,
  multisample, color blend) + pipeline layout. Vulkan은 파이프라인이 대부분 **불변(immutable)**.
- **draw**: `beginRenderPass` → `bindPipeline` → `setViewport/scissor` → `draw(3,1,0,0)` → `endRenderPass`.
- viewport/scissor를 **dynamic state**로 두면 리사이즈에 유리.

## 배우는 개념

### shader module & GLSL→SPIR-V
Vulkan은 GLSL을 직접 안 먹고 **SPIR-V**(바이트코드)만 받는다. 빌드 때 `glslc`로
`.vert`/`.frag` → `.spv` 컴파일 → 런타임에 읽어 `createShaderModule`.
- 정점: `gl_VertexIndex`로 3개 위치를 셰이더에 하드코딩(vertex buffer는 06에서).
- CMake에서 `glslc` 커스텀 커맨드로 빌드 시 자동 컴파일(수동 재컴파일 방지).

### render pass — "무엇을, 어떻게 시작/끝낼지" 선언
color attachment 하나: format=swapchain format, `loadOp=eClear`(시작 시 지움),
`storeOp=eStore`(결과 보존), `initialLayout=eUndefined`, `finalLayout=ePresentSrcKHR`.
→ 04에서 손으로 하던 레이아웃 전환/clear를 render pass가 대신한다(수동 barrier 제거).
subpass 1개 + subpass dependency(외부→subpass)로 동기화.

### framebuffer
render pass의 attachment 슬롯에 **실제 이미지 뷰**를 끼운다. swapchain image view마다
framebuffer 하나(총 이미지 수). extent도 지정. 리사이즈 재생성 시 함께 다시 만든다.

### graphics pipeline — 대부분 불변
04까지는 파이프라인이 없었다(clear/transfer만). 삼각형을 그리려면 **어떻게 그릴지** 전체를
하나의 `VkPipeline`에 굳혀둔다:
- shader stages(vertex, fragment)
- vertex input(지금은 비어 있음 — 하드코딩), input assembly(triangle list)
- viewport/scissor(dynamic 권장), rasterizer(fill, cull), multisample(1x), color blend(불투명)
- pipeline layout(uniform/push constant 없음 → 빈 레이아웃)
- render pass + subpass index

### draw 흐름 (04 대비 바뀌는 부분)
04의 command buffer 기록이 이렇게 바뀐다:
```
beginRenderPass(clear color)   // barrier/clear 대체
  bindPipeline(graphics)
  setViewport / setScissor      // dynamic이면
  draw(3, 1, 0, 0)              // 정점 3개, 인스턴스 1개
endRenderPass                   // finalLayout=PresentSrc 자동 전환
```
acquire/submit/present/동기화(semaphore/fence/frames-in-flight)는 **04 그대로**.

## Expected result

창에 **삼각형**(예: 각 꼭짓점 R/G/B 보간)이 뜬다. validation 0개. 리사이즈해도 유지.

## Files to touch

- `samples/05_hello_triangle/shaders/triangle.vert` / `.frag` (신규)
- CMake: GLSL→SPIR-V 컴파일 통합(`glslc`)
- `cpp/include/mpvk/shader_module.hpp` / `render_pass.hpp` / `framebuffer.hpp` /
  `graphics_pipeline.hpp` (+ src) — 신규(최소)
- `samples/05_hello_triangle/main.cpp` + `CMakeLists.txt`
- `samples/CMakeLists.txt`, `cpp/CMakeLists.txt`

## mpvk 설계 (additions) — 최소

### `mpvk::ShaderModule`
- `.spv` 파일 로드 → `createShaderModule`. RAII 파괴. `handle()`.

### `mpvk::RenderPass`
- color attachment 1개(swapchain format) + subpass + dependency. RAII.

### `mpvk::Framebuffer`
- (render pass, image view, extent) → `createFramebuffer`. 이미지마다 하나(main에서 vector).

### `mpvk::GraphicsPipeline`
- shader 2개 + 고정 기능 + layout + render pass → `createGraphicsPipeline`. pipeline layout도 소유.

넣지 말 것: vertex buffer(06), descriptor(08), depth(10), 여러 subpass/attachment.

## Implementation steps (큰 순서 — 각 단계 후 확인)

1. **셰이더 작성 + SPIR-V 빌드 통합** — triangle.vert/frag, CMake `glslc`로 `.spv` 생성.
2. **`ShaderModule`** — `.spv` 읽어 모듈 생성. 로드 확인.
3. **`RenderPass`** — color attachment(clear/store, finalLayout=PresentSrc) + subpass + dependency.
4. **`Framebuffer`** — swapchain image view마다 하나.
5. **`GraphicsPipeline`** — stage+고정기능+layout+render pass. (dynamic viewport/scissor 권장.)
6. **draw 기록** — 04의 barrier/clear를 beginRenderPass→bindPipeline→draw→endRenderPass로 교체.
7. **리사이즈** — swapchain 재생성 시 framebuffer도 재생성(pipeline은 dynamic이면 유지).

## 구현 시 주의점 (common mistakes)

- **swapchain imageUsage에 `eColorAttachment` 필수**(04에서 이미 포함). render pass 색 첨부라.
- **finalLayout=`ePresentSrcKHR`** → present 전 레이아웃 수동 전환 불필요(render pass가 함).
- **subpass dependency** 빠지면 레이아웃 전환 타이밍 validation 경고. external→0 의존성 하나 필요.
- **viewport Y/좌표계**: Vulkan은 Y가 아래로. 삼각형이 뒤집히면 확인(또는 negative viewport height).
- **dynamic state**로 viewport/scissor를 안 두면 리사이즈마다 파이프라인 재생성해야 함 → dynamic 권장.
- **SPIR-V 경로**: 런타임에 `.spv`를 어디서 읽나. 빌드 산출물 경로를 CMake가 정의(`MPVK_SOURCE_DIR` 등) 하거나 실행 위치 기준. 경로 문제 흔함.
- pipeline layout이 비어도 **반드시 생성**해서 넘겨야 함.

## Self-check

- [ ] 삼각형이 뜨고 validation 0개인가?
- [ ] 리사이즈 시 재생성(framebuffer)되고 유지되는가?
- [ ] render pass load op clear로 배경이 지워지나(수동 barrier 없음)?
- [ ] ShaderModule/RenderPass/Framebuffer/Pipeline 파괴 순서/복사 금지 OK?

## What to send for review

- 셰이더 + CMake glslc 통합 diff
- render pass / pipeline 생성 부분
- draw 기록(begin/bind/draw/end) + 실행 스크린샷/로그

## 새 개념 (완료 시 concepts.md에 추가)

- shader module / GLSL→SPIR-V(glslc)
- render pass (attachment/subpass/dependency, load-op clear)
- framebuffer
- graphics pipeline (고정 기능 + 불변성) / pipeline layout
- dynamic state (viewport/scissor)
- draw command

---

## 구현 가이드 (직접 해보기)

> 첫 목표는 **스텝 1~2**: 셰이더를 SPIR-V로 빌드하고 `ShaderModule`로 로드까지.
> 화면 출력(render pass~pipeline)은 그다음.

### Step 1 — 셰이더 + SPIR-V 빌드 통합 ✅ (세팅 완료)
`samples/05_hello_triangle/shaders/triangle.{vert,frag}` + `CMakeLists.txt`에 glslc 통합 완료:
- `find_program(GLSLC_EXECUTABLE glslc HINTS "$ENV{VULKAN_SDK}/bin")`.
- 셰이더마다 `add_custom_command`(OUTPUT `.spv`, COMMAND glslc, DEPENDS src) →
  `add_custom_target(05_shaders)` → `add_dependencies(05_hello_triangle 05_shaders)`.
  (빌드 시 자동 컴파일, 셰이더 수정 시 재컴파일.)
- **`target_compile_definitions(... SHADER_DIR="${_spv_dir}")`** 로 `.spv` 출력 경로를
  실행 파일에 박음 → CWD 무관하게 찾음. main에서 `SHADER_DIR "/triangle.vert.spv"`.
- 검증: `glslc triangle.{vert,frag}` 빌드 → `shaders loaded: vert=1500, frag=568 bytes`.
- (현재 main은 .spv 로드만 하는 검증 스텁 — Step 2부터 실제 파이프라인으로 교체.)

원래 셰이더 내용(직접 이해):
- `triangle.vert`: `gl_VertexIndex`로 3개 위치(`vec2`) 하드코딩 + 색 출력.
  ```glsl
  #version 450
  layout(location=0) out vec3 v_color;
  vec2 positions[3] = vec2[](vec2(0,-0.5), vec2(0.5,0.5), vec2(-0.5,0.5));
  vec3 colors[3]    = vec3[](vec3(1,0,0), vec3(0,1,0), vec3(0,0,1));
  void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0, 1);
    v_color = colors[gl_VertexIndex];
  }
  ```
  `triangle.frag`: `v_color`를 그대로 출력.
- CMake: `glslc <in> -o <out>.spv`를 커스텀 커맨드/타깃으로. SDK의 `glslc` 경로 사용
  (`$ENV{VULKAN_SDK}/bin/glslc` 또는 `find_program`). 산출 `.spv`를 실행 파일이 찾을
  위치(빌드 디렉터리/알려진 경로)에 둔다.

### Step 2 — `mpvk::ShaderModule` (상세)

**헤더**
```cpp
#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>

namespace mpvk {
class Device;

class ShaderModule {
public:
  ShaderModule(const Device& device, const std::vector<char>& spv);
  ~ShaderModule();
  ShaderModule(const ShaderModule&)            = delete;
  ShaderModule& operator=(const ShaderModule&) = delete;

  vk::ShaderModule handle() const { return module_; }

private:
  vk::Device       vk_device_{nullptr};
  vk::ShaderModule module_{nullptr};
};
}  // namespace mpvk
```
- 입력을 **바이트(`std::vector<char>`)** 로 받는다(파일 I/O는 호출자 몫 = main의 `read_file`).
  경로를 받아 내부에서 읽는 방식도 가능하나, 지금은 관심사 분리로 바이트를 받는다.
- `class Device;` 전방 선언(참조 인자), 복사 금지, `vk::Device` 보관(소멸자용).

**cpp**
```cpp
#include <cstdint>
#include "mpvk/device.hpp"
#include "mpvk/shader_module.hpp"

namespace mpvk {
ShaderModule::ShaderModule(const Device& device, const std::vector<char>& spv)
  : vk_device_{device.handle()} {
  vk::ShaderModuleCreateInfo ci;
  ci.codeSize = spv.size();                                    // BYTES
  ci.pCode    = reinterpret_cast<const uint32_t*>(spv.data()); // SPIR-V words
  module_ = vk_device_.createShaderModule(ci);
}
ShaderModule::~ShaderModule() {
  if (module_) vk_device_.destroyShaderModule(module_);
}
}  // namespace mpvk
```

**핵심 포인트**
- **`codeSize`는 바이트 수**(`spv.size()`), **`pCode`는 `const uint32_t*`**. SPIR-V는 32비트
  워드 스트림이라 `char*` → `uint32_t*` reinterpret_cast.
- **정렬**: `pCode`는 4바이트 정렬 필요. `std::vector<char>`의 버퍼는 충분히 정렬돼 있어 OK
  (`std::string`으로 읽어도 OK). 직접 만든 버퍼면 주의.
- **`codeSize`는 4의 배수**여야 함(워드 단위). 정상 `.spv`는 항상 그렇다. 아니면 validation 에러.
- **수명**: shader module은 **pipeline 생성 후 파괴해도 된다**(pipeline이 바이트코드를 복사).
  → main에서 pipeline 만들 때 지역으로 두고, pipeline 생성 뒤 스코프 벗어나며 파괴돼도 무방.
- RAII: 소멸자에서 `destroyShaderModule`, device보다 먼저.

**main 사용**
```cpp
mpvk::ShaderModule vert{device, read_file(dir + "/triangle.vert.spv")};
mpvk::ShaderModule frag{device, read_file(dir + "/triangle.frag.spv")};
LogI("shader modules: vert={}, frag={}",
     vert.handle() ? "OK" : "NULL", frag.handle() ? "OK" : "NULL");
```
> 여기까지 validation 0개면 Step 2 완료.
> **미룬 것(나중):** GLSL string을 **런타임 컴파일**(`libshaderc`)해서 모듈 만들기 —
> hot-reload/동적 셰이더용. 지금 바이트 기반 생성자는 저수준 프리미티브로 유지하고,
> 나중에 "바이트를 만드는 경로"(`from_glsl`/shaderc)만 얹으면 됨. 현재 설계 변경 불필요.

### Step 3~7
스텝 2 통과 후 render pass → framebuffer → pipeline → draw → 리사이즈 순으로 이어서 안내.
(각 스텝 작게: "만들고 → 실행 확인 → 다음".)
