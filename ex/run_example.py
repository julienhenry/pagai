#!/usr/bin/python

#get library (modules)
#import TAP
import os
from multiprocessing import Process, Lock, Queue, Value

N = 2
bindir = "./bin/"
resdir = "./results/"
srcdir = "../src/"
timeout = 1

def run_bc(bitcode):
	cmd = srcdir + "pagai"
	args = ["-i",bindir+bitcode, "-c", "-o", resdir+bitcode+".res"]
	os.execv(cmd, [cmd] + args[0:])

def run_benchs(l,q,I):
	while 1:
		l.acquire()
		if q.empty():
			l.release()
			print "# Process terminated"
			return 0
		else:
			bitcode = q.get()
    		l.release()
		p = Process(target=run_bc, args=([bitcode]))
		p.start()
		p.join(timeout)
		if p.is_alive():
			p.terminate()
			print "not ok " + str(I.value) + " - file " + bitcode + " has been KILLED"
			#ok(0,"file" + bitcode + " has been KILLED")
		else:
			print "ok " + str(I.value) + " - file " + bitcode
			#ok(1," file " + bitcode)
		I.value = I.value+1

def run():
	# we search for .bc files
	files=os.listdir(bindir)
	files=[filename for filename in files if filename.endswith(".bc")]

	# lock for the queue of filenames
	lock = Lock()
	lock2 = Lock()

	#tap = TAP.Builder.create(len(files))
	#ok = tap.ok
	I = Value('i',1, lock=lock2)
	print "1.." + str(len(files))

	# queue storing the bitcode files that still need to be processed
	q = Queue()
	for f in files:
		q.put(f)
	P = []
	for num in range(N):
		P.append(Process(target=run_benchs, args=(lock,q,I)))
		P[num].start()
		print "PID = " + str(P[num].pid)
	
	for num in range(N):
		#while P[num].is_alive():
		while P[num].exitcode == None:
			#print "wait for " + str(P[num].pid) + " to terminate"
			P[num].join(timeout)
		#print "Process " + str(P[num].pid) + " has terminate"


# if run directly, we launch run()
if __name__ == '__main__':
	run()
	print "EXIT"
