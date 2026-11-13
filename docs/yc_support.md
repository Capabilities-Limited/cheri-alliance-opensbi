OpenSBI YC (CHERI Compartmentalisation) Extension
=================================================

The YC extension lets S-mode supervisor software run small, self-contained RVY
(CHERI) purecap payloads -- **compartments** embedded in the OpenSBI firmware
image -- as isolated, capability-confined computations. The OpenSBI M-mode
firmware, not the supervisor, owns the isolation boundary and enforces it with
CHERI capabilities rather than with privilege level or PMP.

The motivating use is to run inference payloads (for example, one model in
compartment 0 and another in compartment 1) where the firmware confines each
payload to its own code and data and a pair of shared buffers, and nothing else.

The isolation goal is: while a compartment runs, it can reach only its own
code/data image and the two shared buffers it is given, and nothing of the
firmware or of any other compartment.

Security Model
--------------

The security-relevant adversary is a buggy or malicious compartment payload; YC
exists to confine it. A secondary, defence-in-depth concern is a buggy
supervisor driver that passes bad buffer parameters.

* **Supervisor software and its YC driver** - orchestrates calls and owns the
  shared buffers; trusted to choose the buffer parameters. The primary client.
* **OpenSBI M-mode firmware (including the YC code)** - builds the capabilities,
  performs the context switch, and enforces the boundary. The trusted computing
  base.
* **Compartment payload** - the confined computation. Untrusted: the thing
  isolation must contain.
* **Shared buffers** - the supervisor/compartment data channel. Untrusted
  contents.

SBI Interface
-------------

The extension ID is **SBI_EXT_YC** (`0x08FFFFFF`). The function IDs are:

* **SBI_EXT_YC_INIT** (`0x0`) - declared, not implemented
* **SBI_EXT_YC_ENTER** (`0x1`) - enter a compartment
* **SBI_EXT_YC_STOP** (`0x2`) - declared, not implemented
* **SBI_EXT_YC_RESUME** (`0x3`) - resume a suspended compartment
* **SBI_EXT_YC_DISABLE** (`0x4`) - declared, not implemented

Only **ENTER** and **RESUME** are handled; the others return
**SBI_ERR_NOT_SUPPORTED**.

ENTER takes a packed compartment/entry id, a timeout, and the two shared
buffers:
```
a7 = 0x08FFFFFF (extid), a6 = 0x1 (funcid)
a0 = compartment/entry id:  id = a0 & 0xff,  entry = (a0 >> 8) & 0xff
a1 = timeout in time-CSR ticks (0 = no timeout)
a2 = pa0, a3 = size0    (first shared buffer, used as the input buffer)
a4 = pa1, a5 = size1    (second shared buffer, used as the output buffer)
```

RESUME re-enters a compartment that a timeout suspended:
```
a7 = 0x08FFFFFF (extid), a6 = 0x3 (funcid)
a0 = compartment id
a1 = timeout in time-CSR ticks
```

The entry id selects a per-model entry point and is reserved for future
multi-entry models; current single-entry models use entry 0.

The return follows the standard **struct sbiret**:

* **error** = **SBI_SUCCESS** (`0`) - the compartment returned cleanly; the
  supervisor may read the output buffer.
* **error** = **SBI_ERR_TIMEOUT** (`-12`) - the timeout fired; the compartment is
  suspended and may be resumed.
* **error** = **SBI_ERR_FAILED** (`-1`) - the compartment took a fault.
* **error** = **SBI_ERR_ALREADY_STARTED** (`-7`) - the call was rejected before
  the compartment ran: an ENTER whose id is already owned by a hart, or a RESUME
  from a hart that does not own the suspended compartment.
* **value** is `0`. The compartment's own status word is not propagated; a
  payload conveys its application-level result in the output buffer.

Compartment State
-----------------

### Per-compartment record

Each compartment has a **compartment_t** record (a global array indexed by id),
built afresh by each ENTER, saved into by the trap path, and reused by RESUME:

* **pcc** - sealed entry code capability, bounded to the compartment image, with
  the ASR permission (**CHERI_PERM_SYSTEM_REGS**) stripped
* **buf** - capability over the compartment's shared region; the basis for the
  per-call buffer capabilities
* **mtvec** - capability to the compartment trap handler, installed as the
  compartment's trap vector
* **gprs[32]** - saved integer registers. `ra` (x1) is a sealed sentry to the
  return path, `a0` (x10) is the read-only input-buffer capability, and `a1`
  (x11) is the write-only output-buffer capability
* **entries[4]** - per-entry code pointers, reserved for future multi-entry
  models (`mepc` is loaded from `pcc`)
* **mstatus** - `MPP` = M, `MIE`/`MPIE` cleared (`MPIE` set when a timeout is
  armed), `FS` set when FPU support is built, `VS` set when RVV support is built
* **fprs[32], fcsr** - saved FP state (when **CONFIG_SBI_YC_SUPPORT_FPU**)
* **vrs[32], vstart, vl, vtype, vcsr** - saved vector state (when
  **CONFIG_SBI_YC_SUPPORT_RVV**)
