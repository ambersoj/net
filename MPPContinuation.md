# MPP Continuation Brief

> **Purpose**: This document is a handoff and re‑orientation artifact. It captures the current architecture, design principles, and operational state of the MPP system so that a *new ChatGPT session (or human)* can rapidly regain full context and become productive without replaying history.

---

## 1. What MPP Is

**MPP (Message‑Passing Plant)** is a snapshot‑driven, bus‑oriented control architecture for orchestrating stateful components (FSM, NET, etc.) using explicit JSON snapshots and asynchronous observables.

Key properties:

* No threads
* No hidden timers
* No implicit ticks
* Deterministic state transitions
* Explicit I/O via a UDP BUS

Everything happens because a snapshot was applied or a message was observed.

---

## 2. Core Architectural Canon

### Canon 1: Snapshot‑Driven Components

All components:

* Expose *registers* via `serialize_registers()`
* Accept *snapshots* via `apply_snapshot(json)`
* Never mutate state except inside `apply_snapshot()` or explicit action methods it triggers

There is no background work.

---

### Canon 2: FSM Guards See Only Observed History

FSM guards **never see live state**.

They only see:

* What was last published on the BUS
* Before the current FSM state began

This means:

* Side effects must occur in one state
* Observations must be evaluated in a later state

This led to the introduction of explicit *observation states* (e.g. `OBSERVE_TX`).

> **Rule**: *If a guard depends on feedback, it must be separated by at least one state boundary.*

---

### Canon 3: BUS Is an Event Journal, Not a Query Mechanism

The BUS:

* Is UDP‑based (loopback in dev)
* Carries discrete JSON events
* Has no request/response semantics
* Is lossy by design

Consumers must tolerate missed events by designing FSMs that re‑observe or loop.

---

### Canon 4: Flat Control, Structured Status

Snapshots are flat key/value maps.

BUS events follow this schema:

```json
{
  "component": "NET",
  "status": {
    "tx_done": true,
    "rx_done": true
  }
}
```

Why:

* Flat snapshots are easy to compose
* Structured `status` prevents key collisions
* FSM guard syntax becomes `NET.tx_done=true`
* HUD / tooling can introspect generically

---

## 3. Components

### 3.1 FSM Component

**Role**: Orchestrator.

FSM:

* Parses PlantUML
* Applies state notes as snapshots
* Evaluates guards against observed BUS messages
* Never polls other components

Important internals:

* `observed_` map keyed by `component`
* Guards are evaluated against `observed_`
* Observations are updated only via `on_message()`

FSM transitions are atomic and single‑fire per step.

---

### 3.2 NET Component

**Role**: Real network I/O (libnet + pcap).

NET:

* Is snapshot‑driven
* Does no work on BUS messages
* Emits BUS events only on meaningful edges

Key actions:

* `tx_fire` → builds and sends ICMP echo
* `rx_fire` → samples pcap once

Key events:

* TX completion → publishes `{status:{tx_done:true}}`
* RX completion → publishes `{status:{rx_done:true,...}}`

NET is intentionally *edge‑triggered*, not streaming.

---

## 4. FSM Design Pattern (Ping Example)

### States Used

* `INIT` – enable subsystems
* `CONFIGURE` – configure headers, devices, filters
* `TX` – fire transmit
* `WAIT_TX` – quiescent
* `OBSERVE_TX` – evaluate `NET.tx_done`
* `WAIT_REPLY` – fire RX sampling
* `GOT_REPLY` – RX observed
* `DONE` – cleanup

### Why OBSERVE_TX Exists

TX completion occurs *during* TX.

FSM guards cannot see that change until the next step.

Thus:

```
TX → WAIT_TX → OBSERVE_TX → WAIT_REPLY
```

This pattern is canonical.

---

## 5. What Was Just Proven

In the last run:

* ICMP echo was sent on the wire
* Echo reply was captured by pcap
* NET published TX and RX events
* FSM observed them correctly
* FSM reached DONE deterministically

No stdout logging.
No polling.
No race conditions.

---

## 6. Debugging Techniques That Work

* `tcpdump -i lo udp port 3999` → observe BUS
* FSM `observed_` map inspection → validate guards
* `step:true` snapshots → deterministic stepping

Avoid:

* Adding sleeps
* Adding background threads
* Logging as control flow

---

## 7. What Is *Not* Yet Built (By Design)

* Continuous RX loops
* Automatic retries
* Timeouts
* Streaming modes
* HUD UI

These will be layered *on top* of the current model.

---

## 8. Recommended Names for This Document

* **Continuation Brief** (current)
* System Canon
* Architecture Handoff
* Control‑Plane Field Guide
* MPP Operational Primer

---

## 9. How to Resume in a New Session

1. Paste this document
2. Say: *“This is the current state of MPP”*
3. Ask for next goals (timeouts, loops, HUD, refactors)

The system is stable and extensible at this point.

================================================
HUD
================================================

# MPP‑HUD Continuation Brief

> **Purpose**: This document is a handoff and re‑orientation artifact for the **MPP HUD component**. It captures the HUD’s register contract, rendering model, and semantic rules so a new ChatGPT session (or human) can immediately understand how the HUD works, why it is shaped the way it is, and how it should evolve without breaking canon.

