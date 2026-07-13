# 04_clear_screen

> Phase 2 · 상태: ⬜ 예정

## Goal

swapchain을 만들어 창을 **매 프레임 특정 색으로 지우고(clear) 화면에 present** 한다.
삼각형은 아직 없다. 하지만 여기서 만든 **acquire → 기록 → submit → present + 동기화**
루프가 이후 모든 렌더링 샘플의 심장이 된다.

## What this sample teaches

- **swapchain(`VkSwapchainKHR`)**: surface에 대응하는 present용 이미지들의 집합.
  capabilities/format/present mode를 질의해 **골라서** 만든다.
- swapchain **이미지 + 이미지 뷰**.
- **command pool / command buffer**: GPU에 시킬 명령을 기록하는 그릇.
- **이미지 레이아웃 전환(barrier)**: clear/present에 맞는 레이아웃으로 바꾸기.
- **동기화**: semaphore(GPU↔GPU), fence(CPU↔GPU), **frames-in-flight**.
- 렌더 루프 한 프레임의 정석: **acquire → record → submit → present**.
- swapchain **재생성**(리사이즈/out-of-date) 개념.

## 배우는 개념

### swapchain — present용 이미지들의 링
화면에 뭔가 내보내려면, GPU가 그린 이미지를 present 큐로 넘겨야 한다. 그 이미지들의
집합이 swapchain이다. surface에 대해 만들며, 만들기 전에 surface가 뭘 지원하는지
질의해서 고른다:
- **capabilities** (`getSurfaceCapabilitiesKHR`): min/max 이미지 수, 현재/최대 extent.
- **format** (`getSurfaceFormatsKHR`): 색 포맷+색공간. 보통 `B8G8R8A8Srgb` + `SrgbNonlinear` 선호.
- **present mode** (`getSurfacePresentModesKHR`): `eFifo`(항상 지원, vsync)를 기본으로,
  `eMailbox`(저지연) 있으면 선호.
- **extent**: 픽셀 해상도. `currentExtent`가 유효하면 그대로, 아니면 GLFW framebuffer 크기로 clamp.

### swapchain 이미지 & 이미지 뷰
swapchain 이미지는 우리가 만드는 게 아니라 `getSwapchainImagesKHR`로 **받는다**(소유는
swapchain). 각 이미지에 대해 접근용 **image view**를 만든다(뷰는 우리가 소유/파괴).

**왜 image view가 필요한가**
- `VkImage` = 픽셀 데이터 + 메타(format/크기/mip/layer). 파이프라인·프레임버퍼는
  **이미지에 직접 접근하지 못한다.**
- `VkImageView` = 그 이미지를 "어떻게 해석하고 어느 부분을 볼지" 정의하는 **렌즈**.
  렌더 타깃 부착/샘플링/스토리지 모두 view를 통한다.
- 그래서 swapchain 이미지마다 color view를 만든다.
- (참고: 이 샘플의 clear는 `vkCmdClearColorImage`로 **이미지에 직접** 하므로 view는
  05 render pass부터 실제로 쓰인다. 지금은 표준 셋업 + 05 대비.)

**`vk::ImageViewCreateInfo` 필드**
- `image`: 이 뷰가 가리키는 `VkImage`.
- `viewType`: 차원 — swapchain은 `e2D` (`eCube`/`e2DArray` 등도 있음).
- `format`: 텍셀 해석 포맷. 보통 이미지(=swapchain) 포맷과 동일.
- `components`: 채널 swizzle(rgba 재배치). 기본 identity → 생략.
- `subresourceRange`: 이미지의 어느 부분을 뷰가 덮는가.
  - `aspectMask`: color/depth/stencil → swapchain은 `eColor`.
  - `baseMipLevel`/`levelCount`: mip 범위 → swapchain은 `0 / 1`(mip 1개).
  - `baseArrayLayer`/`layerCount`: layer 범위 → `0 / 1`(비배열).
- 즉 `ImageSubresourceRange{eColor, 0, 1, 0, 1}` = "color, mip 0부터 1개, layer 0부터 1개".

### command pool & command buffer
GPU는 직접 호출로 일하지 않는다. **command buffer에 명령을 기록**해 queue에 submit한다.
command buffer는 **command pool**에서 할당한다(pool은 특정 queue family에 묶임).

