#!/usr/bin/env python

import sys

if __name__ == '__main__':
    template = '''
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
'''

    input = sys.argv[1].removeprefix('../include/')
    output = sys.argv[2]

    template += f'\n#include "{input}"'

    with open(output, 'w') as f:
        f.write(template)
        print(f'add test for {input}')