* **hartid** - the owning hart index while the compartment is running, suspended
  or faulted, and `-1` when idle; the atomic hart-pinning word

The record's byte layout is shared with the context-switch assembly through
*include/sbi/sbi_yc_offsets.h* and checked against the C struct at build time
with a static assertion.

### Per-hart OpenSBI save area

While a compartment runs, OpenSBI's own context is held in **struct
sbi_yc_opensbi_context** (the `ycntxt` member of **struct sbi_scratch**, so it
is per-hart): a pointer to the active compartment (`current_compartment`), the
callee-saved registers and M-mode CSRs, and the FP file when FPU support is
built. Its layout is also shared with the assembly and checked with
`assert_member_offset`.

Compartment Lifecycle
---------------------

```
          ENTER(id, bufs, timeout)          timeout (keep pin)
   IDLE ------------------------------> RUNNING ----------------> SUSPENDED
    ^     claim id (hartid -1 -> me)      |  |                        |
    |                                     |  |  RESUME (same hart)     |
    |     clean return (release id -1)    |  +<-----------------------+
    +-------------------------------------+
                                          |
                                          |  fault (keep pin)
                                          v
                                       FAULTED
          terminal: ENTER -> SBI_ERR_ALREADY_STARTED until a future
                    STOP/DISABLE reclaims the id
```

OpenSBI tracks one explicit per-compartment state: the `hartid` owner word.
`hartid == -1` is idle; any owning index means the id is held (running,
suspended, or faulted) by that hart.

* ENTER atomically claims an idle id (sets `hartid` from `-1` to the current
  hart index) and runs the compartment.
* A clean return releases the id (`hartid` back to `-1`).
* A timeout keeps the id pinned and suspends the compartment; only the owning
  hart may RESUME it.
* A fault keeps the id pinned in a terminal state; a bare ENTER will not reuse
  it and returns **SBI_ERR_ALREADY_STARTED**. Reclaiming such a slot is reserved
  for a future STOP/DISABLE path.

The finer running-versus-suspended distinction otherwise lives in the saved
**compartment_t** and in the supervisor driver's bookkeeping.

Execution Mechanism
-------------------

### ENTER (sbi_yc_enter)

1. Validate `id < 2` and `entry < 4`; otherwise return
   **SBI_ERR_INVALID_PARAM**.
2. Atomically claim the id: compare-exchange `hartid` from `-1` to the current
   hart index (acquire ordering). If the id is already owned, return
   **SBI_ERR_ALREADY_STARTED** without touching the record. Only the winning
   hart proceeds, which keeps the global array safe under concurrent entry.
3. Zero the **compartment_t** and restore the owning `hartid`.
4. Build `pcc`: take the firmware's root capability, set its address to the
   compartment entry point, bound it to that symbol, strip ASR, select
   capability mode, and seal it as an entry capability.
5. Build `buf` over the configured shared region with data (non-executable)
   permissions.
6. Build the trap-vector capability, the sealed return sentry in `ra`, and the
   two buffer capabilities from `buf`: the input `a0` narrowed to read-only and
   the output `a1` narrowed to write-only.
7. Read `mstatus`; set `MPP` = M, clear `MIE`/`MPIE`, and set `FS`/`VS` per the
   build configuration.
8. If a timeout was requested, arm the timer and set `MPIE` so the timer
   interrupt can fire inside the compartment.
9. Call `compartment_enter()`. On a clean return, release the id.

### Context switch (compartment_enter, assembly)

Saves OpenSBI's callee-saved registers and M-mode CSRs into `ycntxt` (recording
`current_compartment`), clears **DDC** (`csrw ddc, zero`) so that no ambient
data authority is inherited, loads the compartment's GPRs, `mepc`, `mtvec`,
`mstatus` and FP file from its record, and `mret`s into the compartment at
`pcc`. On a fresh ENTER the record was just zeroed, so no firmware register
state leaks in, and with `DDC` cleared the compartment cannot reach memory
through an inherited data capability even if it switches to integer-pointer
addressing mode.

### Clean return (compartment_return, assembly)

Reached when the compartment returns through its sealed return sentry. Flushes
the instruction cache, zeroes **DDC**, clears `x1`-`x31`, the FP file and (when
built) the vector file, restores OpenSBI's context, sets `a0` to `0`, and
returns to `sbi_yc_enter`. No compartment register state leaks out.

### Timeout or fault (compartment_trap, assembly)

Any trap taken while the compartment runs vectors to the compartment trap
handler. It restores OpenSBI's scratch, recovers `current_compartment`, saves
the compartment's live register state back into its record, sanitises the
registers and **DDC**, restores OpenSBI's context, and returns
**SBI_ERR_TIMEOUT** when the cause is the machine timer interrupt (suspend) or
**SBI_ERR_FAILED** otherwise (fault).

### RESUME (sbi_yc_resume)