---

## 1. What the HUD Is

The **HUD (Head‑Up Display)** is a *pure observer* component whose sole responsibility is to render **control‑plane progress over time** in a human‑legible, terminal‑native form.

The HUD:

* Does **not** issue commands
* Does **not** participate in FSM logic
* Does **not** interpret business semantics
* Does **not** react to BUS messages

Instead, it renders **temporal structure**:

* what has completed
* what is currently active
* what lies ahead
* whether the system is running and connected

It is intentionally boring in implementation and powerful in effect.

---

## 2. Canon: Snapshot‑Driven Only

The HUD is **entirely snapshot‑driven**.

```cpp
void Hud::on_message(const json&) {
    // HUD reacts only to register changes
}
```

All state changes occur via:

* `apply_snapshot()`
* explicit register mutation
* semantic pulses (`row_advance`)

This guarantees:

* deterministic rendering
* replayability
* no timing coupling to the BUS

---

## 3. The HUD Register Contract

The HUD exposes a deliberately small register set:

```cpp
struct HudRegisters {
    uint32_t row_count;
    uint32_t active_row;
    bool     running;
    bool     connected;
    bool     row_advance;   // semantic pulse
    std::string last_detail;
};
```

### Register semantics

| Register      | Meaning                                                  |
| ------------- | -------------------------------------------------------- |
| `row_count`   | Total height of the channel (history + active + future)  |
| `active_row`  | Index of the currently active row                        |
| `running`     | Whether the system is executing                          |
| `connected`   | External connectivity status                             |
| `row_advance` | **Edge‑triggered pulse** indicating temporal progression |
| `last_detail` | Human‑readable status or error detail                    |

These registers describe **control‑plane facts**, not data‑plane content.

---

## 4. Canon: `row_advance` Is a Pulse, Not State

`row_advance` is intentionally **non‑latching**.

```cpp
if (j.value("row_advance", false)) {
    ...
    regs_.row_advance = false; // consume pulse
}
```

This establishes a critical rule:

> **Advancement is an event, not a condition.**

Why this matters:

* prevents repeated advancement on re‑render
* aligns with FSM transition semantics
* allows precise temporal stepping

The HUD consumes the pulse exactly once and converts it into structural history.

---

## 5. The Channel Is a Temporal Ledger

The vertical channel is not a list — it is a **timeline**.

```
  |
 / \
| B |
| B |
| B |
|   |
 \ /
  |
```

Internally this is represented by:

```cpp
std::vector<bool> row_completed_;
```

### Ledger rules

* Once a row is completed, it never reverts
* The active row is always the next uncompleted row
* History is immutable
* The channel only grows downward

This makes the HUD a **ledger**, not a dashboard.

---

## 6. Absolute Overrides Exist (By Design)

Some states (RESET, ERROR, COMPLETE) are **non‑temporal**.

For these cases, the HUD allows an absolute override:

```cpp
if (j.contains("active_row")) {
    regs_.active_row = new_row;
}
```

This enables:

* hard resets
* jumps to terminal rows
* recovery after failure

without violating normal advancement semantics.

---

## 7. ANSI Escape Sequences Are the Transport

The HUD uses raw ANSI terminal control — nothing else.

```cpp
std::cout << "\033[2J\033[H";
```

Meaning:

* `\033[2J` → clear screen
* `\033[H`  → home cursor

Benefits:

* SSH‑friendly
* tmux/screen safe
* zero external dependencies
* maximal portability

The HUD assumes only a monospace terminal.

---

## 8. Rendering Is Idempotent

`render_ascii()` is a **pure function of state**.

It:

* does not mutate registers
* does not advance rows
* does not emit events

It is safe to call:

* on every snapshot
* on parse errors
* on partial updates

If rendering is wrong, the problem is state — not rendering logic.

---

## 9. Visual Encoding Is Semantic

Current encoding uses symbol stability rather than decoration:

* `| B |` → active or completed
* `|   |` → future

Active and completed share the same glyph deliberately — continuity matters more than emphasis.

Color is intentionally deferred until semantics stabilize. When added, color must **reinforce meaning**, never replace it.

---

## 10. Errors Are Rendered, Not Logged

Errors are part of the rendered state:

```cpp
regs_.last_detail = e.what();
render_ascii();
```

This ensures:

* errors are visible without logs
* failures are terminal‑friendly
* no dependency on stderr

HUD errors annotate the system; they do not crash it.

---

## 11. Design Intent Summary

The MPP HUD is:

* snapshot‑driven
* pulse‑aware
* temporally honest
* history‑preserving
* terminal‑native
* dependency‑free
* semantically minimal

It answers only:

* Are we running?
* Are we connected?
* Where are we in time?
* What just happened?

Everything else belongs elsewhere.

---

## 12. Suggested Alternate Names (Optional)

* Temporal Channel Renderer
* Control‑Plane Ledger
* Execution Timeline HUD
* Observer Console

“HUD” is already valid — it now has canonical meaning.

---

## 13. How to Resume in a New Session

1. Paste this document
2. Say: *“This is the current HUD architecture”*
3. Ask for extensions (multi‑lane channels, color semantics, FSM coupling)

The HUD is stable, canonical, and ready to evolve.
