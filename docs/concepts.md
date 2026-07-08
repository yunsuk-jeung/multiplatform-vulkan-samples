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

<!--
새 항목 템플릿:

### <개념 이름>
<한두 줄 설명>
- 처음 등장: [NN_sample_name](samples/NN_sample_name.md)
- 관련: [MM_other_sample](samples/MM_other_sample.md)
-->
