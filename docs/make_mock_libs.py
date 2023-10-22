import os, sys

sys.path.append(os.path.abspath('../py'))

core_mock_module = '''
class _MockObject(type):
    def __getattr__(self, k):
        class attr(metaclass=_MockObject): pass
        return attr

class AbstractSimLoop(metaclass=_MockObject): pass
class AsyncSimLoop(metaclass=_MockObject): pass
class Device(metaclass=_MockObject): pass
class DeviceDebugProbe(metaclass=_MockObject): pass
class Pin(metaclass=_MockObject): pass
class Signal(metaclass=_MockObject): pass
class SignalHook(metaclass=_MockObject): pass
'''

try:
    from yasimavr.lib import core
    print('Core could be imported, no mock module')
except ImportError:
    open('../py/yasimavr/lib/core.py', 'w').write(core_mock_module)
    print('Mock core module created')
