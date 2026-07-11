# 01_device_info

> Phase 0 · 상태: ✅ 완료
> Vulkan instance를 만들고, 시스템의 physical device(GPU)를 열거해 정보를 출력한다.

## 배우는 개념

### VkInstance — Vulkan과의 첫 접점
Vulkan을 쓰려면 가장 먼저 **instance**를 만든다. instance는 애플리케이션과
Vulkan 라이브러리(로더) 사이의 연결고리다. 여기에 앱 이름, 사용할 API 버전,
그리고 켤 **extension / layer** 를 알려준다.

- **extension**: 코어에 없는 추가 기능 (예: surface, portability).
- **layer**: 호출을 가로채는 훅. 대표적으로 validation layer(디버깅용).

### VkPhysicalDevice — 실제 GPU
instance를 만든 뒤 `enumeratePhysicalDevices()` 로 시스템에 꽂힌 GPU들을 얻는다.
이건 *생성*이 아니라 *열거*다. 각 GPU에 대해 properties(이름, API 버전, 종류),
features, 메모리, queue family 등을 **질의**할 수 있다.

### macOS / MoltenVK — portability 드라이버
macOS에는 네이티브 Vulkan 드라이버가 없다. 대신 **MoltenVK**가 Vulkan 호출을
Metal로 번역한다. MoltenVK는 Vulkan 명세를 100% 만족하지 않는
"portability(비적합)" 드라이버라서, Vulkan 1.3.216+ 로더는 기본적으로 이를 숨긴다.

명시적으로 켜주지 않으면 `createInstance`가 `VK_ERROR_INCOMPATIBLE_DRIVER`로 실패한다:

```cpp
extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
```

## mpvk에 추가된 것

### `mpvk::Instance` ([instance.hpp](../../cpp/include/mpvk/instance.hpp))
- 생성자에서 `vk::Instance` 생성 (portability는 `__APPLE__`에서만 opt-in).
- `physical_devices()` — GPU 목록 반환.
- RAII: 소멸자에서 `instance_.destroy()`.
- 복사 금지 (`= delete`) — Vulkan 핸들은 유일 소유가 안전.

## 왜 vulkan.hpp인가
C API(`vkCreateInstance`, 구조체 `sType` 수동 지정...) 대신 `vulkan.hpp`(C++ 바인딩)를
쓴다. 타입 안전, 예외 기반 에러 처리, `setPEnabledExtensionNames()` 같은
편의 setter로 `...Count` 필드를 자동 처리해준다.

## 실행
```bash
./build/debug/samples/01_device_info/01_device_info
```
출력: instance 생성 확인 + GPU 이름/API 버전 목록.

---

## 검증 레이어 (validation layer) — 후속 보강

> 상태: ✅ 완료. Instance에 validation layer + debug messenger 추가,
> sample 02를 validation 켠 상태로 재실행해 경고 0개 확인. 일부러 오류를
>주입해 콜백(`[vk] ...`)이 실제로 찍히는 것까지 검증함.

### 왜 지금 하나
validation layer는 Vulkan API 오용(잘못된 인자, 파괴 순서 위반, 동기화 오류
등)을 런타임에 잡아 메시지로 알려주는 **디버깅 도구**다. 03(surface)부터는
validation 없이는 못 잡는 실수가 쏟아지므로, 그 전에 켜둔다.

### 개념: layer vs extension, 그리고 messenger
검증에는 **두 조각**이 필요하고 역할이 다르다.

- **`VK_LAYER_KHRONOS_validation` (레이어)** — 실제 검증을 수행하는 훅. instance
  생성 시 `ppEnabledLayerNames`로 켠다.
- **`VK_EXT_debug_utils` (확장)** — 검증 메시지를 **내 콜백으로 받는** 통로.
  `vkCreateDebugUtilsMessengerEXT`로 **debug messenger**를 만들고, 콜백에서
  severity/type/메시지를 출력한다. (이게 없으면 메시지 출력 제어가 안 됨)

> 즉 레이어 = "검사하는 주체", debug_utils messenger = "결과를 받는 창구".
> 둘 다 있어야 "검증 결과를 내가 원하는 형태로 본다".

### 함정 (미리 알아둘 것)
1. **지원 확인 후 opt-in.** 레이어/확장이 없는 환경도 있다. 켜기 전에
   `vk::enumerateInstanceLayerProperties()` / `enumerateInstanceExtensionProperties()`
   로 존재를 확인한다. 없는 걸 켜면 `createInstance` 실패.
