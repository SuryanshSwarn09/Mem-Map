# AGENTS.md - Developer & Agent Operating Protocol for Mem-Map

This repository enforces strict development, architectural, and documentation standards for all human developers and AI pair-programming agents.

---

## 1. Documentation-First & Parallel Synchronization Rule
**MANDATORY**: Code changes must NEVER be introduced or committed in isolation without simultaneously updating the project documentation.
- When modifying or adding features, you **MUST** update both:
  1. `README.md`
  2. `docs/GITBOOK_DOCUMENTATION.md`
- Any changes to UI controls, keyboard shortcuts, or visual layouts must be reflected in:
  - The Mermaid.js diagrams (`graph TD`, `sequenceDiagram`, `classDiagram`)
  - The ASCII architecture mockups
  - The Continuous Improvement Ledger table in both documents.

---

## 2. Atomic Micro-Commit Standard
- Work must proceed as **focused, atomic micro-commits** (target: ~10 to 12 micro-commits per major feature).
- Exactly **one commit per completed and verified unit of work**.
- Each commit message must follow clear, declarative formatting: `Add X to Y`, `Implement Z in W`, `Enhance A with B`.
- **DO NOT PUSH TO REMOTE PER COMMIT**: Local micro-commits are created progressively. Pushes to `origin main` must only occur at the very end after the entire feature is verified and all automated unit tests pass.

---

## 3. Mandatory Automated Unit Testing
- Every feature or non-trivial refactoring must be accompanied by an automated unit test in `tests/test_main.cpp`.
- Before committing:
  ```powershell
  $env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH
  ninja -C build
  ./build/test_runner.exe
  ```
- All test suites must report `-> PASSED!` with zero errors and zero compiler warnings.

---

## 4. Continuous Improvement Ledger Maintenance
Whenever a milestone or feature is completed, update the **Parallel Improvement Ledger** table in:
- `README.md` (Section: *Continuous Improvement & Parallel Documentation Sync Matrix*)
- `docs/GITBOOK_DOCUMENTATION.md` (Section 7: *Continuous Improvement & Parallel Documentation Sync Matrix*)
Record:
- Phase name
- Core capabilities added
- Key source modules touched
- Git commit hash or range
- Name of automated unit test verifying the phase
- Verification status (✅ Synced)
