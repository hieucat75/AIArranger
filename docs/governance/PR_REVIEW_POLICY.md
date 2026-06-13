# PR / Review Policy

> **Status:** ACTIVE  
> **Effective from:** Gate 4 (2026-06-13)  
> **Branching model:** gate-* branches

---

## Workflow

Every code change follows this exact flow:

```
branch → push → PR → CODEX REVIEW + MUSICAL REVIEW → PTH approve → merge
```

### Steps

1. **Branch** — Create `gate-N-description` from `main`. Never work directly on main.
2. **Commit** — Each task is a separate commit. Message format: `GN-TNN: description`
3. **Push** — `git push origin gate-N-description`
4. **PR** — Create PR on GitHub: `gate-N-description → main`
5. **Reviews** — Both reviews must pass:
   - **Codex Review** — Technical correctness, architecture compliance, test coverage
   - **Musical Review** — Musical correctness, feel, articulation, groove (see MUSICAL_REVIEW_TEMPLATE.md)
6. **Approve** — PTH approval on GitHub
7. **Merge** — Merge PR via GitHub UI (squash or merge commit)

### Hard Rules

- **No direct push to main** (except initial project skeleton)
- **No merge without both reviews**
- **No merge without PTH approval**
- **No importer merge without COMPATIBILITY_SCORE evidence**
- **No DRM bypass, no copyrighted bundling, no proprietary samples**

## Review Checklist (Codex)

- Architecture compliance (lock-free, no vendor runtime, no silent MegaVoice→GM)
- No regression (all existing tests pass)
- New tests for new code
- No memory leaks, no undefined behavior
- No secrets committed

## Review Checklist (Musical)

See `MUSICAL_REVIEW_TEMPLATE.md`
