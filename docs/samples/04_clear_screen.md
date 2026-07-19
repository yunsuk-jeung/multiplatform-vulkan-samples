# 04_clear_screen

> Phase 2 · 상태: ✅ 완료
> (swapchain+뷰 ✅ / command pool ✅ / 동기화 ✅ / 렌더 루프 ✅ / frames-in-flight ✅ / 재생성 ✅)
> 짙은 파란 화면 렌더, 렌더 루프 중 validation 0개. frames-in-flight=2 순환, 리사이즈 재생성(macOS는 GLFW 콜백 플래그로 트리거).
> 참고: swapchain은 device extension `VK_KHR_swapchain` 필요 → present family 있을 때 Device에서 활성화.

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

### frames-in-flight vs swapchain 이미지 수 — 다른 축
둘은 **다른 것을 센다**. 억지로 같게 맞추지 않는다.
- **swapchain 이미지 수**(예 3): present 엔진이 굴리는 화면 버퍼 개수. **드라이버**가 결정.
- **frames-in-flight**(보통 2): CPU가 GPU보다 얼마나 앞서 달릴지 = CPU 측 자원 세트 수.
  **내가** 정하는 지연/처리량 튜닝 값.

왜 안 맞추나: 역할이 다르고, frames-in-flight를 키우면 CPU 선행이 늘어 **입력 지연↑**(2가 균형점).
게다가 acquire가 주는 **이미지 인덱스(img)는 내가 못 고르므로** "슬롯 f == img"로 가정 불가.
→ 그래서 인덱싱이 갈린다: `imageAvailable`·`inFlight`·command buffer = **f(2)**,
`renderFinished` = **img(이미지 수)**.

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

1. ✅ **swapchain 지원 질의** — `Swapchain` 생성자 안에서 caps/formats/modes 조회.
2. ✅ **`mpvk::Swapchain` 생성** — 선택 로직(익명 namespace 헬퍼) → `createSwapchainKHR`
   → 이미지 획득 → `create_image_views()`. 소멸자 뷰→swapchain. `3 images`, validation 0개.
   (Device에 `VK_KHR_swapchain` 확장 추가 필요했음 — present family 있을 때.)
3. ✅ **command pool + command buffer** — graphics family, `allocate(kFramesInFlight)`.
4. ✅ **동기화 객체** — `imageAvailable`·`inFlight`는 frame(f), `renderFinished`는 image(img).
5. ✅ **렌더 루프 한 프레임** — acquire → reset → barrier/clear/barrier → submit → present.
   짙은 파랑, validation 0개(f=0 고정). 종료 시 `waitIdle` + sync destroy.
6. ✅ **frames-in-flight** 로 N개 슬롯 순환(`f = (f+1) % kFramesInFlight`).
7. ✅ **swapchain 재생성** — out-of-date/suboptimal(예외/반환) + GLFW 리사이즈 콜백 플래그.
8. ✅ 종료 시 `device.waitIdle()` 후 파괴, validation 0개 확인.

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

### Step 2 — `mpvk::Swapchain` (최소 구현) ✅

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

### Step 3 — command pool + command buffers ✅

GPU에 명령을 기록할 그릇 준비. 아직 기록/제출은 안 함(Step 5) — 만들고 validation 0개면 성공.

**개념**
- **command pool**: command buffer 메모리의 출처. 특정 **queue family**에 묶이며,
  거기 기록한 명령은 그 family의 큐에만 submit 가능 → graphics family로 만든다.
- **command buffer**: 실제 명령(barrier/clear/draw)을 기록하는 버퍼. pool에서 할당,
  **pool 파괴 시 함께 해제**(개별 free 불필요).
- pool flag `eResetCommandBuffer`: 매 프레임 command buffer를 **개별 reset** 후 재기록하려고 켠다.

**`mpvk::CommandPool` (최소)**
```cpp
class CommandPool {
public:
  CommandPool(const Device& device, uint32_t queue_family);
  ~CommandPool();
  CommandPool(const CommandPool&)            = delete;
  CommandPool& operator=(const CommandPool&) = delete;

  vk::CommandPool                handle() const { return pool_; }
  std::vector<vk::CommandBuffer> allocate(uint32_t count) const;  // ePrimary

private:
  vk::Device      vk_device_{nullptr};
  vk::CommandPool pool_{nullptr};
};
```
- 생성자: `vk::CommandPoolCreateInfo{flags=eResetCommandBuffer, queueFamilyIndex=queue_family}`
  → `device.handle().createCommandPool(...)`. `vk_device_` 저장.
