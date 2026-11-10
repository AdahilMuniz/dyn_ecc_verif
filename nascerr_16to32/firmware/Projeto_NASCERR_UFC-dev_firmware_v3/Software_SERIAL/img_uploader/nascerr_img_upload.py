#!/usr/bin/env python3
"""
NASCERR Image Upload Tool  (debug edition)
==========================================
Envia uma imagem para o NASCERR via UART usando o protocolo de frames.

Protocolo de frame: [ MAGIC:2 | CMD:1 | LEN:2 | PAYLOAD:N | CRC16:2 ]
  - Todos os campos multi-byte em big-endian
  - IMG_START  (0x01): total_size(4B) + crc16(2B)
  - IMG_CHUNK  (0x02): offset(4B) + data(N B)
  - IMG_FINISH (0x03): sem payload
  - IMG_ABORT  (0x04): sem payload

Modos de uso
------------
  Upload normal:
    python nascerr_img_upload.py --port COM3 --file image.bin

  Upload passo-a-passo (debug interativo):
    python nascerr_img_upload.py --port COM3 --file image.bin --step

  Monitorar serial sem enviar nada:
    python nascerr_img_upload.py --port COM3 --monitor

  Encerrar transmissão em curso e ficar só monitorando:
    python nascerr_img_upload.py --port COM3 --abort --monitor
"""

import argparse
import struct
import sys
import threading
import time
from pathlib import Path

try:
    import serial
except ImportError:
    print("Dependência ausente: pip install pyserial")
    sys.exit(1)

# ── Constantes do protocolo ──────────────────────────────────────────────────

MAGIC           = 0xA5C3
CMD_START       = 0x01
CMD_CHUNK       = 0x02
CMD_FINISH      = 0x03
CMD_ABORT       = 0x04

ACK             = 0x06
ACK_FINISH      = 0x07 
NACK            = 0x15

DEFAULT_BAUD    = 2_000_000
DEFAULT_CHUNK   = 2112
DEFAULT_TIMEOUT = 20.0
DEFAULT_RETRY   = 1000

# ── Cores ANSI ───────────────────────────────────────────────────────────────

class C:
    RST   = "\033[0m"
    BOLD  = "\033[1m"
    DIM   = "\033[2m"
    GREEN = "\033[92m"
    RED   = "\033[91m"
    YEL   = "\033[93m"
    CYN   = "\033[96m"
    MAG   = "\033[95m"
    GRY   = "\033[90m"

def _tag(color: str, label: str, msg: str) -> str:
    return f"{color}{C.BOLD}[{label}]{C.RST} {msg}"

def info(msg):   print(_tag(C.CYN,   "INFO",    msg))
def ok(msg):     print(_tag(C.GREEN, "OK",      msg))
def warn(msg):   print(_tag(C.YEL,   "WARN",    msg))
def err(msg):    print(_tag(C.RED,   "ERRO",    msg))
def fw(msg):     print(_tag(C.MAG,   "FW",      msg), end="", flush=True)
def step(msg):   print(_tag(C.GRY,   "PASSO",   msg))
def dbg(msg):    print(_tag(C.GRY,   "DBG",     msg))

# ── CRC16 (polinômio 0xA001) ─────────────────────────────────────────────────

def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc

# ── Montagem de frame ────────────────────────────────────────────────────────

def build_frame(cmd: int, payload: bytes = b"") -> bytes:
    header = struct.pack(">HBH", MAGIC, cmd, len(payload))
    body   = header + payload
    return body + struct.pack(">H", crc16(body))

def _build_chunk_frame(chunk_id: int, data: bytes) -> bytes:
    payload = struct.pack('>I', chunk_id) + data   # 4B big-endian
    return build_frame(CMD_CHUNK, payload)

# ── Monitor de serial (thread paralela) ─────────────────────────────────────

class SerialMonitor:
    """
    Lê continuamente a porta serial e imprime tudo que chega do firmware.
    Pode ser usada em paralelo com o upload (--verbose) ou sozinha (--monitor).

    O monitor identifica ACK/NACK e os mostra de forma legível, mas repassa
    o byte bruto para quem chamar wait_ack(), que lê pelo mesmo objeto Serial.

    Quando usado no modo --monitor puro, o objeto Serial é criado internamente.
    """

    def __init__(self, ser: serial.Serial, *, echo_ack: bool = False):
        self._ser      = ser
        self._echo_ack = echo_ack   # se True, imprime ACK/NACK além do texto
        self._buf      = bytearray()
        self._stop     = threading.Event()
        self._thread   = threading.Thread(target=self._run, daemon=True)

    def start(self):
        self._thread.start()

    def stop(self):
        self._stop.set()

    def _run(self):
        while not self._stop.is_set():
            try:
                self._ser.timeout = 0.05
                byte = self._ser.read(1)
            except Exception:
                break
            if not byte:
                continue
            b = byte[0]
            if self._echo_ack and b == ACK:
                print(f"\n{C.GREEN}{C.BOLD}[ACK]{C.RST}", flush=True)
            elif self._echo_ack and b == NACK:
                print(f"\n{C.RED}{C.BOLD}[NACK]{C.RST}", flush=True)
            else:
                self._buf.append(b)
                if b == ord('\n'):
                    fw(self._buf.decode('ascii', errors='replace'))
                    self._buf.clear()


