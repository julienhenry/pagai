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

def max(a,b):
    if a > b:
        return a
    return b

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
    print r"<td>Result (pk)</td>"
    print r"<td>Time (in seconds)</td>"
    print r"<td>Result (box)</td>"
    print r"<td>Time (in seconds)</td>"
    print r"<td>Result (pk-O3)</td>"
    print r"<td>Time (in seconds)</td>"
    print r"<td>Result (box-O3)</td>"
    print r"<td>Time (in seconds)</td>"
    print r"</tr>"
    print r"</thead>"
    print r"<tbody>"

header()
title()
points = 0
total_time = dict()
total_time["box"] = 0.0
total_time["pk"] = 0.0

true_results = ""
false_results = ""

def get_status(res,expected):
    points = 0
    if expected not in "UNKNOWN":
        # SV-comp benchmark
        if res in expected:
            status="ok"
            if res in "TRUE":
                points = 2
            if res in "FALSE":
                points = 1
        elif res in "UNKNOWN":
            status="unknown"
        else:
            status="ko"
            if res in "TRUE":
                points = -8
            if res in "FALSE":
                points = -4
    else:
        if res in "TRUE":
            status="ok"
        if res in "UNKNOWN":
            status="unknown"
    return status, points

def get_trclass(statusbox,statuspk,statusbox_opt,statuspk_opt):
    if "ko" in statuspk or "ko" in statusbox or "ko" in statuspk_opt or "ko" in statusbox_opt:
        return "error"
    if "ok" in statuspk or "ok" in statusbox or "ok" in statuspk_opt or "ok" in statusbox_opt:
        return "success"
    return "warning"

for benchmark_name in json_dict:
    try:
        if "result" not in json_dict[benchmark_name]["box"]:
            continue
        expected = json_dict[benchmark_name]["box"]["expected"]

        statusbox,pointsbox = get_status(json_dict[benchmark_name]["box"]["result"], expected)
        statuspk,pointspk = get_status(json_dict[benchmark_name]["pk"]["result"], expected)
        statusbox_opt,pointsbox_opt = get_status(json_dict[benchmark_name]["boxO3"]["result"], expected)
        statuspk_opt,pointspk_opt = get_status(json_dict[benchmark_name]["pkO3"]["result"], expected)
        trclass = get_trclass(statusbox,statuspk,statusbox_opt,statuspk_opt)

        points += max(pointsbox,pointspk)

        if expected in "TRUE":
            true_results += r'<tr class="' + trclass + '\">'
            true_results += '<td>' + escape_html(benchmark_name) + '</td>'
            true_results += '<td class=\"' + statuspk + '\">' + str(json_dict[benchmark_name]["pk"]["result"]) + '</td>'
            true_results += '<td>' + str(json_dict[benchmark_name]["pk"]["time"]) + '</td>'
            true_results += '<td class=\"' + statusbox + '\">' + str(json_dict[benchmark_name]["box"]["result"]) + '</td>'
            true_results += '<td>' + str(json_dict[benchmark_name]["box"]["time"]) + '</td>'
            true_results += '<td class=\"' + statuspk + '\">' + str(json_dict[benchmark_name]["pkO3"]["result"]) + '</td>'
            true_results += '<td>' + str(json_dict[benchmark_name]["pkO3"]["time"]) + '</td>'
            true_results += '<td class=\"' + statusbox + '\">' + str(json_dict[benchmark_name]["boxO3"]["result"]) + '</td>'
            true_results += '<td>' + str(json_dict[benchmark_name]["boxO3"]["time"]) + '</td>'
            true_results += r"</tr>"
        else:
            false_results += r'<tr class="' + trclass + '\">'
            false_results += '<td>' + escape_html(benchmark_name) + '</td>'
            false_results += '<td class=\"' + statuspk + '\">' + str(json_dict[benchmark_name]["pk"]["result"]) + '</td>'
            false_results += '<td>' + str(json_dict[benchmark_name]["pk"]["time"]) + '</td>'
            false_results += '<td class=\"' + statusbox + '\">' + str(json_dict[benchmark_name]["box"]["result"]) + '</td>'
            false_results += '<td>' + str(json_dict[benchmark_name]["box"]["time"]) + '</td>'
            true_results += '<td class=\"' + statuspk + '\">' + str(json_dict[benchmark_name]["pkO3"]["result"]) + '</td>'
            true_results += '<td>' + str(json_dict[benchmark_name]["pkO3"]["time"]) + '</td>'
            true_results += '<td class=\"' + statusbox + '\">' + str(json_dict[benchmark_name]["boxO3"]["result"]) + '</td>'
            true_results += '<td>' + str(json_dict[benchmark_name]["boxO3"]["time"]) + '</td>'
            false_results += r"</tr>"
        #total_time["box"] += float(json_dict[benchmark_name]["box"]["time"])
        #total_time["pk"] += float(json_dict[benchmark_name]["pk"]["time"])
    except:
        continue

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
print "TOTAL TIME (box): " + str(total_time["box"])
print r"</p>"
print r"<p>"
print "TOTAL TIME (pk): " + str(total_time["pk"])
print r"</p>"

footer()
