from datetime import datetime, timezone
import struct
import random
import math
import json
if __name__ == "__main__":
    from util.crc import calculate_crc16
else:
    from nascerr_modules.util.crc import calculate_crc16


class NASCERR_GPS:

    # uint16_t gnss_week;                    /* > Numero da semana GNSS (2 bytes)                */
    # uint32_t gnss_tow;                     /* > Tempo da semana GNSS em segundos (4 bytes)     */

    # float latitude;                        /* > Latitude em graus (4 bytes)                    */
    # float longitude;                       /* > Longitude em graus (4 bytes)                   */
    # float altitude_ellipsoid;              /* > Altitude no elipsoide (4 bytes)                */
    # float altitude_msl;                    /* > Altitude em relacao ao nivel medio do mar (4B) */

    # float ecef_x;                          /* > Coordenada X no sistema ECEF (4 bytes)         */
    # float ecef_y;                          /* > Coordenada Y no sistema ECEF (4 bytes)         */
    # float ecef_z;                          /* > Coordenada Z no sistema ECEF (4 bytes)         */
    # --- Constantes WGS-84 ------------------------------------------------------

    def __init__(self, gnss_week=0, gnss_tow=0, latitude=0.0, longitude=0.0,
                 altitude_ellipsoid=0.0, altitude_msl=0.0, ecef_x=0.0,
                 ecef_y=0.0, ecef_z=0.0):
        self.gnss_week = gnss_week
        self.gnss_tow = gnss_tow
        self.latitude = latitude
        self.longitude = longitude
        self.altitude_ellipsoid = altitude_ellipsoid
        self.altitude_msl = altitude_msl
        self.ecef_x = ecef_x
        self.ecef_y = ecef_y
        self.ecef_z = ecef_z
        # Constantes do sistema WGS-84
        self.A = 6378137.0                 # semi-eixo maior (m)
        self.F = 1 / 298.257223563         # achatamento
        self.E2 = self.F * (2 - self.F)               # excentricidade²

        # mesmo formato que você usa (34 bytes) + 2 CRC
        self.FMT = '<H I f f f f f f f'

    def to_bytes(self):
        """
        Serialize the NASCERR_GPS structure to a bytes object.
        """
        result = struct.pack(
            self.FMT,
            self.gnss_week,
            self.gnss_tow,
            self.latitude,
            self.longitude,
            self.altitude_ellipsoid,
            self.altitude_msl,
            self.ecef_x,
            self.ecef_y,
            self.ecef_z
        )
        result += struct.pack("<H", calculate_crc16(result))
        return result

    def as_dict(self) -> dict:
        """Representação legível (útil para log/JSON)."""
        return {
            "gnss_week":       self.gnss_week,
            "gnss_tow":        self.gnss_tow,
            "latitude_deg":    self.latitude,
            "longitude_deg":   self.longitude,
            "alt_ellipsoid_m": self.altitude_ellipsoid,
            "alt_msl_m":       self.altitude_msl,
            "ecef_x_m":        self.ecef_x,
            "ecef_y_m":        self.ecef_y,
            "ecef_z_m":        self.ecef_z,
        }

    def generate(self) -> bytes:
        """Gera novos dados aleatórios e devolve o pacote pronto (com CRC)."""
        self._fill_random_fields()
        return self.to_bytes()

    def _lla_to_ecef(self, lat_deg: float, lon_deg: float, h: float):
        """Converte Lat/Lon/Alt (graus, graus, metros) → ECEF X,Y,Z (m)."""
        lat = math.radians(lat_deg)
        lon = math.radians(lon_deg)
        sin_lat, cos_lat = math.sin(lat), math.cos(lat)
        N = self.A / math.sqrt(1 - self.E2 * sin_lat * sin_lat)
        x = (N + h) * cos_lat * math.cos(lon)
        y = (N + h) * cos_lat * math.sin(lon)
        z = (N * (1 - self.E2) + h) * sin_lat
        return x, y, z

    def _fill_random_fields(self) -> None:
        """Atualiza todos os campos com valores pseudo-aleatórios coerentes."""
        now = datetime.now(timezone.utc)
        # TODO: fazer atualização automática
        gps_epoch = datetime(2025, 8, 6, tzinfo=timezone.utc)
        total_sec = int((now - gps_epoch).total_seconds())

        self.gnss_week = total_sec // 604_800       # 1 semana = 604 800 s
        self.gnss_tow = total_sec % 604_800

        self.latitude = random.uniform(-90.0,   90.0)
        self.longitude = random.uniform(-180.0, 180.0)
        self.altitude_ellipsoid = random.uniform(-100.0, 20_000.0)
        self.altitude_msl = self.altitude_ellipsoid - \
            random.uniform(-30.0, 70.0)

        self.ecef_x, self.ecef_y, self.ecef_z = self._lla_to_ecef(
            self.latitude, self.longitude, self.altitude_ellipsoid
        )


if __name__ == "__main__":
    gps = NASCERR_GPS()

    pkt = gps.generate()           # bytes prontos para transmitir
    print(f"Pacote (len={len(pkt)}): {pkt}")

    print(json.dumps(gps.as_dict(), indent=4))
    print(struct.unpack(gps.FMT, pkt[:-2]))
