"""mrr log merger tool
   usage: $ python3 logmerger.py ${filename}
   author: Hwan hee, Lee
   email: hwanhee.lee@navercorp.com
"""

import sys

try:
    filename = sys.argv[1]
    f = open(filename, 'r')

    result = {}
    gaplist = list(range(0, 820, 20))
    for gap in gaplist:
        result[gap] = 0

    lines = f.readlines()
    for line in lines:
        if line.endswith(']\n'):
            res_key = int(line.split(' ')[0].replace(':', ''))
            res_value = int(line.split(' ')[1].split('[')[0])
            for gap in gaplist:
                if res_key == gap:
                    result[gap] += res_value

    print('>>>>> Result of merging %s(unit: ms)' % (filename))
    for k, v in sorted(result.items()):
        print('%s:\t%s' % (k, v))

    f.close()
except IndexError:
    print('usage: $ python3 logmerger.py ${filename}')