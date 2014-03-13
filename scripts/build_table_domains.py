#!/usr/bin/python

import sys
import json
import os

input = sys.argv[2]
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
        #index = escaped_name.find('_')
        index = 15
        escaped_name = escaped_name[0:index]
    return escaped_name


def begin_tabular():
    print r"\begin{tabular}{|l|rrrr|rrrr|rrrr|rrrr|}\hline"
def end_tabular():
    print r"\hline"
    print r"\end{tabular}"

def print_line_header():
    print r"\multirow{2}{*}{Benchmark}"\
    + ' & ' +r"\multicolumn{3}{c|}{PK // OCT}"\
    + ' & ' +r"\multicolumn{3}{c|}{PK // BOX}"\
    + ' & ' +r"\multicolumn{3}{c|}{OCT // BOX}"\
    + ' & ' +r"\multicolumn{3}{c|}{PKGRID // PK}"\
    + r"\\ \cline{2-13}"
    print \
      ' & ' + r"\multicolumn{1}{c|}{$\sqsubset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsupset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$=$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\neq$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsubset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsupset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$=$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\neq$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsubset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsupset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$=$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\neq$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsubset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\sqsupset$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$=$}"\
    + ' & ' + r"\multicolumn{1}{c|}{$\neq$}"\
    + r"\\ \hline"

begin_tabular()
#print_line_header()

t = sys.argv[1] # technique

for benchmark_name in json_dict:
    print escape_latex(benchmark_name) \
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PK // OCT"]["lt"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PK // OCT"]["gt"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PK // OCT"]["eq"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PK // OCT"]["un"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PK // BOX"]["lt"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PK // BOX"]["gt"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PK // BOX"]["eq"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PK // BOX"]["un"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["OCT // BOX"]["lt"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["OCT // BOX"]["gt"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["OCT // BOX"]["eq"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["OCT // BOX"]["un"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PKGRID // PK"]["lt"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PKGRID // PK"]["gt"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PKGRID // PK"]["eq"])\
    + ' & ' + str(json_dict[benchmark_name]["domain"][t]["comparison"]["PKGRID // PK"]["un"])\
    + r"\\"
end_tabular()
