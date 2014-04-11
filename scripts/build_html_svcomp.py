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

def header():
    print '<!DOCTYPE html>\n\
<html lang="en">\n\
  <head>\n\
    <meta charset="utf-8">\n\
    <title>PAGAI - Results</title>\n\
    <meta name="viewport" content="width=device-width, initial-scale=1.0">\n\
    <meta name="description" content="">\n\
    <meta name="author" content="">\n\
    <link href="/home/henry/git/pagai/scripts/bootstrap/css/bootstrap.css" rel="stylesheet">\n\
    </head>\n\
    <body>'

    print '\
<style media=\"screen\" type=\"text/css\">\n\
.ok {\n\
    color: green;\n\
    font-weight: bold\n\
}\n\
.ko {\n\
    color: red;\n\
    font-weight: bold\n\
}\n\
.unknown {\n\
    color: orange;\n\
}\n\
</style>\n\
'

def title():
    print r'<h1>PAGAI Results</h1>'

def footer():
    print r'</body>'

def escape_html(name):
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
    return name
    return escaped_name


def begin_tabular():
    print r'<table class="table table-bordered table-condensed table-hover">'

def end_tabular():
    print r"</tbody>"
    print r"</table>"

def print_line_header():
    print r"<thead>"
    print r"<tr>"
    print r"<td>Benchmark</td>"
    print r"<td>Result</td>"
    print r"<td>Time (in seconds)</td>"
    print r"</tr>"
    print r"</thead>"
    print r"<tbody>"

header()
title()
points = 0
total_time = 0.0

true_results = ""
false_results = ""

for benchmark_name in json_dict:
    if "result" not in json_dict[benchmark_name]:
        continue
    res = json_dict[benchmark_name]["result"]
    expected = json_dict[benchmark_name]["expected"]
    if expected not in "UNKNOWN":
        # SV-comp benchmark
        if res in expected:
            status="ok"
            trclass="success"
            if res in "TRUE":
                points += 2
            if res in "FALSE":
                points += 1
        elif res in "UNKNOWN":
            status="unknown"
            trclass="warning"
        else:
            status="ko"
            trclass="error"
            if res in "TRUE":
                points -= 8
            if res in "FALSE":
                points -= 4
    else:
        if res in "TRUE":
            status="ok"
        if res in "UNKNOWN":
            status="unknown"

    if expected in "TRUE":
        true_results += r'<tr class="' + trclass + '\">'
        true_results += '<td>' + escape_html(benchmark_name) + '</td>'
        true_results += '<td class=\"' + status + '\">' + str(json_dict[benchmark_name]["result"]) + '</td>'
        true_results += '<td>' + str(json_dict[benchmark_name]["time"]) + '</td>'
        true_results += r"</tr>"
    else:
        false_results += r'<tr class="' + trclass + '\">'
        false_results += '<td>' + escape_html(benchmark_name) + '</td>'
        false_results += '<td class=\"' + status + '\">' + str(json_dict[benchmark_name]["result"]) + '</td>'
        false_results += '<td>' + str(json_dict[benchmark_name]["time"]) + '</td>'
        false_results += r"</tr>"
    total_time += float(json_dict[benchmark_name]["time"])

print r"<div>"
print r"<p>TRUE Benchmarks</p>"
begin_tabular()
print_line_header()
print true_results
end_tabular()
print r"</div>"

print r"<div>"

print r"<p>FALSE Benchmarks</p>"
begin_tabular()
print_line_header()
print false_results
end_tabular()
print r"</div>"

print r"<p>"
print "TOTAL POINTS: " + str(points)
print r"</p>"
print r"<p>"
print "TOTAL TIME: " + str(total_time)
print r"</p>"

footer()
