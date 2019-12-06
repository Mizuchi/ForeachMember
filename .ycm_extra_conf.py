def FlagsForFile(filename, **kwargs):
    return {
        'flags': ['-std=c++14', '-Wall', '-Wextra', '-Werror', '-I.'],
        'do_cache': True
    }
