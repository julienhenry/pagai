#!/usr/bin/python

import sys
import json
import os
import re


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

def remove_color(str):
    ansi_escape = re.compile(r'\x1b[^m]*m')
    return ansi_escape.sub('', str)

def sum_time(l):
    sum_l = []
    current = 0
    for elt in l:
        current = current+float(elt)
        sum_l.append(current)
    return sum_l



li = []
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

    res_pk = json_dict[benchmark_name]["pk"]["result"]
    res_box = json_dict[benchmark_name]["box"]["result"]
    time_pk = float(remove_color(json_dict[benchmark_name]["pk"]["time"]))
    time_box = float(remove_color(json_dict[benchmark_name]["box"]["time"]))

    if "TRUE" not in res_pk and "TRUE" not in res_box:
        # not proved, 0 points
        continue

    if "TRUE" in res_pk:
        min_time = time_pk
    else:
        min_time = time_box
    if "TRUE" in res_box:
        min_time = min(min_time,time_box)
    li.append(min_time)


li.sort()
points = 0
for elt in sum_time(li):
    points = points + 2
    print str(points)+" "+str(elt)