def monitor_loop(port: str, baud: int) -> None:
    """
    Abre a porta serial e fica imprimindo tudo que chega — sem enviar nada.
    Ctrl+C encerra.
    """
    info(f"Modo monitor — porta {port} @ {baud} baud")
    info("Pressione Ctrl+C para sair.\n")
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except serial.SerialException as e:
        err(f"Não foi possível abrir a porta: {e}")
        sys.exit(1)

    buf = bytearray()
    try:
        with ser:
            while True:
                byte = ser.read(1)
                if not byte:
                    continue
                b = byte[0]
                if b == ACK:
                    print(f"{C.GREEN}{C.BOLD}[ACK]{C.RST}", flush=True)
                elif b == NACK:
                    print(f"{C.RED}{C.BOLD}[NACK]{C.RST}", flush=True)
                else:
                    buf.append(b)
                    if b == ord('\n'):
                        fw(buf.decode('ascii', errors='replace'))
                        buf.clear()
    except KeyboardInterrupt:
        print("\n")
        info("Monitor encerrado.")

# ── Espera ACK/NACK ──────────────────────────────────────────────────────────

def wait_ack(ser: serial.Serial, timeout: float = DEFAULT_TIMEOUT,
             verbose: bool = False, ack_byte: int = ACK) -> tuple:
    deadline  = time.time() + timeout
    debug_buf = bytearray()

    while time.time() < deadline:
        ser.timeout = min(0.05, deadline - time.time())
        byte = ser.read(1)
        if not byte:
            continue
        b = byte[0]

        if b == ack_byte:                        # ACK esperado para este comando
            if debug_buf:
                fw(debug_buf.decode('ascii', errors='replace') + "\n")
            return True, None

        elif b == NACK:
            if debug_buf:
                fw(debug_buf.decode('ascii', errors='replace') + "\n")
            ser.timeout = 0.02
            extra = ser.read(2)
            next_exp = struct.unpack(">H", extra)[0] if len(extra) == 2 else None
            return False, next_exp

        else:
            # Inclui 0x06 stray (chunk dup) quando à espera de 0x07 (FINISH)
            debug_buf.append(b)
            if b == ord('\n'):
                fw(debug_buf.decode('ascii', errors='replace'))
                debug_buf.clear()

    if debug_buf:
        fw(debug_buf.decode('ascii', errors='replace') + "\n")
    return None, None

# ── Abort ────────────────────────────────────────────────────────────────────

def _send_abort(ser: serial.Serial) -> None:
    ser.write(build_frame(CMD_ABORT))
    wait_ack(ser, timeout=3.0)

def abort(port: str, baud: int) -> None:
    info(f"Enviando IMG_ABORT para {port}...")
    try:
        with serial.Serial(port, baud, timeout=3.0) as ser:
            _send_abort(ser)
        ok("Abort enviado.")
    except serial.SerialException as e:
        err(str(e))

# ── Lógica de envio de um frame com retry ────────────────────────────────────

def _send_frame(ser: serial.Serial, frame: bytes, label: str,
                retries: int, timeout: float, verbose: bool, ack_byte: int = ACK) -> bool:
    if verbose:
        magic   = struct.unpack(">H", frame[0:2])[0]
        cmd     = frame[2]
        length  = struct.unpack(">H", frame[3:5])[0]
        payload = frame[5:5+length]
        crc_rx  = struct.unpack(">H", frame[-2:])[0]
        dbg(f"{label} — MAGIC=0x{magic:04X}  CMD=0x{cmd:02X}  LEN={length}  "
            f"PAYLOAD={payload.hex()}  CRC=0x{crc_rx:04X}")

    for attempt in range(1, retries + 1):
        ser.write(frame)
        ser.flush()
        result, next_expected_id = wait_ack(ser, timeout=timeout, verbose=verbose, ack_byte=ack_byte)
        if result is True:
            if attempt > 1:
                warn(f"{label} aceito após {attempt} tentativas.")
            return True

    return False

