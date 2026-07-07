# docs — 학습 기록

이 프로젝트는 [Khronos Vulkan-Samples](https://github.com/KhronosGroup/Vulkan-Samples)
의 샘플들을 **바닥부터 직접 구현**하면서 Vulkan을 배우는 게 목표다.

Khronos처럼 거대한 프레임워크를 먼저 만들지 않는다. 대신
**"샘플이 처음 필요로 하는 순간에 프레임워크(`mpvk`) 조각을 하나씩 추가"** 하는
증분(incremental) 방식으로 간다.

## 문서

- [roadmap.md](roadmap.md) — 전체 학습 경로. 어떤 순서로, 무엇을 배우고,
  각 단계에서 `mpvk`에 어떤 클래스를 추가하는지.
- [progress.md](progress.md) — 진행 로그(저널). 작업할 때마다 여기에 기록한다.

## 구조

```
cpp/                 # mpvk : 모든 샘플이 링크하는 공용 프레임워크 라이브러리
  include/mpvk/      #   공개 헤더
  src/               #   구현
samples/             # 샘플별 실행파일 (NN_name/main.cpp)
third_party/         # Vulkan SDK (로컬 설치)
docs/                # 이 문서들
```

## 빌드 / 실행

```bash
direnv allow                       # Vulkan SDK 환경변수 로드 (최초 1회)
cmake --preset debug
cmake --build --preset debug
./build/debug/samples/01_device_info/01_device_info
```
