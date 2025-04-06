# lcd_hd44780.py
#
# Copyright 2025 Clement Savergne <csavergne@yahoo.com>
#
# This file is part of yasim-avr.
#
# yasim-avr is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# yasim-avr is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.


import sys
from enum import IntEnum
from yasimavr.lib.core import CallableSignalHook, Wire
from PyQt6 import QtCore, QtGui, QtWidgets


__all__ = ['HD44780_Display', 'HD44780_Driver', 'HD44780_Interface4Bits']


#Each entry of the charset is 'charcode' : 'dot pattern', coding 40 pixels in a 5x8 arrangement.
#A dot pattern is composed of 64 bits, each set of 8 bits coding a line, starting at the top.
#For each line the MSB codes the left-most pixel, and 3 LSBs are unused.
#Each bit is set to 1 if the corresponding dot is lit up and 0 if not.

_Charset = {
    0x20: 0x0000000000000000, #space
    0x21: 0x2020202000002000, #!
    0x22: 0x5050500000000000, #"
    0x23: 0x5050f850f8500000, ##
    0x24: 0x2078a07028f02000, #$
    0x25: 0xc8d0205898000000, #%
    0x26: 0x6090a040a8906800, #&
    0x27: 0x6020400000000000, #'
    0x28: 0x1020404040201000, #(
    0x29: 0x4020101010204000, #)
    0x2A: 0x0020a870a8200000, #*
    0x2B: 0x002020f820200000, #+
    0x2C: 0x0000000060204000, #,
    0x2D: 0x000000f800000000, #-
    0x2E: 0x0000000000606000, #.
    0x2F: 0x0008102040800000, #/

    0x30: 0x708898a8c8887000, #0
    0x31: 0x2060202020207000, #1
    0x32: 0x708808102040f800, #2
    0x33: 0xf810201008887000, #3
    0x34: 0x10305090f8101000, #4
    0x35: 0xf880f00808887000, #5
    0x36: 0x304080f088887000, #6
    0x37: 0xf808101020202000, #7
    0x38: 0x7088887088887000, #8
    0x39: 0x7088887808106000, #9

    0x3A: 0x0060600060600000, #:
    0x3B: 0x0060600060204000, #;
    0x3C: 0x1020408040201000, #<
    0x3D: 0x0000f800f8000000, #=
    0x3E: 0x4020100810204000, #>
    0x3F: 0x7088081020002000, #?
    0x40: 0x70880868a8a87000, #@

    0x41: 0x20508888f8888800, #A
    0x42: 0xf08888f08888f000, #B
    0x43: 0x7088808080887000, #C
    0x44: 0xe09088888890e000, #D
    0x45: 0xf88080f08080f800, #E
    0x46: 0xf88080f080808000, #F
    0x47: 0x708880b888887800, #G
    0x48: 0x888888f888888800, #H
    0x49: 0x7020202020207000, #I
    0x4A: 0x7810101010906000, #J
    0x4B: 0x8890a0c0a0908800, #K
    0x4C: 0x808080808080f800, #L
    0x4D: 0x88d8a8a888888800, #M
    0x4E: 0x8888c8a898888800, #N
    0x4F: 0x7088888888887000, #O
    0x50: 0xf08888f080808000, #P
    0x51: 0xf0888888a8906800, #Q
    0x52: 0xf08888f0a0908800, #R
    0x53: 0x7088807008887000, #S
    0x54: 0xf820202020202000, #T
    0x55: 0x8888888888887000, #U
    0x56: 0x8888888888502000, #V
    0x57: 0x88888888a8a85000, #W
    0x58: 0x8888502050888800, #X
    0x59: 0x8888502020202000, #Y
    0x5A: 0xf80810204080f800, #Z

    0x61: 0x0000700878887800, #a
    0x62: 0x8080f0888888f000, #b
    0x63: 0x0000788080807800, #c
    0x64: 0x0808788888887800, #d
    0x65: 0x00007088f8807000, #e
    0x66: 0x1820f82020202000, #f
    0x67: 0x0000788878087000, #g
    0x68: 0x8080b0c888888800, #h
    0x69: 0x2000602020207000, #i
    0x6A: 0x1000301010906000, #j
    0x6B: 0x4040485060504800, #k
    0x6C: 0x6020202020207000, #l
    0x6D: 0x0000d0a8a8a8a800, #m
    0x6E: 0x0000b0c888888800, #n
    0x6F: 0x0000708888887000, #o
    0x70: 0x0000f088f0808000, #p
    0x71: 0x0000788878080800, #q
    0x72: 0x0000586040404000, #r
    0x73: 0x000070807008f000, #s
    0x74: 0x2020f82020201800, #t
    0x75: 0x0000888888986800, #u
    0x76: 0x0000888888502000, #v
    0x77: 0x00008888a8a85000, #w
    0x78: 0x0000885020508800, #x
    0x79: 0x0000888878087000, #y
    0x7A: 0x0000f8102040f800, #z
}


