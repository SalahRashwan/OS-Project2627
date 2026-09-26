#!/usr/bin/env python3
# @File: style_check.py
# @Purpose: Test-only static checker for the course style rules that can
#           be checked mechanically in src/*.c, include/*.h, and
#           tests/*.c: function definition headers (@Name/@Def/@Arg/@Ret
#           with an "argument = meaning" entry per parameter), the
#           45-line function limit, system includes only in headers,
#           b-prefixed integers, constant-first comparisons, include
#           guards, and prohibited stdio/stat/system calls.
#           Usage: python3 tests/style_check.py
#           Exit status 0 = no finding, 1 = at least one finding.
#           It cannot judge naming quality or comment usefulness; those
#           still need human review.
# @Author: Salah Ahmed Salaheldin Adly Rashwan
# @Date: 2026-09-26
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
FORBIDDEN = re.compile(
    r'\b(printf|fprintf|dprintf|scanf|fscanf|sscanf|gets|puts|fputs|putchar|'
    r'getchar|fgets|fopen|fclose|fread|fwrite|getline|perror|system|popen|'
    r'stat|fstat|lstat|fstatat|statx)\s*\(')
DEF_RE = re.compile(
    r'^(?:static\s+)?[A-Za-z_][\w \t\*]*?\b(\w+)\s*\(([^;{}]*?)\)\s*\{',
    re.M | re.S)
HEADER_RE = re.compile(r'/\*{47}\n((?: \*.*\n)+?) \*{47}/\s*$')
CONST_RIGHT = re.compile(
    r'\b[a-z]\w*(?:->\w+|\.\w+|\[\w+\])*\s*(?:<=|>=|==|!=|<|>)\s*'
    r'(?:-?\d+|[A-Z][A-Z0-9_]+|\'[^\']+\')\s*(?:\)|&&|\|\||;)')
STRING_RE = re.compile(r'"(?:\\.|[^"\\])*"')
findings = []


def report(path, line, msg):
    findings.append(f'{path.relative_to(ROOT)}:{line}: {msg}')


def param_names(params):
    names = []
    for part in params.split(','):
        part = part.strip()
        if part in ('', 'void', '...'):
            continue
        found = re.findall(r'(\w+)\s*(?:\[[^\]]*\])?\s*$', part)
        if found:
            names.append(found[0])
    return names


def function_end(lines, start):
    depth = 0
    seen = False
    for i in range(start - 1, len(lines)):
        code = STRING_RE.sub('""', lines[i])
        depth += code.count('{') - code.count('}')
        seen = seen or '{' in code
        if seen and depth == 0:
            return i + 1
    return start


def check_definition(path, text, lines, match):
    name = match.group(1)
    if name in ('if', 'for', 'while', 'switch', 'return'):
        return
    start = text.count('\n', 0, match.start()) + 1
    above = '\n'.join(lines[max(0, start - 60):start - 1])
    header = HEADER_RE.search(above)
    if not header:
        report(path, start, f'{name}: missing definition header')
    else:
        block = header.group(1)
        for tag in ('@Name: ' + name, '@Def:', '@Arg:', '@Ret:'):
            if tag not in block:
                report(path, start, f'{name}: header lacks "{tag}"')
        arg = re.search(r'@Arg:(.*?)@Ret:', block, re.S)
        for pname in param_names(match.group(2)):
            pattern = r'\b' + pname + r'\b(?:\[\d\])?\s*='
            if arg and not re.search(pattern, arg.group(1)):
                report(path, start, f'{name}: @Arg lacks "{pname} = meaning"')
    end = function_end(lines, start)
    if end - start + 1 > 45:
        report(path, start, f'{name}: {end - start + 1} lines (limit 45)')


def check_lines(path, lines):
    in_comment = False
    for number, line in enumerate(lines, 1):
        stripped = line.strip()
        if in_comment:
            in_comment = '*/' not in stripped
            continue
        if stripped.startswith('/*'):
            in_comment = '*/' not in stripped
            continue
        code = STRING_RE.sub('""', line)
        if FORBIDDEN.search(code):
            report(path, number, 'prohibited I/O/stat/system call')
        if re.search(r'\bint\s+\*?b[A-Z]\w*', code):
            report(path, number, 'b-prefixed integer (course prefix is n)')
        if CONST_RIGHT.search(code):
            report(path, number, 'constant on the right of a comparison')


def check_source(path):
    text = path.read_text()
    lines = text.split('\n')
    if re.search(r'^#include\s*<', text, re.M):
        report(path, 1, 'system include in a .c file (belongs in its header)')
    if not re.match(r'/\*\n \* @File: ' + re.escape(path.name), text):
        report(path, 1, 'missing @File header')
    for tag in ('@Purpose:', '@Author:', '@Date:'):
        if tag not in text[:1500]:
            report(path, 1, f'file header lacks {tag}')
    for match in DEF_RE.finditer(text):
        check_definition(path, text, lines, match)
    check_lines(path, lines)


def main():
    for path in sorted((ROOT / 'src').glob('*.c')) + sorted((ROOT / 'tests').glob('*.c')):
        check_source(path)
    for path in sorted((ROOT / 'include').glob('*.h')):
        text = path.read_text()
        if not re.match(r'#ifndef (\w+)\n#define \1\n', text):
            report(path, 1, 'missing include guard at top of header')
        check_lines(path, text.split('\n'))
    for finding in findings:
        print(finding)
    print(f'style_check: {len(findings)} finding(s)')
    return 1 if findings else 0


if __name__ == '__main__':
    sys.exit(main())
