"""
Verify the stylized facts in one simulation run.

Usage:  uv run verify.py [TAG]
        TAG selects out/quotes_TAG.csv; omit it for out/quotes.csv
"""

from __future__ import annotations

import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import polars as pl
from scipy import stats
from statsmodels.stats.diagnostic import acorr_ljungbox
from statsmodels.tsa.stattools import acf

# Load data
ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "out"
FIGS = Path(__file__).resolve().parent / "figures"


def load(tag: str | None) -> tuple[pl.DataFrame, pl.DataFrame]:
    suffix = f"_{tag}" if tag else ""
    quotes_path = OUT / f"quotes{suffix}.csv"
    trades_path = OUT / f"trades{suffix}.csv"

    for p in (quotes_path, trades_path):
        if not p.exists():
            raise FileNotFoundError(f"{p} not found; run the simulation first")

    quotes = pl.read_csv(quotes_path, null_values=[""])
    trades = pl.read_csv(trades_path)
    return quotes, trades


# Statistics
def log_returns(prices: np.ndarray) -> np.ndarray:
    return np.diff(np.log(prices))


def kurtosis_by_aggregation(prices: np.ndarray, levels=(1, 5, 10, 25, 50, 100)) -> dict[int, float]:
    """Excess kurtosis of returns sampled every `m` steps.

    Genuine fat tails persist under aggregation; kurtosis driven by
    zero-inflation collapses toward 0 as 1/m.
    """
    return {m: float(stats.kurtosis(log_returns(prices[::m]))) for m in levels}


def acf_with_band(x: np.ndarray, nlags: int) -> tuple[np.ndarray, float]:
    """Autocorrelation and the 95% band under a white-noise null."""
    values = acf(x, nlags=nlags, fft=True)[1:]  # drop lag 0, which is always 1
    band = 1.96 / np.sqrt(len(x))
    return values, band


def clustering_horizon(values: np.ndarray, band: float, run: int = 5) -> int:
    """Last lag before `run` consecutive lags fall within the band.

    Only positive excursions count: a negative autocorrelation is not
    clustering. Returns 0 if the series never clusters.
    """
    inside = np.abs(values) <= band
    for i in range(len(values) - run + 1):
        if inside[i:i + run].all():
            return i
    return len(values)


# Figures
def fig_price(quotes: pl.DataFrame, p_f: float, path: Path) -> None:
    t = quotes["timestamp"].to_numpy()
    p = quotes["price"].to_numpy()

    fig, (ax_p, ax_s) = plt.subplots(
        2, 1, figsize=(11, 6), sharex=True, height_ratios=[3, 1]
    )

    ax_p.plot(t, p, lw=0.6, color="#1b3a4b")
    ax_p.axhline(p_f, color="#c1440e", lw=1, ls="--", label=f"fundamental {p_f:g}")
    ax_p.set_ylabel("price")
    ax_p.legend(frameon=False)
    ax_p.set_title("Transaction price and spread")

    spread = quotes["spread"].to_numpy()
    ax_s.plot(t, spread, lw=0.4, color="#7a5c3e")
    ax_s.set_ylabel("spread")
    ax_s.set_xlabel("step")

    fig.tight_layout()
    fig.savefig(path, dpi=150)
    plt.close(fig)


def fig_distribution(returns: np.ndarray, path: Path) -> None:
    """Return density against a fitted Gaussian, on a log scale.

    A log y-axis is essential: on a linear scale the tails are invisible,
    which is exactly where the two distributions differ.
    """
    r = returns[returns != 0]
    z = (r - r.mean()) / r.std()

    fig, (ax_d, ax_q) = plt.subplots(1, 2, figsize=(11, 4.5))

    bins = np.linspace(z.min(), z.max(), 121)
    ax_d.hist(z, bins=bins, density=True, color="#1b3a4b", alpha=0.75, label="simulated")
    grid = np.linspace(z.min(), z.max(), 400)
    ax_d.plot(grid, stats.norm.pdf(grid), color="#c1440e", lw=1.5, label="Gaussian")
    ax_d.set_yscale("log")
    ax_d.set_xlabel("standardized return")
    ax_d.set_ylabel("density")
    ax_d.legend(frameon=False)
    ax_d.set_title(f"excess kurtosis {stats.kurtosis(r):.2f}")

    stats.probplot(z, dist="norm", plot=ax_q)
    ax_q.set_title("Normal Q-Q")
    ax_q.get_lines()[0].set(markersize=2, color="#1b3a4b")
    ax_q.get_lines()[1].set(color="#c1440e")

    fig.tight_layout()
    fig.savefig(path, dpi=150)
    plt.close(fig)


