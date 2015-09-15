import argparse

from PyQt5.QtWidgets import QMainWindow

from cetech.qtapi import QtConsoleAPI
from console.ui.mainwindow import Ui_MainWindow

from shared.logwidget import LogWidget
from shared.consolewidget import ConsoleWidget
from shared.replwidget import REPLWidget

class MainWindow(QMainWindow, Ui_MainWindow):


    def __init__(self):
        super(MainWindow, self).__init__()
        self.setupUi(self)

        self.centralwidget.hide()

        self.parser = argparse.ArgumentParser("cetech_console")
        self.parser.add_argument("-d", "--data-dir", type=str, help="data dir")
        self.parser.add_argument("-a", "--console-address", type=str, help="console address", default='localhost')
        self.parser.add_argument("-p", "--console-port", type=int, help="console port", default=2222)
        self.args = self.parser.parse_args()

        self.data_dir = self.args.data_dir
        self.console_port = self.args.console_port
        self.console_address = self.args.console_address

        self.api = QtConsoleAPI(self.console_address, self.console_port)

        self.console_widget = ConsoleWidget(self.api)
        self.console_dock_widget.setWidget(self.console_widget)

        self.log_widget = LogWidget(self.api, ingore_where='lua\.*.*')
        self.log_dock_widget.setWidget(self.log_widget)

        self.repl_widget = REPLWidget(self.api)
        self.repl_dock_widget.setWidget(self.repl_widget)

        self.api.start_tick(10)

    def closeEvent(self, evnt):
        self.api.disconnect()

        self.statusbar.showMessage("Disconnecting ...")

        while (self.api.connect):
            self.api.tick()

        self.statusbar.showMessage("Disconnected")

        super(MainWindow, self).closeEvent(evnt)
