import sys
import subprocess
import os
from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QVBoxLayout, QLabel
from PyQt5.QtGui import QFont, QColor, QPalette
from PyQt5.QtCore import Qt, QSize, QPoint


class AdblockApp(QWidget):
    def __init__(self):
        super().__init__()

        self.project_root = os.path.dirname(os.path.abspath(__file__))
        backend_exe_name = 'adblocker.exe' if os.name == 'nt' else 'adblocker'
        self.backend_path = os.path.join(self.project_root, 'backend', backend_exe_name)
        hosts_path = os.path.join(self.project_root, 'backend', 'hosts.txt')

        if not os.path.exists(self.backend_path):
            print(f"HATA: C++ programı burada bulunamadı:\n{self.backend_path}")
            print("Lütfen 'backend' klasörüne gidip C++ kodunu derlediğinizden")
            print("ve adını 'adblocker' (Mac için uzantısız) olarak ayarladığınızdan emin olun.")

        if not os.path.exists(hosts_path):
            print(f"UYARI: hosts.txt dosyası burada bulunamadı:\n{hosts_path}")
            print("Backend çalışacak ancak hiçbir reklamı engellemeyecek.")

        self.backend_process = None
        self.is_connected = False

        self.initUI()

    def initUI(self):
        self.setGeometry(300, 300, 320, 480)
        self.setWindowTitle('AD-Block')

        self.setStyleSheet("""
            QWidget {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                                            stop:0 #E04B4B, stop:1 #4B4BE0);
                border-radius: 20px;
            }
        """)

        self.titleLabel = QLabel("AD-Block", self)
        self.titleLabel.setFont(QFont('Arial', 24, QFont.Bold))
        self.titleLabel.setAlignment(Qt.AlignCenter)
        self.titleLabel.setStyleSheet("color: white; background: transparent;")
        self.titleLabel.setGeometry(0, 50, self.width(), 40)

        self.toggleButton = QPushButton(self)
        self.toggleButton.setText('OPEN')

        button_size = 180
        self.toggleButton.setFixedSize(button_size, button_size)

        button_x = (self.width() - button_size) // 2
        button_y = (self.height() - button_size) // 2
        self.toggleButton.move(button_x, button_y)

        self.toggleButton.setFont(QFont('Arial', 24, QFont.Bold))

        self.toggleButton.clicked.connect(self.toggle_connection)

        self.update_button_style()

        self.center_window()

        self.show()

    def center_window(self):
        qr = self.frameGeometry()
        cp = QApplication.desktop().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())

    def toggle_connection(self):
        self.is_connected = not self.is_connected

        if self.is_connected:
            try:
                self.backend_process = subprocess.Popen(
                    [self.backend_path],
                    cwd=self.project_root
                )
                print(f"Backend başlatıldı. (PID: {self.backend_process.pid})")
            except Exception as e:
                print(f"HATA: Backend başlatılamadı! {e}")
                print("Lütfen script'i 'sudo python3 app.py' ile (Yönetici olarak) çalıştırdığınızdan emin olun.")
                self.is_connected = False
        else:
            if self.backend_process:
                self.backend_process.terminate()
                self.backend_process.wait()
                print("Backend durduruldu.")
                self.backend_process = None

        self.update_button_style()

    def update_button_style(self):
        if self.is_connected:
            self.toggleButton.setText('CLOSE')
            self.toggleButton.setStyleSheet(f"""
                QPushButton {{
                    background-color: #E04B4B;
                    color: white;
                    border: none;
                    border-radius: {self.toggleButton.width() // 2}px;
                    box-shadow: 0px 8px 15px rgba(0, 0, 0, 0.4);
                }}
                QPushButton:hover {{
                    background-color: #C0392B;
                }}
                QPushButton:pressed {{
                    background-color: #A03020;
                    box-shadow: 0px 2px 5px rgba(0, 0, 0, 0.4);
                    padding-top: 2px;
                }}
            """)
        else:
            self.toggleButton.setText('OPEN')
            self.toggleButton.setStyleSheet(f"""
                QPushButton {{
                    background-color: #2ecc71;
                    color: white;
                    border: none;
                    border-radius: {self.toggleButton.width() // 2}px;
                    box-shadow: 0px 8px 15px rgba(0, 0, 0, 0.4);
                }}
                QPushButton:hover {{
                    background-color: #27ae60;
                }}
                QPushButton:pressed {{
                    background-color: #1abc9c;
                    box-shadow: 0px 2px 5px rgba(0, 0, 0, 0.4);
                    padding-top: 2px;
                }}
            """)

    def closeEvent(self, event):
        if self.is_connected:
            self.toggle_connection()
        event.accept()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = AdblockApp()
    sys.exit(app.exec_())