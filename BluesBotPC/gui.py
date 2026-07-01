"""
Script Runner — PyQt6 GUI
Usage: python script_runner.py
Edit the CONFIG section below.
"""

import sys
import os
import subprocess

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QComboBox, QTextEdit, QFrame, QSizePolicy
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal
from PyQt6.QtGui import QColor, QTextCursor

# ─── CONFIG ──────────────────────────────────────────────────────────────────
MAIN_SCRIPT  = "bleSend.py"          
INPUT_DIR    = "."                
FILE_EXTS    = [".txt"]   
WINDOW_TITLE = "BluesBot Library"
# ─────────────────────────────────────────────────────────────────────────────

STYLESHEET = """
QMainWindow, QWidget#root { background-color: #111318; }
QWidget {
    background-color: #111318;
    color: #E8EAF0;
    font-family: 'Segoe UI', 'SF Pro Display', sans-serif;
    font-size: 13px;
}
QLabel#header {
    color: #E8EAF0; font-size: 20px; font-weight: 700;
    letter-spacing: 1px; background: transparent;
}
QLabel#subtitle {
    color: #555B6E; font-size: 11px; letter-spacing: 2px; background: transparent;
}
QLabel#section {
    color: #555B6E; font-size: 10px; font-weight: 600;
    letter-spacing: 2px; background: transparent;
}
QLabel#dirpath {
    color: #3A4055; font-family: 'Consolas','Courier New',monospace;
    font-size: 11px; background: transparent;
}
QComboBox#filedrop {
    background-color: #1B1F28; border: 1px solid #252A36;
    border-radius: 8px; padding: 10px 14px; color: #C8CEDA;
    font-family: 'Consolas','Courier New',monospace; font-size: 12px; min-height: 40px;
}
QComboBox#filedrop:focus { border: 1px solid #3D5AFE; background-color: #1E2230; }
QComboBox#filedrop::drop-down { border: none; width: 32px; }
QComboBox#filedrop::down-arrow {
    border-left: 5px solid transparent; border-right: 5px solid transparent;
    border-top: 6px solid #555B6E; width: 0; height: 0; margin-right: 10px;
}
QComboBox QAbstractItemView {
    background-color: #1B1F28; border: 1px solid #252A36;
    color: #C8CEDA; selection-background-color: #3D5AFE;
    selection-color: #ffffff; padding: 4px;
    font-family: 'Consolas','Courier New',monospace; font-size: 12px; outline: none;
}
QPushButton#refresh {
    background-color: #1B1F28; border: 1px solid #252A36; border-radius: 8px;
    padding: 10px 16px; color: #8B92A8; font-weight: 600; font-size: 14px;
    min-width: 44px; min-height: 42px;
}
QPushButton#refresh:hover { background-color: #22283A; border-color: #3D5AFE; color: #E8EAF0; }
QPushButton#run {
    background-color: #3D5AFE; border: none; border-radius: 8px;
    padding: 12px 32px; color: #ffffff; font-weight: 700;
    font-size: 13px; letter-spacing: 0.5px; min-width: 120px; min-height: 42px;
}
QPushButton#run:hover { background-color: #536DFE; }
QPushButton#run:pressed { background-color: #304FFE; }
QPushButton#run:disabled { background-color: #1E2230; color: #3A4055; }
QPushButton#stop {
    background-color: transparent; border: 1px solid #3A1F1F; border-radius: 8px;
    padding: 12px 24px; color: #EF5350; font-weight: 600;
    font-size: 12px; min-width: 90px; min-height: 42px;
}
QPushButton#stop:hover { background-color: #2A1515; border-color: #EF5350; }
QPushButton#stop:disabled { color: #3A2828; border-color: #251A1A; }
QTextEdit#console {
    background-color: #0D1017; border: 1px solid #1C2030; border-radius: 10px;
    padding: 14px; color: #A8B0C8;
    font-family: 'Consolas','Courier New',monospace; font-size: 12px;
    selection-background-color: #3D5AFE55;
}
QLabel#status {
    font-size: 11px; font-weight: 600; letter-spacing: 1px;
    background: transparent; padding: 2px 0;
}
QFrame#divider { background-color: #1C2030; max-height: 1px; }
QPushButton#clear {
    background: transparent; border: none; color: #3A4055;
    font-size: 11px; font-weight: 600; letter-spacing: 1px; padding: 2px 6px;
}
QPushButton#clear:hover { color: #8B92A8; }
"""


