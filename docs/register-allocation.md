# MiniGPU Register Allocation & Liveness Analysis

## 1. Liveness Analysis

Liveness is computed using standard backward data-flow analysis over the Control-Flow Graph (CFG).

### Data-Flow Equations
For each basic block $B$:
- $\text{USE}[B]$: Registers read before being defined in $B$.
- $\text{DEF}[B]$: Registers written in $B$.
- $\text{LIVE\_OUT}[B] = \bigcup_{S \in \text{succ}(B)} \text{LIVE\_IN}[S]$
- $\text{LIVE\_IN}[B] = \text{USE}[B] \cup (\text{LIVE\_OUT}[B] \setminus \text{DEF}[B])$

Iteration runs backward until the sets reach a fixed point.

### Live Intervals
Instructions are sequenced sequentially from index $0$ to $N$. For each virtual register $r$, a live interval $[\text{start}, \text{end}]$ is computed, spanning from its definition point to its last observed use.

## 2. Interference Graph & Coloring

An interference graph is constructed where:
- Each vertex is a virtual register.
- An undirected edge connects two registers if their live intervals overlap in time.

### Chaitin-Briggs Graph Coloring Algorithm
1. **Simplify**:
   - Find a vertex with degree $< K$ (where $K$ is the configurable register budget).
   - Push to the selection stack and remove from the graph.
2. **Spill Heuristic**:
   - If all vertices have degree $\ge K$, select the vertex with the maximum degree as a spill candidate and push to the stack.
3. **Select**:
   - Pop vertices from the stack in reverse order.
   - Assign the lowest physical register color ($R_0, R_1, \dots, R_{K-1}$) not currently taken by interfering neighbors.
   - If no color is available, the vertex is spilled to `__local_frame[spill_slot]`.
4. **Verification**:
   - An allocation verifier checks that no two adjacent vertices in the interference graph share the same physical register.
