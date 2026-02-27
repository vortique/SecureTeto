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
    QTabWidget,
)
from PyQt6.QtCore import QObject, Qt, QThread, pyqtSignal
from PyQt6.QtGui import QFont

import bindings as lib


class CreateWorker(QObject):
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


class ExtractWorker(QObject):
    finished = pyqtSignal()
    result_ready = pyqtSignal(int)
    error = pyqtSignal(str)

    def __init__(self, archive_file: str, dir_path: str) -> None:
        super().__init__()
        self.archive_file = archive_file
        self.dir_path = dir_path

    def run(self):
        try:
            result = lib.extract_archive(self.archive_file, self.dir_path)
            if result == 0:
                self.result_ready.emit(result)
            else:
                self.error.emit("Failed to extract archive.")
        except Exception as e:
            self.error.emit(str(e))
        finally:
            self.finished.emit()


class PathRow(QWidget):
    """A label + line-edit + browse-button row."""

    def __init__(
        self, label: str, placeholder: str, pick_dir: bool = False, parent=None
    ):
        super().__init__(parent)
        self._pick_dir = pick_dir

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(8)

        lbl = QLabel(label)
        lbl.setObjectName("label")
        lbl.setFixedWidth(70)

        self.input = QLineEdit()
        self.input.setPlaceholderText(placeholder)
        self.input.setObjectName("path_input")

        btn = QPushButton("Browse")
        btn.setObjectName("browse_btn")
        btn.setFixedWidth(80)
        btn.clicked.connect(self._browse)

        layout.addWidget(lbl)
        layout.addWidget(self.input)
        layout.addWidget(btn)

    def _browse(self):
        if self._pick_dir:
            path = QFileDialog.getExistingDirectory(self, "Select Directory")
        else:
            path, _ = QFileDialog.getOpenFileName(self, "Select File")
        if path:
            self.input.setText(path)

    def text(self) -> str:
        return self.input.text().strip()


class CreateTab(QWidget):
    def __init__(self, show_error, show_info, parent=None):
        super().__init__(parent)
        self._show_error = show_error
        self._show_info = show_info
        self._worker_thread: QThread | None = None
        self._worker: CreateWorker | None = None
        self._build_ui()

    def _build_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 16, 0, 0)
        layout.setSpacing(14)

        desc = QLabel("Pack a directory into a new archive file.")
        desc.setObjectName("desc")
        layout.addWidget(desc)

        self.archive_row = PathRow("Archive", "Output archive path…", pick_dir=False)
        layout.addWidget(self.archive_row)

        self.dir_row = PathRow("Directory", "Source directory…", pick_dir=True)
        layout.addWidget(self.dir_row)

        layout.addStretch()

        self.start_btn = QPushButton("Create Archive")
        self.start_btn.setObjectName("start_btn")
        self.start_btn.setFixedHeight(44)
        self.start_btn.clicked.connect(self._on_start)
        layout.addWidget(self.start_btn)

    def _on_start(self):
        archive = self.archive_row.text()
        directory = self.dir_row.text()
        if not archive:
            self._show_error("Please select an archive file path.")
            return
        if not directory:
            self._show_error("Please select a source directory.")
            return
        self._start_worker(archive, directory)

    def _start_worker(self, archive, directory):
        self.start_btn.setEnabled(False)
        self._worker_thread = QThread()
        self._worker = CreateWorker(archive, directory)
        self._worker.moveToThread(self._worker_thread)
        self._worker_thread.started.connect(self._worker.run)
        self._worker.finished.connect(self._worker_thread.quit)
        self._worker.finished.connect(self._worker.deleteLater)
        self._worker_thread.finished.connect(self._worker_thread.deleteLater)
        self._worker.result_ready.connect(self._on_result)
        self._worker.error.connect(self._on_error)
        self._worker_thread.start()

    def _on_result(self, result):
        self.start_btn.setEnabled(True)
        self._show_info("Archive created successfully.")

    def _on_error(self, msg):
        self.start_btn.setEnabled(True)
        self._show_error(f"Error: {msg}")


