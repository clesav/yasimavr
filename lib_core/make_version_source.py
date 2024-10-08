
def extract_version(version_file):
    str_ver = open(version_file).readline()
    str_ver = str_ver.strip('\t\r\n')

    v = str_ver.encode('ascii').split(b'.')
    v = [int(i) for i in v]
    int_ver = (v[0] << 24) | (v[1] << 16) | (v[2] << 8)

    return str_ver, int_ver


src_template = '''//File generated automatically, do not edit manually.
#ifndef __YASIMAVR_VERSION_H__
#define __YASIMAVR_VERSION_H__
#define YASIMAVR_VERSION {0:#x}
#define YASIMAVR_VERSION_STR {1}
#endif
'''


def generate_version_source(version_file, path_src):
    str_ver, int_ver = extract_version(version_file)

    src = src_template.format(int_ver, str_ver)

    try:
        old_src = open(path_src).read()
    except FileNotFoundError:
        old_src = ''

    if src != old_src:
        open(path_src, 'w').write(src)


if __name__ == '__main__':
    import sys
    generate_version_source(sys.argv[1], sys.argv[2])
