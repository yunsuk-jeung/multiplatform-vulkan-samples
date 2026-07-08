# 02_logical_device

> Phase 1 · 상태: ✅ 완료 (PhysicalDevice ✅ / Device ✅, 정상 실행 확인)
>
> ⚠️ 주의: 이 샘플을 검증할 때 **validation layer는 실제로 켜져 있지 않았다**
> (Instance가 아직 레이어/​debug messenger를 활성화하지 않음). "경고 0개"는
> 검증기가 안 돌았다는 뜻이지 "검증 통과"가 아니다. → 아래 "다음에 할 일" 참고.

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

---

## 구현 가이드 (직접 해보기)

> 답안 복붙이 아니라, 순서·API·함정만 짚는다. 몸통은 직접 채운다.
> 막히면 각 단계의 "쓸 API" 힌트를 보고 `vulkan.hpp` 문서를 찾아보자.

### Step 1 — `cpp/include/mpvk/physical_device.hpp` ✅

만들 것: `mpvk::PhysicalDevice` 클래스.

- 멤버: `vk::PhysicalDevice handle_;`, `uint32_t graphics_family_;`
- 생성자: `explicit PhysicalDevice(const Instance& instance);`
  - 이 안에서 "적합한 GPU 고르기 + graphics family 찾기"를 한다.
- 접근자(전부 `const`, 인라인): `handle()`, `graphics_family()`
- 부가로 GPU 이름 출력을 위해 `properties()` 또는 `name()` 하나 있으면 편함
  → `handle_.getProperties()` 를 그때그때 불러도 됨.
- `#include <mpvk/instance.hpp>` 필요 (생성자 인자 타입).

> 복사 금지(`= delete`)는 이 클래스엔 굳이 안 걸어도 됨 — 소유하는 리소스가
> 없고(그냥 핸들 값) 그래서 복사돼도 안전. `Instance`/`Device`와 다른 점.

> **실전 노트 (구현 후 확정)**
> - 멤버 기본값은 `vk::PhysicalDevice handle_{nullptr};`, `uint32_t graphics_family_{0};`
>   로 in-class 초기화. 접근자는 인라인 `const`.
> - `#include <cstdint>` (uint32_t) 필요. `vk::` 타입은 `mpvk/instance.hpp`가
>   끌어오는 `<vulkan/vulkan.hpp>`로 간접 포함되지만, cpp에서 직접 쓰면
>   IWYU상 직접 include가 더 안전.

### Step 2 — `cpp/src/physical_device.cpp` ✅

선택 알고리즘 (생성자 몸통):

1. `instance.physical_devices()` 로 후보 목록을 받는다.
2. 각 후보 `pd`에 대해:
   - `auto families = pd.getQueueFamilyProperties();` → `std::vector<vk::QueueFamilyProperties>`
   - 인덱스 `i`를 돌며 `families[i].queueFlags & vk::QueueFlagBits::eGraphics` 가
     참인 첫 family를 찾는다.
   - 찾으면 `handle_ = pd; graphics_family_ = i;` 저장하고 종료.
3. 하나도 못 찾으면 예외 (`throw std::runtime_error(...)`).

> `queueFlags & eGraphics` 는 `vk::QueueFlags` (비트마스크 래퍼)를 돌려준다.
> `if (flags & bit)` 형태로 bool 판정 가능. 명시적으로 하려면
> `(families[i].queueFlags & vk::QueueFlagBits::eGraphics) ? ... `.

> 지금은 "graphics 있는 첫 GPU"면 충분. 나중에 discrete 우선 점수제로 확장.

> **실전 노트 (구현 후 확정)**
> - **이중 루프 탈출은 `return`으로.** 적합 family를 찾으면 `handle_`/
>   `graphics_family_` 저장 후 즉시 `return;` → 안쪽/바깥 루프를 한 번에 빠져나온다.
>   `break` 하나로는 바깥 루프가 계속 돌아 다른 GPU까지 훑는 버그가 났었다.
> - **루프 인덱스는 `uint32_t`.** `size()`는 `size_t`이고 `graphics_family_`도
>   `uint32_t`라, `int i`로 돌면 `-Wsign-compare` 경고 + 타입 불일치. `for (uint32_t i ...)`.
> - 못 찾으면 `throw std::runtime_error(...)` → `<stdexcept>` 포함. main에서는
>   `catch (const std::exception&)` (값 catch는 slicing).

