import sys
from PyQt6.QtWidgets import (
    QApplication,
    QMainWindow,
    QMessageBox,
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QFileDialog,
    QFrame,
)
from PyQt6.QtCore import QObject, Qt, QThread, QObject, pyqtSignal
from PyQt6.QtGui import QFont, QColor, QPalette

import bindings as lib


class Worker(QObject):
    finished = pyqtSignal()
    result_ready = pyqtSignal(int)
    error = pyqtSignal(str)

    def __init__(self, archive_file: str, dir_path: str) -> None:

        super().__init__()
        self.archive_file = archive_file
        self.dir_path = dir_path

    def run(self):
        try:
            result = lib.create_archive(self.archive_file, self.dir_path)
            if result == 0:
                self.result_ready.emit(result)
            else:
                self.error.emit("Failed to create archive.")
        except Exception as e:
            self.error.emit(str(e))
        finally:
            self.finished.emit()


class FileSelector(QMainWindow):
    def __init__(self):
        super().__init__()
        self._worker_thread: QThread | None = None
        self._worker: Worker | None = None
        self.setWindowTitle("SecureTeto")
        self.setMinimumWidth(560)
        self.setFixedHeight(260)
        self._build_ui()
        self._apply_styles()

    def _build_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        root = QVBoxLayout(central_widget)
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
        self.setStyleSheet(
            """
            QWidget {
                background-color: #1a1a1c; 
                color: #fce4ec;
                font-family: 'Segoe UI', 'SF Pro Display', sans-serif;
            }

            QLabel#title {
                font-size: 18px;
                font-weight: 700;
                color: #ff3e4d; 
                letter-spacing: 1.5px;
            }

            QFrame#divider {
                background-color: #a32638;
                max-height: 1px;
            }

            QLabel#label {
                font-size: 13px;
                font-weight: 600;
                color: #ffb3ba; 
                padding-top: 4px;
            }

            QLineEdit#path_input {
                background-color: #252529; 
                border: 1px solid #44444a;
                border-radius: 6px;
                padding: 8px 12px;
                font-size: 13px;
                color: #ffffff;
            }

            QLineEdit#path_input:focus {
                border: 1px solid #ff3e4d;
                outline: none;
            }

            QLineEdit#path_input::placeholder {
                color: #66666e;
            }

            QPushButton#browse_btn {
                background-color: #2d2d32;
                border: 1px solid #44444a;
                border-radius: 6px;
                padding: 7px 14px;
                font-size: 12px;
                font-weight: 600;
                color: #ffb3ba;
            }

            QPushButton#browse_btn:hover {
                background-color: #38383e;
                border-color: #ff3e4d;
                color: #ffffff;
            }

            QPushButton#browse_btn:pressed {
                background-color: #1a1a1c;
            }

            QPushButton#start_btn {
                background-color: #ff3e4d;
                border: none;
                border-radius: 8px;
                font-size: 14px;
                font-weight: 700;
                color: #ffffff;
                letter-spacing: 1px;
            }

            QPushButton#start_btn:hover {
                background-color: #ff5766;
            }

            QPushButton#start_btn:pressed {
                background-color: #d63031;
            }
        """
        )

    def _pick_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select File")
        if path:
            self.file_input.setText(path)

    def _pick_dir(self):
        path = QFileDialog.getExistingDirectory(self, "Select Directory")
        if path:
            self.dir_input.setText(path)
            
    def _get_dirs(self):
        file_path = self.file_input.text().strip()
        dir_path = self.dir_input.text().strip()
        
        return (file_path, dir_path)

    def _on_start(self):
        file_path, dir_path = self._get_dirs()

        if not file_path:
            self._show_error("Please select an archive file.")
            return

        if not dir_path:
            self._show_error("Please select a destination directory.")
            return

        self._start_processing(file_path, dir_path)

    def _show_error(self, message):
        QMessageBox.critical(self, "Error", message)

    def _show_info(self, message):
        QMessageBox.information(self, "Info", message)

    def _start_processing(self, file_path, dir_path):
        print(f"Processing file: {file_path}")
        print(f"Destination directory: {dir_path}")
        self._start_worker()

    def on_result(self, result):
        self.start_btn.setEnabled(True)
        self._show_info(f"Archive created successfully. Result: {result}")

    def on_error(self, error_message):
        self.start_btn.setEnabled(True)
        self._show_error(f"Error during processing: {error_message}")
        
    def _start_worker(self):
        self.start_btn.setEnabled(False)
        
        file_path, dir_path = self._get_dirs()
        
        self._worker_thread = QThread()
        self._worker = Worker(file_path, dir_path)
        
        self._worker.moveToThread(self._worker_thread)
        
        self._worker_thread.started.connect(self._worker.run)
        self._worker.finished.connect(self._worker_thread.quit)
        self._worker.finished.connect(self._worker.deleteLater)
        self._worker_thread.finished.connect(self._worker_thread.deleteLater)
        self._worker.result_ready.connect(self.on_result)
        self._worker.error.connect(self.on_error)
        
        self._worker_thread.start()


def start_gui():
    app = QApplication(sys.argv)
    window = FileSelector()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    start_gui()