### 이미지 레이아웃과 barrier
이미지는 용도별 **레이아웃**(undefined / transfer-dst / present-src ...)을 가진다.
render pass 없이 지우려면 직접 전환한다:
`UNDEFINED → TRANSFER_DST_OPTIMAL`(clear) → `TRANSFER_DST → PRESENT_SRC_KHR`(present).
전환은 `vkCmdPipelineBarrier`(vulkan.hpp `pipelineBarrier2`/`ImageMemoryBarrier`)로.
> clear 자체는 `vkCmdClearColorImage`. (render pass의 load-op clear는 **05**에서.)

### 동기화 — 이 샘플의 진짜 난이도
GPU는 비동기다. 순서를 강제하지 않으면 "아직 안 그린 이미지를 present"하는 참사가 난다.
- **semaphore** (GPU↔GPU, 큐 작업 간 순서):
  - `imageAvailable`: acquire가 끝나(이미지 준비됨) → submit이 이걸 **wait**.
  - `renderFinished`: submit이 끝나면 **signal** → present가 이걸 **wait**.
- **fence** (CPU↔GPU): submit이 끝났는지 **CPU가** 알기 위해. 다음 프레임에서 이
  command buffer/자원을 재사용하기 전에 `waitForFences`.
- **frames-in-flight**: 위 (command buffer + 2 semaphore + fence) 세트를 N개(보통 2) 두고
  번갈아 써서 CPU와 GPU가 겹쳐 일하게 한다. N=1이면 매 프레임 CPU가 GPU를 기다려 느림.

### 한 프레임의 흐름
```
waitForFences(frame.fence)            // 이 슬롯의 이전 작업 끝날 때까지
resetFences(frame.fence)
acquireNextImageKHR(sem=imageAvailable)  // 다음 이미지 인덱스
record(cmd): barrier→clear→barrier
submit(cmd, wait=imageAvailable, signal=renderFinished, fence=frame.fence)
presentKHR(wait=renderFinished, imageIndex)
```

### swapchain 재생성 (out-of-date)
창 리사이즈/최소화 시 `acquireNextImageKHR`/`presentKHR`가 `eErrorOutOfDateKHR`(또는
`eSuboptimalKHR`)를 준다. 이때 GPU idle 대기 후 swapchain(및 뷰)을 **다시 만든다**.
> 처음엔 고정 크기로 루프만 돌리고, 재생성은 그 다음에 붙여도 된다.

## Expected result

- 창이 뜨고 **일정 색(예: 짙은 파랑)으로 계속 채워진** 채 유지된다.
- 창 닫으면 깔끔히 종료, **validation 0개**.
- (재생성 붙이면) 리사이즈해도 안 깨진다.

## Files to touch

- `cpp/include/mpvk/swapchain.hpp` / `src/swapchain.cpp` (신규)
- command pool / 동기화 헬퍼 (신규 — 최소한으로; 우선 main/작은 클래스)
- `cpp/CMakeLists.txt` (새 소스 등록)
- `samples/04_clear_screen/main.cpp` + `CMakeLists.txt` (신규)
- `samples/CMakeLists.txt` (add_subdirectory)
- 기존 `Device`에 필요한 접근자 추가 가능(예: present/graphics queue는 이미 있음)

## mpvk 설계 (additions) — 최소한으로

### `mpvk::Swapchain`
- 입력: `Device`, `PhysicalDevice`(capabilities 질의), `Surface`, `Window`(extent).
- 보관: `vk::SwapchainKHR`, `vk::Format`, `vk::Extent2D`, `images`, `imageViews`.
- 제공: `handle()`, `format()`, `extent()`, `images()`, `image_views()`.
- RAII: 뷰들 파괴 후 swapchain 파괴(device 소유이므로 device보다 먼저).
- (재생성은 메서드 `recreate()` 또는 파괴+재생성으로 — 나중 단계.)

### command pool / command buffer
- graphics family로 `CommandPool` 하나. frames-in-flight 개수만큼 command buffer 할당.
- 작게: `mpvk::CommandPool`(pool 소유 + allocate 헬퍼) 정도. 큰 추상화 금지.

### 동기화
- per-frame: `imageAvailable`(semaphore), `renderFinished`(semaphore), `inFlight`(fence).
- 우선 main에서 vector로 들고 있어도 됨(반복 나오면 나중에 헬퍼로).

넣지 말 것: render pass, framebuffer, graphics pipeline, shader(전부 05). RenderGraph 류 금지.

## Implementation steps (큰 순서 — 각 단계 후 실행 확인)

1. **swapchain 지원 질의** — capabilities/formats/present modes 받아 로그(03에서 미룬 것).
2. **`mpvk::Swapchain` 생성** — format/present mode/extent 선택 → `createSwapchainKHR`
   → 이미지 획득 → 이미지 뷰 생성. 여기까지 만들고 파괴만 확인(아직 렌더 X).
