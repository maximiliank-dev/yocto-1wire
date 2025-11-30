import sys
from PySide6.QtWidgets import QApplication

from gui import *

if __name__ == "__main__":

    app = QApplication(sys.argv)

    # Create an instance of our custom QWidget
    main_app = Window()

    # Show the application window
    main_app.show()

    app.exec()
