"""
NASCERR Serial Monitor — Timing Analyser
=========================================
Uso:
    python nascerr_monitor.py --port COM3 --baud 2000000
    python nascerr_monitor.py --port /dev/ttyUSB0 --baud 115200 --log session.txt
    python nascerr_monitor.py --file session.txt

Dependências:
    pip install pyserial rich
"""

import argparse
import re
import sys
from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional

try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False

try:
    from rich.console import Console
    from rich.table import Table
    from rich.panel import Panel
    from rich import box
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False

# ─── Regex ───────────────────────────────────────────────────────────────────

RE_WRITE = re.compile(
    r"EXP_WRITE_TASK start\s+\|\s+id=(\d+)\s+block=(\d+)\s+nblocks=(\d+)"
    r"\s+sch=(\d+)\s+ms\s+repeat=(\d+)\s+tick=(\d+)"
)

RE_READ = re.compile(
    r"EXP_READ_TASK start\s+\|\s+id=(\d+)\s+block=(\d+)\s+nblocks=(\d+)"
    r"\s+delay=(\d+)\s+ms\s+tick=(\d+)"
)

# g_readExp check | current_tick=4883447  sched_time=600  delay=600000  num_done=1
RE_CHECK = re.compile(
    r"g_readExp check\s+\|\s+current_tick=(\d+)\s+sched_time=(\d+)"
    r"\s+delay=(\d+)\s+num_done=(\d+)"
)

RE_EXEC = re.compile(r"READ exec_time=(\d+)\s*ms")

# sch_next_read | num_done=1  ms_to_last=0_600600  ideal_last=674499  ideal_next=1274499  now=719270  result=555229 ms
RE_NEXT = re.compile(
    r"sch_next_read\s+\|\s+num_done=(\d+)\s+ms_to_last=\S+\s+"
    r"ideal_last=(\d+)\s+ideal_next=(\d+)\s+now=(\d+)\s+result=(\d+)\s+ms"
)

RE_WALL = re.compile(r"(\d{2}/\d{2}/\d{4}\s+\d{2}:\d{2}:\d{2}\.\d+)")


# ─── Estruturas ──────────────────────────────────────────────────────────────

@dataclass
class ReadEvent:
    exp_id:    int
    delay_ms:  int
    tick:      int
    num_done:  int = 0          # capturado do g_readExp check (1-based)
    exec_time: Optional[int] = None
    wall_time: Optional[datetime] = None
    ideal_next_tick: Optional[int] = None
    result_ms: Optional[int] = None


@dataclass
class ExperimentState:
    exp_id:     int
    delay_ms:   int = 0
    repeat:     int = 0
    sch_ms:     int = 0          # scheduler_time (ms até a escrita)
    write_tick: Optional[int] = None
    reads:      list = field(default_factory=list)


# ─── Analisador ──────────────────────────────────────────────────────────────