- `allocate`: `vk::CommandBufferAllocateInfo{pool_, vk::CommandBufferLevel::ePrimary, count}`
  → `vk_device_.allocateCommandBuffers(...)`.
- 소멸자: `vk_device_.destroyCommandPool(pool_)` (command buffer 자동 해제).

**main**
```cpp
mpvk::CommandPool command_pool{device, gpu.graphics_family()};
auto command_buffers = command_pool.allocate(2);  // 2 = future frames-in-flight
LogI("command buffers: {}", command_buffers.size());
```
> 여기까지 validation 0개면 Step 3 완료. 기록은 Step 5.

> **실전 노트**
> - 풀은 **graphics family**로 만든다(렌더 명령은 graphics 큐에 submit). present family
>   아님 — present는 command buffer를 안 쓴다(`queuePresentKHR`). graphics==present인
>   GPU(M4=0/0)에선 present로 해도 우연히 통과하지만 개념/이식성상 틀림.
> - command buffer 개수는 **frames-in-flight(=2)** 로. 이미지 수(3)와 별개 — 이미지
>   인덱스는 acquire로 따로 받는다.
> - windowed면 present가 보장되므로(PhysicalDevice가 surface 받고 present 없으면 throw)
>   main에서 present_family 재확인 불필요. optional 체크는 headless 경로용.

### Step 4 — 동기화 객체 (semaphore + fence) ✅

frames-in-flight(=2)만큼 동기화 3종 세트를 만든다. 아직 렌더 X — 만들고 파괴만, validation 0개.

**개념**
- **semaphore**: 큐 작업 간 순서(GPU↔GPU). CPU는 상태를 못 본다.
- **fence**: GPU 작업 완료를 **CPU가** 아는 수단. 이 슬롯을 재사용하기 전 `waitForFences`.

**만들 것 — 인덱싱 기준이 다르다 (중요, validation이 잡음)**
| 객체 | 인덱스 | 개수 |
|---|---|---|
| `imageAvailable` semaphore | frame `f` | kFramesInFlight(2) |
| `inFlight` fence | frame `f` | kFramesInFlight(2) |
| `renderFinished` semaphore | **image `img`** | **swapchain 이미지 수(3)** |

- `imageAvailable` — acquire 완료 신호 → submit이 wait. 프레임 슬롯 재사용은 fence로 막음.
- `inFlight` fence — 이 슬롯의 GPU 작업 종료를 CPU가 대기.
- `renderFinished` — submit 완료 → present가 wait. **present에 묶이고 present 완료는 CPU가
  fence로 못 보므로, 프레임(f)이 아니라 이미지(img)마다 따로** 둬야 한다. `render_finished[f]`로
  하면 `VUID-vkQueueSubmit-pSignalSemaphores-00067` ("may still be in use by VkSwapchainKHR").
  → submit `setSignalSemaphores(render_finished[img])`, present `setWaitSemaphores(render_finished[img])`.

**구현 (우선 main에 vector로; Step 6에서 per-frame 구조체로 이동)**
```cpp
constexpr uint32_t kFramesInFlight = 2;
auto dev = device.handle();

// per frame-in-flight
std::vector<vk::Semaphore> image_available;  // (f)
std::vector<vk::Fence>     in_flight;        // (f)
for (uint32_t i = 0; i < kFramesInFlight; ++i) {
  image_available.push_back(dev.createSemaphore({}));
  in_flight.push_back(dev.createFence(
      {vk::FenceCreateFlagBits::eSignaled}));  // signaled: first frame won't block
}
// per swapchain image (indexed by acquired img, NOT by f)
std::vector<vk::Semaphore> render_finished;
for (size_t i = 0; i < swapchain.image_views().size(); ++i) {
  render_finished.push_back(dev.createSemaphore({}));
}
// teardown: destroySemaphore (image_available + render_finished) / destroyFence, each
```
> **핵심 함정**: fence를 `eSignaled`로 생성 — 첫 프레임 `waitForFences`가 즉시 통과.
> 안 그러면 첫 프레임에서 영원히 대기(아무도 signal 안 함).
> **teardown**: semaphore/fence는 raw 핸들 → `std::vector` 소멸로 파괴 안 됨.
> device 파괴 전 `device.waitIdle()` 후 `destroySemaphore`/`destroyFence`를 직접 호출.

