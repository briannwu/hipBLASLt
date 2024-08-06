#! /usr/bin/python3

import sys

def Remove(line):

    line = ''.join(line.split('nlohmann::json_abi_v3_11_2::'))
    line = ''.join(line.split('detail::'))
    line = ''.join(line.split('std::'))

    while True:#len(line) > 120:
        part1 = line.split('<')
        if len(part1) < 2:
            break
        replace_part = '<'+part1[-1].split('>')[0]+'>'
        part2 = line.split(replace_part)

        if len(part2) < 2:
            replace_part = '<'+part1[-1].split('…')[0]+'…'
            part2 = line.split(replace_part)
            if len(part2) < 2:
                break
            part2[-2] += '…'

        if '[' in replace_part or ']' in replace_part:
            if len(replace_part.split(']')) != len(replace_part.split('[')):
                break

        line = ''.join(part2)

    strplit = line.split('__cxx11::basic_string const&')
    for s in strplit[1:]:
        s += 'cstring&'
    line = ''.join(strplit)

    strplit = line.split('__cxx11::basic_string&')
    for s in strplit[1:]:
        s += 'string&'
    line = ''.join(strplit)
    line = ''.join(line.split('back_insert_iterator'))
    line = ''.join(line.split('decltype ((to_json({parm#1}, (forward)({parm#2}))),((void)()))'))

    return line

'''
def Remove(line):
    line = ''.join(line.split('nlohmann::json_abi_v3_11_2::'))
    line = ''.join(line.split('detail::'))

    while True:#len(line) > 120:
        part1 = line.split('<')
        if len(part1) < 2:
            break
        replace_part = '<'+part1[-1].split('>')[0]+'>'
        part2 = line.split(replace_part)
        if len(part2) < 2:
            break
        if '[' in replace_part or ']' in replace_part:
            if len(replace_part.split(']')) != len(replace_part.split('[')):
                break
        line = ''.join(part2)
    return line'''

lines = []
for line in sys.stdin:
    #lines += [Remove(l) for l in line.split('\n')]
    lines += [Remove(line)]

lines = ''.join(lines)
print(Remove(lines))
#[print(l) for l in lines]