class TimingAnalyser:

    TOLERANCE_PCT = 20.0

    def __init__(self, kernel_freq: int = 1000):
        self.freq  = kernel_freq
        self.exps: dict = {}
        self._pending_read: Optional[ReadEvent] = None
        self._pending_next: Optional[ReadEvent] = None

    def feed_line(self, line: str) -> Optional[dict]:
        wall = _parse_wall(line)

        # WRITE
        m = RE_WRITE.search(line)
        if m:
            exp_id, _, _, sch_ms, repeat, tick = (int(m.group(i)) for i in range(1, 7))
            self.exps[exp_id] = ExperimentState(
                exp_id=exp_id, repeat=repeat, sch_ms=sch_ms, write_tick=tick
            )
            return {"kind": "WRITE", "exp_id": exp_id, "repeat": repeat,
                    "sch_ms": sch_ms, "tick": tick, "wall_time": wall}

        # READ START
        m = RE_READ.search(line)
        if m:
            exp_id, _, _, delay_ms, tick = (int(m.group(i)) for i in range(1, 6))
            state = self.exps.setdefault(
                exp_id, ExperimentState(exp_id=exp_id, delay_ms=delay_ms)
            )
            state.delay_ms = delay_ms
            ev = ReadEvent(exp_id=exp_id, delay_ms=delay_ms, tick=tick, wall_time=wall)
            state.reads.append(ev)
            self._pending_read = ev
            self._pending_next = ev
            return None

        # g_readExp check — captura num_done do firmware
        m = RE_CHECK.search(line)
        if m and self._pending_read is not None:
            num_done = int(m.group(4))
            self._pending_read.num_done = num_done

            # num_done=0 → última leitura: sch_next_read NÃO será chamado
            # emite resultado imediatamente para não perder a leitura final
            if num_done == 0 and self._pending_next is not None:
                ev = self._pending_next
                self._pending_next = None
                self._pending_read = None
                return self._make_result(ev)
            return None

        # sch_next_read — caso normal: emite resultado com fw_drift disponível
        m = RE_NEXT.search(line)
        if m and self._pending_next is not None:
            self._pending_next.ideal_next_tick = int(m.group(3))
            self._pending_next.result_ms       = int(m.group(5))
            ev = self._pending_next
            self._pending_next = None
            return self._make_result(ev)

        # exec_time
        m = RE_EXEC.search(line)
        if m and self._pending_read is not None:
            self._pending_read.exec_time = int(m.group(1))
            self._pending_read = None

        return None

    def _make_result(self, ev: ReadEvent) -> Optional[dict]:
        state = self.exps.get(ev.exp_id)
        if state is None or ev not in state.reads:
            return None

        idx = state.reads.index(ev)

        if idx == 0:
            # Primeira leitura: referência é o write_tick
            # Esperado: scheduler_time + delay (tempo total do comando até 1ª leitura)
            if state.write_tick is None:
                return None
            prev_tick    = state.write_tick
            expected_ms  = state.sch_ms + ev.delay_ms
            is_first     = True
        else:
            prev_tick    = state.reads[idx - 1].tick
            expected_ms  = ev.delay_ms
            is_first     = False
        delta_ms    = (ev.tick - prev_tick) * 1000 // self.freq
        error_ms    = delta_ms - expected_ms
        error_pct   = (error_ms / expected_ms * 100.0) if expected_ms else 0.0

        # drift em relação ao ideal calculado pelo firmware
        # Para a 1ª leitura, ideal_next_tick aponta para a 2ª leitura (não a atual) — não exibir
        drift_ms = None
        if ev.ideal_next_tick is not None and idx > 0:
            drift_ms = (ev.tick - ev.ideal_next_tick) * 1000 // self.freq

        # drift acumulado desde a primeira leitura (relativo ao delay periódico)
        first_tick = state.reads[0].tick
        cum_ideal  = idx * ev.delay_ms
        cum_actual = (ev.tick - first_tick) * 1000 // self.freq
        cum_drift  = cum_actual - cum_ideal

        # num_done: prefere valor do firmware, fallback pelo índice 1-based
        num_done = ev.num_done if ev.num_done > 0 else (idx + 1)

        # exec_time da leitura anterior (para detectar EXEC>DELAY)
        prev_exec = state.reads[idx - 1].exec_time if idx > 0 else None

        status = self._classify(error_pct, prev_exec, expected_ms)

        return {
            "kind":        "READ",
            "exp_id":      ev.exp_id,
            "num_done":    num_done,
            "repeat":      state.repeat,
            "tick":        ev.tick,
            "wall_time":   ev.wall_time,
            "delta_ms":    delta_ms,
            "expected_ms": expected_ms,
            "error_ms":    error_ms,
            "error_pct":   error_pct,
            "drift_ms":    drift_ms,
            "cum_drift":   cum_drift,
            "exec_time":   prev_exec,
            "result_ms":   ev.result_ms,
            "status":      status,
        }

    def _classify(self, error_pct, exec_time, delay_ms):
        if exec_time is not None and exec_time > delay_ms:
            return "EXEC>DELAY"
        if abs(error_pct) <= self.TOLERANCE_PCT:
            return "OK"
        if abs(error_pct) <= 50.0:
            return "WARN"
        return "FAIL"

    def summary(self) -> list:
        rows = []
        for state in self.exps.values():
            if len(state.reads) < 2:
                continue
            deltas = [
                (state.reads[i].tick - state.reads[i-1].tick) * 1000 // self.freq
                for i in range(1, len(state.reads))
            ]
            rows.append({
                "exp_id":    state.exp_id,
                "repeat":    state.repeat,
                "delay_ms":  state.delay_ms,
                "total":     len(state.reads),
                "mean_ms":   sum(deltas) // len(deltas),
                "min_ms":    min(deltas),
                "max_ms":    max(deltas),
                "jitter_ms": max(deltas) - min(deltas),
            })
        return rows


