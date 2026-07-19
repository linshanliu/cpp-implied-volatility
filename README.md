# Arbitrage-Free Volatility Surface

A C++ pipeline that takes raw market option prices, extracts their implied
volatilities, fits a smooth **volatility surface** across strikes and
maturities, and — crucially — checks that the fitted surface contains **no
arbitrage**.

The input is a real option chain (an S&P-linked underlying is included as sample
data). The output is a calibrated surface plus a per-maturity diagnostic report
saying whether each fitted slice is arbitrage-free and, if not, where it fails.

---

## Table of contents

1. [What a volatility surface is, and why you'd build one](#1-what-a-volatility-surface-is-and-why-youd-build-one)
2. [The pipeline at a glance](#2-the-pipeline-at-a-glance)
3. [Step 1 — from prices to implied volatilities](#3-step-1--from-prices-to-implied-volatilities)
4. [Step 2 — fitting a smile with SVI](#4-step-2--fitting-a-smile-with-svi)
5. [Step 3 — the no-arbitrage check (the point of the project)](#5-step-3--the-no-arbitrage-check-the-point-of-the-project)
6. [How the code is organised](#6-how-the-code-is-organised)
7. [Walking through one run](#7-walking-through-one-run)
8. [Known limitations](#8-known-limitations)
9. [Building and running](#9-building-and-running)

---

## 1. What a volatility surface is, and why you'd build one

The Black–Scholes formula turns one number — the **volatility** `σ` — into an
option price. Run it backwards: given a market price, solve for the `σ` that
reproduces it. That number is the option's **implied volatility**.

Here's the catch that makes this whole field exist. If Black–Scholes were
literally true, every option on the same underlying would imply the *same* `σ`.
They don't. Plot implied volatility against strike and you get a curved shape —
the **volatility smile** (or skew). Plot it against strike *and* maturity and
you get a **surface**.

Why anyone needs the surface:

- **Pricing options that aren't quoted.** The market only quotes a handful of
  strikes and maturities. To price something in between — or a more exotic
  contract — you need volatility *everywhere*, so you fit a continuous surface
  through the quoted points.
- **Feeding more advanced models.** A local-volatility model (Dupire) is built
  directly from an arbitrage-free surface. A surface with arbitrage in it
  produces a broken local-vol model (negative variances, imaginary
  volatilities). So the surface has to be clean before anything downstream can
  use it.

That second point is why the arbitrage check in Step 3 is the heart of this
project, not an afterthought.

---

## 2. The pipeline at a glance

```
raw option chain (CSV)
        │
        ▼
   clean & filter        DataCleaner
        │
        ▼
  invert each price  →  implied vol      EuropeanOption  (Newton–Raphson)
        │
        ▼
  group by maturity, fit a smile         SVISlice        (SVI + Nelder–Mead)
        │
        ▼
  check each fitted smile for arbitrage  SVISlice        (Durrleman condition)
        │
        ▼
  per-maturity diagnostic report         mainSVI
```

Each stage is one class doing one job. The rest of this README follows the data
through those stages.

---

## 3. Step 1 — from prices to implied volatilities

**The problem.** Black–Scholes gives `price = BS(σ)`. We have the price and want
`σ`. There's no formula to invert it, so we solve it numerically.

**The method: Newton–Raphson.** To solve `BS(σ) = market_price`, start from a
guess and repeatedly correct it:

```
σ  ←  σ  −  ( BS(σ) − market_price ) / BS'(σ)
```

`BS'(σ)` — the price's sensitivity to volatility — is called **vega**, and it
has a closed form, so each step is cheap. The iteration converges very fast
(quadratically) because the BS price is smooth and monotic in `σ`.

This lives in `EuropeanOption`:

- `BS_Price(...)` — the Black–Scholes price for a given `σ`,
- `Vega(...)` — the derivative used in the Newton step,
- `ImpliedVolatility(...)` — the Newton loop itself.

**Two things the code guards against**, because a naive Newton loop breaks on
real data:

- **Vanishing vega.** For deep in- or out-of-the-money options vega gets tiny,
  and dividing by a tiny number sends the iteration flying off. The loop detects
  `|vega| < ε` and stops rather than producing garbage.
- **Non-convergence.** If the price is outside the model's arbitrage bounds (bad
  data, stale quote), no `σ` exists. Rather than loop forever or return a
  nonsense number, `ImpliedVolatility` throws — the caller then knows to discard
  that quote.

The result of this stage is: every usable market quote now carries a clean
implied volatility.

---

## 4. Step 2 — fitting a smile with SVI

We now have a scatter of `(strike, implied vol)` points at each maturity. We want
a smooth curve through them — the smile. We can't just connect the dots: a
piecewise-linear interpolation isn't smooth and, worse, has no way to guarantee
no-arbitrage.

**The parameterisation: SVI** (Stochastic Volatility Inspired, Gatheral 2004).
Instead of fitting `σ` directly, we fit **total variance** `w = σ² · T` as a
function of **log-moneyness** `k = ln(K/F)` (how far the strike is from the
forward `F`, on a log scale). The SVI formula is:

```
w(k) = a + b · [ ρ·(k − m) + √((k − m)² + σ²) ]
```

Five parameters, each with a clear geometric role (documented in `SVISlice.hpp`):

| Param | Controls                                           |
|-------|----------------------------------------------------|
| `a`   | overall level of the curve                          |
| `b`   | steepness of the two wings                           |
| `ρ`   | skew / tilt of the smile (asymmetry left vs right)  |
| `m`   | horizontal position of the minimum                   |
| `σ`   | curvature at the bottom (how rounded the trough is) |

SVI is the industry-standard smile parameterisation because it's flexible enough
to fit real smiles with just five numbers, and — the reason it was invented — it
comes with **explicit conditions for no-arbitrage** (Step 3).

**The fit: calibration.** For each maturity, we find the five parameters that
make `w(k)` pass as closely as possible through the market points, by minimising
the sum of squared errors. This is done in `SVISlice::Calibrate`.

Two details worth knowing:

- **Custom Nelder–Mead.** The minimiser is a from-scratch Nelder–Mead simplex
  search (`SVISlice::NelderMead`) — no external optimisation library. It's a
  derivative-free method: it crawls a "simplex" of trial points downhill by
  reflecting, expanding and contracting. Robust and self-contained.
- **Unconstrained reparameterisation.** The parameters have constraints
  (`b > 0`, `σ > 0`, `−1 < ρ < 1`). Rather than use a constrained optimiser, the
  code optimises transformed variables that are automatically valid:
  `b = exp(x)`, `σ = exp(x)`, `ρ = tanh(x)`. The optimiser then roams freely over
  all of ℝ and every point it tries is a legal SVI. (See `Transform` /
  `Untransform`.) This is a standard trick that turns a hard constrained problem
  into an easy unconstrained one.

One preliminary the code handles: **the forward `F`**. Log-moneyness needs the
forward price for each maturity, which isn't given directly. It's recovered from
the quotes themselves using **put–call parity** (`F = C − P + K`, averaged over
strikes where both a call and a put trade). See `EstimateForwards` in `mainSVI`.

---

## 5. Step 3 — the no-arbitrage check (the point of the project)

A curve can fit the market points beautifully and still be **nonsense** — it can
imply things like a negative probability, which means a risk-free money machine
exists. A surface with arbitrage in it is useless for anything downstream. So
after fitting, every slice is checked.

**What "no arbitrage" means here.** From an option price curve you can back out
the market's implied **probability density** of the future price. Mathematically
that density is proportional to the *second derivative* of price with respect to
strike. A density must be non-negative everywhere. If the fitted curve is too
sharply bent in the wrong direction, that "density" goes negative — impossible,
and exploitable. This specific failure is called **butterfly arbitrage**.

**The Durrleman condition.** Gatheral & Jacquier showed this reduces to a single
function of the fitted curve and its first two derivatives:

```
g(k) = (1 − k·w'/2w)²  −  (w'²/4)·(1/w + 1/4)  +  w''/2   ≥  0   for all k
```

If `g(k) ≥ 0` everywhere, the slice is butterfly-arbitrage-free.

**Why doing this on SVI is clean** (and worth pointing out in the code): because
SVI is an explicit formula, `w`, `w'` and `w''` are all available **analytically**
— exact derivatives, no finite differencing. The alternative (numerically
differentiating an interpolated price curve twice) amplifies noise badly; a
second numerical derivative on noisy data is a mess. Getting the derivatives
exactly from the SVI formula sidesteps that entirely. This is
`SVISlice::CheckButterflyArbitrage`, and its header comment makes exactly this
point.

The code also checks a cheaper **necessary** condition on the wings,
`b(1 + |ρ|) ≤ 4/T` (`SatisfiesLargeMoneynessCondition`), which catches obviously
bad fits before the full scan.

**What comes out.** `CheckButterflyArbitrage` returns not just a yes/no but the
**worst point**: the minimum value of `g(k)` and the log-moneyness where it
occurs. So the report doesn't just say "this slice has arbitrage" — it says
where, which is what you'd need to diagnose the fit.

> Note on scope. This checks **butterfly** arbitrage (within a single maturity).
> The other kind — **calendar** arbitrage (inconsistency *across* maturities) —
> requires comparing slices against each other and is noted in the code as
> belonging one level up, in the surface, rather than in an individual slice. It
> is not yet implemented. Together these two are exactly the conditions for the
> surface to feed a valid local-volatility model.

---

## 6. How the code is organised

Each class owns one stage. Data flows through them in order.

```
OptionQuote        ← a plain struct: one row of the option chain
                     (strike, maturity, bid/ask/mid, type, IV, ...)

CSVReader          ← reads the raw chain into a vector<OptionQuote>
DataCleaner        ← filters out unusable quotes (bad prices, etc.)
CSVWriter          ← writes results back out

EuropeanOption     ← Black–Scholes price, vega, and the
                     Newton–Raphson implied-vol inversion (Step 1)

SVISlice           ← ONE maturity: fits the SVI smile (Step 2) and
                     runs the Durrleman arbitrage check (Step 3)

VolatilitySurface  ← holds smiles across maturities; interpolates
                     vol at an arbitrary (strike, maturity)

mainSVI            ← the driver: forwards via put–call parity, then
                     calibrate + check each maturity, print the report
```

The central design choice is that **`SVISlice` deliberately handles one maturity
only**. Its header explains why: butterfly arbitrage is a within-slice property
(one maturity), so it belongs in the slice; calendar arbitrage is a
between-slice property, so it belongs one level up in `VolatilitySurface`. Keeping
that boundary clean is what stops the two concerns from tangling.

`VolatilitySurface` is a second, simpler representation: it stores the observed
smiles maturity by maturity and interpolates between them (linear in strike,
sorted maturities via `std::map`). It's the lookup layer; `SVISlice` is the
modelling layer.

---

## 7. Walking through one run

`mainSVI.cpp` drives the SVI calibration and arbitrage check:

1. **Load** the option quotes (strike, maturity, mid price, type, IV).
2. **Estimate the forward** `F` for each maturity by put–call parity across
   matched call/put strikes (`EstimateForwards`).
3. **For each maturity:**
   - build an `SVISlice` with that maturity and forward,
   - `Calibrate(...)` — fit the five SVI parameters to the quotes,
   - `CheckButterflyArbitrage()` — scan the fitted curve for violations.
4. **Print a table**, one row per maturity:

   | column     | meaning                                              |
   |------------|------------------------------------------------------|
   | `T`        | maturity                                             |
   | `F`        | forward price                                        |
   | `RMSE`     | calibration error, in vol terms                      |
   | `b, ρ, σ`  | fitted SVI parameters                                |
   | `arb_free` | yes / NO                                             |
   | `worstG`   | worst (most negative) Durrleman `g(k)` on the slice  |

That table *is* the deliverable: for every maturity, how good the fit is and
whether it's arbitrage-free.

---

## 8. Known limitations

- **Calendar arbitrage not yet checked.** Only butterfly (within-maturity)
  arbitrage is verified. The cross-maturity check belongs in `VolatilitySurface`
  and is on the to-do list. Both together are what a downstream local-vol model
  needs.
- **Two surface representations coexist.** `SVISlice` (parametric SVI fit) and
  `VolatilitySurface` (interpolated observed smiles) are parallel approaches;
  the project would be clearer with the driver committing to one as the primary
  output.
- **Forward estimation needs matched pairs.** The put–call-parity forward
  requires both a call and a put at the same strike/maturity; maturities without
  such pairs are skipped.
- **Single underlying tested.** The pipeline has been run on the included sample
  chain; it hasn't been stress-tested across many underlyings or market regimes.

---

## 9. Building and running

The project ships as a Visual Studio solution. `mainSVI.cpp` is the entry point
for the SVI calibration and arbitrage report; it reads the sample option data
and prints the per-maturity diagnostic table described in §7.

Sample data included:
- `SPCX_options_raw.csv` — raw option chain,
- `SPCX_options_IV.csv` — the same quotes carrying computed implied vols.

A cross-platform CMake build is planned so the project builds without Visual
Studio.
