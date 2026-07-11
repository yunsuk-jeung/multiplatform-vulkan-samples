# C++ Notes — 작업 중 나온 C++ 상식

Vulkan과 무관하게, 샘플을 구현하며 마주친 **일반 C++ 코딩 상식**을 모아둔다.
(Vulkan 개념은 [concepts.md](concepts.md), 샘플별 노트는 [samples/](samples/).)

새 항목은 아래에 계속 추가한다. 각 항목: **규칙 + 왜 + 예시**.

---

### 지역 변수 값 반환에 `std::move`를 붙이지 마라 (pessimizing move)
- **규칙:** 함수에서 지역 변수를 값으로 반환할 땐 `return x;` — `return std::move(x);` 금지.
- **왜:** 값 반환은 NRVO(named return value optimization, 복사 생략)가 적용된다.
  `std::move`를 붙이면 반환 타입이 rvalue가 되어 **NRVO가 막히고** 강제로 이동 생성자가
  호출된다(대개 더 느리고, 최소한 무의미). 컴파일러가 `-Wpessimizing-move`로 경고한다.
- **예시:**
  ```cpp
  std::vector<const char*> f() {
    std::vector<const char*> v;
    // ...
    return v;              // ✅ NRVO
    // return std::move(v); // ❌ pessimizing move
  }
  ```
- 등장: [03_window_surface](samples/03_window_surface.md) `Window::required_instance_extensions`

### 전방 선언만으로 참조/포인터 매개변수 선언 가능
- **규칙:** 함수 매개변수가 참조(`const T&`)든 포인터(`const T*`)든, 헤더에는
  `class T;`(또는 `struct T;`) **전방 선언만** 있으면 된다. 완전 정의는 멤버 접근이
  일어나는 `.cpp`에서 include 하면 된다.
- **왜:** "전방 선언하면 포인터만 된다"는 오해. 완전 타입이 필요한 건 **값 멤버**
  (`T m_;`), `sizeof`, 멤버 접근뿐이다. 참조/포인터 매개변수 *선언*은 크기를 몰라도 된다.
  (단 전방 선언 줄 자체는 있어야 한다 — 없으면 `unknown type name`.)
- 등장: [02_logical_device](samples/02_logical_device.md) `Device(const PhysicalDevice&)`

### 전방 선언 class-key는 실제 정의와 맞춰라 (`struct` vs `class`)
- **규칙:** C 라이브러리 타입 전방 선언은 실제 정의의 키워드를 따른다. 예:
  GLFW는 `typedef struct GLFWwindow ...` → `struct GLFWwindow;`.
- **왜:** clang/gcc는 class-key 불일치를 무시하지만 **MSVC는 C4099 경고**. 멀티플랫폼 주의.
- 등장: [03_window_surface](samples/03_window_surface.md)

### C-API 핸들에 `std::unique_ptr` 쓸 땐 커스텀 deleter
- **규칙:** `new`가 아니라 C 함수(`glfwCreateWindow` 등)로 만든 자원을 `unique_ptr`에
  담으려면 **커스텀 deleter**(그 C의 destroy 함수 호출)를 줘야 한다. 기본 deleter는
  `delete`를 호출 → `new` 산물이 아니면 UB.
- **부수 규칙:** 불완전 타입을 담은 `unique_ptr`는 소멸 지점에서 완전 타입이 필요 →
  소멸자를 헤더 inline 두지 말고 `.cpp`에 정의(`= default` 가능). pimpl과 동일 이유.
- **대안:** 자원이 하나뿐이고 클래스가 이미 RAII면 raw 포인터 + 소멸자에서 destroy가 더 단순.
- 등장: [03_window_surface](samples/03_window_surface.md) `Window`

### 예외는 `const` 참조로 잡아라
- **규칙:** `catch (const std::exception& e)` — `catch (std::exception e)`(값) 금지.
- **왜:** 값으로 잡으면 파생 예외가 **slicing** 되어 정보가 잘린다(+불필요한 복사).
- 등장: [02_logical_device](samples/02_logical_device.md) main

### `size()`와 비교하는 루프 인덱스는 부호 없는 타입
- **규칙:** `for (uint32_t i = 0; i < v.size(); ++i)` — `int i` 금지.
- **왜:** `size()`는 `size_t`(부호 없음)라 `int`와 비교하면 `-Wsign-compare` 경고 +
  대입 대상이 `uint32_t`(예: Vulkan family index)면 타입도 어긋난다.
- 등장: [02_logical_device](samples/02_logical_device.md) queue family 탐색
