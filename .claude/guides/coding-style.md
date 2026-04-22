# 코딩 스타일 가이드

**적용 시점**: C++ 코드(`.h`/`.cpp`)를 작성하거나 수정할 때

---

## C++ 네이밍 규칙

```cpp
// 클래스명: PascalCase
class DescriptorHeapManager { };

// 멤버 함수: PascalCase
void AllocateDescriptor();

// 멤버 변수: m_ 접두사 + camelCase
ID3D12Device* m_device;
UINT m_descriptorSize;

// 지역 변수: camelCase
int frameIndex = 0;

// 상수: k 접두사 + PascalCase
constexpr UINT kMaxDescriptors = 1024;
```

---

## 문서화 주석

공개 API에는 문서화 주석을 작성합니다:

```cpp
/**
 * @brief 디스크립터를 할당합니다
 * @param type 디스크립터 타입
 * @return 할당된 디스크립터 핸들
 */
D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type);
```

---

## 소유권 표현

```cpp
std::unique_ptr<Texture> texture;     // 독점 소유
std::shared_ptr<Material> material;   // 공유 소유
Mesh* mesh;                            // 비소유 참조
```

---

## 관련 문서

- 아키텍처 설계 원칙 (인터페이스, DI 등): `.claude/guides/architecture.md`
- DX12 관련 코드 패턴: `.claude/guides/dx12.md`
