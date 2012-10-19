#!/usr/bin/python

import sys
from os import system, remove

def getFileBetween(filename,begin,end):
	f = open(filename,"r")
	res = ""
	start = False
	for line in f:
		if end in line :
			return res
		if start :
			res += line
		if begin in line :
			start = True
	f.close()

def get_techniques(filename):
	res = set()
	string = getFileBetween(filename,"TIME:","TIME_END")
	for lines in string.rstrip().split('\n') :
		elements = lines.split(' ', 2 )
		res.add(elements[2])
	return res


def to_technique(string):
	res = ""
	ok = False
	for c in string:
		if c != '\t' and c != ' ' and c != '/' :
			ok = True
		if ok :
			res += c
	res = res.replace("PATH FOCUSING","PF")
	res = res.replace("GUIDED","G")
	res = res.replace("CLASSIC","S")
	res = res.replace("COMBINED","C")
	res = res.replace("DISJUNCTIVE","DIS")
	return res

def process_time(filename,time_s_array, time_ms_array):
	string = getFileBetween(filename,"TIME:","TIME_END")
	if not string :
		return
	for lines in string.rstrip().split('\n') :
		elements = lines.split(' ', 2 )
		t = to_technique(elements[2])
		if t in time_s_array:
			time_s_array[t] += int(elements[0])
		else:
			time_s_array[t] = int(elements[0])
		if t in time_ms_array:
			time_ms_array[t] += int(elements[1])
		else:
			time_ms_array[t] = int(elements[1])
		if time_ms_array[t] >= 1000000 :
			time_ms_array[t] -= 1000000
			time_s_array[t] += 1

def process_warnings(filename,warnings_array):
	string = getFileBetween(filename,"WARNINGS:","WARNINGS_END")
	if not string :
		return
	for lines in string.rstrip().split('\n') :
		elements = lines.split(' ', 1 )
		t = to_technique(elements[1])
		if t in warnings_array:
			warnings_array[t] += int(elements[0])
		else:
			warnings_array[t] = int(elements[0])

def process_count_functions(filename):
	string = getFileBetween(filename,"FUNCTIONS:","FUNCTIONS_END")
	if not string :
		return 0
	n = 0
	for lines in string.rstrip().split('\n') :
		n += int(lines)
	return n

def process_count_functions_skipped(filename):
	string = getFileBetween(filename,"IGNORED:","IGNORED_END")
	if not string :
		return 0
	n = 0
	for lines in string.rstrip().split('\n') :
		n += int(lines)
	return n

def process_matrix(filename,matrix):
	string = getFileBetween(filename,"MATRIX:","MATRIX_END")
	if not string :
		return 
	for lines in string.rstrip().split('\n') :
		elements = lines.split(' ', 4 )
		t = to_technique(elements[4])
		if t in matrix :
			matrix[t][0] += int(elements[0])
			matrix[t][1] += int(elements[1])
			matrix[t][2] += int(elements[2])
			matrix[t][3] += int(elements[3])
		else :
			matrix[t] = dict()
			matrix[t][0] = int(elements[0])
			matrix[t][1] = int(elements[1])
			matrix[t][2] = int(elements[2])
			matrix[t][3] = int(elements[3])

def compute_bb_number(matrix):
	n = 0
	for t in matrix :
		for val in matrix[t] :
			n += matrix[t][val]
		return n

def compute_matrix_average(matrix):
	matrix_average = dict()
	n_blocks = compute_bb_number(matrix)
	#compute the averages
	for t in matrix :
		if t not in matrix_average :
			matrix_average[t] = dict()
		for val in matrix[t] :
			matrix_average[t][val] = float(matrix[t][val]) * 100. / float(n_blocks) 
	return matrix_average

def generate_gnuplot_from_matrix(matrix,root_dir,bench):
	f = open('/tmp/data_gnuplot', 'w')
	for t in matrix :
		f.write('"'+t+'" ')
		for val in matrix[t] :
			f.write(str(matrix[t][val])+' ')
		f.write("\n")
	f.close()
	system('gnuplot '+root_dir+'/techniques.gnuplot')

def generate_gnuplot_from_time_array(time_s_array,time_ms_array,root_dir,bench):
	f = open('/tmp/data_time_gnuplot', 'w')
	for t in time_s_array :
		ms = str(time_ms_array[t])
		while len(ms) != 6 :
			ms = "0"+ms
		f.write('"'+t+'" '+str(time_s_array[t])+"."+ms+"\n") 
	f.close()
	system('gnuplot '+root_dir+'/techniques_time.gnuplot')

def process_input_files():
	time_s_array = dict()
	time_ms_array = dict()
	warnings_array = dict()
	matrix = dict()
	n_func = 0
	n_skipped = 0
	root_dir = sys.argv[1]
	bench = sys.argv[2]
	for filename in sys.argv[3:]: 
		process_time(filename,time_s_array,time_ms_array)
		process_warnings(filename,warnings_array)
		n_func += process_count_functions(filename)
		n_skipped += process_count_functions_skipped(filename)
		process_matrix(filename,matrix)

	matrix_average = compute_matrix_average(matrix)
	#print time_s_array
	#print time_ms_array
	#print warnings_array
	#print matrix
	#print n_func
	#print n_skipped
	#print matrix_average
	generate_gnuplot_from_matrix(matrix_average,root_dir,bench)
	generate_gnuplot_from_time_array(time_s_array,time_ms_array,root_dir,bench)

process_input_files()

