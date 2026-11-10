import hashlib

def calculate_crc16(data):
    """
    Calculate the CRC16 for a given byte array.

    @param data: Byte array to calculate CRC16 for.
    @return: Calculated CRC16 as an integer.
    """
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return crc


def encode_data(data):
    """
    Encode data by appending a CRC16 checksum and converting to a hexadecimal string.

    @param data: Byte array to encode.
    @return: Encoded data as a hexadecimal string.
    """
    crc = calculate_crc16(data)
    encoded_data = data + crc.to_bytes(2, byteorder='big')
    return encoded_data.hex()


def verify_crc16(data_with_crc):
    """
    Verify the CRC16 checksum of the given data.

    @param data_with_crc: Hexadecimal string containing data and CRC16.
    @return: True if the CRC16 is correct, False otherwise.
    """
    # Verificar se a string tem pelo menos 4 caracteres
    if len(data_with_crc) < 4:
        print("Error: data_with_crc is too short to contain a valid CRC16.")
        return False

    try:
        # Tentar extrair e converter o CRC recebido
        crc_received = int(data_with_crc[-4:], 16)
        # print(f"CRC received: {hex(crc_received)}")
    except ValueError as e:
        print(f"Error converting CRC: {e}")
        return False

    try:
        # Converter a parte dos dados para bytearray
        data = bytearray.fromhex(data_with_crc[:-4])
    except ValueError as e:
        # print(f"Error converting data to bytearray: {e}")
        return False

    # Calcular o CRC dos dados
    crc_calculated = calculate_crc16(data)
    # print(f"CRC calculated: {hex(crc_calculated)}")

    # Retornar se o CRC calculado é igual ao recebido
    return crc_received == crc_calculated


def decode_data(data_with_crc):
    """
    Decode the data by removing the CRC16 checksum.

    @param data_with_crc: Hexadecimal string containing data and CRC16.
    @return: Decoded data as a byte array.
    """
    data = bytearray.fromhex(data_with_crc[:-4])
    return data


def file_to_byte_array(file_path):
    """
    Read a binary file and return its contents as a byte array.

    @param file_path: Path to the binary file.
    @return: Byte array of the file's contents, or None if an error occurs.
    """
    try:
        with open(file_path, "rb") as file:
            byte_array = bytearray(file.read())
            return byte_array
    except FileNotFoundError:
        print("File not found.")
        return None
    except Exception as e:
        print("An error occurred:", str(e))
        return None


def bytearray_to_hex_string(byte_array):
    """
    Convert a byte array to a hexadecimal string.

    @param byte_array: Byte array to convert.
    @return: Hexadecimal string representation of the byte array.
    """
    hex_string = ''.join(format(byte, '02X') for byte in byte_array)
    return hex_string

def calculate_stm32_compatible_sha256(firmware_data: bytes) -> bytes:
    """
    Calcula o hash SHA-256 de uma forma compatível com o periférico HASH
    do STM32, que processa os dados em palavras de 32 bits (4 bytes)
    com a ordem dos bytes invertida.

    :param firmware_data: O conteúdo do firmware como um objeto bytes.
    :return: O digest do hash SHA-256 de 32 bytes.
    """
    padded_data = firmware_data
    if len(padded_data) % 4 != 0:
        padding_needed = 4 - (len(padded_data) % 4)
        padded_data += b'\x00' * padding_needed

    sha_object = hashlib.sha256()

    for i in range(0, len(padded_data), 4):
        chunk = padded_data[i:i+4]

        reversed_chunk = chunk[::-1]
        sha_object.update(reversed_chunk)

    return sha_object.digest()