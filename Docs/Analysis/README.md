# Analysis - 분석 및 테스트

"성능은 어떠한가? 어떻게 검증하는가?"

이 섹션에서는 DX12 게임 엔진의 성능 분석, 테스트, 디버깅 방법을 다룹니다. 벤치마크 결과와 실험 노트를 기록합니다.

## 📑 문서 목록

### Performance/ - 성능 분석
- [BenchmarkStrategy.md](./Performance/BenchmarkStrategy.md) - 벤치마크 전략
- [ProfilingTools.md](./Performance/ProfilingTools.md) - 프로파일링 도구
- [GPUTiming.md](./Performance/GPUTiming.md) - GPU 타이밍 측정
- [Results/](./Performance/Results/) - 벤치마크 결과들

### Testing/ - 테스트
- [TestStrategy.md](./Testing/TestStrategy.md) - 테스트 전략
- [UnitTests.md](./Testing/UnitTests.md) - 유닛 테스트
- [IntegrationTests.md](./Testing/IntegrationTests.md) - 통합 테스트

### Debugging/ - 디버깅
- [DebuggingGuide.md](./Debugging/DebuggingGuide.md) - 디버깅 가이드
- [DebugLayer.md](./Debugging/DebugLayer.md) - DX12 Debug Layer 활용
- [PIXUsage.md](./Debugging/PIXUsage.md) - PIX 사용법
- [CommonIssues.md](./Debugging/CommonIssues.md) - 흔한 문제들

### Research/ - 연구 노트
- [ExperimentNotes.md](./Research/ExperimentNotes.md) - 실험 및 삽질 기록

## 🔗 다른 카테고리와의 관계

- **[Concepts](../Concepts/)**: 분석 대상 개념 이해
- **[Structure](../Structure/)**: 구조적 결정에 대한 분석
- **[Usage](../Usage/)**: 구현 후 성능 측정

## 📝 문서 작성 가이드

Analysis 문서는 다음을 포함해야 합니다:
1. 목적 (무엇을 분석/테스트하는가)
2. 환경 (하드웨어, 설정)
3. 방법론 (어떻게 측정/테스트했는가)
4. 결과 (Before/After 데이터)
5. 분석 (결과 해석)
6. 결론 (배운 점, 권장사항)
7. 관련 문서 링크

템플릿: [AnalysisTemplate.md](../Templates/AnalysisTemplate.md)
