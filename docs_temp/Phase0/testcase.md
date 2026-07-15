# Phase 0 테스트케이스

Phase 0은 requirement.md 기반 기능 동작이 아니라 PoC 이식/패키지 구조 셋업(스캐폴딩) 단계라, Given/When/Then 형태의 통상적인 TestCase는 없다.

검증은 다음으로 대체한다.
- `/system-test` 스킬을 통한 Release 빌드 성공 확인 (`.claude/skills/system-test/SKILL.md`)
- Review 에이전트의 아키텍처(design.md §2~4, §11) 준수 여부 검토

발견된 특이사항/이상점은 [abnormal.md](abnormal.md) 참고. Phase 1부터는 이 파일에 실제 TestCase(테스트 ID/사전 조건/실행 동작/기대 결과)를 기록한다.
