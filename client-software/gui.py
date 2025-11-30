from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QLabel, QPushButton, QLineEdit, QMessageBox
from PySide6.QtCore import Qt, QFile, QTextStream, Signal, Slot, QDateTime

from datetime import datetime

from ControlWidget import ControlWidget
from Graph2D import Graph2D
from net.OnewireClient import OnewireClient
from DebugWindow import DebugWindow


class Window(QMainWindow):
    """
        Main Window.
        Has a 
    """
    def __init__(self):
        super().__init__()
        self.init_ui()

        self.client = None


    def init_ui(self):
        """
            Initialize all elements
        """
        self.setWindowTitle('Onewire GUI')
        self.setGeometry(100, 100, 1000, 700)

        self.widget = QWidget()
        self.setCentralWidget(self.widget)


        layout = QVBoxLayout()
        self.widget.setLayout(layout)

        # Debug window shows debug messages
        self.debugWindow = DebugWindow()
        self.debugWindow.show()
        self.debugWindow.setVisible(False)

        # Control Widget shows 
        self.control_widget =  ControlWidget(self.debugWindow.append_message)
        layout.addWidget(self.control_widget)
        self.control_widget.signal_new_temperature[float].connect(self.update_temp)

        # Graph
        now = QDateTime.currentDateTime()

        self.graph = Graph2D(data=[[ now.addSecs(i) for i in range(0, 3) ], [0, 1, 2]])
        layout.addWidget(self.graph)

        # Show the debug window
        self.debug_button = QPushButton("Debug")
        self.debug_button.clicked.connect(self.connect_debug_window)
        layout.addWidget(self.debug_button)


    def connect_debug_window(self):
        self.debugWindow.setVisible( not self.debugWindow.isVisible())

    @Slot(float)
    def update_temp(self, tmp : float):
        now = datetime.now()
        time = now.strftime("%H:%M:%S")
        now = QDateTime.currentDateTime()

        print(f"{now=}, {tmp=}")

        self.graph.data_update( [[now], [tmp]] )
        self.update()