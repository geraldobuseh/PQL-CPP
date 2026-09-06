# Personal Quant Lab — Global Agent Instructions

This document defines project-wide rules and governance for Personal Quant Lab.

## Five-Agent Operating Model

Personal Quant Lab uses five specialized agents with distinct roles and mandates. One agent owns implementation; four exist to challenge it from different angles.

```
                    JIRA TICKET
                         |
            +------------+------------+
            |                         |
            v                         v
      ARCHITECT AGENT           QUANT AGENT
            |                         |
            +------------+------------+
                         |
                         v
                 IMPLEMENTER AGENT
                         |
              +----------+----------+
              |                     |
              v                     v
       VALIDATION AGENT       C++ REVIEW AGENT
              |                     |
              +----------+----------+
                         |
                         v
                       ME (GERALD)
                 merge / reject / iterate
```

### The Five Roles

| Agent | Mandate |
|-------|---------|
| **Architect** | Design, interfaces, scope, invariants |
| **Quant Reviewer** | Financial correctness and experimental validity |
| **Implementer** | Production code |
| **Validator** | Tests, failure cases, regression evidence |
| **C++ Reviewer** | C++20 correctness, ownership, performance hygiene |

The developer remains the governance authority and decides whether to merge, iterate, reject, or escalate to a new ticket.

## Repository Structure

All agent roles and workflows are documented in:

```
AGENTS.md                    (this file)

.codex/
├── agents/
│   ├── architect.md
│   ├── quant-reviewer.md
│   ├── implementer.md
│   ├── validator.md
│   └── cpp-reviewer.md
│
├── workflows/
│   └── jira-ticket.md
│
└── reviews/                (persistent review artifacts for substantial tickets)
    ├── README.md
    └── PQL-XXX/
        ├── architecture.md
        ├── quant.md
        ├── validation.md
        └── cpp-review.md
```

See [.codex/reviews/README.md](.codex/reviews/README.md) for guidance on when and how to create review artifacts.

## Core Principles

### Financial Integrity

Personal Quant Lab is a financial system. Every implementation must be:

- **Deterministic**: Same inputs always produce same results
- **Auditable**: Every financial event is traceable
- **Conservative**: Never silently repair inconsistent state
- **Tested**: Financial invariants must be provable

### Architectural Invariants

The system follows intentional flow:

```
Strategy → Order → Broker / Execution → Transaction → Portfolio → Analytics
```

Critical rules:

- Strategies propose orders; they never mutate portfolio state
- Execution validates and places orders
- Transactions are the durable source of financial truth
- Portfolios are computed from transactions, never directly mutated
- Analytics consumes clean portfolio state

### Code Quality

Prioritize in this order:

1. Correctness
2. Determinism
3. Simplicity
4. Financial integrity
5. Maintainability
6. Testability
7. Observability
8. Performance

## Jira Workflow

For every non-trivial Jira ticket:

1. **Architecture Agent** determines the smallest coherent design
2. **Quant Agent** validates financial correctness (in parallel)
3. **Implementer** builds the required code
4. **Validator** challenges the implementation
5. **C++ Reviewer** checks code quality (in parallel with validation)
6. **You** merge, iterate, or reject

See `.codex/workflows/jira-ticket.md` for detailed workflow.

## Merge Criteria

A ticket is merge-ready only when:

- ✓ Acceptance criteria are satisfied
- ✓ Required tests pass
- ✓ No unresolved CRITICAL findings exist
- ✓ No unresolved HIGH financial correctness issue exists
- ✓ Implementation remains within scope
- ✓ Relevant invariants hold
- ✓ Remaining validation debt is explicit

## Agent Roles

Detailed role specifications:

- [Architect](`.codex/agents/architect.md`)
- [Quant Reviewer](`.codex/agents/quant-reviewer.md`)
- [Implementer](`.codex/agents/implementer.md`)
- [Validator](`.codex/agents/validator.md`)
- [C++ Reviewer](`.codex/agents/cpp-reviewer.md`)

## Workflows

- [Jira Ticket Workflow](`.codex/workflows/jira-ticket.md`)
