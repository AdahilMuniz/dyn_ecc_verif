import serial
import time

# Configurações da porta serial
PORTA_COM = 'COM3'  # Substitua pela COM correta no seu sistema (ex: COM4, /dev/ttyUSB0 no Linux)
BAUD_RATE = 6000000
LOG_FILE = "serial_log.txt"

def current_timestamp():
    return time.strftime("[%Y-%m-%d %H:%M:%S]")

def escutar_serial():
    try:
        with serial.Serial(PORTA_COM, BAUD_RATE, timeout=1) as ser, open(LOG_FILE, "a", encoding="utf-8") as log_file:
            print(f"{current_timestamp()} Escutando {PORTA_COM} a {BAUD_RATE} bps...\n")
            while True:
                if ser.in_waiting:
                    linha = ser.readline().decode(errors='ignore').strip()
                    if linha:
                        entrada_log = f"{current_timestamp()} {linha}\n"
                        print(entrada_log, end="")
                        log_file.write(entrada_log)
                        log_file.flush()  # Salva imediatamente
    except serial.SerialException as e:
        print(f"Erro ao acessar a porta serial: {e}")
    except KeyboardInterrupt:
        print("\nEncerrado pelo usuário.")

if __name__ == "__main__":
    escutar_serial()
