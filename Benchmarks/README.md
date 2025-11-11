# Benchmarks

성능 측정 및 최적화를 위한 벤치마크 시스템

## 📊 벤치마크 카테고리

### 1. 렌더링 성능
- **Draw Call Overhead**: 드로우 콜 수에 따른 성능 측정
- **State Change Cost**: 파이프라인 상태 전환 비용
- **Descriptor Binding**: 디스크립터 바인딩 오버헤드

### 2. 메모리 관리
- **Upload Heap Performance**: 업로드 힙 전송 속도
- **Resource Creation**: 리소스 생성 시간
- **Descriptor Allocation**: 디스크립터 할당 속도

### 3. 멀티스레딩
- **Command List Recording**: 커맨드 리스트 기록 병렬화 효율
- **CPU Utilization**: CPU 사용률 측정

### 4. 최신 기술
- **DXR Performance**: 레이트레이싱 성능
- **Mesh Shader Throughput**: 메시 셰이더 처리량
- **VRS Impact**: Variable Rate Shading 효과

## 📈 측정 지표

- **FPS**: Frames Per Second
- **Frame Time**: 프레임당 소요 시간 (ms)
- **GPU Time**: GPU 실행 시간
- **CPU Time**: CPU 작업 시간
- **Memory Usage**: 메모리 사용량
- **Draw Calls**: 드로우 콜 수
- **Triangles**: 렌더링된 삼각형 수

## 🔧 벤치마크 실행 방법

```bash
# 전체 벤치마크 실행
DX12GameEngine.exe --benchmark all

# 특정 카테고리만 실행
DX12GameEngine.exe --benchmark rendering

# 결과 출력 형식 지정
DX12GameEngine.exe --benchmark all --format csv
```

## 📁 결과 파일 형식

### CSV 형식
```csv
Timestamp,BenchmarkName,FPS,FrameTime(ms),GPUTime(ms),CPUTime(ms),DrawCalls,Triangles
2024-01-15 10:30:00,DrawCallOverhead_1000,120.5,8.3,7.2,1.1,1000,500000
```

### JSON 형식
```json
{
  "timestamp": "2024-01-15T10:30:00",
  "system_info": {
    "gpu": "NVIDIA RTX 4090",
    "driver": "546.33",
    "os": "Windows 11"
  },
  "results": [
    {
      "name": "DrawCallOverhead_1000",
      "fps": 120.5,
      "frame_time_ms": 8.3,
      "gpu_time_ms": 7.2,
      "cpu_time_ms": 1.1,
      "draw_calls": 1000,
      "triangles": 500000
    }
  ]
}
```

## 🎯 최적화 목표

| 카테고리 | 현재 | 목표 | 상태 |
|---------|------|------|------|
| 드로우 콜 오버헤드 | - | < 0.01ms | 🔴 미측정 |
| 디스크립터 바인딩 | - | < 0.005ms | 🔴 미측정 |
| 커맨드 리스트 기록 | - | > 90% 병렬화 | 🔴 미측정 |

## 📝 벤치마크 추가 가이드

1. `Benchmarks/` 디렉토리에 벤치마크 코드 추가
2. `BenchmarkRegistry`에 등록
3. 측정 지표 정의
4. 결과 분석 및 문서화

## 🔗 참고 자료

- [GPU Performance for Game Artists](http://www.fragmentbuffer.com/)
- [NVIDIA Nsight Graphics](https://developer.nvidia.com/nsight-graphics)
- [PIX for Windows](https://devblogs.microsoft.com/pix/)
