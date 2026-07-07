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