### Step 3 — `cpp/include/mpvk/device.hpp` ✅

만들 것: `mpvk::Device` 클래스.

- 멤버: `vk::Device device_;`, `vk::Queue graphics_queue_;`, `uint32_t graphics_family_;`
- 생성자: `explicit Device(const PhysicalDevice& gpu);`
- 소멸자: `~Device();` (몸통은 .cpp에서 `device_.destroy()`)
- 복사 금지 (`Instance`와 동일한 이유 — 유일 소유).
- 접근자: `handle()`, `graphics_queue()`, `graphics_family()`.
- `#include <mpvk/physical_device.hpp>`.

> **실전 노트 (구현 후 확정) — 전방 선언 vs 참조/포인터**
> - 생성자 인자는 **`const PhysicalDevice&`(참조)** 로. non-owning + null 불가인
>   필수 의존이라 참조가 의도에 맞다. 포인터는 "null 가능"을 암시해 부적절.
> - **오해 주의:** "전방 선언하려면 포인터여야 한다"는 틀렸다. 함수 매개변수는
>   참조든 포인터든 **불완전 타입(전방 선언만)으로 선언 가능**하다. 완전 정의가
>   필요한 건 값 멤버(`Foo f_;`)·`sizeof`·멤버 접근뿐이고, 멤버 접근은 `.cpp`에서
>   헤더를 include 하면 해결된다.
> - 그래서 헤더는 `#include`를 하지 말고 **`class PhysicalDevice;` 전방 선언만**
>   두면 결합도가 낮아진다. 단 전방 선언 *줄 자체는 반드시 있어야* 한다 —
>   빼면 `unknown type name 'PhysicalDevice'`(→ `vk::PhysicalDevice` 오해) 에러.
> - `uint32_t` 때문에 `<cstdint>`가 필요하나 `<vulkan/vulkan.hpp>`로 간접 포함됨.

### Step 4 — `cpp/src/device.cpp` ✅

생성자 몸통 순서:

1. **queue create info** 구성. 함정: priority 필수.
   ```cpp
   float priority = 1.0f;
   vk::DeviceQueueCreateInfo queue_info{};
   queue_info.queueFamilyIndex = gpu.graphics_family();
   queue_info.setQueuePriorities(priority);   // count=1, ptr=&priority 자동
   ```
2. **device extension** 목록 구성. macOS 함정: portability_subset.
   - `gpu.handle().enumerateDeviceExtensionProperties()` 로 지원 목록을 받고,
   - 그 안에 `"VK_KHR_portability_subset"` 가 **있으면** extensions에 추가.
   - 없는데 넣으면 device 생성 실패(`VK_ERROR_EXTENSION_NOT_PRESENT`),
     있는데 안 넣으면 MoltenVK에서 validation VUID 에러 → "있을 때만"이 핵심.
   - **함정: 매크로 `VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME`는 기본 include로
     안 보인다.** 이 상수는 `vulkan_beta.h`에 있고 `VK_ENABLE_BETA_EXTENSIONS`
     게이트 뒤에 있어서, `#include <vulkan/vulkan.hpp>`만으로는 심볼을 못 찾는다.
     런타임 확장이 beta인 게 아니라 *C 매크로 상수만* beta 헤더에 있는 것.
   - 해결: **매크로 대신 문자열 리터럴 `"VK_KHR_portability_subset"`를 직접 사용.**
     beta 헤더 의존이 없어 3개 플랫폼에서 그대로 컴파일된다. (매크로를 굳이 쓰려면
     vulkan include 전에 `#define VK_ENABLE_BETA_EXTENSIONS` 해야 하지만, 다른
     beta 확장 심볼까지 열려서 학습 프로젝트엔 비권장.)
   - 비교는 `ext.extensionName`(C 문자열)이라 `==`가 아니라 `std::strcmp(...) == 0`
     (`<cstring>` 필요).
3. **`vk::DeviceCreateInfo`** 구성:
   ```cpp
   vk::DeviceCreateInfo create_info{};
   create_info.setQueueCreateInfos(queue_info);          // 배열/단일 다 받음
   create_info.setPEnabledExtensionNames(extensions);
   // (features는 지금 불필요 → 생략)
   ```
4. `device_ = gpu.handle().createDevice(create_info);`
5. `graphics_family_ = gpu.graphics_family();`
6. `graphics_queue_ = device_.getQueue(graphics_family_, 0);`  // 0번째 큐