2. **디버그 빌드에서만.** release엔 넣지 않는다 (`#ifndef NDEBUG` 또는 CMake 옵션).
3. **EXT 함수는 동적 디스패치 필요 (vulkan.hpp 핵심 함정).**
   `createDebugUtilsMessengerEXT`는 확장 함수라 기본 정적 디스패처엔 없다.
   `vk::detail::DispatchLoaderDynamic dldi(handle_, vkGetInstanceProcAddr);` 를
   만들어 호출에 넘기거나, default dynamic dispatcher를 init 해야 한다.
   안 하면 링크/런타임 에러. → 학습용으로는 dldi를 Instance에 보관하는 방식 추천.
4. **파괴 순서.** messenger는 instance보다 **먼저** 파괴한다
   (`destroyDebugUtilsMessengerEXT`). 소멸자에서 messenger → instance 순서.
5. **(선택) instance 생성/파괴 구간도 잡으려면** `DebugUtilsMessengerCreateInfoEXT`를
   `InstanceCreateInfo.pNext`에 연결한다. 지금은 생략 가능 — 생성 후 messenger만으로 충분.

### mpvk 추가 (Instance 확장)
- 멤버(디버그 빌드): `vk::DebugUtilsMessengerEXT debug_messenger_`, 동적 디스패처 보관.
- static 콜백 함수: `VKAPI_ATTR VkBool32 VKAPI_CALL` 시그니처, severity+메시지 출력, `VK_FALSE` 반환.
- 생성자: (지원 시) 레이어/확장 enable → createInstance → messenger 생성.
- 소멸자: messenger 파괴(instance보다 먼저).

### 구현 스텝 (작게, 순서대로)
1. ✅ **레이어 + 확장 enable** — 디버그 빌드 & 지원 시에만 `VK_LAYER_KHRONOS_validation`,
   `VK_EXT_debug_utils` 추가. 이 상태로 먼저 빌드/실행해 정상 동작 확인.
2. ✅ **debug 콜백 작성** — 메시지 출력, `VK_FALSE` 반환.
3. ✅ **messenger 생성** — 동적 디스패처로 `createDebugUtilsMessengerEXT`. severity/type 마스크 설정.
4. ✅ **소멸자에서 messenger 먼저 파괴** — 파괴 순서 확인.
5. ✅ **재검증** — sample 02 재실행 → 경고 0개. priority 제거로 콜백 동작 확인 후 원복.
6. ✅ **concepts.md** 에 항목 추가.

> **실전 노트 (구현 후 확정) — 순서·타입·디스패처가 핵심 함정이었다**
> - **레이어/확장 검출은 `createInstance` 前, messenger 생성은 `createInstance` 後.**
>   messenger는 `handle_`(VkInstance)가 있어야 만든다. 순서를 섞으면 dispatcher의
>   `init()`이 `instance && getInstanceProcAddr` assert로 죽는다. 실제로 밟았음.
> - **콜백 시그니처는 `vk::` 타입으로.** 이 SDK의 vulkan.hpp는 `pfnUserCallback`을
>   `vk::DebugUtilsMessageSeverityFlagBitsEXT` / `vk::DebugUtilsMessageTypeFlagsEXT`
>   (= `Flags<...>`) 강타입으로 정의한다. C 타입(`Vk...`)으로 쓰면 대입 시 타입
>   불일치 컴파일 에러. (`data->pMessage`는 vk:: 타입에도 그대로 있음.)
> - **EXT 함수엔 동적 디스패처 필수.** `vk::detail::DispatchLoaderDynamic`(이 SDK는
>   `vk::` 바로 밑이 아니라 `detail::` 안)을 `handle_`+`vkGetInstanceProcAddr`로 만들고,
>   `create/destroyDebugUtilsMessengerEXT` 세 번째 인자로 넘긴다. 파괴 때도 필요하니 멤버로 보관.
> - **파괴 순서:** messenger → instance. 소멸자에서 messenger 먼저.
> - **조용한 미활성화 주의:** 레이어를 못 찾으면 빈 채로 넘어가 validation이 조용히
>   안 켜진다. "경고 0개"가 "검증 통과"인지 "검증기 미가동"인지 구분하려면, 일부러
>   오류를 한 번 주입해 콜백이 찍히는지 확인하는 습관이 중요.
>
> 남은 폴리시(기능 영향 없음): 콜백 출력을 spdlog로 통일, 잘못 들어간 `<cerrno>` →
> `<cstdio>`(또는 spdlog 사용 시 제거), 익명 namespace 안 `static` 중복 제거.
