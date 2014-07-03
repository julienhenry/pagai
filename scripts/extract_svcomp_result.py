#!/usr/bin/python

import sys
from xml.dom import minidom

input = sys.argv[1]

def get_expected_status(filename):
    if "true" in filename:
        return "true"
    else:
        return "false"

def sum_time(l):
    sum_l = []
    current = 0
    for elt in l:
        current = current+float(elt)
        sum_l.append(current)
    return sum_l

li = []
xmldoc = minidom.parse(input)
filelist = xmldoc.getElementsByTagName('sourcefile')
for s in filelist :
    eltlist = s.getElementsByTagName('column')
    filename = s.attributes["name"].value
    expected_status = get_expected_status(filename)
    for e in eltlist :
        title = e.attributes["title"].value
        value = e.attributes["value"].value
        if "status" in title:
            status = value
        if "cputime" in title:
            cputime = value
    if "true" in expected_status and "true" in status:
        li.append(float(cputime))


li.sort()
points = 0
#for elt in sum_time(li):
#    points = points + 2
#    print str(points)+" "+str(elt)

for elt in li:
    points = points + 2
    print str(points)+" "+str(elt)
