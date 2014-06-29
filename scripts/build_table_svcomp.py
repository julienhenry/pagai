#!/usr/bin/python

import sys
import json
import os

input = sys.argv[1]
if os.path.isfile(input):
    json_data=open(input)
    json_dict = json.load(json_data)
    json_data.close()
else:
    json_dict = dict()

def escape_latex(name):
    CHARS = {
        '&':  r'\&',
        '%':  r'\%',
        '$':  r'\$',
        '#':  r'\#',
        '_':  r'\_',
        '{':  r'\}',
        '}':  r'\}',
        '~':  r'\~',
        '^':  r'\^',
        '\\': r'\backslash',
    }
    escaped_name="".join([CHARS.get(char, char) for char in name])
    if len(escaped_name) > 15:
        index = escaped_name.find('\_true')
        #index = 25
        escaped_name = escaped_name[0:index]
    return escaped_name


def begin_tabular():
    print r"\begin{tabular}{|l|r|S|r|S|}\hline"
def end_tabular():
    print r"\hline"
    print r"\end{tabular}"

def print_line_header():
    print \
    "Benchmark & result & time & result & time"\
    + r"\\ \hline"


begin_tabular()
print_line_header()
for benchmark_name in json_dict:
    if "box" not in json_dict[benchmark_name]:
        continue
    if "pk" not in json_dict[benchmark_name]:
        continue
    if "result" not in json_dict[benchmark_name]["box"]:
        continue
    if "result" not in json_dict[benchmark_name]["pk"]:
        continue
    if "FALSE" in json_dict[benchmark_name]["box"]["expected"]:
        continue
    print escape_latex(benchmark_name) \
    + ' & ' + str(json_dict[benchmark_name]["pk"]["result"])\
    + ' & ' + str(json_dict[benchmark_name]["pk"]["time"])\
    + ' & ' + str(json_dict[benchmark_name]["box"]["result"])\
    + ' & ' + str(json_dict[benchmark_name]["box"]["time"])\
    + r"\\"
end_tabular()
