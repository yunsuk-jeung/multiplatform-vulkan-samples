# Roadmap — 무엇을 어떤 순서로

원칙: **한 샘플 = 하나의 새 개념 + 그 개념에 필요한 최소한의 프레임워크 조각.**
큰 점프(특히 "빈 화면 지우기"와 "첫 삼각형")는 일부러 여러 단계로 쪼갠다.

각 단계에는:
- **개념**: 이 샘플에서 새로 배우는 Vulkan 개념
- **mpvk 추가**: 이때 프레임워크에 처음 들어가는 클래스/헬퍼
- **대응**: Khronos Vulkan-Samples 중 참고할 샘플

범례: ✅ 완료 · 🚧 진행중 · ⬜ 예정

---

## Phase 0 — 토대 (Foundations)

### ✅ [01_device_info](samples/01_device_info.md)
- **개념**: Vulkan instance 생성, physical device 열거, device properties 읽기.
  macOS에서 MoltenVK(portability) 드라이버를 opt-in 하는 법.
- **mpvk 추가**: `Instance`

---

## Phase 1 — 디바이스와 큐 (Device & Queues)

### ⬜ [02_logical_device](samples/02_logical_device.md)
- **개념**: queue family(그래픽/컴퓨트/전송)의 의미, 원하는 큐를 가진 physical device
  고르기, logical device(`VkDevice`) 생성, device에서 queue 얻기,
  device extension / feature 활성화.
- **mpvk 추가**:
  - `PhysicalDevice` — 선택 헬퍼 + queue family 조회
  - `Device` — `VkDevice` 소유 + queue 핸들 보관
- 아직 화면 없음. "GPU에게 일을 시킬 준비" 단계.

---

## Phase 2 — 화면 띄우기 (Window → Surface → Swapchain)

### ⬜ 03_window_surface
- **개념**: GLFW로 창 열기, `VkSurfaceKHR` 생성, surface가 지원하는
  format/present mode 질의. 창의 present 큐 family 확인.
- **mpvk 추가**:
  - `Window` — GLFW 래퍼 (창 + 이벤트 루프)
  - surface 생성 (Device/Instance에 통합)
- GLFW를 `third_party`에 추가하는 첫 외부 의존성 단계.

### ⬜ 04_clear_screen
- **개념**: 렌더 루프의 뼈대. swapchain 이미지 acquire → command buffer에
  clear 기록 → submit → present. semaphore/fence로 CPU-GPU, GPU-GPU 동기화.
  frames-in-flight 개념.
- **mpvk 추가**:
  - `Swapchain` — swapchain + 이미지/뷰
  - `CommandPool` / command buffer 헬퍼
  - 동기화 프리미티브 헬퍼 (semaphore, fence)
  - `RenderLoop` 혹은 프레임 관리 헬퍼
- 삼각형은 아직 없지만, **여기서 만든 루프가 이후 모든 샘플의 심장**이다.

---

## Phase 3 — 첫 삼각형 (Hello Triangle)

### ⬜ 05_hello_triangle
- **개념**: render pass, framebuffer, graphics pipeline(고정+프로그래머블 스테이지),
  shader module(SPIR-V), viewport/scissor. 정점은 셰이더에 하드코딩.
- **mpvk 추가**:
  - `RenderPass`
  - `Framebuffer`
  - `ShaderModule` (+ CMake에서 GLSL→SPIR-V 컴파일 통합; SDK의 `glslc` 사용)
  - `GraphicsPipeline`
- Khronos repo의 첫 샘플 **Hello Triangle** 에 대응.
- 참고: Vulkan 1.3+ 의 **dynamic rendering**을 쓰면 render pass/framebuffer를
  생략할 수 있다(모던 방식). 학습을 위해 **고전 render pass 먼저**, 이후
  별도 샘플에서 dynamic rendering 버전을 다룬다.

---

## Phase 4 — 정점 버퍼와 메모리 (Vertex Buffers & Memory)

### ⬜ 06_vertex_buffer
- **개념**: `VkBuffer` + device memory, memory type/heap, host-visible vs
  device-local, staging buffer로 GPU 전용 메모리에 업로드. vertex input 바인딩/어트리뷰트.
- **mpvk 추가**: `Buffer`, 메모리 할당 헬퍼(우선 직접 구현)

### ⬜ 07_index_buffer
- **개념**: index buffer, `vkCmdDrawIndexed`, 정점 재사용.
- **mpvk 추가**: (06 재사용)

---

## Phase 5 — 유니폼과 디스크립터 (Uniforms & Descriptors)

### ⬜ 08_uniform_buffer
- **개념**: MVP 행렬 전달, uniform buffer, descriptor set/layout/pool,
  프레임마다 유니폼 갱신. 회전하는 도형. (수학용 `glm` 도입)
- **mpvk 추가**: `DescriptorSetLayout`, `DescriptorPool`, descriptor set 헬퍼

---

## Phase 6 — 텍스처 (Textures)

### ⬜ 09_texture_mapping
- **개념**: 이미지 파일 로드(`stb_image`), staging→device-local 이미지 복사,
  image layout transition(파이프라인 배리어), sampler, combined image sampler.
- **mpvk 추가**: `Image`(device-local), `Sampler`

---

## Phase 7 — 깊이와 3D (Depth & 3D)

### ⬜ 10_depth_buffer
- **개념**: depth attachment, depth test, render pass에 depth 추가.
### ⬜ 11_model_loading
- **개념**: glTF/OBJ 모델 로드(`tinygltf`/`tinyobjloader`), 실제 메시 렌더.

---

## Phase 8 이후 — Khronos 샘플 직접 따라가기

여기까지 오면 `mpvk`가 웬만한 걸 다 갖춰서, 이제 Khronos "API samples"를
직접 읽고 구현할 수 있다. 관심/난이도 순으로 취사선택:

- Dynamic uniform buffers
- Instancing
- Compute (compute pipeline, SSBO)
- **Dynamic rendering** (Vulkan 1.3, render pass 없이 — 모던 방식)
- Push constants
- Multisampling (MSAA)
- Shadow mapping
- Deferred rendering
- HDR / bloom
- Tessellation, geometry shader (MoltenVK 지원 여부 주의)

---

## 주요 결정 사항 (defaults)

| 항목 | 선택 | 이유 |
|---|---|---|
| 윈도잉 | **GLFW** | 크로스플랫폼, Vulkan surface 지원 간단 |
| C++ 바인딩 | **vulkan.hpp** | 이미 사용중, RAII/타입안전 |
| 메모리 할당 | **직접 → 나중에 VMA** | 처음엔 원리 이해, 규모 커지면 VMA 도입 |
| 렌더링 | **고전 render pass → dynamic rendering** | 기초 먼저, 모던은 별도 샘플 |
| 셰이더 | **GLSL → glslc → SPIR-V** (CMake 통합) | SDK에 이미 포함 |
| 수학 | **glm** | 사실상 표준 |
