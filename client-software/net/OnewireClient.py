from .NetworkWrapper import NetworkWrapper
import time


class OnewireClient:
    """Provides communicates with the the onewire tcp-server.
       Can read the temperature and the Device ID
    """

    def __init__(self, host, port, log=None):
        self.c = NetworkWrapper(host, port)
        self.log = log

    def close(self):
        self.c.close()

    def read_id(self) -> bytes:
        """Reads the Onewire ID."""

        self.c.send("FLUSH")

        if self.log is not None:
            self.log("[OneWire]: sending RA")
        self.c.send("RA")

        return self.c.receive()

    def set_enable_crc(self, en: bool):
        """Enables the CRC check in the driver."""
        if en:
            self.c.send("ECRC")
        else:
            self.c.send("DCRC")

        data = self.c.receive()
        if self.log is not None:
            self.log(f"[OneWire]: received CRC {data}")


    def read_temperature(self) -> float:
        """Read the temperature and sets"""
        if self.log is not None:
            self.log("[OneWire]: sending CT")

        self.c.send("FLUSH")

        self.c.send("CT")

        data = self.c.receive()
        if self.log is not None:
            self.log(f"[OneWire]: received CT {data}")

        if self.log is not None:
            self.log("[OneWire]: sending RS")
        self.c.send("RS")

        data = self.c.receive()
        if self.log is not None:
            self.log(f"[OneWire]: received RS {data}")

        b0, b1 = data[0], data[1]

        concat = (b1 << 8) + b0

        return float(concat) / 2.0**4
