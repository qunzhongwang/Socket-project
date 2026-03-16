#!/usr/bin/env python3
"""
plotter.py  –  Visualise TCP buffer-size vs throughput from Step 8.

Usage:
    python plotter.py                                  # macOS defaults
    python plotter.py throughput_results_linux.csv linux

Outputs:
    throughput_linear[_suffix].png
    throughput_loglog[_suffix].png
"""

import sys
import csv
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

SID = "1155210998"

# ---------------------------------------------------------------------------


def load_csv(path):
    rows = []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            rows.append(
                {
                    "requested_buf": int(row["requested_buf"]),
                    "actual_buf": int(row["actual_buf"]),
                    "throughput": float(row["throughput_bytes_per_sec"]),
                }
            )
    return rows


def deduplicate(rows):
    """Group by actual_buf, average throughput, track requested range."""
    from collections import OrderedDict

    groups = OrderedDict()
    for r in rows:
        key = r["actual_buf"]
        if key not in groups:
            groups[key] = {"actual_buf": key, "throughputs": [], "requested": []}
        groups[key]["throughputs"].append(r["throughput"])
        groups[key]["requested"].append(r["requested_buf"])
    result = []
    for g in groups.values():
        avg_tp = sum(g["throughputs"]) / len(g["throughputs"])
        req = g["requested"]
        if len(req) == 1:
            label = f"req={req[0]}"
        else:
            label = f"req={min(req)}–{max(req)}"
        result.append(
            {
                "actual_buf": g["actual_buf"],
                "throughput": avg_tp,
                "label": label,
                "count": len(req),
            }
        )
    return result


def mbps(bps):
    return bps / 1e6


def setup_axes(ax, title, xlabel, ylabel, grid_which="major"):
    ax.set_title(title, fontsize=13, fontweight="bold", pad=10)
    ax.set_xlabel(xlabel, fontsize=11, labelpad=6)
    ax.set_ylabel(ylabel, fontsize=11, labelpad=6)
    ax.grid(True, which=grid_which, linestyle=":", alpha=0.5, color="#aaa")
    ax.spines[["top", "right"]].set_visible(False)


# ---------------------------------------------------------------------------
# Linear scale
# ---------------------------------------------------------------------------