소멸자: `if (device_) device_.destroy();`

> **실전 노트 (구현 후 확정)**
> - **`std::pmr::vector` 오타 주의.** extensions는 평범한 `std::vector<const char*>`.
>   자동완성으로 `std::pmr::vector`가 튀어나오기 쉬운데 별개 타입(PMR 할당자)이다.
> - **`#ifdef __APPLE__` 게이트는 불필요.** "지원 목록에 있을 때만 추가" 쿼리가
>   플랫폼 차이를 스스로 흡수한다(portability_subset은 MoltenVK에만 존재 →
>   Linux/Windows 쿼리엔 안 나옴). 플랫폼 매크로 하드코딩은 멀티플랫폼 정책상
>   피하는 게 낫다 — **항상 쿼리**하도록 두면 self-adjusting.
> - `std::strcmp` → `#include <cstring>` 직접 포함(IWYU).
> - `constexpr float priority`는 명명된 lvalue라 `setQueuePriorities(priority)`에
>   안전(임시 아님 → dangling 없음).

### Step 5 — CMake 등록 ✅

- `cpp/CMakeLists.txt` 의 `add_library(mpvk ...)` 소스 목록에
  `src/physical_device.cpp`, `src/device.cpp` 추가.
- `samples/CMakeLists.txt` 에 `add_subdirectory(02_logical_device)` 추가.
- `samples/02_logical_device/CMakeLists.txt` 신규 작성
  (01 것을 참고: `add_executable` + `target_link_libraries(... mpvk::mpvk)`).

### Step 6 — `samples/02_logical_device/main.cpp` ✅

흐름:
```cpp
mpvk::Instance   instance("02_logical_device");
mpvk::PhysicalDevice gpu(instance);      // GPU 선택
mpvk::Device     device(gpu);            // logical device + queue

// 출력: gpu 이름, gpu.graphics_family(), device.graphics_queue() 유효성
```
- queue가 유효한지: `vk::Queue`는 `bool` 변환/`!= nullptr` 로 확인 가능
  (`if (device.graphics_queue())`).

### Step 7 — 빌드 & 검증 ✅
```bash
cmake --preset debug            # 새 파일 인식시키려면 재구성
cmake --build --preset debug
./build/debug/samples/02_logical_device/02_logical_device
```
기대 출력 예:
```
Selected GPU: Apple M_ (graphics family = 0)
Logical device created. Graphics queue: OK
```

### 흔한 에러
- **`VK_ERROR_EXTENSION_NOT_PRESENT`**: portability_subset를 지원 확인 없이
  넣었을 때. Step 4-2의 "있을 때만" 확인.
- **validation: priority 누락**: Step 4-1 확인.
- **링크 에러(undefined symbol)**: CMake 소스 목록에 .cpp 추가 빠짐 (Step 5).
- **재구성 안 됨**: 새 CMake 파일/타깃은 `cmake --preset debug` 를 다시 돌려야 인식.
- **`unknown type name 'PhysicalDevice'`**: device.hpp에 `class PhysicalDevice;`
  전방 선언 줄이 빠짐 (참조 인자라도 이름 선언은 필요).

---

## 다음에 할 일 (후속 · 아직 안 함)

> 오늘은 여기까지. 다음 세션 시작점.

1. **[우선] Instance에 validation layer + debug messenger 추가** — sample 03 들어가기 **전에**.
   - 디버그 빌드에서만 `VK_LAYER_KHRONOS_validation` 레이어 enable (지원 확인 후 opt-in)
   - `VK_EXT_debug_utils` 확장 + debug messenger 콜백으로 검증 메시지 수신/출력
   - release 빌드에선 생략
   - 완료 후 이 샘플을 **실제 validation 켠 상태로 다시 실행**해서 경고 0개 재확인
     (현재 상단 ⚠️ 주의 해소). concepts.md에 "validation layer / debug messenger" 항목 추가.
2. **그다음 sample 03 (`03_window_surface`)** — GLFW 창 + `VkSurfaceKHR` + present queue.

### 남은 소소한 정리 (선택)
- main.cpp의 `"wtf %s"` 에러 메시지 → 개행 포함 + 설명적 문구로.
- device.cpp 소멸자의 `handle_ = nullptr;` 는 불필요(무해) — 취향껏.