### Step 5 — 렌더 루프 한 프레임 (acquire → clear → present) ✅

이 샘플의 핵심. 우선 **frame slot 0 고정**으로 한 프레임 완성(순환은 Step 6).

**사전 준비**: `Swapchain`에 이미지 접근자 추가 — clear는 이미지에 직접 하므로.
```cpp
const std::vector<vk::Image>& images() const { return images_; }
```

**draw_frame — A~F 순서로 (slot f = 0)**

루프 위에서 한 번 준비:
```cpp
vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
vk::ClearColorValue color{std::array<float, 4>{0.0f, 0.0f, 0.2f, 1.0f}};  // dark blue
auto vk_device = device.handle();
```

A. 이 슬롯 이전 작업 대기:
```cpp
(void) vk_device.waitForFences(in_flight[f], vk::True, UINT64_MAX);
```
B. 다음 이미지 획득 (준비되면 `image_available` signal):
```cpp
uint32_t img = vk_device.acquireNextImageKHR(
    swapchain.handle(), UINT64_MAX, image_available[f], nullptr).value;
```
C. fence reset (acquire 성공 후, submit 직전):
```cpp
vk_device.resetFences(in_flight[f]);
```
D. command buffer 기록 (barrier → clear → barrier):
```cpp
auto& cb = cmd_buffers[f];
cb.reset();
cb.begin(vk::CommandBufferBeginInfo{});

// UNDEFINED -> TRANSFER_DST (prepare for clear)
vk::ImageMemoryBarrier to_dst{};
to_dst.oldLayout           = vk::ImageLayout::eUndefined;
to_dst.newLayout           = vk::ImageLayout::eTransferDstOptimal;
to_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
to_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
to_dst.image               = swapchain.images()[img];
to_dst.subresourceRange    = range;
to_dst.dstAccessMask       = vk::AccessFlagBits::eTransferWrite;
cb.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                   vk::PipelineStageFlagBits::eTransfer,
                   {}, {}, {}, to_dst);

cb.clearColorImage(swapchain.images()[img],
                   vk::ImageLayout::eTransferDstOptimal, color, range);

// TRANSFER_DST -> PRESENT_SRC (prepare for present)
vk::ImageMemoryBarrier to_present{};
to_present.oldLayout           = vk::ImageLayout::eTransferDstOptimal;
to_present.newLayout           = vk::ImageLayout::ePresentSrcKHR;
to_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
to_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
to_present.image               = swapchain.images()[img];
to_present.subresourceRange    = range;
to_present.srcAccessMask       = vk::AccessFlagBits::eTransferWrite;
cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                   vk::PipelineStageFlagBits::eBottomOfPipe,
                   {}, {}, {}, to_present);

cb.end();
```
E. submit (여기서 `in_flight` fence를 signal):
```cpp
vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eTransfer;
vk::SubmitInfo submit{};
submit.setWaitSemaphores(image_available[f]);
submit.setWaitDstStageMask(wait_stage);
submit.setCommandBuffers(cb);
submit.setSignalSemaphores(render_finished[img]);   // per image, not f
device.graphics_queue().submit(submit, in_flight[f]);
```
F. present (⚠️ `setSwapchains`에 `handle()` 임시값 직접 금지 → 로컬에 담기):
```cpp
vk::SwapchainKHR sc = swapchain.handle();  // named lvalue (NoTemporaries setter)

vk::PresentInfoKHR present{};
present.setWaitSemaphores(render_finished[img]);   // per image, not f
present.setSwapchains(sc);
present.setImageIndices(img);
(void) device.present_queue().presentKHR(present);

window.poll_events();
```
> `set...`는 `ArrayProxyNoTemporaries` → 값 반환(임시)을 거부(dangling 방지). cpp_notes 참고.

한 줄 요약: **A wait → B acquire → C reset → D record(barrier/clear/barrier) → E submit → F present**.

**개념/함정**
- barrier 2개로 레이아웃 전환: `UNDEFINED→TRANSFER_DST_OPTIMAL`(clear 대상) →
  `TRANSFER_DST→PRESENT_SRC_KHR`(present). `cb.pipelineBarrier(...)`.
