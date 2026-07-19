# Concepts — Vulkan 개념 인덱스

샘플을 진행하며 처음 등장한 Vulkan 개념을 **개념 중심**으로 모아두는 지도(map)다.

- 각 항목은 **짧다**: 한두 줄 설명 + 그 개념을 배우는 **샘플로 가는 링크**.
- 자세한 설명은 여기 두지 않는다. 그건 해당 샘플 문서(`docs/samples/NN_*.md`)의 몫이다.
- 하나의 개념이 여러 샘플에 걸쳐 있으면 "관련" 링크를 덧붙인다(중복 항목을 만들지 않는다).
- 새 개념이 샘플에 처음 등장하면, 샘플 문서와 **함께** 이 인덱스에도 항목을 추가한다.

순서: 개념이 처음 등장한 순서.

---

## Phase 0 — 토대

### Vulkan instance (`VkInstance`)
애플리케이션과 Vulkan 로더 사이의 연결. 확장/레이어를 여기서 opt-in 한다.
- 처음 등장: [01_device_info](roadmap.md#-01_device_info)

### Physical device 열거 (`VkPhysicalDevice`)
시스템의 GPU들을 나열하고 properties/features를 질의한다.
- 처음 등장: [01_device_info](roadmap.md#-01_device_info)

### Portability enumeration (MoltenVK)
macOS에서 MoltenVK를 통해 Vulkan을 쓰려면 portability 확장을 opt-in 해야 한다.
- 처음 등장: [01_device_info](roadmap.md#-01_device_info)

### Validation layer & debug messenger
`VK_LAYER_KHRONOS_validation`(레이어)이 API 오용을 런타임에 검사하고,
`VK_EXT_debug_utils`로 만든 debug messenger 콜백이 그 메시지를 받아 출력한다.
디버그 빌드에서만 opt-in. EXT 함수라 동적 디스패처(`DispatchLoaderDynamic`)가 필요.
- 처음 등장: [01_device_info](samples/01_device_info.md)

## Phase 1 — 디바이스와 큐

### Logical device (`VkDevice`)
고른 GPU에 대한 "내 세션". 쓸 기능/확장/큐를 골라 생성하며, 이후 거의 모든
Vulkan 객체의 부모다. Physical device는 열거해 얻지만 logical device는 생성한다.
- 처음 등장: [02_logical_device](samples/02_logical_device.md)
- 관련: [01_device_info](roadmap.md#-01_device_info) (physical device 열거)

### Queue와 queue family
GPU에 일을 시키는 유일한 통로(command buffer → queue submit). 처리 종류
(graphics/compute/transfer)가 같은 큐들이 family로 묶인다. 원하는 비트를 가진
family index를 찾아 device 생성 시 요청하고, 생성 후 `getQueue`로 핸들을 얻는다.
- 처음 등장: [02_logical_device](samples/02_logical_device.md)

### Device extension opt-in (지원 확인 후 활성화)
확장은 `enumerateDeviceExtensionProperties()`로 지원 여부를 확인한 뒤,
지원할 때만 `DeviceCreateInfo`에 이름을 넘겨 켠다. macOS(MoltenVK)에선
`VK_KHR_portability_subset`가 이 규칙의 대표 사례(있으면 반드시 켬).
- 처음 등장: [02_logical_device](samples/02_logical_device.md)
- 관련: [01_device_info](roadmap.md#-01_device_info) (instance 레벨 portability)

### 객체 소유권과 파괴 순서 (RAII)
자식 객체는 부모보다 먼저 파괴돼야 한다(device는 instance보다 먼저). RAII
소멸자 + 복사 금지로 유일 소유를 강제하고, 선언 순서로 파괴 역순을 보장한다.
- 처음 등장: [02_logical_device](samples/02_logical_device.md)

## Phase 2 — 창과 surface

### WSI / surface (`VkSurfaceKHR`)
Vulkan은 창을 만들지 않는다. 플랫폼 윈도우(GLFW)와 Vulkan을 잇는 instance 레벨
객체가 surface다. `VK_KHR_surface` + 플랫폼별 확장(예: `VK_EXT_metal_surface`)이
필요하고, 그 목록은 `glfwGetRequiredInstanceExtensions`가 알려준다.
surface는 instance보다 먼저 파괴한다(`destroySurfaceKHR`).
- 처음 등장: [03_window_surface](samples/03_window_surface.md)

### Present queue family
"화면에 출력(present) 가능한" queue family. queueFlags가 아니라 **surface 기준**
(`getSurfaceSupportKHR(family, surface)`)으로 결정되며 graphics family와 다를 수 있다.
surface가 없으면(headless) present family도 없다.
- 처음 등장: [03_window_surface](samples/03_window_surface.md)
- 관련: [02_logical_device](samples/02_logical_device.md) (queue family 기초)

### Image & image view (`VkImage` / `VkImageView`)
`VkImage`는 픽셀 데이터 + 메타(format/크기/mip/layer)다. 파이프라인·프레임버퍼는
이미지에 **직접** 접근하지 못하고, "어떻게 해석·어느 부분을 볼지"를 정의한
`VkImageView`(렌즈)를 통해 접근한다(렌더 타깃/샘플링/스토리지). swapchain 이미지마다
color image view를 만들어 렌더 타깃으로 쓴다. 필드 상세는 sample doc 참고.
- 처음 등장: [04_clear_screen](samples/04_clear_screen.md)

### frames-in-flight vs swapchain 이미지 수
서로 다른 축이라 개수를 맞추지 않는다. **swapchain 이미지 수**(예 3)는 present 엔진이 굴리는
화면 버퍼 개수로 **드라이버**가 결정. **frames-in-flight**(보통 2)는 CPU가 GPU보다 얼마나
앞서 달릴지 = CPU 측 자원 세트 수로 **내가** 정하는 지연/처리량 튜닝값. acquire가 주는 이미지
인덱스(img)는 못 고르므로 "슬롯 f == img" 가정 불가 → `imageAvailable`·`inFlight`·command
buffer는 f로, `renderFinished`는 img로 인덱싱.
- 처음 등장: [04_clear_screen](samples/04_clear_screen.md)

### Swapchain (`VkSwapchainKHR`)
surface에 대응하는 present용 이미지 링. capabilities/format/present mode를 질의해 골라
만든다. 이미지는 `getSwapchainImagesKHR`로 받고(소유는 swapchain), device extension
`VK_KHR_swapchain` 필요. 리사이즈 시 재생성.
- 처음 등장: [04_clear_screen](samples/04_clear_screen.md)

### Command pool / command buffer
GPU는 command buffer에 명령을 기록해 queue에 submit한다. buffer는 특정 queue family에
묶인 command pool에서 할당하고, pool 파괴 시 함께 해제된다.
- 처음 등장: [04_clear_screen](samples/04_clear_screen.md)

### Image layout & pipeline barrier
이미지는 용도별 레이아웃(undefined/transfer-dst/present-src...)을 갖고, `pipelineBarrier`로
전환한다. render pass 없이 clear할 땐 `UNDEFINED→TRANSFER_DST→PRESENT_SRC`로 직접 전환.
- 처음 등장: [04_clear_screen](samples/04_clear_screen.md)

### 동기화: semaphore & fence, 렌더 루프
semaphore=큐 작업 간 순서(GPU↔GPU), fence=CPU가 GPU 완료 대기. 한 프레임:
waitFence → acquire → reset → record → submit(signal renderFinished+fence) → present.
- 처음 등장: [04_clear_screen](samples/04_clear_screen.md)

### GLFW 통합 (윈도잉 의존성)
첫 외부 윈도잉 라이브러리. 창 생성/입력/플랫폼 차이를 흡수하고
`glfwCreateWindowSurface`로 surface 생성을 감춘다. `GLFW_NO_API`로 OpenGL 컨텍스트를
끄고, 소스 빌드로 `libs/`에 설치해 `find_package(glfw3 CONFIG)`로 소비한다.
- 처음 등장: [03_window_surface](samples/03_window_surface.md)

<!--
새 항목 템플릿:

### <개념 이름>
<한두 줄 설명>
- 처음 등장: [NN_sample_name](samples/NN_sample_name.md)
- 관련: [MM_other_sample](samples/MM_other_sample.md)
-->
