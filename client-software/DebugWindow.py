from PySide6.QtWidgets import QWidget, QVBoxLayout, QTextEdit, QLabel


class DebugWindow(QWidget):
    """
        Debug Window to show Log messages
    """

    text = ""

    def __init__(self):
        super().__init__()

        self.init_ui()

    def init_ui(self):
        """
            Initialize the Debug Window
        """
        self.setWindowTitle('Debug Window')
        self.resize(250, 150)

        self.layout = QVBoxLayout()
        self.label = QLabel("Another Window")
        self.layout.addWidget(self.label)

        self.layout = QVBoxLayout()
        self.setLayout(self.layout)

        self.label2 = QLabel("Reveived Messages")
        current_font = self.label2.font()
        current_font.setBold(True)
        self.label2.setFont(current_font)

        self.textedit = QTextEdit()
        self.textedit.setReadOnly(True)
        self.textedit.setPlaceholderText("<Debug Log>")
        self.layout.addWidget(self.textedit)

    def append_message(self, s: str):
        """Add a new log message and append it to the bottom"""
        self.text += s + "\n"
        self.textedit.setPlainText(self.text)
