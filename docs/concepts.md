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