class _ShiftMode(IntEnum):
    ShiftCursorLeft = 0
    ShiftCursorRight = 1
    ShiftDisplayLeft = 2
    ShiftDisplayRight = 3


class _CursorMode(IntEnum):
    Off = 0
    On = 1
    Blink = 2


class HD44780_Display(QtWidgets.QWidget):
    '''Implementation of a Qt Widget to draw a LCD display.
    '''

    #Constants to adjust the geometry and appearance
    FrameMargin = 4
    CharSpacing = 8
    PointSpacing = 2
    PointSize = 4
    ColorPointOn = QtGui.QColorConstants.Green
    ColorPointOff = QtGui.QColorConstants.Black
    ColorBackground = QtGui.QColorConstants.DarkGray
    PointRowsPerChar = 8
    PointColsPerChar = 5


    def __init__(self, driver, ncols, nrows, parent=None):
        '''Initialisation of a LCD display widget.
        \arg driver instance of HD44780_Driver
        \arg ncols number of character columns
        \arg nrows number of character rows
        \arg parent parent widget
        '''

        super().__init__(parent)

        if not (0 < ncols <= 40 and 1 <= nrows <= 2):
            raise Exception("Invalid row/col arguments")
        if driver is None:
            raise Exception("Invalid driver argument")

        self._driver = driver
        self._ncols = ncols
        self._nrows = nrows
        self._cursor_on = False
        self._blink_tid = -1

        char_width = self.PointColsPerChar * self.PointSize + (self.PointColsPerChar - 1) * self.PointSpacing
        char_height = self.PointRowsPerChar * self.PointSize + (self.PointRowsPerChar - 1) * self.PointSpacing
        disp_width = self._ncols * char_width + (self._ncols + 1) * self.CharSpacing
        disp_height = self._nrows * char_height + (self._nrows + 1) * self.CharSpacing
        self._disp_size = (disp_width, disp_height)

        driver.sigDisplayChange.connect(self.update)
        driver.sigCursorModeChange.connect(self._slot_CursorModeChange)


    def _slot_CursorModeChange(self, mode):
        mode = _CursorMode(mode)
        if mode == _CursorMode.Off:
            enable_blink = False
            self._cursor_on = False
        elif mode == _CursorMode.On:
            enable_blink = False
            self._cursor_on = True
        else: #Blink
            enable_blink = True

        #Start or kill the blink timer if necessary
        if enable_blink and self._blink_tid == -1:
            self._blink_tid = self.startTimer(500)
        elif self._blink_tid > 0 and not enable_blink:
            self.killTimer(self._blink_tid)
            self._blink_tid = -1

        self.update()


    #QWidget override
    def timerEvent(self, event):
        if event.timerId() == self._blink_tid:
            self._cursor_on = not self._cursor_on
            self.update()


    #QWidget override
    def sizeHint(self):
        disp_width, disp_height = self._disp_size
        return QtCore.QSize(disp_width + 2 * self.FrameMargin,
                            disp_height + 2 * self.FrameMargin)


    def _draw_char(self, painter, xc, yc, pos):
        #Find the pattern for the character at position 'pos'
        charcode = self._driver._chars[pos]
        if charcode < 0x10:
            pattern = self._driver._custom_patterns[charcode & 0x07]
        else:
            pattern = _Charset.get(charcode, 0)

        #If the cursor is shown, force on the 8th line of points
        if self._cursor_on and pos == self._driver._cursor_pos:
            pattern |= 0x0000000000000000F8

        #Iterate through each point composing the pattern and draw it using
        #either the color OFF or ON depending on the pattern bits
        yp = yc
        for _ in range(self.PointRowsPerChar):
            xp = xc
            for _ in range(self.PointColsPerChar):
                if (pattern & 0x8000000000000000) and self._driver._display_on:
                    color = self.ColorPointOn
                else:
                    color = self.ColorPointOff

                painter.fillRect(xp, yp, self.PointSize, self.PointSize, color)

                pattern = (pattern & 0x7FFFFFFFFFFFFFFF) << 1
                xp += self.PointSize + self.PointSpacing

            pattern = (pattern & 0x1FFFFFFFFFFFFFFF) << 3
            yp += self.PointSize + self.PointSpacing


    #QWidget override
    def paintEvent(self, event):
        super().paintEvent(event)

        #Initialise a painter for this widget
        painter = QtGui.QPainter(self)

        #Fill the background
        w, h = self._disp_size
        painter.fillRect(self.FrameMargin, self.FrameMargin, w, h, self.ColorBackground)

        #Draw each character
        char_width = self.PointColsPerChar * self.PointSize + (self.PointColsPerChar - 1) * self.PointSpacing
        char_height = self.PointRowsPerChar * self.PointSize + (self.PointRowsPerChar - 1) * self.PointSpacing
        yc = self.FrameMargin + self.CharSpacing
        for j in range(self._nrows):
            pos = j * 40
            xc = self.FrameMargin + self.CharSpacing
            for _ in range(self._ncols):
                self._draw_char(painter, xc, yc, pos)
                pos += 1
                xc += char_width + self.CharSpacing
            yc += char_height + self.CharSpacing


