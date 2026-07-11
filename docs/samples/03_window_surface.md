# 03_window_surface

> Phase 2 · 상태: 🚧 진행 중
> (GLFW 빌드 통합 ✅ / Window ✅ / Surface ⬜ / Instance surface 확장 ⬜ /
>  present family ⬜ / Device present queue ⬜)

## Goal

GLFW로 실제 **창**을 열고, 그 창에 대응하는 **`VkSurfaceKHR`** 를 만든 뒤,
이 surface에 **present(화면 출력)가 가능한 queue family**를 찾는다.
아직 아무것도 그리지 않는다 — "그릴 표면과 출력 통로를 확보"하는 단계.
다음 샘플(04, swapchain)의 토대다.

## What this sample teaches

- Vulkan은 **창을 만들지 않는다** — 플랫폼 윈도우(GLFW)와 Vulkan을 잇는 게 surface.
- **WSI(Window System Integration)**: `VK_KHR_surface` + 플랫폼별 surface 확장.
- instance extension을 **GLFW가 요구하는 목록**으로 채우는 법
  (`glfwGetRequiredInstanceExtensions`).
- **present queue family** 는 graphics family와 다를 수 있다 —
  surface 지원 여부(`getSurfaceSupportKHR`)로 따로 찾아야 한다.
- surface의 **capabilities / formats / present modes** 질의 (04에서 쓸 정보).
- 새 파괴 순서 규칙: **surface는 instance보다 먼저** 파괴.
- 첫 **외부 의존성(GLFW)** 을 빌드 시스템에 통합.

## 배우는 개념

### surface — Vulkan과 창을 잇는 다리
Vulkan 코어에는 "창"이 없다. 화면에 뭔가 내보내려면 OS의 윈도우가 필요하고,
그 윈도우와 Vulkan을 연결하는 핸들이 **`VkSurfaceKHR`** 다. surface는
`VK_KHR_surface`(플랫폼 공통) + 플랫폼별 확장(예: macOS `VK_EXT_metal_surface`)로
제공되는 **instance 레벨** 객체다.

> 그래서 surface 관련 확장은 **instance 생성 시** 켜야 한다. GLFW가 현재 플랫폼에
> 필요한 목록을 알려준다: `glfwGetRequiredInstanceExtensions(&count)`.

### 왜 GLFW인가
창 생성·입력·플랫폼 차이를 GLFW가 흡수한다. `glfwCreateWindowSurface()` 한 방으로
플랫폼별 surface 생성 코드를 감춰줘서, 우리는 `VkSurfaceKHR`만 받으면 된다.
중요: **OpenGL 컨텍스트를 만들지 않도록** `glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)`.

### present queue family — graphics와 별개일 수 있다
지금까진 graphics 비트를 가진 family만 골랐다. 하지만 "화면에 출력(present)"이
가능한 family는 **queueFlags가 아니라 surface에 대해** 결정된다:

```cpp
bool ok = physicalDevice.getSurfaceSupportKHR(familyIndex, surface);
```

- graphics family가 present도 지원하는 경우가 대부분이지만 **보장되지 않는다**.
- 그래서 이제 GPU를 고를 때 **graphics family + present family** 둘 다 찾아야 한다.
  (같은 인덱스일 수도, 다를 수도 있다.)

### surface 지원 정보 질의 (04 준비)
swapchain을 만들려면 surface가 뭘 지원하는지 알아야 한다. 이번엔 **질의해서
출력만** 한다:
- `getSurfaceCapabilitiesKHR` — min/max 이미지 수, 해상도(extent) 범위 등
- `getSurfaceFormatsKHR` — 색 포맷/색공간 목록
- `getSurfacePresentModesKHR` — FIFO/Mailbox/Immediate 등

### 생성/파괴 순서 (이번 샘플의 핵심 난이도)
의존이 사슬처럼 얽힌다:

```
Instance (surface 확장 포함)
  → Window (GLFW 창)
  → Surface (Instance + Window 필요)
  → PhysicalDevice 선택 (Surface 필요 — present family 확인)
  → Device (graphics + present queue)
```

파괴는 **역순**:
```
Device → Surface → Window → Instance
```
- surface는 `instance.destroySurfaceKHR(surface)` 로 파괴 → **instance보다 먼저**.
- GLFW: 창 파괴(`glfwDestroyWindow`) 후 `glfwTerminate()`.

## Expected result

콘솔(spdlog)에:
- 창이 열림(잠깐 떠 있다 닫혀도 됨 — 이벤트 루프는 최소)
- 선택된 GPU, **graphics family index / present family index**
- surface format 개수, present mode 개수 등 요약
- validation 경고 0개

## Files to touch