3. **command pool + command buffer** 할당.
4. **동기화 객체** — frames-in-flight(=2) 만큼 semaphore 2개 + fence.
5. **렌더 루프 한 프레임** — acquire → record(barrier→clear→barrier) → submit → present.
   고정 크기로 색이 채워지는지 확인.
6. **frames-in-flight** 로 N개 슬롯 순환.
7. **swapchain 재생성** — out-of-date/suboptimal + 리사이즈 처리.
8. 종료 시 `device.waitIdle()` 후 파괴, validation 0개 확인.

## 구현 시 주의점 (common mistakes)

- **파괴 순서**: imageViews → swapchain → (surface → instance). device.waitIdle 후 파괴.
  동기화 객체/파이프라인 자원은 device보다 먼저.
- **종료 전 `device.waitIdle()`**: GPU가 쓰는 중인 자원을 파괴하면 validation 에러.
- **present mode 기본은 `eFifo`** (항상 지원). Mailbox는 있을 때만.
- **extent clamp**: `currentExtent.width == 0xFFFFFFFF`면 직접 정해야 함(GLFW
  `glfwGetFramebufferSize`로 픽셀 크기, min/max로 clamp). Retina(고해상도) 주의.
- **imageCount**: `minImageCount + 1` 권장, `maxImageCount`(0이면 무제한) 넘지 않게 clamp.
- **acquire/present 반환값 확인**: `eErrorOutOfDateKHR`/`eSuboptimalKHR` → 재생성.
  vulkan.hpp는 이걸 예외로 던질 수 있으니 처리 방식 주의(비-예외 경로 필요할 수 있음).
- **semaphore vs fence 혼동 금지**: GPU 간 순서=semaphore, CPU가 기다림=fence.
- **최소화(minimize) 시 extent 0**: 크기 0이면 루프에서 기다렸다가 복구.

## Self-check

- [ ] 창이 일정 색으로 채워지고 validation 0개인가?
- [ ] 종료 시 `waitIdle` 후 파괴하는가? (파괴 순서: views→swapchain→device)
- [ ] semaphore/fence 역할을 구분해 쓰는가?
- [ ] frames-in-flight 슬롯을 순환하는가?
- [ ] (선택) 리사이즈 시 재생성되는가?

## What to send for review

- `swapchain.hpp`/`swapchain.cpp` (특히 선택 로직 + 파괴 순서)
- command pool/동기화 생성 부분
- 렌더 루프 한 프레임(acquire/submit/present + 대기) 부분
- 실행 결과(색 채워짐 + validation 0개)

## 새 개념 (완료 시 concepts.md에 추가)

- swapchain (`VkSwapchainKHR`) + 지원 질의/선택
- swapchain 이미지 & image view
- command pool / command buffer
- 이미지 레이아웃 전환 / pipeline barrier
- 동기화: semaphore, fence, frames-in-flight
- 렌더 루프(acquire→submit→present)
- swapchain 재생성(out-of-date)

---

## 구현 가이드 (직접 해보기)

> 첫 목표는 **스텝 1~2**: surface 지원을 질의해 로그로 찍고, `mpvk::Swapchain`을
> 만들어 이미지/뷰까지 확보(아직 렌더 X). 여기가 되면 command/sync/loop로 넘어간다.

### Step 1 — swapchain 지원 질의 (03에서 이월)
`PhysicalDevice`(또는 main)에서 surface에 대해:
```cpp
auto caps    = gpu.getSurfaceCapabilitiesKHR(surface);
auto formats = gpu.getSurfaceFormatsKHR(surface);
auto modes   = gpu.getSurfacePresentModesKHR(surface);
LogI("surface: formats={}, present_modes={}, images={}..{}",
     formats.size(), modes.size(), caps.minImageCount, caps.maxImageCount);
```
- 이 값들이 다음 스텝(swapchain 생성)의 입력이다. 우선 개수/범위만 확인.

### Step 2 — `mpvk::Swapchain` (최소 구현)

> 목표: swapchain + 이미지 뷰까지 만들고 **파괴만** 해도 validation 0개. 렌더는 Step 5부터.
> 지금은 **최소**로 — present mode 우선순위 리스트/vsync/`Settings` 전역/Unorm+수동감마는
> 전부 **나중**(반복 나타나면 승격). 아래는 생성자를 8조각으로 나눈 세부.

**헤더 요구사항 (swapchain.hpp)**
- include: `<vector>` + `<vulkan/vulkan.hpp>` (중복 `"vulkan/vulkan.hpp"` 금지).
- 타입은 전부 참조 인자 → **전방 선언만**(`class PhysicalDevice/Device/Surface/Window;`),
  `mpvk/*.hpp` include 하지 말 것(결합도↓).