def scan_files(directory, extensions):
    try:
        abs_dir = os.path.abspath(directory)
        files = [
            f for f in os.listdir(abs_dir)
            if os.path.isfile(os.path.join(abs_dir, f))
            and os.path.splitext(f)[1].lower() in [e.lower() for e in extensions]
        ]
        return sorted(files)
    except Exception:
        return []


class RunnerThread(QThread):
    output_ready = pyqtSignal(str, str)
    finished = pyqtSignal(int)

    def __init__(self, script, input_file):
        super().__init__()
        self.script = script
        self.input_file = input_file
        self._process = None

    def run(self):
        cmd = [sys.executable, self.script, self.input_file]
        try:
            self._process = subprocess.Popen(
                cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                text=True, bufsize=1,
                cwd=os.path.dirname(os.path.abspath(self.script)) or "."
            )
            for line in self._process.stdout:
                self.output_ready.emit(line, "stdout")
            for line in self._process.stderr:
                self.output_ready.emit(line, "stderr")
            self._process.wait()
            self.finished.emit(self._process.returncode)
        except FileNotFoundError:
            self.output_ready.emit(f"Error: script not found — '{self.script}'\n", "stderr")
            self.finished.emit(-1)
        except Exception as e:
            self.output_ready.emit(f"Error: {e}\n", "stderr")
            self.finished.emit(-1)

    def stop(self):
        if self._process and self._process.poll() is None:
            self._process.terminate()


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle(WINDOW_TITLE)
        self.resize(740, 560)
        self.setMinimumSize(540, 400)
        self._thread = None
        self._build_ui()
        self._refresh_files()

    def _build_ui(self):
        root = QWidget()
        root.setObjectName("root")
        self.setCentralWidget(root)

        layout = QVBoxLayout(root)
        layout.setContentsMargins(28, 24, 28, 20)
        layout.setSpacing(0)

        # Header
        header = QLabel(WINDOW_TITLE.upper())
        header.setObjectName("header")
        subtitle = QLabel("The Autonomous Harmonica Playing Robot")
        subtitle.setObjectName("subtitle")
        layout.addWidget(header)
        layout.addSpacing(2)
        layout.addWidget(subtitle)
        layout.addSpacing(24)
        '''
        # Section label row
        sec_row = QHBoxLayout()
        sec_label = QLabel("INPUT FILE")
        sec_label.setObjectName("section")
        ext_label = QLabel("  ·  " + "  ".join(e.upper() for e in FILE_EXTS))
        ext_label.setObjectName("dirpath")
        dir_label = QLabel(f"  from  {os.path.abspath(INPUT_DIR)}")
        dir_label.setObjectName("dirpath")
        sec_row.addWidget(sec_label)
        sec_row.addWidget(ext_label)
        sec_row.addWidget(dir_label)
        sec_row.addStretch()
        layout.addLayout(sec_row)
        layout.addSpacing(8)
        '''
        # Dropdown + refresh
        file_row = QHBoxLayout()
        file_row.setSpacing(8)

        self.dropdown = QComboBox()
        self.dropdown.setObjectName("filedrop")
        self.dropdown.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        self.dropdown.setFixedHeight(42)

        refresh_btn = QPushButton("↺")
        refresh_btn.setObjectName("refresh")
        refresh_btn.setFixedHeight(42)
        refresh_btn.setFixedWidth(44)
        refresh_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        refresh_btn.setToolTip("Refresh file list")
        refresh_btn.clicked.connect(self._refresh_files)

        file_row.addWidget(self.dropdown)
        file_row.addWidget(refresh_btn)
        layout.addLayout(file_row)
        layout.addSpacing(20)

        # Divider
        divider = QFrame()
        divider.setObjectName("divider")
        divider.setFrameShape(QFrame.Shape.HLine)
        layout.addWidget(divider)
        layout.addSpacing(20)

        # Console header
        console_header = QHBoxLayout()
        console_sec = QLabel("OUTPUT")
        console_sec.setObjectName("section")
        self.status_label = QLabel("READY")
        self.status_label.setObjectName("status")
        self.status_label.setStyleSheet("color: #3A4055;")
        clear_btn = QPushButton("CLEAR")
        clear_btn.setObjectName("clear")
        clear_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        clear_btn.clicked.connect(self._clear_console)
        console_header.addWidget(console_sec)
        console_header.addStretch()
        console_header.addWidget(self.status_label)
        console_header.addSpacing(12)
        console_header.addWidget(clear_btn)
        layout.addLayout(console_header)
        layout.addSpacing(8)

        # Console
        self.console = QTextEdit()
        self.console.setObjectName("console")
        self.console.setReadOnly(True)
        layout.addWidget(self.console)
        layout.addSpacing(16)

        # Buttons
        btn_row = QHBoxLayout()
        btn_row.setSpacing(10)
        btn_row.addStretch()

        self.stop_btn = QPushButton("Stop")
        self.stop_btn.setObjectName("stop")
        self.stop_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        self.stop_btn.setEnabled(False)
        self.stop_btn.clicked.connect(self._stop)

        self.run_btn = QPushButton("▶  Play Song")
        self.run_btn.setObjectName("run")
        self.run_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        self.run_btn.clicked.connect(self._run)

        btn_row.addWidget(self.stop_btn)
        btn_row.addWidget(self.run_btn)
        layout.addLayout(btn_row)

    def _refresh_files(self):
        current = self.dropdown.currentText()
        files = scan_files(INPUT_DIR, FILE_EXTS)
        self.dropdown.clear()
        if files:
            self.dropdown.addItems(files)
            if current in files:
                self.dropdown.setCurrentText(current)
            self.run_btn.setEnabled(True)
        else:
            exts = ", ".join(FILE_EXTS)
            self.dropdown.addItem(f"No {exts} files found in {os.path.abspath(INPUT_DIR)}")
            self.run_btn.setEnabled(False)

    def _run(self):
        filename = self.dropdown.currentText()
        if not filename or not os.path.splitext(filename)[1]:
            return
        input_path = os.path.join(os.path.abspath(INPUT_DIR), filename)
        self._clear_console()
        self._log(f"▶  {os.path.basename(MAIN_SCRIPT)}  ←  {filename}\n", "info")
        self.run_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)
        self._set_status("RUNNING", "#3D5AFE")
        self._thread = RunnerThread(MAIN_SCRIPT, input_path)
        self._thread.output_ready.connect(self._log)
        self._thread.finished.connect(self._on_finished)
        self._thread.start()

    def _stop(self):
        if self._thread:
            self._thread.stop()
        self._set_status("STOPPED", "#EF5350")

    def _on_finished(self, code):
        self.run_btn.setEnabled(True)
        self.stop_btn.setEnabled(False)
        if code == 0:
            self._log("\n✓  Finished successfully.\n", "info")
            self._set_status("DONE", "#4CAF50")
        elif code == -1:
            self._set_status("ERROR", "#EF5350")
        else:
            self._log(f"\n✗  Exited with code {code}.\n", "stderr")
            self._set_status(f"EXIT {code}", "#EF5350")

    def _log(self, text, kind="stdout"):
        colors = {"stdout": "#A8B0C8", "stderr": "#EF9A9A", "info": "#536DFE"}
        self.console.moveCursor(QTextCursor.MoveOperation.End)
        self.console.setTextColor(QColor(colors.get(kind, "#A8B0C8")))
        self.console.insertPlainText(text)
        self.console.moveCursor(QTextCursor.MoveOperation.End)

    def _clear_console(self):
        self.console.clear()

    def _set_status(self, text, color):
        self.status_label.setText(text)
        self.status_label.setStyleSheet(f"color: {color};")


def main():
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    app.setStyleSheet(STYLESHEET)
    win = MainWindow()
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()