class HD44780_Driver(QtCore.QObject):
    '''Implementation of a LCD driver.
    The main class feature is to decode HD44780 commands.
    '''

    sigDisplayChange = QtCore.pyqtSignal()
    sigCursorModeChange = QtCore.pyqtSignal(int)


    def __init__(self, parent=None):
        super().__init__(parent)
        self._chars = [0x20] * 80
        self._custom_patterns = [0] * 8
        self._cursor_pos = 0
        self._entry_shift_mode = _ShiftMode.ShiftCursorRight
        self._display_on = False
        self._command_pattern = False
        self._interface_8bits = True


    def interface_8bits(self):
        return self._interface_8bits


    def shift_display(self, mode):
        mode = int(_ShiftMode(mode))
        right_or_left = bool(mode & 1)
        shift_disp = bool(mode & 2)

        if right_or_left:
            self._cursor_pos = (self._cursor_pos + 1) % 80

            if shift_disp:
                c = self._chars[39]
                self._chars[1:40] = self._chars[0:39]
                self._chars[0] = c

                c = self._chars[79]
                self._chars[41:80] = self._chars[40:79]
                self._chars[40] = c

        else:
            self._cursor_pos = (self._cursor_pos - 1) % 80

            if shift_disp:
                c = self._chars[0]
                self._chars[0:39] = self._chars[1:40]
                self._chars[39] = c

                c = self._chars[40]
                self._chars[40:79] = self._chars[41:80]
                self._chars[79] = c

        self.sigDisplayChange.emit()


    def set_cursor_pos(self, pos):
        pos &= 0x7F
        if pos < 0x40:
            self._cursor_pos = pos
        else:
            self._cursor_pos = pos - 0x40 + 40

        self.sigDisplayChange.emit()


    def clear(self):
        for i in range(80):
            self._chars[i] = 0x20
        self._cursor_pos = 0
        self._entry_shift_mode = _ShiftMode.ShiftCursorRight
        self.sigDisplayChange.emit()


    def home(self):
        self._cursor_pos = 0
        self.sigDisplayChange.emit()


    def set_entry_shift_mode(self, mode):
        self._entry_shift_mode = _ShiftMode(mode)


    def set_display(self, on):
        self._display_on = bool(on)
        self.sigDisplayChange.emit()


    def set_cursor_mode(self, mode):
        self.sigCursorModeChange.emit(_CursorMode(mode))


    def set_charcode(self, charcode):
        #Data is a character code, add to the character table and shift the cursor
        self._chars[self._cursor_pos] = charcode
        self.shift_display(self._entry_shift_mode)
        self.sigDisplayChange.emit()


    def set_pattern(self, line_pattern):
        #Find the character index and line where to write the new pattern
        pos = (self._cursor_pos >> 3) & 7
        line = self._cursor_pos & 7
        #Get the current character pattern and replace the line
        char_pattern = self._custom_patterns[pos]
        pattern_bitmask = (1 << 60) - 1
        point_bitmask = 1 << line
        for _ in range(5):
            if line_pattern & 0x10:
                char_pattern |= point_bitmask
            else:
                char_pattern &= pattern_bitmask - point_bitmask

            line_pattern <<= 1
            point_bitmask <<= 12

        #Store the new pattern
        self._custom_patterns[pos] = char_pattern
        #Shift of the cursor only, so we're only interested in the direction
        shift_mode = _ShiftMode(self._entry_shift_mode & 0x01)
        self.shift_display(shift_mode)
        #Emit a display change signal
        self.sigDisplayChange.emit()


    def execute_write(self, data):
        '''Execute a data write.
        Depending on the previous command, data is either a charcode, or a line pattern.
        '''
        if self._command_pattern:
            self.set_pattern(data & 0x1F)
        else:
            self.set_charcode(data)


    def execute_command(self, command):
        '''Execute a command byte. See the HD44780 datasheet for the details.'''
        #Clear Display: 0x00000001
        if command == 0x01:
            self.clear()

        #Return Home : 0x0000001x
        elif (command & 0xFE) == 0x02:
            self.home()

        #Entry Mode Set : 0x000001xx
        elif (command & 0xFC) == 0x04:
            right_or_left = (command & 0x02)
            disp_or_cursor = (command & 0x01)
            if disp_or_cursor:
                mode = _ShiftMode.ShiftDisplayRight if right_or_left else _ShiftMode.ShiftDisplayLeft
            else:
                mode = _ShiftMode.ShiftCursorRight if right_or_left else _ShiftMode.ShiftCursorLeft

            self.set_entry_shift_mode(mode)

        #Display Mode : 0x00001xxx
        elif (command & 0xF8) == 0x08:
            self.set_display(command & 0x04)
            if command & 0x01:
                self.set_cursor_mode(_CursorMode.Blink)
            elif command & 0x02:
                self.set_cursor_mode(_CursorMode.On)
            else:
                self.set_cursor_mode(_CursorMode.Off)

        #Cursor/Display Shift : 0x0001xxxx
        elif (command & 0xF0) == 0x10:
            right_or_left = (command & 0x02)
            disp_or_cursor = (command & 0x01)
            if disp_or_cursor:
                mode = _ShiftMode.ShiftCursorRight if right_or_left else _ShiftMode.ShiftCursorLeft
            else:
                mode = _ShiftMode.ShiftDisplayRight if right_or_left else _ShiftMode.ShiftDisplayLeft

            self.shift_display(mode)

        #Function Set : 0x001xxxxx
        elif (command & 0xE0) == 0x20:
            self._interface_8bits = bool(command & 0x10)

        #Set CGRAM Address : 0x01xxxxxx
        elif (command & 0xC0) == 0x40:
            self.set_cursor_pos(command & 0x3F)
            self._command_pattern = True

        #Set DDRAM Address : 0x1xxxxxxx
        elif (command & 0x80) == 0x80:
            self.set_cursor_pos(command & 0x7F)
            self._command_pattern = False


