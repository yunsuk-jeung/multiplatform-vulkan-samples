# 02_logical_device

## Goal

Queue family을 이해하고, 그래픽스 큐를 가진 physical device를 골라
**logical device(`VkDevice`)** 를 만든 뒤, 거기서 **queue 핸들**을 얻는다.
화면은 아직 없다. "GPU를 고르고 명령 제출 통로를 확보"하는 단계.

## What this sample teaches

- physical device와 logical device의 차이, 그리고 관계
- queue와 queue family이 무엇이고 왜 먼저 골라야 하는지
- `VkDevice`가 이후 거의 모든 Vulkan 객체의 부모라는 점
- device 생성 시 extension/feature를 opt-in 하는 자리(개념만)
- 객체 소유권과 파괴 순서 (device는 instance보다 먼저 파괴)

## New Vulkan concepts

- **Queue family & queue flags**: GPU가 제공하는 큐 그룹. 각 family은
  `graphics / compute / transfer / sparse` 등의 능력 비트를 가진다.
- **Logical device (`VkDevice`)**: 하나의 physical device에 대한
  애플리케이션의 논리적 연결. 여기서 버퍼/파이프라인/커맨드가 만들어진다.
- **Device queue**: device를 만들 때 요청한 큐. 나중에 command buffer를
  제출(submit)하는 대상.
- (개념만) **device extension / feature** 활성화 지점.

## Expected result

콘솔에:

- 선택된 GPU 이름
- 고른 graphics queue family index
- "logical device created", "queue acquired" 같은 확인 로그

validation layer 켠 상태에서 경고/에러 0개. 창은 없다.

## Files to touch

- `cpp/include/mpvk/physical_device.hpp` / `cpp/src/physical_device.cpp` (신규)
- `cpp/include/mpvk/device.hpp` / `cpp/src/device.cpp` (신규)
- `cpp/CMakeLists.txt` (새 소스 등록)
- `samples/02_logical_device/main.cpp` (신규)
- `samples/02_logical_device/CMakeLists.txt` (신규)
- `samples/CMakeLists.txt` (새 샘플 add_subdirectory)

## mpvk additions

이번 샘플이 필요로 하는 만큼만.

- `PhysicalDevice`
  - queue family properties 조회
  - "graphics 큐를 가진 첫 GPU 고르기" 정적 헬퍼 (없으면 throw)
  - 선택된 `vk::PhysicalDevice` 핸들과 graphics queue family index 보관
- `Device`
  - `VkDevice` 소유(RAII) — 소멸자에서 `destroy()`
  - graphics queue 핸들 하나 보관
  - `handle()`, `graphics_queue()` 접근자

넣지 말 것: present/transfer 큐, 확장 로더, 여러 큐, VMA.

## Implementation steps

1. `Instance::physical_devices()` 로 목록을 받는다.
2. `PhysicalDevice`에서 각 device의 `getQueueFamilyProperties()` 를 돌며
   `vk::QueueFlagBits::eGraphics` 비트를 가진 family index를 찾는다.
3. 그런 family을 가진 **첫** device를 고른다. 없으면 예외.
4. `Device`에서:
   - `vk::DeviceQueueCreateInfo` (그 family index, count=1, priority 1.0f)
   - `vk::DeviceCreateInfo` 에 위 큐 info 연결
   - `physical.createDevice(...)` 로 `vk::Device` 생성
   - `device.getQueue(familyIndex, 0)` 로 큐 핸들 저장
5. `main.cpp`: instance → physical device 선택 → device 생성 →
   선택 정보 로그 출력.
6. CMake에 샘플/소스 등록, 빌드 후 실행.

## Common mistakes

- **파괴 순서**: device를 instance보다 나중에 파괴하면 validation 에러.
  소유권/소멸자 순서로 device가 먼저 죽게 한다.
- queue family이 하나도 없다고 가정하기 → 반드시 "못 찾음" 경로 처리.
- `DeviceQueueCreateInfo::pQueuePriorities` 를 빠뜨리면 UB/경고.
  priority 배열(길이 = queueCount)을 반드시 채운다.
- `getQueue`를 device 생성에서 요청하지 않은 (family, index)로 호출.
- vulkan.hpp의 예외/`vk::Result` 처리 일관성 (기존 Instance 코드 스타일에 맞추기).

## Self-check

- [ ] validation layer 켠 상태에서 경고 0개인가?
- [ ] `VkDevice`가 instance보다 **먼저** 파괴되는가?
- [ ] physical device를 못 찾았을 때 깔끔히 실패하는가?
- [ ] `Device`가 복사 불가(delete)인가? 이동은 필요한 만큼만.
- [ ] graphics family index가 실제로 graphics 비트를 가진 family인가?

## What to send for review

- `physical_device.hpp` / `device.hpp` 헤더
- `Device` 소멸자(파괴 순서/소유권이 드러나는 부분)
- `main.cpp`에서 device 생성 + 큐 획득 부분