- `scripts/install_dependencies_mac.sh` / `_ubuntu.sh` — GLFW `fetch_source` + 빌드 추가
- `cpp/CMakeLists.txt` — `find_package(glfw3 ...)` + 링크
- `cpp/include/mpvk/window.hpp` / `src/window.cpp` (신규) — GLFW 창 래퍼
- `cpp/include/mpvk/surface.hpp` / `src/surface.cpp` (신규) — `VkSurfaceKHR` RAII
- `cpp/include/mpvk/instance.hpp` / `src/instance.cpp` — surface 확장 추가
- `cpp/include/mpvk/physical_device.hpp` / `src/physical_device.cpp` — present family 탐색
- `cpp/include/mpvk/device.hpp` / `src/device.cpp` — present queue 확보
- `samples/03_window_surface/main.cpp` + `CMakeLists.txt` (신규)
- `samples/CMakeLists.txt` — `add_subdirectory(03_window_surface)`

## mpvk 설계 (additions)

의존 방향: `Instance` → `Window` → `Surface` → `PhysicalDevice` → `Device`.

### `mpvk::Window` (GLFW 래퍼)
- 생성자: `glfwInit`(최초), `GLFW_NO_API` 힌트, `glfwCreateWindow`.
- 소멸자: `glfwDestroyWindow` (+ 참조 카운트/마지막이면 `glfwTerminate`).
- 제공: `handle()`(GLFWwindow*), `create_surface(const Instance&)` 또는 크기 질의.
- 복사 금지(창 소유).
- **정적 헬퍼**: `required_instance_extensions()` — `glfwGetRequiredInstanceExtensions` 래핑.

### `mpvk::Surface` (VkSurfaceKHR RAII)
- 생성자: `glfwCreateWindowSurface(instance, window, nullptr, &raw)`.
- 소멸자: `instance.destroySurfaceKHR(surface_)` — instance 참조 보관 필요.
- 복사 금지. 제공: `handle()`.

### `Instance` 변경
- 생성자에서 surface 확장 추가. GLFW 목록(`required_instance_extensions`)을
  extensions에 합친다. (portability enumeration은 그대로 유지.)
- 주의: GLFW 확장 목록을 얻으려면 `glfwInit()`가 먼저 되어 있어야 한다 →
  Window/GLFW 초기화 타이밍을 Instance보다 앞당길지 설계 결정 필요(아래 함정).

### `PhysicalDevice` 변경
- 이제 surface를 받아 **present family**도 찾는다.
- 보관: `graphics_family_`, `present_family_`.
- 선택 기준: graphics family 존재 + 그 surface에 present 가능한 family 존재.

### `Device` 변경
- graphics + present queue 둘 다 확보. 두 family가 같으면 큐 하나만 요청,
  다르면 `DeviceQueueCreateInfo` 두 개.

넣지 말 것: swapchain, image view, 렌더 루프(전부 04).

## Implementation steps (큰 순서)

1. **GLFW 빌드 통합** — install 스크립트에 GLFW `fetch_source` + cmake 빌드,
   `find_package(glfw3)`. 우선 **빈 창 하나 띄우기**까지만 되게.
2. **`mpvk::Window`** — 창 열고/닫기. main에서 창만 잠깐 띄워 확인.
3. **Instance에 surface 확장 추가** — `glfwGetRequiredInstanceExtensions`.
4. **`mpvk::Surface`** — `glfwCreateWindowSurface`. 파괴 순서(instance보다 먼저) 확인.
5. **PhysicalDevice에 present family 탐색** — `getSurfaceSupportKHR`.
6. **Device에 present queue** — family 중복 처리.
7. **surface 정보 질의 & 출력** — capabilities/formats/present modes 개수.
8. main에서 전체 흐름 연결 + validation 0개 확인.

## 구현 시 주의점 (common mistakes)

### GLFW 초기화 타이밍 (닭-달걀)
`glfwGetRequiredInstanceExtensions`는 **`glfwInit()` 이후**에만 유효하다. 그런데
Instance가 그 확장을 필요로 한다. 두 가지 해법:
- (A) `Window` 생성을 Instance보다 **먼저** 하고, Instance가 확장 목록을 받는다.
- (B) `glfwInit()`만 별도로 먼저 부르고 창은 나중에.
→ 이번엔 (A)를 권장: main에서 `Window`를 먼저 만들고 그 다음 `Instance`.
   (창-먼저가 surface 생성 순서와도 자연스럽다.)

### present family ≠ graphics family 가능
같다고 가정하고 하드코딩하지 말 것. 반드시 `getSurfaceSupportKHR`로 확인.

### 파괴 순서
surface → instance, window → glfwTerminate. RAII 소멸자 순서(선언 역순)로 보장.

### macOS
GLFW가 `VK_EXT_metal_surface`(또는 상응)를 required 목록에 넣어준다. portability
enumeration 플래그/확장은 **그대로 유지**하면서 GLFW 목록을 **합친다**(덮어쓰지 말 것).

### validation
surface 확장/ present 관련 실수는 이제 messenger가 잡아준다 — 로그 주시.

## Self-check

- [ ] 창이 뜨고, 정상 종료 시 validation 경고 0개인가?
- [ ] graphics/present family index를 각각 출력하는가? (같을 수도 다를 수도)
- [ ] surface가 instance보다 **먼저** 파괴되는가?
- [ ] GLFW 확장을 portability 확장과 **합쳤는가**(덮어쓰지 않았는가)?
- [ ] Window/Surface/Device가 복사 불가인가?

## What to send for review