# ── Prompt interativo do modo step ───────────────────────────────────────────

def _prompt(msg: str) -> str:
    """
    Exibe um prompt e aguarda Enter.
    Retorna 'skip', 'abort' ou '' para continuar.
    Comandos: Enter=continuar, s=skip, a=abort, q=quit
    """
    print(f"\n{C.YEL}{C.BOLD}┌─ {msg}{C.RST}")
    print(f"{C.YEL}│  [Enter] enviar   [s] pular   [a] abort   [q] sair{C.RST}")
    try:
        ans = input(f"{C.YEL}└─► {C.RST}").strip().lower()
    except (EOFError, KeyboardInterrupt):
        return "quit"
    if ans in ("a", "abort"):
        return "abort"
    if ans in ("s", "skip"):
        return "skip"
    if ans in ("q", "quit"):
        return "quit"
    return ""

# ── Upload principal ─────────────────────────────────────────────────────────

def upload(port: str,
           baud: int,
           image_path: str,
           chunk_size: int,
           retries: int,
           verbose: bool,
           step_mode: bool) -> bool:

    data    = Path(image_path).read_bytes()
    total   = len(data)
    img_crc = crc16(data)

    print()
    info(f"Imagem : {image_path}")
    info(f"Tamanho: {total:,} bytes")
    info(f"CRC16  : 0x{img_crc:04X}")
    info(f"Porta  : {port} @ {baud} baud")
    info(f"Chunk  : {chunk_size} bytes")
    if step_mode:
        warn("Modo passo-a-passo ativado — confirme cada fase manualmente.")
    print()

    try:
        ser = serial.Serial(port, baud, timeout=DEFAULT_TIMEOUT)
    except serial.SerialException as e:
        err(f"Erro ao abrir porta: {e}")
        return False

    with ser:

        # ── FASE 1: START ────────────────────────────────────────────────────
        payload_start = struct.pack(">IH", total, img_crc)
        frame_start   = build_frame(CMD_START, payload_start)

        if step_mode:
            ans = _prompt("FASE 1 — Enviar IMG_START")
            if ans == "quit":
                return False
            if ans == "abort":
                _send_abort(ser); return False

        step("Enviando IMG_START...")
        if not _send_frame(ser, frame_start, "START", retries, DEFAULT_TIMEOUT, verbose):
            err("Falha no START. Abortando.")
            _send_abort(ser)
            return False
        ok("ACK recebido — START aceito.")

        # ── Modo step: monitor entre fases ───────────────────────────────────
        if step_mode:
            info("Serial livre — veja os logs do firmware acima.")
            info("Aguardando sua confirmação para os chunks...")

        # ── FASE 2: CHUNKs ──────────────────────────────────────────────────
        total_chunks = (total + chunk_size - 1) // chunk_size
        confirmed    = [False] * total_chunks  # chunks confirmados pelo firmware
        attempts     = [0]     * total_chunks  # tentativas por chunk

        step(f"Enviando {total_chunks} chunk(s) de até {chunk_size}B...")

        i = 0
        while i < total_chunks:

            if confirmed[i]:
                i += 1
                continue

            offset     = i * chunk_size
            chunk_data = data[offset:offset + chunk_size]
            frame      = _build_chunk_frame(i, chunk_data)

            attempts[i] += 1
            if attempts[i] > retries:
                print()
                err(f"Chunk {i+1}/{total_chunks} (id={i}) excedeu {retries} tentativas.")
                _send_abort(ser)
                return False

            if step_mode:
                ans = _prompt(
                    f"CHUNK {i+1}/{total_chunks}"
                    f" | offset=0x{offset:08X} | {len(chunk_data)}B"
                )
                if ans == "quit":  return False
                if ans == "abort": _send_abort(ser); return False
                if ans == "skip":
                    warn(f"Chunk {i+1} pulado.")
                    confirmed[i] = True; i += 1; continue
            else:
                tentativa_str = (f" | tentativa {attempts[i]}/{retries}"
                                 if attempts[i] > 1 else "")
                print(f"\r{C.GRY}{C.BOLD}[CHUNK]{C.RST} {i+1}/{total_chunks}"
                      f" | offset 0x{offset:08X}"
                      f" | {len(chunk_data)}B{tentativa_str}",
                      end="", flush=True)

            if verbose:
                print()
                dbg(f"chunk_id={i}  offset=0x{offset:08X}  "
                    f"data={len(chunk_data)}B  frame={len(frame)}B")

            # Envia directamente para capturar next_exp do wait_ack
            ser.write(frame)
            ser.flush()
            result, next_exp = wait_ack(ser, timeout=DEFAULT_TIMEOUT)

            if result is True:                               # ── ACK ──
                confirmed[i] = True
                if attempts[i] > 1:
                    warn(f"\nChunk {i+1} (id={i}) aceito na tentativa {attempts[i]}.")
                if step_mode:
                    ok(f"ACK — chunk {i+1} (id={i}) aceito.")
                i += 1

            elif result is False:                            # ── NACK ──
                print()
                if next_exp is not None and next_exp != i:
                    # Firmware indicou onde está: salta directamente
                    warn(f"NACK: firmware espera id={next_exp}"
                         f" (Python em id={i})"
                         f" — voltando para chunk {next_exp + 1}/{total_chunks}.")
                    for j in range(next_exp, total_chunks):
                        confirmed[j] = False
                    i = next_exp
                else:
                    # next_exp == i ou None: retenta o mesmo chunk
                    warn(f"NACK no chunk {i+1} (id={i}),"
                         f" tentativa {attempts[i]}/{retries}.")

            else:                                            # ── timeout ──
                print()
                warn(f"Timeout no chunk {i+1} (id={i}),"
                     f" tentativa {attempts[i]}/{retries}.")

        if not step_mode:
            print()  # nova linha após barra de progresso

        # ── FASE 3: FINISH ───────────────────────────────────────────────────
        time.sleep(1.0)
        frame_finish = build_frame(CMD_FINISH)

        if step_mode:
            ans = _prompt("FASE 3 — Enviar IMG_FINISH (validação CRC no firmware)")
            if ans == "quit":
                return False
            if ans == "abort":
                _send_abort(ser); return False

        step("Enviando IMG_FINISH...")
        if verbose:
            dbg(f"frame ({len(frame_finish)}B): {frame_finish.hex()}")

        finish_ok = False
        for finish_attempt in range(1, retries + 1):
            ser.write(frame_finish)
            ser.flush()
            result, next_exp = wait_ack(ser, timeout=15.0, ack_byte=ACK_FINISH)

            if result is True:
                finish_ok = True
                break

            if result is False and next_exp is not None and 0 <= next_exp < total_chunks:
                # Firmware indica chunk ausente — reenvia antes de tentar FINISH de novo
                warn(f"FINISH NACK: firmware espera chunk id={next_exp}"
                     f" ({next_exp+1}/{total_chunks}), reenviando...")
                offset_r    = next_exp * chunk_size
                chunk_data_r = data[offset_r:offset_r + chunk_size]
                frame_r      = _build_chunk_frame(next_exp, chunk_data_r)
                for _ in range(retries):
                    ser.write(frame_r)
                    ser.flush()
                    cr, _ = wait_ack(ser, timeout=DEFAULT_TIMEOUT)
                    if cr is True:
                        ok(f"Chunk {next_exp+1}/{total_chunks} (id={next_exp}) reconfirmado.")
                        confirmed[next_exp] = True
                        break
                else:
                    err(f"Não foi possível reenviar chunk {next_exp+1}.")
                    _send_abort(ser)
                    return False
            elif result is None:
                warn(f"Timeout no FINISH, tentativa {finish_attempt}/{retries}.")

        if not finish_ok:
            err("Falha no FINISH (validação CRC falhou ou timeout).")
            return False

        ok("ACK — imagem gravada com sucesso!")

        # ── Pós-upload: continua monitorando se pedido ───────────────────────
        if step_mode:
            print()
            info("Upload concluído. Modo monitor ativo — pressione Ctrl+C para sair.")
            _monitor_after(ser)

    return True


