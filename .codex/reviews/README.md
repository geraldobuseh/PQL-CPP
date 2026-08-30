# Review Artifacts

This directory stores persistent review evidence for substantial Jira tickets.

## Purpose

Not every ticket needs formal review artifacts. But for critical implementations—especially those involving financial algorithms, core infrastructure, or complex logic—documenting what agents discovered turns your repository into an engineering history.

Six months later, you can answer:

- **What did we believe?** — assumptions, invariants, design constraints
- **What did we build?** — implementation decisions, tradeoffs
- **What risks did we identify?** — financial correctness issues, performance concerns, edge cases
- **What tests proved it?** — validation evidence, test results, edge case coverage
- **What did we still not know?** — validation debt, deferred work, open questions

## Structure

Reviews are organized by ticket ID:

```
.codex/reviews/
├── PQL-001/
│   ├── architecture.md
│   ├── quant.md
│   ├── validation.md
│   └── cpp-review.md
├── PQL-022/
│   ├── architecture.md
│   ├── quant.md
│   ├── validation.md
│   └── cpp-review.md
└── ...
```

## When to Create

Create review artifacts for:

- ✓ **Financial algorithms** — any code that affects portfolio valuation, P&L calculation, or risk assessment
- ✓ **Core infrastructure** — system boundaries, persistence layers, execution engines
- ✓ **Complex logic** — unusual algorithms, concurrent code, specialized domain logic
- ✓ **High-risk work** — features with significant test debt or assumptions requiring later validation
- ✓ **Precedent-setting decisions** — architectural choices that affect future tickets

Skip review artifacts for:

- ✗ Minor bug fixes or refactors without new behavior
- ✗ Simple UI changes
- ✗ Documentation-only tickets
- ✗ Straightforward feature additions with no novel risks

## Contents

### architecture.md

From the Architecture Agent:

```markdown
# PQL-XXX: Architecture Review

## Objective
[What capability is required?]

## Current Architecture
[Existing components affected]

## Proposed Design
[Smallest coherent change]

## Contracts
[Interfaces/domain contracts affected]

## Invariants
[What must remain true]

## Risks
[What could materially go wrong]

## Validation Requirements
[Evidence needed for correctness]

## Decision
ARCHITECTURE READY / ARCHITECTURE CHANGES REQUIRED
[Reasoning]
```

### quant.md

From the Quant Reviewer:

```markdown
# PQL-XXX: Quantitative Correctness Review

## Financial Assumptions
[Return calculations, compounding, cost basis, P&L, etc.]

## Invariants
[Financial constraints]

## Bias / Leakage Risks
[Look-ahead bias, survivorship bias, data snooping, etc.]

## Required Edge Cases
[Critical scenarios to test]

## Benchmark Requirements
[What comparison is appropriate?]

## Findings
[CRITICAL/HIGH/MEDIUM/LOW severity issues]

## Decision
QUANT REVIEW PASSED / QUANT CHANGES REQUIRED
[Reasoning]
```

### validation.md

From the Validation Agent:

```markdown
# PQL-XXX: Validation Report

## Acceptance Criteria
- [PASS/FAIL/UNVERIFIED] Criterion 1 — [Evidence]
- [PASS/FAIL/UNVERIFIED] Criterion 2 — [Evidence]

## Tests Run
[Actual commands executed]

## Failures Found
[CRITICAL/HIGH/MEDIUM/LOW] — Issue description

## Regression Risk
[Potential impact on existing behavior]

## Validation Debt
[Unverified items for future work]

## Decision
VALIDATION PASSED / VALIDATION FAILED
[Reasoning]
```

### cpp-review.md

From the C++ Reviewer:

```markdown
# PQL-XXX: C++20 Code Review

## Ownership & Lifetime
[Resource ownership patterns, potential lifetime issues]

## Undefined Behavior
[Dangling pointers, invalid iterators, data races, etc.]

## RAII & Exception Safety
[Cleanup correctness, exception-safe patterns]

## API Clarity
[Interface design, const correctness]

## Findings
[CRITICAL/HIGH/MEDIUM/LOW] — Specific issues with file/line locations

## Decision
C++ REVIEW PASSED / C++ CHANGES REQUIRED
[Reasoning]
```

## Usage

1. **During ticket processing**: Each agent completes their review and saves to the appropriate file
2. **Before merge**: Ensure all required review files exist and all decisions are PASSED
3. **After merge**: Review artifacts become permanent history—link to them from commit messages or PR descriptions when relevant
4. **For future work**: Reference past reviews to understand design decisions, financial assumptions, and identified risks

## Example: Financial Algorithm Review

Imagine implementing a new portfolio valuation method. A thorough review would capture:

**architecture.md** → Design preserves transaction-based source of truth
**quant.md** → Assumptions about rounding, cost basis treatment, handling missing dividends
**validation.md** → Edge case tests (zero shares, negative cash, missing history)
**cpp-review.md** → Precision handling for decimal math, iterator safety

Six months later when debugging a P&L discrepancy, you know *exactly* what assumptions were embedded, what edge cases were tested, and what was left unverified.

## Never Fabricate

Never add review artifacts with fabricated evidence:

- ✗ Do not claim tests passed if they were not run
- ✗ Do not claim risks were reviewed if they were not examined
- ✗ Do not mark items UNVERIFIED unless they genuinely were not tested

Mark validation debt explicitly. An honest "UNVERIFIED" is more valuable than a false "PASSED".
