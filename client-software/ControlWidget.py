from PySide6.QtWidgets import QWidget, QHBoxLayout, QLabel, QPushButton, QLineEdit, QMessageBox, QGridLayout, QCheckBox
from PySide6.QtGui import QPalette
from PySide6.QtCore import Qt, Signal, Slot

from net.OnewireClient import OnewireClient
from collections.abc import Callable

from MyTimer import *


class ControlWidget(QWidget):
    """Initialize the TCP connection.
       Read the temperature from DS18B20"""
    signal_new_temperature = Signal((float,))
    disconnect_color = Qt.yellow

    def __init__(self, log: Callable[[str], None] = None, parent=None):
        super().__init__(parent)
        self.init()
        self.client_network = None
        self.log = log

    def init(self):
        """Initialize the GUI elements."""

        self.layout = QGridLayout()
        self.layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setLayout(self.layout)

        # TCP connectoion
        self.button0 = QPushButton("Connect")
        self.button0.clicked.connect(self.button_connect_signal)

        self.button_close = QPushButton("Disconnect")
        self.button_close.clicked.connect(self.button_close_signal)

        self.label1 = QLabel("Address")
        current_font = self.label1.font()
        current_font.setBold(True)
        self.label1.setFont(current_font)

        self.line = QLineEdit()
        self.line.setText("192.168.178.24")
        self.layout.addWidget(self.button0, 0, 0)
        self.layout.addWidget(self.label1, 0, 1)
        self.layout.addWidget(self.line, 0, 2)
        self.layout.addWidget(self.button_close, 1, 0)

        self.label2 = QLabel("Port")
        current_font = self.label2.font()
        current_font.setBold(True)
        self.label2.setFont(current_font)

        self.line2 = QLineEdit()
        self.line2.setText("1033")

        self.layout.addWidget(self.label2, 1, 1)
        self.layout.addWidget(self.line2, 1, 2)

        # Change the color when connected
        palette = self.palette()
        palette.setColor(self.backgroundRole(), self.disconnect_color)
        self.setAutoFillBackground(True)
        self.setPalette(palette)

        # Button to read from the sensor
        self.buttons = ActionWidget(parent=self)
        self.layout.addWidget(self.buttons, 2, 1)
        self.buttons.signal_read_id[int].connect(self.read_id)
        self.buttons.signal_read_temperature[int].connect(
            self.read_temperature)
        self.buttons.signal_set_enable_crc[int].connect(self.set_enable_crc)



    def button_close_signal(self):
        self.client_network = None

        palette = self.palette()
        palette.setColor(self.backgroundRole(), self.disconnect_color)
        self.setPalette(palette)

    def button_connect_signal(self):
        self.client_network = OnewireClient(self.line.text(), self.line2.text(), self.log)

        palette = self.palette()
        palette.setColor(self.backgroundRole(), Qt.green)
        self.setPalette(palette)

    @Slot(int)
    def read_id(self):
        if self.client_network is not None:
            self.client_network.read_id()
        else:
            messageBox = QMessageBox()
            messageBox.critical(None, "Network Error",
                                "Not connected to client")

    @Slot(int)
    def read_temperature(self):
        if self.client_network is not None:
            value = self.client_network.read_temperature()
            self.signal_new_temperature[float].emit(value)
        else:
            messageBox = QMessageBox()
            messageBox.critical(None, "Network Error",
                                "Not connected to client")

    @Slot(int)
    def set_enable_crc(self, val):
        if self.client_network is not None:
            self.client_network.set_enable_crc(val)
        else:
            messageBox = QMessageBox()
            messageBox.critical(None, "Network Error",
                                "Not connected to client")


class ActionWidget(QWidget):
    """
        Action Widget
        Manages the button clicks and the checkboxes
        Send the 1-Wire commands to the raspberry (triggers the sending)
        Each button click is transfered to the outside using the signals 'signal_read_id' and 'signal_read_temperature'
    """

    signal_read_id = Signal((int, ))
    signal_read_temperature = Signal((int, ))
    signal_set_enable_crc = Signal((int, ))

    def __init__(self, parent=None):
        super().__init__(parent)
        self.init_ui()

    def init_ui(self):
        """
            Initialize all elements
        """

        self.net_client = None
        self.timer = MyTimer(10, self.connect_read_temp)

        self.layout = QHBoxLayout()
        self.setLayout(self.layout)

        self.read_id = QPushButton("Read ID")
        self.read_id.clicked.connect(self.connect_read_id)
        self.layout.addWidget(self.read_id)

        self.read_temp = QPushButton("Read Temperature")
        self.read_temp.clicked.connect(self.connect_read_temp)
        self.layout.addWidget(self.read_temp)

        self.checkbox = QCheckBox("Read Temperature")
        self.checkbox.setChecked(False)
        self.checkbox.stateChanged.connect(self.on_checkbox_toggled)
        self.layout.addWidget(self.checkbox)

        self.crc_checkbox = QCheckBox("Enable CRC Check")
        self.crc_checkbox.setChecked(False)
        self.crc_checkbox.stateChanged.connect(self.on_crc_checkbox_toggled)

        self.layout.addWidget(self.crc_checkbox)

    def connect_read_temp(self):
        self.signal_read_temperature[int].emit(0)

    def connect_read_id(self):
        self.signal_read_id[int].emit(0)

    def on_checkbox_toggled(self):
        if self.checkbox.isChecked():
            self.timer.start()
        else:
            self.timer.stop()

    def on_crc_checkbox_toggled(self):
        self.signal_set_enable_crc[int].emit(self.checkbox.isChecked())