def plot_linear(deduped, platform, suffix):
    xs = [d["actual_buf"] for d in deduped]
    ys_mb = [mbps(d["throughput"]) for d in deduped]
    labels = [d["label"] for d in deduped]

    color = "#1a6faf"
    title = f"TCP Throughput vs Receiver Buffer Size\nIERG3310 Project — Student ID: {SID}"

    fig, ax = plt.subplots(figsize=(9, 6))
    fig.suptitle(title, fontsize=12, fontweight="bold")

    ax.fill_between(xs, ys_mb, alpha=0.10, color=color)
    ax.plot(xs, ys_mb, color=color, linewidth=2.2, zorder=3, label="Measured throughput")
    ax.scatter(
        xs, ys_mb, s=60, color="#e05c2a", zorder=4,
        edgecolors="white", linewidths=1.3, label="Data point",
    )

    # Smart annotation: alternate above/below, offset from cluster
    ann_kw = dict(
        fontsize=8, color="#111",
        bbox=dict(boxstyle="round,pad=0.3", fc="#eef6ff", ec=color, lw=0.9, alpha=0.93),
    )
    n = len(xs)
    for i, (x, y, lbl) in enumerate(zip(xs, ys_mb, labels)):
        # Alternate vertical offset to avoid overlap
        dy = 15 if (i % 2 == 0) else -20
        va = "bottom" if dy > 0 else "top"
        # Push first/last points inward
        if i == 0:
            ha, dx = "left", 10
        elif i == n - 1:
            ha, dx = "right", -10
        else:
            ha, dx = "left", 10

        ax.annotate(
            f"{y:.1f} MB/s\n(buf={x} B)\n{lbl}",
            xy=(x, y), xytext=(dx, dy), textcoords="offset points",
            arrowprops=dict(arrowstyle="->", color=color, lw=0.85),
            ha=ha, va=va, **ann_kw,
        )

    ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda v, _: f"{int(v):,}"))
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda v, _: f"{v:.0f}"))
    setup_axes(ax, f"Linear Scale ({platform})", "Actual Receiver Buffer Size (bytes)", "Throughput (MB/s)")
    ax.legend(fontsize=9, loc="upper left")

    fig.tight_layout()
    out = f"throughput_linear{suffix}.png"
    fig.savefig(out, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved → {out}")


# ---------------------------------------------------------------------------
# Log-log scale
# ---------------------------------------------------------------------------


def plot_loglog(deduped, platform, suffix):
    xs = np.array([d["actual_buf"] for d in deduped], dtype=float)
    ys_mb = np.array([mbps(d["throughput"]) for d in deduped])
    labels = [d["label"] for d in deduped]

    color = "#6a0dad"
    title = f"TCP Throughput vs Receiver Buffer Size\nIERG3310 Project — Student ID: {SID}"

    fig, ax = plt.subplots(figsize=(9, 6))
    fig.suptitle(title, fontsize=12, fontweight="bold")

    # Power-law fit
    log_x = np.log10(xs)
    log_y = np.log10(ys_mb)
    slope, log_a = np.polyfit(log_x, log_y, 1)
    a = 10**log_a
    x_fit = np.logspace(np.log10(min(xs)), np.log10(max(xs)), 300)
    ax.loglog(
        x_fit, a * x_fit**slope, "--", color="#bbb", linewidth=1.5,
        label=f"Power-law fit  (slope = {slope:.2f})", zorder=2,
    )

    ax.loglog(xs, ys_mb, color=color, linewidth=2.2, zorder=3, label="Measured throughput")
    ax.scatter(
        xs, ys_mb, s=60, color="#e05c2a", zorder=4,
        edgecolors="white", linewidths=1.3, label="Data point",
    )

    ann_kw = dict(
        fontsize=8, color="#111",
        bbox=dict(boxstyle="round,pad=0.3", fc="#f5eeff", ec=color, lw=0.9, alpha=0.93),
    )
    n = len(xs)
    for i, (x, y, lbl) in enumerate(zip(xs, ys_mb, labels)):
        dy = 12 if (i % 2 == 0) else -18
        va = "bottom" if dy > 0 else "top"
        if i == n - 1:
            ha, dx = "right", -12
        else:
            ha, dx = "left", 10
        ax.annotate(
            f"{y:.1f} MB/s\n(buf={int(x)} B)\n{lbl}",
            xy=(x, y), xytext=(dx, dy), textcoords="offset points",
            arrowprops=dict(arrowstyle="->", color=color, lw=0.85),
            ha=ha, va=va, **ann_kw,
        )

    ax.xaxis.set_major_formatter(ticker.LogFormatterMathtext())
    ax.yaxis.set_major_formatter(ticker.LogFormatterMathtext())
    setup_axes(ax, f"Log-Log Scale ({platform})", "Actual Receiver Buffer Size (bytes)", "Throughput (MB/s)", grid_which="both")
    ax.legend(fontsize=9, loc="upper left")

    fig.tight_layout()
    out = f"throughput_loglog{suffix}.png"
    fig.savefig(out, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved → {out}")


# ---------------------------------------------------------------------------


def main():
    csv_file = sys.argv[1] if len(sys.argv) > 1 else "throughput_results.csv"
    suffix_arg = sys.argv[2] if len(sys.argv) > 2 else ""
    suffix = f"_{suffix_arg}" if suffix_arg else ""
    platform = suffix_arg.capitalize() if suffix_arg else "macOS"

    rows = load_csv(csv_file)
    deduped = deduplicate(rows)

    print(f"Loaded {len(rows)} rows → {len(deduped)} unique actual_buf values ({platform})")
    for d in deduped:
        print(f"  actual_buf={d['actual_buf']:>6}, throughput={mbps(d['throughput']):.1f} MB/s, {d['label']}, n={d['count']}")

    plot_linear(deduped, platform, suffix)
    plot_loglog(deduped, platform, suffix)


if __name__ == "__main__":
    main()