def _monitor_after(ser: serial.Serial) -> None:
    """Fica lendo a serial após o upload até Ctrl+C."""
    buf = bytearray()
    try:
        while True:
            ser.timeout = 0.1
            byte = ser.read(1)
            if not byte:
                continue
            b = byte[0]
            if b == ACK:
                print(f"{C.GREEN}{C.BOLD}[ACK]{C.RST}", flush=True)
            elif b == NACK:
                print(f"{C.RED}{C.BOLD}[NACK]{C.RST}", flush=True)
            else:
                buf.append(b)
                if b == ord('\n'):
                    fw(buf.decode('ascii', errors='replace'))
                    buf.clear()
    except KeyboardInterrupt:
        print()
        info("Monitor encerrado.")

# ── Self-test ────────────────────────────────────────────────────────────────

def self_test() -> None:
    payload  = struct.pack(">IH", 1024, crc16(b"\xAB" * 1024))
    frame    = build_frame(CMD_START, payload)

    magic_rx = struct.unpack(">H", frame[:2])[0]
    assert magic_rx == MAGIC, f"magic errado: {magic_rx:#06x}"

    body_crc = crc16(frame[:-2])
    recv_crc = struct.unpack(">H", frame[-2:])[0]
    assert body_crc == recv_crc, f"CRC mismatch: calc={body_crc:#06x} recv={recv_crc:#06x}"

    # Testa chunk frame
    dummy_data = bytes(range(256))
    fc = build_frame(CMD_CHUNK, struct.pack(">I", 0) + dummy_data)
    assert crc16(fc[:-2]) == struct.unpack(">H", fc[-2:])[0], "CRC chunk errado"

    # Testa finish/abort frames (sem payload)
    for cmd in (CMD_FINISH, CMD_ABORT):
        f = build_frame(cmd)
        assert crc16(f[:-2]) == struct.unpack(">H", f[-2:])[0], f"CRC cmd={cmd} errado"

    ok("Self-test passou em todos os casos.")