class HD44780_Interface4Bits:
    '''Implementation of a 4-bits (6 lines) interface for a HD44780.
    '''

    PIN_D4 = 0
    PIN_D5 = 1
    PIN_D6 = 2
    PIN_D7 = 3
    PIN_EN = 4
    PIN_RS = 5


    def __init__(self, driver, pins):
        '''Initialisation of a LCD display widget.
        \arg driver instance of HD44780_Driver
        \arg pins list of 6 Pin objects, in the following order: [ D4, D5, D6, D7, EN, RS ]
        '''

        self._driver = driver
        self._first_nibble = True
        self._instruction = 0
        self._data = 0

        #Create a hook and connect it to each pin signal and get the initial pin digital state
        self._hook = CallableSignalHook(self._pin_hook_raised)
        self._pin_states = []
        for i, p in enumerate(pins):
            p.set_external_state('Z')
            self._pin_states.append(1 if p.digital_state() else 0)
            p.signal().connect(self._hook, i)


    #Hook called on a pin signal
    def _pin_hook_raised(self, sigdata, pin_index):
        #Filter on digital state changes
        if sigdata.sigid != Wire.SignalId.DigitalChange:
            return

        old_state = self._pin_states[pin_index]
        new_state = 1 if sigdata.data.value() else 0
        self._pin_states[pin_index] = new_state

        #Process the nibble only on a negative edge of the EN pin
        if not (pin_index == self.PIN_EN and old_state and not new_state):
            return

        nibble = self._pin_states[self.PIN_D4] + \
                 self._pin_states[self.PIN_D5] * 2 + \
                 self._pin_states[self.PIN_D6] * 4 + \
                 self._pin_states[self.PIN_D7] * 8

        #Only used at initialisation, until the interface is configured to 4 bits
        if self._driver.interface_8bits():
            self._first_nibble = False
            self._driver.execute_command(nibble << 4)

        elif self._first_nibble:
            #Store the first (lower) nibble. Depending on RS, it's either a data or an instruction
            if self._pin_states[self.PIN_RS]:
                self._data = (self._data & 0x0F) | (nibble << 4)
            else:
                self._instruction = (self._instruction & 0x0F) | (nibble << 4)
        else:
            #Store the second (upper) nibble and execute the consolidated 8-bits command (data or instruction)
            if self._pin_states[self.PIN_RS]:
                self._data = (self._data & 0xF0) | nibble
                self._driver.execute_write(self._data)
            else:
                self._instruction = (self._instruction & 0xF0) | nibble
                self._driver.execute_command(self._instruction)

        self._first_nibble = not self._first_nibble