# ─── Helpers de formatação ────────────────────────────────────────────────────

def _parse_wall(line: str) -> Optional[datetime]:
    m = RE_WALL.search(line)
    if m:
        try:
            return datetime.strptime(m.group(1), "%d/%m/%Y %H:%M:%S.%f")
        except ValueError:
            pass
    return None


STATUS_COLOR = {
    "OK":         "green",
    "WARN":       "yellow",
    "FAIL":       "red",
    "EXEC>DELAY": "magenta",
}


def _fmt_ms(ms: Optional[int], signed: bool = False) -> str:
    if ms is None:
        return "—"
    sign = ("+" if ms >= 0 else "") if signed else ""
    neg  = "-" if ms < 0 else ""
    a    = abs(ms)
    if a >= 60_000:
        mn, s = divmod(a // 1000, 60)
        return f"{sign}{neg}{mn}m{s:02d}s"
    if a >= 1_000:
        return f"{sign}{ms / 1000:.1f}s"
    return f"{sign}{ms}ms"


def print_event(ev: dict, console=None) -> None:
    if ev["kind"] == "WRITE":
        wall = ev["wall_time"].strftime("%H:%M:%S") if ev["wall_time"] else "--------"
        line = (
            f"[{wall}] ─── WRITE  id={ev['exp_id']:>3}  "
            f"repeat={ev['repeat']}  sch={_fmt_ms(ev['sch_ms'])}  tick={ev['tick']}"
        )
        if RICH_AVAILABLE and console:
            console.print(f"[bold cyan]{line}[/bold cyan]")
        else:
            print(line)
        return

    status = ev["status"]
    color  = STATUS_COLOR.get(status, "white")
    wall   = ev["wall_time"].strftime("%H:%M:%S") if ev["wall_time"] else "--------"

    # "read#3/10" — total é repeat+1 (leituras vão de 1 até repeat+1)
    total = ev["repeat"] + 1 if ev["repeat"] > 0 else 0
    label = f"read#{ev['num_done']}/{total}" if total > 0 \
            else f"read#{ev['num_done']}"

    line = (
        f"[{wall}]  id={ev['exp_id']:>3}  {label:<13}"
        f"  Δ={_fmt_ms(ev['delta_ms']):>9}"
        f"  exp={_fmt_ms(ev['expected_ms']):>9}"
        f"  err={_fmt_ms(ev['error_ms'], signed=True):>9} ({ev['error_pct']:>+6.1f}%)"
        f"  cum={_fmt_ms(ev['cum_drift'], signed=True):>9}"
    )
    if ev["drift_ms"] is not None:
        line += f"  fw={_fmt_ms(ev['drift_ms'], signed=True):>7}"
    if ev["exec_time"] is not None:
        line += f"  exec={_fmt_ms(ev['exec_time'])}"
    line += f"  [{status}]"

    if RICH_AVAILABLE and console:
        console.print(f"[{color}]{line}[/{color}]")
    else:
        print(line)


def print_summary(rows: list, console=None) -> None:
    if not rows:
        print("Sem dados suficientes para sumário.")
        return

    if RICH_AVAILABLE and console:
        tbl = Table(title="Sumário por Experimento", box=box.ROUNDED, show_lines=True)
        for col, just in [
            ("id","right"), ("repeat","right"), ("delay","right"),
            ("leituras","right"), ("média Δ","right"), ("min Δ","right"),
            ("max Δ","right"), ("jitter","right"), ("status","center")
        ]:
            tbl.add_column(col, justify=just)
        for r in rows:
            ok = r["jitter_ms"] < r["delay_ms"] * 0.1
            st = "OK" if ok else "WARN"
            c  = STATUS_COLOR[st]
            tbl.add_row(
                str(r["exp_id"]), str(r["repeat"]),
                _fmt_ms(r["delay_ms"]), str(r["total"]),
                _fmt_ms(r["mean_ms"]), _fmt_ms(r["min_ms"]),
                _fmt_ms(r["max_ms"]), _fmt_ms(r["jitter_ms"]),
                f"[{c}]{st}[/{c}]",
            )
        console.print(tbl)
    else:
        print("\n=== Sumário ===")
        h = f"{'id':>4}  {'rep':>4}  {'delay':>9}  {'leit':>6}  {'média':>9}  {'min':>9}  {'max':>9}  {'jitter':>9}"
        print(h)
        print("-" * len(h))
        for r in rows:
            print(
                f"{r['exp_id']:>4}  {r['repeat']:>4}  {_fmt_ms(r['delay_ms']):>9}  "
                f"{r['total']:>6}  {_fmt_ms(r['mean_ms']):>9}  {_fmt_ms(r['min_ms']):>9}  "
                f"{_fmt_ms(r['max_ms']):>9}  {_fmt_ms(r['jitter_ms']):>9}"
            )


# ─── Fontes de linha ──────────────────────────────────────────────────────────

def lines_from_file(path: str):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            yield line.rstrip("\r\n")


def lines_from_serial(port: str, baud: int, log_path: Optional[str] = None):
    if not SERIAL_AVAILABLE:
        print("pyserial não instalado: pip install pyserial")
        sys.exit(1)
    log_file = open(log_path, "w", encoding="utf-8") if log_path else None
    try:
        with serial.Serial(port, baud, timeout=1) as ser:
            print(f"Conectado em {port} @ {baud} baud")
            buf = ""
            while True:
                raw = ser.read(256)
                if raw:
                    buf += raw.decode("utf-8", errors="replace")
                    while "\n" in buf:
                        ln, buf = buf.split("\n", 1)
                        ln = ln.rstrip("\r")
                        # timestamp individual por linha
                        ts = datetime.now().strftime("%d/%m/%Y %H:%M:%S.%f")[:-3]
                        if log_file:
                            log_file.write(f"{ts} [RX] - {ln}\n")
                            log_file.flush()
                        yield ln
    finally:
        if log_file:
            log_file.close()


# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="NASCERR Serial Monitor")
    src = parser.add_mutually_exclusive_group(required=True)
    src.add_argument("--port", help="Porta serial (ex: COM3, /dev/ttyUSB0)")
    src.add_argument("--file", help="Arquivo de log para replay")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--log",  help="Salvar sessão em arquivo")
    parser.add_argument("--freq", type=int, default=1000,
                        help="Freq. do kernel RTOS em Hz (default: 1000)")
    parser.add_argument("--tol",  type=float, default=20.0,
                        help="Tolerância %% OK (default: 20)")
    args = parser.parse_args()

    console  = Console() if RICH_AVAILABLE else None
    analyser = TimingAnalyser(kernel_freq=args.freq)
    analyser.TOLERANCE_PCT = args.tol

    if console:
        console.print(Panel(
            "[bold cyan]NASCERR Serial Monitor[/bold cyan]\n"
            f"freq={args.freq}Hz  tolerância=±{args.tol:.0f}%  Ctrl+C → sumário",
            box=box.ROUNDED
        ))
    else:
        print(f"NASCERR Monitor | freq={args.freq}Hz tol=±{args.tol:.0f}%\n")

    hdr = (
        f"{'hora':>8}  {'id':>4}  {'leitura':<13}"
        f"  {'Δ real':>9}  {'esperado':>9}  {'erro':>9}           {'drift acum':>9}"
    )
    if console:
        console.print(f"[dim]{hdr}[/dim]")
        console.rule()
    else:
        print(hdr)
        print("-" * len(hdr))

    source = (lines_from_file(args.file) if args.file
              else lines_from_serial(args.port, args.baud, args.log))

    try:
        for line in source:
            result = analyser.feed_line(line)
            if result:
                print_event(result, console)
    except KeyboardInterrupt:
        pass

    print()
    print_summary(analyser.summary(), console)


if __name__ == "__main__":
    main()