class ExtractTab(QWidget):
    def __init__(self, show_error, show_info, parent=None):
        super().__init__(parent)
        self._show_error = show_error
        self._show_info = show_info
        self._worker_thread: QThread | None = None
        self._worker: ExtractWorker | None = None
        self._build_ui()

    def _build_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 16, 0, 0)
        layout.setSpacing(14)

        desc = QLabel("Extract the contents of an archive into a directory.")
        desc.setObjectName("desc")
        layout.addWidget(desc)

        self.archive_row = PathRow("Archive", "Select archive file…", pick_dir=False)
        layout.addWidget(self.archive_row)

        self.dir_row = PathRow("Output Dir", "Destination directory…", pick_dir=True)
        layout.addWidget(self.dir_row)

        layout.addStretch()

        self.start_btn = QPushButton("Extract Archive")
        self.start_btn.setObjectName("start_btn")
        self.start_btn.setFixedHeight(44)
        self.start_btn.clicked.connect(self._on_start)
        layout.addWidget(self.start_btn)

    def _on_start(self):
        archive = self.archive_row.text()
        directory = self.dir_row.text()
        if not archive:
            self._show_error("Please select an archive file.")
            return
        if not directory:
            self._show_error("Please select a destination directory.")
            return
        self._start_worker(archive, directory)

    def _start_worker(self, archive, directory):
        self.start_btn.setEnabled(False)
        self._worker_thread = QThread()
        self._worker = ExtractWorker(archive, directory)
        self._worker.moveToThread(self._worker_thread)
        self._worker_thread.started.connect(self._worker.run)
        self._worker.finished.connect(self._worker_thread.quit)
        self._worker.finished.connect(self._worker.deleteLater)
        self._worker_thread.finished.connect(self._worker_thread.deleteLater)
        self._worker.result_ready.connect(self._on_result)
        self._worker.error.connect(self._on_error)
        self._worker_thread.start()

    def _on_result(self, result):
        self.start_btn.setEnabled(True)
        self._show_info("Archive extracted successfully.")

    def _on_error(self, msg):
        self.start_btn.setEnabled(True)
        self._show_error(f"Error: {msg}")


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SecureTeto")
        self.setMinimumWidth(720)
        self.setFixedHeight(400)
        self._build_ui()
        self._apply_styles()

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)

        root = QVBoxLayout(central)
        root.setContentsMargins(32, 28, 32, 28)
        root.setSpacing(16)

        # Title
        title = QLabel("SecureTeto")
        title.setObjectName("title")
        root.addWidget(title)

        # Divider
        line = QFrame()
        line.setFrameShape(QFrame.Shape.HLine)
        line.setObjectName("divider")
        root.addWidget(line)

        # Tabs
        self.tabs = QTabWidget()
        self.tabs.setObjectName("tabs")
        self.tabs.addTab(
            CreateTab(self._show_error, self._show_info),
            "  Create  ",
        )
        self.tabs.addTab(
            ExtractTab(self._show_error, self._show_info),
            "  Extract  ",
        )
        root.addWidget(self.tabs)

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

            QLabel#desc {
                font-size: 12px;
                color: #888;
                margin-bottom: 4px;
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

            QPushButton#start_btn:disabled {
                background-color: #5a1a20;
                color: #888;
            }

            QTabWidget#tabs {
                background-color: transparent;
            }

            QTabWidget#tabs::pane {
                border: 1px solid #2e2e33;
                border-radius: 6px;
                background-color: #1e1e21;
                padding: 8px;
            }

            QTabBar::tab {
                background-color: #252529;
                color: #888;
                border: 1px solid #2e2e33;
                border-bottom: none;
                border-radius: 6px 6px 0 0;
                padding: 6px 16px;
                font-size: 13px;
                font-weight: 600;
            }

            QTabBar::tab:selected {
                background-color: #1e1e21;
                color: #ff3e4d;
                border-color: #2e2e33;
            }

            QTabBar::tab:hover:!selected {
                color: #ffb3ba;
                background-color: #2d2d32;
            }
        """
        )

    def _show_error(self, message):
        QMessageBox.critical(self, "Error", message)

    def _show_info(self, message):
        QMessageBox.information(self, "Info", message)


def start_gui():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    start_gui()
