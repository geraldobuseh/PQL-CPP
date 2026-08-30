# Learning Checkpoint 001: Bets vs. Features

## The Question

Why is "build a momentum strategy" weaker product thinking than "test whether recent price momentum contains useful predictive information?"

---

## The Answer

Because they are fundamentally different kinds of work.

### "Build a momentum strategy" is a *feature*.

**Feature thinking asks**: What should I build?

- Start with a mechanism: "Let me buy high-momentum stocks"
- Build it: Write the code, define the rules
- Run it: Execute the strategy
- Evaluate: Did it make money?

**The problem**: This is aspiration engineering, not experimental science.

If a momentum strategy makes money in a bull market, you've learned nothing about whether momentum is useful. Bull markets make almost everything profitable.

If the strategy loses money, you blame the implementation ("I didn't tune it right") instead of questioning the premise.

If the strategy performs inconsistently, you add complexity ("add a volatility filter," "add risk limits") instead of testing the core assumption.

Feature thinking conflates building *something* with learning *something*.

---

### "Test whether momentum contains predictive information" is a *bet*.

**Bet thinking asks**: What do I want to learn? What would prove or disprove it?

- Start with a hypothesis: "Recent price changes predict future price changes"
- Define falsification: "If this is true, a rule using momentum should outperform SPY by X%, with Y Sharpe, across Z markets"
- Design the experiment: Pick your signal, define your rules, control for costs and bias
- Run it: Execute against a concrete benchmark
- Interpret the evidence: Did the experiment resolve the hypothesis?

**The strength**: This is experimental rigor.

If momentum strategies consistently underperform SPY after fees in multiple market regimes, you've learned something: momentum (as tested) doesn't contain predictive power. The conclusion is repeatable and falsifiable.

If momentum strategies outperform, you have evidence (not proof, but evidence) of potential edge. You can replicate the experiment, test in new regimes, or dig into why.

If inconsistent results arise, you ask "Is my hypothesis wrong?" not "Did I tune the implementation correctly?"

Bet thinking separates the *mechanism* (momentum-based rules) from the *claim* (momentum predicts returns).

---

## Why This Matters for PQL

Personal Quant Lab is built on the bet framework, not the feature framework.

**Features we will NOT build:**
- ✗ "A momentum strategy" (too specific to a mechanism)
- ✗ "A machine-learning forecaster" (sounds impressive, but what are we testing?)
- ✗ "An automated portfolio optimizer" (optimization for what purpose?)

**Bets we WILL formalize:**
- ✓ "Test whether recent momentum contains predictive power"
- ✓ "Test whether mean-reversion strategies survive real execution costs"
- ✓ "Test whether diversification reduces volatility below SPY"

Each bet is falsifiable. Each has a clear resolution. Each produces evidence that either supports or refutes a claim.

---

## The Operational Difference

### Feature thinking in practice:

```
Manager: "Build a momentum strategy"
Engineer: [writes code]
Manager: "Did it work?"
Engineer: "Yes, +15% return"
Manager: "Great, ship it"

[6 months later in a flat market]
Manager: "Why did it lose money?"
Engineer: "Market was wrong, not the strategy"
```

No learning. No falsification. No way to know if the strategy ever had edge.

### Bet thinking in practice:

```
Manager: "Test whether momentum predicts future returns"
Engineer: "OK. I'll run momentum rules on 10 years of SPY data. 
          I'll compare to SPY total return.
          If momentum beats SPY by 3%+ annually with acceptable Sharpe
          in multiple regimes, I'll call it evidence of edge."
Manager: "What if it loses?"
Engineer: "Then momentum, as I tested it, doesn't work. 
          We reject the hypothesis and move to the next bet."

[Results come back: momentum underperforms in bear markets]
Manager: "So what did we learn?"
Engineer: "Momentum works in rallies but dies in drawdowns. 
          That tells us something about the regime-dependence of the signal.
          We can now test a *new* hypothesis:
          Can we combine momentum with volatility regime detection?"
```

Each result is actionable. Failure teaches something. Success is evidence, not just luck.

---

## Three Levels of Thinking

### Level 1: Build Mode

"I have an idea. Let me code it and see what happens."

- *Example*: Implement a momentum backtest
- *Risk*: No falsification; feature thinking; assumes success
- *Suitable for*: Exploration, prototyping, learning the tools
- *Unsuitable for*: Investment decisions, system architecture, production

### Level 2: Testing Mode (This is Bet Thinking)

"I have a hypothesis. Let me design an experiment to test it fairly."

- *Example*: Define what momentum means, what "works" means, what the baseline is, run the test, interpret results
- *Risk*: Rigorous but slower; requires discipline to avoid cherry-picking
- *Suitable for*: Investment decisions, financial claims, repeatable processes
- *Unsuitable for*: Rapid exploration

### Level 3: Validation Mode

"I have a hypothesis with supporting evidence. Let me verify it holds in new data."

- *Example*: Test the momentum hypothesis on future periods, new markets, new instruments
- *Risk*: Deferral; only used for high-stakes decisions
- *Suitable for*: Deploying strategies for real capital
- *Unsuitable for*: Initial exploration

---

## PQL-001 is Level 2 Work

The Bet Register (PQL-001) formalizes the hypothesis and resolution signal. It asks:

- What do we want to know?
- How will we know it?
- What would prove us wrong?

This is the foundation. Without it, we're in Level 1 (build mode) forever.

Strategies will be tested against this bet framework. If a strategy beats SPY persistently and verifiably, we have evidence of edge worth believing.

If a strategy loses or performs inconsistently, we move to the next bet.

---

## Takeaway

**Build mode** produces code. **Bet mode** produces learning.

Personal Quant Lab chooses bet mode because our goal is not "deploy a strategy" but "understand whether edge is possible" and "if so, under what conditions."

Bets force clarity:
- Hypothesis: What are we testing?
- Resolution: How will we know?
- Benchmark: Against what?
- Costs: What's realistic?
- Regimes: Does it work everywhere?

Every substantial decision in PQL flows through this framework. Strategy proposals, risk models, and portfolio changes must be formalized as bets, not features.