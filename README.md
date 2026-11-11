# clone-lin-curve-gety

Experimental LIN clone of Curve Finance 3pool `get_D` / `get_y` (i64-positive scale: `SCALE=1e9`, `N=3`, 128-bit Newton `(a*b+add)/d`, fail-closed if the quotient does not fit i64).

- **Upstream:** [curvefi/curve-contract](https://github.com/curvefi/curve-contract) `contracts/pools/3pool/StableSwap3Pool.vy` commit `bae23a2fc9a0302d174ea1ffce8219a44a92149c`. Curve.Fi publishes that tree as informational-only (no reproduction license); this repository does **not** vendor the Vyper source. Hashes only: sha256 `03e0bf29ae2fe945a3629b8085062da8f9d9d25957e86001dfc3df68c895f9dc`, git blob `04d170189a1b5ddfecfc0af9ea3fe23eb0840423`.
- **LIN kernel:** [`src/lin_curve_gety.lin`](src/lin_curve_gety.lin) (sha256 `0cf9bafedb1274974ebb11208f1461b90a0ba1843e15b56e9b58574f17243de2`, identical to lin-open)
- **C11 oracle copy:** [`test/oracles/curve_gety_c11.c`](test/oracles/curve_gety_c11.c)
- **Proofs (lin-open only):** [kbelludoo/lin-open#262](https://github.com/kbelludoo/lin-open/pull/262) `examples/curve_gety/` and `python3 test/prove_curve_gety_external.py` (16 PASS / 0 FAIL / 1 SKIP JIT this run)
- **Class:** EXPERIMENTAL. Not Ethereum Curve 3pool uint256. Not `RATES`/`PRECISION`/fee/`exchange`. Not a production pool replacement.

Named goldens (i64-positive, equal `1e9` balances): `get_D(..., A=100)` → **3000000000**; identity `get_y` → **1000000000**; `get_y` after +10% `x0` at `A=100` → **900099889** vs `A=2000` → **900005047**; `get_dy` no-fee +10% → **99900110**; `get_dy dx=0` fail-closed **0** (uint256 would wrap `xp[j]-y-1 = -1`); imbalanced `D` → **3099778671**; naive i64 `D^2` wraps to **-8838116264523025375**.
