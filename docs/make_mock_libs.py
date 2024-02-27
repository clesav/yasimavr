import os, sys

sys.path.append(os.path.abspath('../py'))

lib_path = '../py/yasimavr/lib'


core_mock_classes = [
    'AbstractSimLoop',
    'AsyncSimLoop',
    'Device',
    'DeviceDebugProbe',
    'Signal',
    'SignalHook',
]


arch_avr_mock_classes = [
    'ArchAVR_Device'
]


arch_xt_mock_classes = [
    'ArchXT_Device'
]


mock_base_module = '''
class _MockObject(type):
    def __getattr__(self, k):
        class attr(metaclass=_MockObject): pass
        return attr

'''


def _make_mock_module(mod_name, class_list):
    
    try:
        from yasimavr.lib import _mock_base
    except ImportError:
        open(os.path.join(lib_path, '_mock_base.py'), 'w').write(mock_base_module)
        print('Mock base module created')
    else:
        del _mock_base

    module_content = 'from ._mock_base import _MockObject\n'
    for cls in class_list:
        module_content += 'class %s(metaclass=_MockObject): pass\n' % cls
    open(os.path.join(lib_path, mod_name + '.py'), 'w').write(module_content)


try:
    from yasimavr.lib import core
    print('Core could be imported, no mock module')
except ImportError:
    _make_mock_module('core', core_mock_classes)
    print('Mock core module created')


try:
    from yasimavr.lib import arch_avr
    print('arch_avr could be imported, no mock module')
except ImportError:
    _make_mock_module('arch_avr', arch_avr_mock_classes)
    print('Mock arch_avr module created')


try:
    from yasimavr.lib import arch_xt
    print('arch_xt could be imported, no mock module')
except ImportError:
    _make_mock_module('arch_xt', arch_xt_mock_classes)
    print('Mock arch_xt module created')
