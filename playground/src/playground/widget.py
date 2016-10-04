﻿from PyQt5.QtCore import Qt
from PyQt5.QtGui import QCursor
from PyQt5.QtWidgets import QWidget


class Widget(QWidget):
    def __init__(self, parent, api, log_url):
        self.api = api
        self.ready = False
        self.last_pos = (0, 0)
        self.instance = None

        self.start_pos = None
        self.move_skip = False

        self.left = False
        self.midle = False
        self.right = False

        super(Widget, self).__init__(parent, Qt.ForeignWindow)

        self.setFocusPolicy(Qt.ClickFocus)

    def resizeEvent(self, event):
        if self.api and self.ready:
            size = event.size()
            self.api.resize(size.width(), size.height())

        event.accept()

    def _move_mouse(self, x, y, left, midle, right):
        self.api.lua_execute(script="EditorInput:Move(%d, %d, %s, %s, %s)" % (x, y, left, midle, right))

    def mouseMoveEvent(self, event):
        """ QWidget.mouseMoveEvent(QMouseEvent) 
        :type event: PyQt5.QtGui.QMouseEvent.QMouseEvent
        """

        if not self.instance:
            return

        if not self.instance.ready:
            return

        if self.move_skip:
            self.move_skip = False
            return

        if not self.start_pos:
            self.start_pos = event.globalPos()

        if self.left or self.right:
            self.move_skip = True
            QCursor.setPos(self.start_pos)

        else:
            self.start_pos = None

        if self.start_pos is not None:
            self._move_mouse(self.start_pos.x() - event.globalPos().x(),
                             self.start_pos.y() - event.globalPos().y(),
                             str(self.left).lower(), str(self.midle).lower(), str(self.right).lower())

    def mousePressEvent(self, event):  # real signature unknown; restored from __doc__
        """ QWidget.mousePressEvent(QMouseEvent) """

        buttons = event.buttons()

        self.left = buttons == Qt.LeftButton
        self.midle = buttons == Qt.MidButton
        self.right = buttons == Qt.RightButton

        self.setCursor(Qt.BlankCursor)

    def mouseReleaseEvent(self, event):
        """ QWidget.mouseReleaseEvent(QMouseEvent) """
        buttons = event.buttons()

        self.setCursor(Qt.ArrowCursor)

    def keyPressEvent(self, event):
        """ QWidget.keyPressEvent(QKeyEvent)
        :type event: PyQt5.QtGui.QKeyEvent
        """
        btn = event.text()

        btn_map = {
            "w": "up",
            "s": "down",
            "a": "left",
            "d": "right",
        }

        if btn in btn_map:
            self.api.lua_execute(script="EditorInput.keyboard.%s = true" % (btn_map[btn]))

    def keyReleaseEvent(self, event):
        """ QWidget.keyReleaseEvent(QKeyEvent)
        :type event: PyQt5.QtGui.QKeyEvent
        """
        btn = event.text()

        btn_map = {
            "w": "up",
            "s": "down",
            "a": "left",
            "d": "right",
        }

        if btn in btn_map:
            self.api.lua_execute(script="EditorInput.keyboard.%s = false" % (btn_map[btn]))

    def set_instance(self, instance):
        self.instance = instance