- submit `waitDstStageMask = eTransfer` — image_available를 transfer 단계에서 대기(clear가 transfer).
- clear 색: `vk::ClearColorValue{std::array{0.0f, 0.0f, 0.2f, 1.0f}}`.
- vulkan.hpp `acquireNextImageKHR`/`presentKHR`는 `eErrorOutOfDateKHR`를 **예외로** 던질 수 있음
  → 지금은 무시, Step 7에서 재생성 처리.
- `waitForFences`는 `[[nodiscard]]` → `(void)` 또는 결과 확인.
- **fence reset은 submit 직전에** — `waitForFences` 직후 무조건 reset하면, submit을
  건너뛰는 경로(out-of-date 등)에서 fence가 unsignaled로 남아 다음 wait가 **데드락**.
  순서: wait → acquire → **reset** → record → **submit(=fence signal)** → present.
  submit이 없으면 fence를 다시 signal할 주체가 없어 2번째 프레임부터 hang.

### Step 6 — frames-in-flight 순환

지금은 `f = 0` 고정이라 CPU가 매 프레임 GPU를 기다린다(사실상 frames-in-flight=1).
슬롯을 순환시켜 CPU가 다음 프레임을 미리 기록하게 한다.

**바꿀 것 (한 줄)**: 프레임 끝에서 슬롯 전진.
```cpp
f = (f + 1) % kFramesInFlight;   // present 뒤, 루프 끝
```
- `image_available[f]`, `in_flight[f]`, `cmd_buffers[f]` 가 이제 슬롯을 번갈아 사용.
- `renderFinished[img_idx]`는 그대로 **이미지** 인덱스(f 아님).

**개념/확인**
- f=0 고정(=1 in flight)이면 wait→submit→wait로 CPU-GPU가 직렬. 2 슬롯이면 슬롯 0 GPU 작업
  중에 CPU가 슬롯 1 기록 → 겹쳐 돈다.
- 각 슬롯의 `inFlight` fence가 "이 슬롯 재사용 전 이전 작업 완료"를 보장(A의 waitForFences).
- 확인: 여전히 파란 화면 + validation 0개. (동작은 같아 보이지만 CPU/GPU 병렬성↑.)

### Step 7 — swapchain 재생성 (out-of-date/리사이즈) ✅

창 리사이즈 시 swapchain이 창 크기와 안 맞으면 `acquireNextImageKHR`/`presentKHR`가
`eErrorOutOfDateKHR`(또는 `eSuboptimalKHR`)를 준다. 이때 swapchain(+image views)을 다시 만든다.

**① Swapchain: 생성 로직을 재사용 가능하게 + `recreate()`**
지금 생성 로직이 생성자 몸통에 있으니, private `create(...)`로 빼서 ctor와 recreate가 공유:
```cpp
// swapchain.hpp
void recreate(const PhysicalDevice& gpu, const Window& window, const Surface& surface);
private:
  void create(const PhysicalDevice& gpu, const Window& window, const Surface& surface);
```
```cpp
// swapchain.cpp
Swapchain::Swapchain(...) : vk_device_{device.handle()} { create(gpu, window, surface); }

void Swapchain::recreate(const PhysicalDevice& gpu, const Window& window,
                         const Surface& surface) {
  vk_device_.waitIdle();
  for (auto v : image_views_) vk_device_.destroyImageView(v);
  if (handle_) vk_device_.destroySwapchainKHR(handle_);
  image_views_.clear();
  images_.clear();
  create(gpu, window, surface);   // 선택/생성/이미지/뷰 다시
}
```
> `create()`는 기존 생성자 몸통(질의→선택→create info→createSwapchainKHR→이미지→
> `create_image_views()`)을 그대로 옮긴 것.

**② main: 감지 → 재생성 → 프레임 skip**
vulkan.hpp는 `eErrorOutOfDateKHR`를 **예외(`vk::OutOfDateKHRError`)로 던진다**.
`eSuboptimalKHR`는 성공 코드라 반환된다. 그래서:
```cpp
// acquire: out-of-date면 재생성하고 이 프레임 건너뜀
uint32_t img_idx;
try {
  img_idx = vk_device.acquireNextImageKHR(
      swapchain.handle(), UINT64_MAX, image_available[f], nullptr).value;
} catch (const vk::OutOfDateKHRError&) {
  swapchain.recreate(gpu, window, surface);
  continue;                     // fence는 reset 전이라 여전히 signaled → OK
}
// ... reset → record → submit ...

// present: out-of-date/suboptimal이면 재생성
vk::Result pr = vk::Result::eSuccess;
try {
  pr = device.present_queue().presentKHR(present);
} catch (const vk::OutOfDateKHRError&) {
  pr = vk::Result::eErrorOutOfDateKHR;
}
if (pr == vk::Result::eErrorOutOfDateKHR || pr == vk::Result::eSuboptimalKHR
    || window.was_resized()) {           // ← macOS는 이 플래그가 유일한 트리거
  window.reset_resized();
  swapchain.recreate(gpu, window, surface);
}
```