def fig_autocorrelation(returns: np.ndarray, path: Path, nlags: int = 300) -> None:
    a_r, band = acf_with_band(returns, nlags)
    a_abs, _ = acf_with_band(np.abs(returns), nlags)
    lags = np.arange(1, nlags + 1)
    h = clustering_horizon(a_abs, band)

    fig, ax = plt.subplots(figsize=(11, 4.5))
    ax.axhspan(-band, band, color="0.85", label="95% white-noise band")
    ax.plot(lags, a_r, lw=0.9, color="#1b3a4b", label=r"$r_t$")
    ax.plot(lags, a_abs, lw=0.9, color="#c1440e", label=r"$|r_t|$")
    ax.axhline(0, color="black", lw=0.5)
    ax.set_xlabel("lag")
    ax.set_ylabel("autocorrelation")
    ax.legend(frameon=False)
    ax.set_title(
        f"acf(r,1) = {a_r[0]:+.3f}   acf(|r|,1) = {a_abs[0]:+.3f}   "
        f"clustering horizon {h} lags"
    )

    fig.tight_layout()
    fig.savefig(path, dpi=150)
    plt.close(fig)


# Report
def report(quotes: pl.DataFrame, trades: pl.DataFrame, p_f: float = 100.0) -> None:
    p_step = quotes["price"].to_numpy()
    r_step = log_returns(p_step)
    r_trade = log_returns(trades["price"].to_numpy())

    a_abs, band = acf_with_band(np.abs(r_step), 400)
    a_r, _ = acf_with_band(r_step, 400)

    print(f"steps            {len(p_step)}")
    print(f"trades           {len(trades)}")
    print(f"zero returns     {100 * np.mean(r_step == 0):.1f}%")
    print(f"price range      {p_step.min():.2f} .. {p_step.max():.2f}   (p_f = {p_f:g})")
    print(f"within 10% of pf {100 * np.mean(np.abs(np.log(p_step / p_f)) < 0.1):.1f}% of steps")

    spread = quotes["spread"].drop_nulls().to_numpy()
    price_at_quote = quotes.drop_nulls("spread")["price"].to_numpy()
    print(f"spread           {np.median(spread):.3f} "
          f"({100 * np.median(spread / price_at_quote):.2f}% of price)")

    print("\nFAT TAILS  (excess kurtosis; collapses toward 0 if driven by zeros)")
    for m, k in kurtosis_by_aggregation(p_step).items():
        print(f"  aggregation {m:>3}   {k:7.2f}")
    print(f"  trade time      {stats.kurtosis(r_trade):7.2f}")

    print("\nVOLATILITY CLUSTERING")
    print(f"  acf(r, 1)             {a_r[0]:+.4f}   (near zero = no linear predictability)")
    print(f"  acf(|r|, 1)           {a_abs[0]:+.4f}")
    print(f"  acf(|r|, 10)          {a_abs[9]:+.4f}")
    print(f"  acf(|r|, 50)          {a_abs[49]:+.4f}")
    print(f"  clustering horizon    {clustering_horizon(a_abs, band)} lags")

    print("\n  Ljung-Box (two-sided: rejects on negative correlation too)")

    lags = [10, 20, 50, 100]
    lb_r = acorr_ljungbox(r_step, lags=lags, return_df=True)
    lb_abs = acorr_ljungbox(np.abs(r_step), lags=lags, return_df=True)
    print(f"  {'m':>5} {'p(r)':>12} {'p(|r|)':>12}")
    for m in lags:
        print(f"  {m:>5} {lb_r.loc[m, 'lb_pvalue']:12.2e} "
              f"{lb_abs.loc[m, 'lb_pvalue']:12.2e}")

    # Ljung-Box cannot distinguish clustering from bid-ask bounce, so report
    # the signed lag-1 terms and how much of the statistic they contribute.
    share = 10 * a_abs[0] ** 2 / sum((10 - k) * a_abs[k] ** 2 for k in range(10))
    print(f"\n  acf(|r|, 1) is {a_abs[0]:+.4f} and supplies "
          f"{100 * share:.0f}% of the m=10 statistic")
    print("  A negative lag 1 means depletion, not clustering.")


# Main
def main() -> None:
    tag = sys.argv[1] if len(sys.argv) > 1 else None
    quotes, trades = load(tag)

    FIGS.mkdir(exist_ok=True)
    suffix = f"_{tag}" if tag else ""

    report(quotes, trades)

    p_step = quotes["price"].to_numpy()
    fig_price(quotes, 100.0, FIGS / f"price{suffix}.png")
    fig_distribution(log_returns(p_step), FIGS / f"returns{suffix}.png")
    fig_autocorrelation(log_returns(p_step), FIGS / f"acf{suffix}.png")

    print(f"\nwrote figures to {FIGS}")


if __name__ == "__main__":
    main()