- **복사 금지**(`= delete`) — swapchain·image view 소유.
- 소멸자에서 파괴하려면 **`vk::Device device_{nullptr};` 멤버 보관**(Surface가 instance 들고 있던 것과 동일 이유).
- 멤버: `handle_`, `format_`, `extent_`, `images_`, `image_views_`(underscore 일관).
- 접근자: `handle()`, `format()`, `extent()`, `const std::vector<vk::ImageView>& image_views()`.

**(선택) Window 헬퍼** — extent용 픽셀 크기. GLFW를 Window에 가둠:
```cpp
vk::Extent2D Window::framebuffer_size() const {   // glfwGetFramebufferSize (pixels!)
  int w = 0, h = 0;
  glfwGetFramebufferSize(handle_, &w, &h);
  return {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
}
```

**생성자 8조각**
1. **질의**: `gpu.handle().getSurface{Capabilities,Formats,PresentModes}KHR(surface.handle())`.
2. **format**: `eB8G8R8A8Srgb`+`eSrgbNonlinear` 선호, 없으면 `formats[0]`(항상 유효).
   - `...Srgb` = 하드웨어 자동 감마. `...Unorm`+수동 감마는 나중(셰이더 단계).
3. **present mode** (최소): `eFifo` 기본(항상 지원), `eMailbox` 있으면만 선호.
   ```cpp
   vk::PresentModeKHR mode = vk::PresentModeKHR::eFifo;
   for (auto m : modes) { if (m == vk::PresentModeKHR::eMailbox) { mode = m; break; } }
   ```
4. **extent**: `caps.currentExtent.width != UINT32_MAX`면 그대로, 아니면
   `window.framebuffer_size()` → `std::clamp`(min/maxImageExtent). `<algorithm>`,`<cstdint>` 필요.
   - **Retina 함정**: 논리 크기(800×600) 아니라 **프레임버퍼 픽셀** 써야 함.
5. **imageCount**: `caps.minImageCount + 1`, `maxImageCount>0`면 그 값으로 clamp.
   - +1 이유: 최소만 요청하면 다음 이미지 획득 시 드라이버 대기(스톨) 가능. maxImageCount==0=무제한.
6. **create info**:
   ```cpp
   ci.imageUsage = vk::ImageUsageFlagBits::eColorAttachment
                 | vk::ImageUsageFlagBits::eTransferDst;   // cleared via transfer op
   uint32_t gfx = gpu.graphics_family();
   auto     pf  = gpu.present_family();                    // std::optional
   uint32_t families[] = {gfx, pf ? *pf : gfx};
   if (pf && *pf != gfx) { ci.imageSharingMode = eConcurrent; ci.setQueueFamilyIndices(families); }
   else                  { ci.imageSharingMode = eExclusive; }  // 우리 M4: 0/0 → Exclusive
   ci.preTransform=caps.currentTransform; ci.compositeAlpha=eOpaque;
   ci.presentMode=mode; ci.clipped=VK_TRUE; ci.oldSwapchain=nullptr;
   ```
   - `eTransferDst`: render pass 없이 `vkCmdClearColorImage`(transfer 계열)로 지울 거라.
   - Concurrent vs Exclusive: 두 family 다를 때만 Concurrent(공유). 같으면 Exclusive(소유권 전환 불필요).
7. **생성 → 이미지 → 뷰**: `createSwapchainKHR` → `getSwapchainImagesKHR`(swapchain 소유, 파괴 X)
   → 각 이미지 `createImageView`(`e2D`, format_, `ImageSubresourceRange{eColor,0,1,0,1}`).
   `device_ = device.handle();` 저장.
8. **소멸자**: `for (v : image_views_) device_.destroyImageView(v);` → `device_.destroySwapchainKHR(handle_)`.

**검증**: main에 `mpvk::Swapchain swapchain{physical_device, device, surface, window};`
+ `LogI("swapchain: {} images", swapchain.image_views().size());` → 창 뜨고 validation 0개.

### Step 3~8
스텝 2가 리뷰 통과하면 command pool → 동기화 → 렌더 루프 순으로 이어서 안내한다.
(각 스텝은 작게: "만들고 → 실행 확인 → 다음".)

> **미룬 것(나중):** present mode 우선순위 리스트 + `pick` 헬퍼, vsync 토글(생성자 인자),
> `Settings` 전역/`WindowInfo`/`RenderContext`, Unorm+수동 감마. 필요해질 때 도입.
