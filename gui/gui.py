import sys
from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QPushButton, QFileDialog, QFrame
)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont, QColor, QPalette


class FileSelector(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SecureTeto")
        self.setMinimumWidth(560)
        self.setFixedHeight(260)
        self._build_ui()
        self._apply_styles()

    def _build_ui(self):
        root = QVBoxLayout(self)
        root.setContentsMargins(32, 28, 32, 28)
        root.setSpacing(20)

        # Title
        title = QLabel("SecureTeto")
        title.setObjectName("title")
        root.addWidget(title)

        # Divider
        line = QFrame()
        line.setFrameShape(QFrame.Shape.HLine)
        line.setObjectName("divider")
        root.addWidget(line)

        # File row
        file_layout = QHBoxLayout()
        file_layout.setSpacing(8)
        file_label = QLabel("Archive")
        file_label.setObjectName("label")
        file_label.setFixedWidth(60)
        self.file_input = QLineEdit()
        self.file_input.setPlaceholderText("Select a file…")
        self.file_input.setObjectName("path_input")
        file_browse = QPushButton("Browse")
        file_browse.setObjectName("browse_btn")
        file_browse.setFixedWidth(80)
        file_browse.clicked.connect(self._pick_file)
        file_layout.addWidget(file_label)
        file_layout.addWidget(self.file_input)
        file_layout.addWidget(file_browse)
        root.addLayout(file_layout)

        # Directory row
        dir_layout = QHBoxLayout()
        dir_layout.setSpacing(8)
        dir_label = QLabel("Directory")
        dir_label.setObjectName("label")
        dir_label.setFixedWidth(60)
        self.dir_input = QLineEdit()
        self.dir_input.setPlaceholderText("Select a directory…")
        self.dir_input.setObjectName("path_input")
        dir_browse = QPushButton("Browse")
        dir_browse.setObjectName("browse_btn")
        dir_browse.setFixedWidth(80)
        dir_browse.clicked.connect(self._pick_dir)
        dir_layout.addWidget(dir_label)
        dir_layout.addWidget(self.dir_input)
        dir_layout.addWidget(dir_browse)
        root.addLayout(dir_layout)

        root.addStretch()

        # Start button
        self.start_btn = QPushButton("Start")
        self.start_btn.setObjectName("start_btn")
        self.start_btn.setFixedHeight(44)
        self.start_btn.clicked.connect(self._on_start)
        root.addWidget(self.start_btn)

    def _apply_styles(self):
        self.setStyleSheet("""
            QWidget {
                background-color: #0f1117;
                color: #e2e8f0;
                font-family: 'Segoe UI', 'SF Pro Display', sans-serif;
            }
            QLabel#title {
                font-size: 18px;
                font-weight: 700;
                color: #f8fafc;
                letter-spacing: 1px;
            }
            QFrame#divider {
                color: #1e293b;
                max-height: 1px;
                background: #1e293b;
            }
            QLabel#label {
                font-size: 13px;
                font-weight: 600;
                color: #94a3b8;
                padding-top: 4px;
            }
            QLineEdit#path_input {
                background-color: #1e293b;
                border: 1px solid #334155;
                border-radius: 6px;
                padding: 8px 12px;
                font-size: 13px;
                color: #e2e8f0;
            }
            QLineEdit#path_input:focus {
                border: 1px solid #3b82f6;
                outline: none;
            }
            QLineEdit#path_input::placeholder {
                color: #475569;
            }
            QPushButton#browse_btn {
                background-color: #1e293b;
                border: 1px solid #334155;
                border-radius: 6px;
                padding: 7px 14px;
                font-size: 12px;
                font-weight: 600;
                color: #94a3b8;
            }
            QPushButton#browse_btn:hover {
                background-color: #273449;
                border-color: #3b82f6;
                color: #e2e8f0;
            }
            QPushButton#browse_btn:pressed {
                background-color: #172033;
            }
            QPushButton#start_btn {
                background-color: #3b82f6;
                border: none;
                border-radius: 8px;
                font-size: 14px;
                font-weight: 700;
                color: #ffffff;
                letter-spacing: 1px;
            }
            QPushButton#start_btn:hover {
                background-color: #2563eb;
            }
            QPushButton#start_btn:pressed {
                background-color: #1d4ed8;
            }
        """)

    def _pick_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select File")
        if path:
            self.file_input.setText(path)

    def _pick_dir(self):
        path = QFileDialog.getExistingDirectory(self, "Select Directory")
        if path:
            self.dir_input.setText(path)

    def _on_start(self):
        file_path = self.file_input.text().strip()
        dir_path = self.dir_input.text().strip()
        print(f"File: {file_path}")
        print(f"Directory: {dir_path}")
        # TODO: Logic


def start_gui():
    app = QApplication(sys.argv)
    window = FileSelector()
    window.show()
    sys.exit(app.exec())
    
if __name__ == "__main__":
    start_gui()