# ── CLI ──────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="NASCERR Image Upload — debug edition",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos:
  Upload normal:
    python nascerr_img_upload.py --port COM3 --file image.bin

  Passo-a-passo (debug interativo):
    python nascerr_img_upload.py --port COM3 --file image.bin --step

  Só monitorar a serial (sem enviar nada):
    python nascerr_img_upload.py --port COM3 --monitor

  Abortar transmissão em curso e ficar monitorando:
    python nascerr_img_upload.py --port COM3 --abort --monitor
        """
    )
    parser.add_argument("--port",    required=True,
                        help="Porta serial (ex: COM3, /dev/ttyUSB0)")
    parser.add_argument("--baud",    type=int, default=DEFAULT_BAUD,
                        help=f"Baudrate (padrão: {DEFAULT_BAUD})")
    parser.add_argument("--file",    help="Caminho para o arquivo de imagem")
    parser.add_argument("--chunk",   type=int, default=DEFAULT_CHUNK,
                        help=f"Tamanho do chunk em bytes (padrão: {DEFAULT_CHUNK})")
    parser.add_argument("--retry",   type=int, default=DEFAULT_RETRY,
                        help=f"Tentativas por frame (padrão: {DEFAULT_RETRY})")
    parser.add_argument("--abort",   action="store_true",
                        help="Envia IMG_ABORT antes de qualquer outra ação")
    parser.add_argument("--monitor", action="store_true",
                        help="Após abort (ou sem --file), fica monitorando a serial")
    parser.add_argument("--step",    action="store_true",
                        help="Modo passo-a-passo: confirma cada fase no teclado")
    parser.add_argument("--test",    action="store_true",
                        help="Executa self-test local e sai (não precisa de hardware)")
    parser.add_argument("--verbose", action="store_true",
                        help="Exibe bytes de cada frame enviado")
    args = parser.parse_args()

    # Self-test não precisa de porta real
    if args.test:
        self_test()
        return

    # Abort (opcional) → depois decide se monitora ou sai
    if args.abort:
        abort(args.port, args.baud)
        if not args.monitor and not args.file:
            return

    # Só monitor (sem enviar nada)
    if args.monitor and not args.file:
        monitor_loop(args.port, args.baud)
        return

    # Upload
    if not args.file:
        parser.error("--file é obrigatório para upload (ou use --monitor)")

    if not Path(args.file).exists():
        err(f"Arquivo não encontrado: {args.file}")
        sys.exit(1)

    if not (1 <= args.chunk <= 2112):
        err("--chunk deve estar entre 1 e 2112")
        sys.exit(1)

    t0 = time.time()
    ok_flag = upload(
        port       = args.port,
        baud       = args.baud,
        image_path = args.file,
        chunk_size = args.chunk,
        retries    = args.retry,
        verbose    = args.verbose,
        step_mode  = args.step,
    )
    elapsed = time.time() - t0

    print()
    if ok_flag:
        size = Path(args.file).stat().st_size
        ok(f"Concluído em {elapsed:.1f}s  ({size / elapsed / 1024:.1f} KB/s)")

        # Se não estava em step (que já abre monitor internamente), pergunta se quer monitorar
        if args.monitor and not args.step:
            info("Abrindo monitor pós-upload — Ctrl+C para sair.")
            try:
                with serial.Serial(args.port, args.baud, timeout=0.1) as ser:
                    _monitor_after(ser)
            except serial.SerialException as e:
                err(str(e))

        sys.exit(0)
    else:
        err(f"Falha após {elapsed:.1f}s")
        sys.exit(1)


if __name__ == "__main__":
    main()