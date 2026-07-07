# 02_logical_device

> Phase 1 · 상태: ⬜ 예정

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

## 배우는 개념

### physical device vs logical device — 왜 나뉘어 있나
- **`VkPhysicalDevice`**: 실제 GPU를 나타내는 읽기 전용 핸들. 생성이 아니라
  instance에서 *열거*해 얻는다. "뭘 할 수 있나"를 질의하는 용도.
- **`VkDevice` (logical device)**: 그 GPU에 대한 "내 세션". 내가 쓸
  기능/확장/큐를 골라 *생성*한다. 앞으로 거의 모든 Vulkan 함수(`vkCmd...`,
  버퍼/이미지 생성 등)의 첫 인자가 이 logical device다.

> 비유: physical device = 식당 **메뉴판**(구경), logical device = 실제 **주문**(계약).

### queue와 queue family — GPU에게 일 시키는 유일한 통로
GPU에게 작업을 시키는 방법은 하나뿐이다: **command buffer를 만들어 queue에
submit** 한다. 그런데 queue마다 처리 가능한 작업 종류가 다르다.

- **graphics** — 그리기
- **compute** — 범용 계산
- **transfer** — 메모리 복사
- (sparse, video ...)

같은 종류의 queue들이 **queue family**로 묶여 있다. 절차:

1. `physicalDevice.getQueueFamilyProperties()` 로 family 목록을 얻는다.
2. `queueFlags & vk::QueueFlagBits::eGraphics` 인 family의 **인덱스**를 찾는다.
3. logical device 생성 시 "이 family에서 큐 N개 달라"고 요청한다.
4. 생성 후 `device.getQueue(familyIndex, 0)` 으로 큐 핸들을 꺼낸다.

> present(화면 출력)용 queue는 surface가 생기는 **Phase 2**에서 추가한다.
> 이번엔 graphics queue만 확보한다.

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

## mpvk 설계 (additions)

이번 샘플이 필요로 하는 만큼만. 의존 방향은 단방향: `Instance` → `PhysicalDevice` → `Device`.

### `mpvk::PhysicalDevice` (선택 헬퍼)
- `Instance`의 후보들 중 **적합한 GPU를 고르는** 역할.
- 선택 기준: graphics queue family가 있는가.
  (나중엔 "discrete GPU 우선" 점수제로 확장)
- queue family properties 조회, "graphics 큐를 가진 첫 GPU 고르기" (없으면 throw).
- 보관: `vk::PhysicalDevice handle_`, `uint32_t graphics_family_`.
- 별도 클래스로 두는 이유: GPU 선택 + queue family 탐색 로직이 앞으로 계속
  커지므로 `Device`에서 분리.

### `mpvk::Device` (logical device 소유)
- 생성자에서 `PhysicalDevice`를 받아 `vk::Device` 생성 + graphics queue 확보.
- RAII: 소멸자에서 `device_.destroy()`.
- 보관: `vk::Device device_`, `vk::Queue graphics_queue_`, family index.
- 제공: `handle()`, `graphics_queue()`, `graphics_family()`.
- 복사 금지.

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

## 구현 시 주의점 (common mistakes)

### queue priority는 필수
`vk::DeviceQueueCreateInfo`에 `pQueuePriorities`(0.0~1.0 float 배열)를 반드시
넘겨야 한다. 큐가 하나여도:
```cpp
float priority = 1.0f;
queue_info.setQueuePriorities(priority);
```
빼먹으면 validation error.

### macOS: device 레벨에서도 portability
instance에서 portability를 opt-in 했듯, logical device에도
`VK_KHR_portability_subset` device extension을 켜야 한다.
단, **지원 목록에 있을 때만** 켠다(`enumerateDeviceExtensionProperties()`로 확인).

### vulkan.hpp setter
`setQueueCreateInfos(...)`, `setPEnabledExtensionNames(...)` 로 count/pointer를
자동 처리. C API의 `...Count` 필드를 직접 만질 필요 없다.

### 파괴 순서
device를 instance보다 나중에 파괴하면 validation 에러.
소유권/소멸자 순서로 device가 먼저 죽게 한다.

### 기타
- queue family이 하나도 없다고 가정하기 → 반드시 "못 찾음" 경로 처리.
- `getQueue`를 device 생성에서 요청하지 않은 (family, index)로 호출하지 말 것.
- vulkan.hpp의 예외/`vk::Result` 처리 일관성 (기존 Instance 코드 스타일에 맞추기).

## Self-check

- [ ] validation layer 켠 상태에서 경고 0개인가?
- [ ] `VkDevice`가 instance보다 **먼저** 파괴되는가?
- [ ] physical device를 못 찾았을 때 깔끔히 실패하는가?
- [ ] `Device`가 복사 불가(delete)인가? 이동은 필요한 만큼만.
- [ ] graphics family index가 실제로 graphics 비트를 가진 family인가?

## 샘플이 검증할 것 (`02_logical_device/main.cpp`)
```
Instance 생성
 → PhysicalDevice 선택 (고른 GPU 이름 + graphics family 인덱스 출력)
 → Device 생성 (logical device 성공)
 → graphics queue 핸들 유효성 출력
```

## What to send for review

- `physical_device.hpp` / `device.hpp` 헤더
- `Device` 소멸자(파괴 순서/소유권이 드러나는 부분)
- `main.cpp`에서 device 생성 + 큐 획득 부분