- GLFW 빌드 통합 diff (install 스크립트 + cpp CMake)
- `window.hpp` / `surface.hpp` 헤더
- `Instance` 확장 병합 부분, `PhysicalDevice` present family 탐색 부분
- main.cpp 전체 흐름 + 실행 로그

## 새 개념 (완료 시 concepts.md에 추가)

- **WSI / surface (`VkSurfaceKHR`)** — 창과 Vulkan을 잇는 다리, instance 레벨 확장.
- **present queue family** — surface 기준으로 결정, graphics와 다를 수 있음.
- **surface 지원 질의** — capabilities/formats/present modes.
- **GLFW 통합** — 첫 외부 윈도잉 의존성.

---

## 구현 가이드 (직접 해보기)

> 첫 목표는 **스텝 1~2**: GLFW를 빌드에 넣고 빈 창 하나 띄우기.
> Vulkan surface는 창이 확실히 뜬 다음에 붙인다.

### Step 1 — GLFW 빌드 통합 ✅ (mac, 스크립트 작성/검증 완료)
GLFW는 **소스에서 빌드**한다 (spdlog와 동일한 `fetch_source` + `build_library` 패턴).
[install_dependencies_mac.sh](../../scripts/install_dependencies_mac.sh):
- `fetch_source "glfw" ".../glfw.git" "3.4" ...` 로 소스 받고,
  `build_library "glfw" ... "-DGLFW_BUILD_{EXAMPLES,TESTS,DOCS}=OFF"` 로 빌드/설치.
- **prebuilt zip 방식은 폐기.** 이유: 소스 빌드는 GLFW가 **자기 `glfw3Config.cmake`
  (+ Targets/Version)를 자동 생성**해서 config를 손으로 쓸 필요가 없고, macOS
  프레임워크(Cocoa/IOKit/CoreFoundation) 링크도 그 config가 알아서 넣는다.
  게다가 ubuntu와도 동일 경로로 처리 가능(리눅스는 prebuilt가 아예 없음).
- 검증: `find_package(glfw3 CONFIG)` → `glfw3_FOUND=1`, target `glfw`에
  프레임워크+Threads 자동 포함. debug/release 둘 다 설치 확인.

완료(✅):
- `cpp/CMakeLists.txt`에 `find_package(glfw3 ... CONFIG)` + `target_link_libraries(... glfw)`.
- `src/window.cpp` / `src/surface.cpp` 를 mpvk 소스 목록에 등록.

남은 것:
- ⬜ **ubuntu**: 같은 `fetch_source`+`build_library` 를 `install_dependencies_ubuntu.sh`
  에도 대칭 추가. 단 GLFW 소스 빌드에 X11/Wayland dev 패키지 필요(apt) — 나중에.

### Step 2 — `mpvk::Window` ✅
- 헤더: `explicit Window(int w, int h, const char* title);`, `~Window();`,
  복사 금지, `GLFWwindow* handle() const;`, `should_close()`, `poll_events()`,
  정적 `required_instance_extensions()`(Step 3에서 사용).
- cpp: `glfwInit()`(반환값 체크) → `glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)`
  → `glfwCreateWindow(...)`(null 체크) . 소멸자에서 `glfwDestroyWindow` + `glfwTerminate`.
- main에서 `while(!window.should_close()) window.poll_events();` 로 창이 뜨고
  X로 닫히는지 눈으로 확인.

> **실전 노트 (구현 후 확정) — GLFW 래핑 함정**
> - **헤더 전방 선언은 `struct GLFWwindow;`.** GLFW가 `typedef struct GLFWwindow ...`
>   라서. `class`로 쓰면 clang/gcc는 통과하지만 MSVC C4099 경고(멀티플랫폼 주의).
> - **`std::unique_ptr<GLFWwindow>`는 그냥은 안 된다.** 기본 deleter가 `delete`를
>   호출하는데 GLFW 창은 `new`가 아니라 `glfwCreateWindow` 산물이라 UB. 쓰려면
>   **커스텀 deleter(`glfwDestroyWindow`) + `~Window()`를 .cpp에 정의**(incomplete
>   타입 소멸 규칙, pimpl과 동일)가 필요. → 이 단계에선 **raw 포인터 RAII**가 더 단순.
> - **생성자 필수 3종:** `glfwInit()` 먼저(안 하면 create가 `NOT_INITIALIZED`로 null),
>   `GLFW_CLIENT_API=GLFW_NO_API`(안 하면 불필요한 OpenGL 컨텍스트 생성),
>   create 결과 null 체크 후 throw.
> - **`poll_events()`가 왜 필요한가:** OS 이벤트 큐를 비워 창을 살아있게 하고,
>   X 버튼 클릭이 `glfwWindowShouldClose`에 반영되게 한다. 안 부르면 응답없음 +
>   루프가 안 끝남. (렌더링 앱은 non-blocking인 `PollEvents`, `WaitEvents` 아님.)
> - `glfwTerminate()`는 소멸자에서 무조건 호출 → **창 하나 가정**. 여러 창이면 재검토.

> 여기까지 완료. 나머지(surface/present/device 확장)는 Step 3부터 이어서.