class _CharEditor(QtWidgets.QWidget):
    '''Simple helper widget to edit a character pattern.
    '''

    def __init__(self):
        super().__init__()

        layout = QtWidgets.QGridLayout(self)
        layout.setHorizontalSpacing(2)
        layout.setVerticalSpacing(2)
        layout.setContentsMargins(4, 4, 4, 4)

        self._bts = []
        self._btgp = QtWidgets.QButtonGroup(self)
        self._btgp.setExclusive(False)
        self._btgp.idToggled.connect(self._dot_toggled)
        for r in range(8):
            for c in range(5):
                bt = QtWidgets.QToolButton()
                layout.addWidget(bt, r, c)
                bt.setCheckable(True)
                bt.setContentsMargins(2, 2, 2, 2)
                bt.setMinimumSize(32, 32)
                self._btgp.addButton(bt, r * 5 + c)

        self._value = [0] * 8

        self._result = QtWidgets.QLineEdit()
        self._result.setMaxLength(22)
        layout.addWidget(self._result, 13, 0, 1, 5)
        self._result.setReadOnly(True)

        self._bt_clear = QtWidgets.QPushButton('Clear')
        layout.addWidget(self._bt_clear, 14, 0, 1, 3)
        self._bt_clear.clicked.connect(self._clear_clicked)

        self._update_value()

    def _update_value(self):
        s = '0x' + ('%02x' * 8) % tuple(self._value)
        self._result.setText(s)

    def _dot_toggled(self, btid):
        c = btid % 5
        r = btid // 5
        mask = (1 << (7 - c))
        state = self._btgp.button(btid).isChecked()
        if state:
            self._value[r] |= mask
        else:
            self._value[r] &= (0xFF - mask)

        self._update_value()

    def _clear_clicked(self):
        self._value = [0] * 8
        self._update_value()
        for bt in self._btgp.buttons():
            bt.setChecked(False)


def main_editor():
    app = QtWidgets.QApplication([])
    w = QtWidgets.QMainWindow()
    editor = _CharEditor()
    w.setCentralWidget(editor)
    w.show()
    app.exec()


def main_test():
    app = QtWidgets.QApplication([])
    w = QtWidgets.QMainWindow()
    lcd_driver = HD44780_Driver(w)
    lcd_display = HD44780_Display(lcd_driver, 16, 2, w)
    w.setCentralWidget(lcd_display)
    w.show()

    lcd_driver.set_display(True)
    for c in 'abcdefghijklmno':
        v = ord(c) & 0x7F
        lcd_driver.set_charcode(v)

    app.exec()


if __name__ == '__main__':
    if sys.argv[1] == 'test':
        main_test()
    elif sys.argv[1] == 'edit':
        main_editor()
