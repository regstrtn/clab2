from __future__ import print_function
import matplotlib.pyplot as plt
from datetime import datetime
from collections import defaultdict
import numpy as np
import matplotlib
	

def readfile():
	f = open("log", "r")
	clients = {}
	times = []
	for line in f:
		s = line.split('|')
		if(s[0] in clients):
			clients[s[0]] += len(s[3])
		else:
			clients[s[0]] = len(s[3])
		print(datetime.fromtimestamp(
	        int(s[2])
	    ).strftime('%d-%m %H:%M:%S'))

	for key in clients:
		print(key, clients[key])
	
	#objects = ('Python', 'C++', 'Java', 'Perl', 'Scala', 'Lisp')
	objects = clients.keys()
	y_pos = np.arange(len(objects))
	performance = clients.values()
	plt.bar(y_pos, performance, align='center', alpha=0.5)
	plt.xticks(y_pos, objects)
	plt.ylabel('Bytes sent')
	plt.title('Server load graph')
	plt.savefig("barplot.png")
	plt.close()
	#plt.show()
	
	

def heatmap():
	f = open("log", "r")
	clients = defaultdict(dict)
	times = []
	for line in f:
		s = line.split('|')
		clients[s[0]][s[1]] = 0
		clients[s[1]][s[0]] = 0
		
	f.seek(0,0)
	for line in f:
		s = line.split('|')
		clients[s[0]][s[1]] += len(s[3])
		#print(datetime.fromtimestamp(
	     #   int(s[2])
	    #).strftime('%d-%m %H:%M:%S'))

	msg = []
	for key in clients.keys():
		clients[key][key] = 0
	for key in clients.keys():
		msg.append(clients[key].values())
	msg = np.array(msg)
	np.savetxt('foo.csv', msg, delimiter = ",", fmt = '%6.0f')
	heatmap = plt.pcolor(msg, cmap=matplotlib.cm.Blues)
	xlab = clients.keys()
	y_pos = np.arange(len(xlab))
	plt.xticks(y_pos, xlab)
	plt.yticks(y_pos, xlab)
	#plt.xticks(clients.keys())
	#plt.yticks(clients.keys())
	plt.title('Clientwise message heatmap')
	plt.colorbar()
	#plt.show()
	plt.savefig("heatmap.png")


def main():
	readfile()
	heatmap()

if __name__ == '__main__':
	main()