Validates `id` and `entry`, then checks that the compartment is owned by the
current hart (otherwise **SBI_ERR_ALREADY_STARTED**), so a compartment is only
resumed on the hart that entered it. It re-arms `MPP`/`MIE`/`MPIE` and the timer
and calls `compartment_enter()` on the saved record, whose `mepc` points at the
interrupted instruction; it rebuilds no capability. A clean return releases the
id.

Isolation Argument
------------------

A compartment runs at M-mode but in capability (purecap) mode with: `PCC`
bounded to its image and lacking ASR; `DDC` zero; `ra` a sealed sentry; a
read-only input capability and a write-only output capability, each confined to
the compartment's shared slot; and every other integer register and the whole
FP file zero. In CHERI every memory access is authorised by a capability, so a
compartment's reachable memory is the transitive closure of the capabilities it
holds. Because

* it holds no authority beyond its image and the two buffers (`DDC` is cleared
  and the buffers are narrowed from the slot-bounded `buf`),
* it cannot fabricate authority (CHERI capability monotonicity), and
* it cannot read authority out of a CSR (ASR is stripped, so a CSR access
  traps),

its reachable set is confined to its image and the two shared buffers.
Isolation rests wholly on CHERI, not on privilege level: a compartment runs with
`MPP` = M, but privileged non-CSR instructions are benign, while CSR access and
`mret` require ASR and therefore trap.

The per-id atomic `hartid` claim keeps the global **compartment_t** array safe
when several harts call YC: at most one hart owns an id at a time, so two harts
cannot enter or resume the same compartment concurrently.

Concurrency and Interrupts
--------------------------

YC is intended for a single hart driving the compartments. The per-id `hartid`
claim makes concurrent use by several harts safe -- at most one hart owns a
compartment id at a time, and the others receive **SBI_ERR_ALREADY_STARTED** --
but running compartments on several harts in parallel is not a validated
configuration: the two compartments use fixed shared-buffer regions, so a
compartment id is a singleton, and the timer and interrupt behaviour below is
reasoned for a single active hart.

A compartment runs in M-mode, and while it runs its trap vector is the
compartment trap handler, so every trap it takes returns control to OpenSBI: a
synchronous exception or a non-timer interrupt as a fault (**SBI_ERR_FAILED**),
and the machine timer interrupt as a timeout (**SBI_ERR_TIMEOUT**). Two
consequences follow for interrupts:

* With a timeout armed, the compartment runs with machine interrupts enabled, so
  an inter-processor interrupt delivered to the hart -- for example a TLB
  shootdown or a remote fence -- is taken as a fault and ends the compartment.
* With no timeout, the compartment runs with machine interrupts disabled, so the
  hart does not service inter-processor interrupts until the compartment returns.

Keep a hart that is running a compartment out of cross-hart IPI operations (TLB
and fence masks), or bound the compartment run time, if either matters for the
platform.

The timeout is armed with the OpenSBI timer event, which is the same mechanism
that schedules the supervisor timer interrupt. It is not saved and restored
around a compartment, so a supervisor timer deadline that was set across an ENTER
is not preserved.

Configuration Options
---------------------

* **CONFIG_SBI_ECALL_YC** (default y) - build the YC extension.
* **CONFIG_SBI_YC_SUPPORT_FPU** (default y) - save and restore each
  compartment's FP state.
* **CONFIG_SBI_YC_SUPPORT_RVV** (default n) - save and restore each
  compartment's vector state.
* **CONFIG_YC_BUFFER0_ADDRESS** / **CONFIG_YC_BUFFER0_SIZE** and
  **CONFIG_YC_BUFFER1_ADDRESS** / **CONFIG_YC_BUFFER1_SIZE** - the two shared
  regions. They default to `0xa0000000` and the following 256 KiB region, 256
  KiB each.

The compartment payloads are embedded at build time by pointing
**FW_COMPARTMENT0_PATH** and **FW_COMPARTMENT1_PATH** at the payload binaries
when building the `fw_compartment` firmware.

Current Limitations
-------------------

* YC is validated for a single hart entering compartments; concurrent multi-hart
  execution is guarded but not a supported configuration (see Concurrency and
  Interrupts).
* A faulted compartment id stays pinned until a future STOP/DISABLE path can
  reclaim it; a bare ENTER cannot reuse it.
* The compartment timeout reuses the supervisor timer and is not restored on
  exit, and an inter-processor interrupt ends a running compartment (see
  Concurrency and Interrupts).
* Isolation covers architectural state: the integer registers, the FP and vector
  files, and `DDC` are cleared on exit, and the two compartments use disjoint
  buffers. It does not address microarchitectural side channels -- caches, the
  TLB, and branch predictors are not flushed or partitioned on a compartment
  boundary, so mutually distrusting compartments, or a compartment and its
  caller, are not isolated against cache-timing or contention channels.
* The per-compartment `entries[]` table is reserved for future multi-entry
  models; current single-entry models use entry 0.
* YC targets RVY (CHERI) purecap platforms; the capability operations it relies
  on are meaningless on non-CHERI platforms.
