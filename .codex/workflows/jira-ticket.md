# Jira Ticket Multi-Agent Workflow

For each non-trivial Personal Quant Lab Jira ticket, follow this workflow.

## Inputs

- Jira ticket ID
- ticket description
- acceptance criteria
- repository state
- AGENTS.md

## Agent Execution Order

```
JIRA TICKET
    |
    +--> ARCHITECTURE
    |
    +--> QUANT REVIEW
    |
    v
IMPLEMENTATION
    |
    +--> VALIDATION
    |
    +--> C++ REVIEW
    |
    v
HUMAN GOVERNANCE
```

### Stage 1: Design Review (Parallel)

**Architecture Agent** and **Quant Agent** may work independently and concurrently.

Their outputs feed the Implementation Agent.

### Stage 2: Implementation

**Implementer** builds the smallest coherent change that satisfies the ticket and preserves project invariants.

### Stage 3: Quality Review (Parallel)

**Validation Agent** and **C++ Reviewer** work independently and concurrently.

Both challenge the implementation from different angles.

### Saving Review Artifacts (Conditional)

For substantial tickets—especially those involving financial algorithms, core infrastructure, or complex domain logic—save review artifacts to `.codex/reviews/PQL-XXX/`:

```
.codex/reviews/PQL-022/
├── architecture.md      (from Architecture Agent)
├── quant.md             (from Quant Reviewer)
├── validation.md        (from Validation Agent)
└── cpp-review.md        (from C++ Reviewer)
```

Review artifacts create an engineering history. Months later, you can reconstruct:
- What did we believe? (assumptions, invariants)
- What did we build? (decisions, tradeoffs)
- What risks did we identify? (financial, correctness, performance)
- What tests proved it? (validation evidence)
- What did we still not know? (validation debt)

See [.codex/reviews/README.md](.codex/reviews/README.md) for structure and content guidance.

Not every ticket needs review artifacts. Skip them for minor bug fixes, simple UI changes, or straightforward feature work.

### Stage 4: Governance

The human owner receives all evidence and decides whether to merge.

## Merge Conditions

A ticket is merge-ready only when:

- ✓ acceptance criteria are satisfied
- ✓ required tests pass
- ✓ no unresolved CRITICAL findings exist
- ✓ no unresolved HIGH financial correctness issue exists
- ✓ implementation remains within scope
- ✓ relevant invariants hold
- ✓ remaining validation debt is explicit

## Human Decision

The developer decides:

- **MERGE** — all criteria satisfied; ready for production
- **ITERATE** — specific changes needed; return to affected agent(s)
- **REJECT** — fundamental issues; consider closing or creating new ticket
- **NEW BET** — discovery that belongs in a separate ticket; record for future work

## Agent Accountability

Each agent is responsible for:

- **Architect**: Design soundness, scope, invariant preservation
- **Quant**: Financial correctness, experimental validity, bias detection
- **Implementer**: Code quality, test coverage, determinism
- **Validator**: Evidence of correctness, edge case coverage, regression detection
- **C++ Reviewer**: Ownership clarity, undefined behavior, maintainability

Never fabricate evidence. If something was not tested, mark it unverified.

## Common Patterns

### Scope Creep

If implementation discovers required work outside the ticket:

- **Document as Required**
- Implementer pauses
- Architect reassesses scope
- Iterate with developer

### Financial Assumption Mismatch

If Quant Agent identifies incorrect assumptions:

- Stop implementation
- Architect and Quant collaborate on correction
- Resume implementation with updated assumptions

### Validation Failure

If Validator finds defect:

- Document root cause
- Classify severity
- If CRITICAL or HIGH: return to Implementer for fix
- Add regression test before resuming validation

### C++ Issue

If C++ Reviewer finds correctness issue:

- Return to Implementer for fix
- Re-run validation if fix affects behavior
- Resume C++ review

### Persistent Disagreement

If you and an agent disagree fundamentally:

- Document both positions
- Merge with a clear plan to address in follow-up ticket
- Never silently override financial correctness concerns