**③ Window: 리사이즈 콜백 + 플래그 (macOS 필수 트리거)**
GLFW 콜백은 C 함수라 인스턴스에 접근하려면 **user pointer**를 쓴다.
```cpp
// window.hpp
bool was_resized() const { return framebuffer_resized_; }
void reset_resized() { framebuffer_resized_ = false; }
private:
  static void on_framebuffer_resize(GLFWwindow* w, int width, int height);
  bool framebuffer_resized_{false};
```
```cpp
// window.cpp — 생성자에서 glfwCreateWindow 뒤
glfwSetWindowUserPointer(handle_, this);                  // 콜백에서 Window* 찾으려고
glfwSetFramebufferSizeCallback(handle_, on_framebuffer_resize);

// static 콜백: user pointer로 this를 찾아 플래그만 세운다
void Window::on_framebuffer_resize(GLFWwindow* w, int, int) {
  auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
  self->framebuffer_resized_ = true;
}
```
- **static 멤버 함수**라 C 함수 포인터로 넘길 수 있고(=this 없음), Window의 private 멤버 접근 가능.
- 콜백은 `glfwPollEvents()` 중 **메인 스레드에서** 불리므로 별도 동기화 불필요.
- 헤더는 `struct GLFWwindow;` 전방 선언이라 `GLFWwindow*` 파라미터 OK.

main에선 위 ②의 present 후 `window.was_resized()`를 함께 검사(플래그 확인 후 `reset_resized()`).

**함정/주의**
- **fence 순서 덕에 안전**: acquire 실패는 `resetFences` 전이라 fence가 signaled로 남음 →
  다음 프레임 wait 즉시 통과(데드락 없음). 그래서 acquire out-of-date는 `continue`로 skip.
- **acquire의 suboptimal은 성공** → 그 프레임은 그냥 그리고, present 쪽에서 재생성 판단.
- **최소화(minimize)**: `framebuffer_size()`가 0×0이면 swapchain 생성 불가 → 크기가
  0이 아닐 때까지 `glfwWaitEvents()`로 대기 후 재생성.
- **이미지 수 변동 가능**: 재생성 시 이미지 수가 바뀌면 `render_finished`(이미지당) 크기도
  맞춰야 함. 리사이즈는 보통 같은 수라 이 샘플은 단순화(같다고 가정) — 바뀌면 재생성.
- **macOS/MoltenVK 특이사항 (중요)**: 리사이즈해도 `eErrorOutOfDateKHR`가 **잘 안 뜬다**.
  MoltenVK가 Metal(CAMetalLayer) drawable을 자동으로 새 크기에 맞추고, 단색 clear라 시각적
  차이도 없어서다. → **out-of-date에만 의존하면 mac에선 재생성이 트리거되지 않는다.**
  - Linux X11/Wayland·Windows: 리사이즈 시 out-of-date/suboptimal이 거의 항상 발생.
  - macOS: **`glfwSetFramebufferSizeCallback`으로 `resized` 플래그**를 세워 present 후
    재생성하는 방식이 사실상 필수(콜백이 유일한 신뢰 트리거).
  - "리사이즈해도 안 깨진다"는 잘 돼서가 아니라 MoltenVK 자동적응 + 단색이라 안 보이는 것.
    recreate가 실제로 불리는지는 로그로 확인.

### Step 8 — 종료 정리 ✅(부분)
종료 시 `device.waitIdle()` 후 파괴(순서: sync → command pool → image_views → swapchain → device).
sync 객체(semaphore/fence)는 raw 핸들이라 main에서 직접 destroy(현재 반영됨).

> **미룬 것(나중):** present mode 우선순위 리스트 + `pick` 헬퍼, vsync 토글(생성자 인자),
> `Settings` 전역/`WindowInfo`/`RenderContext`, Unorm+수동 감마. 필요해질 때 도입.
