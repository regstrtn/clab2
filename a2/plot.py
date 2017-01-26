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
	
	plt.plot(clients.values())
	plt.xticks(range(1, 6), clients.keys())
	
	plt.show()

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
		msg.append(clients[key].values())
	
	print(msg)
	
	msg = np.array(msg)
	heatmap = plt.pcolor(msg, cmap=matplotlib.cm.hot)
	plt.colorbar()
	plt.show()
	
	plt.colormesh(msg)
	plt.show()


def main():
	#readfile()
	heatmap()

if __name__ == '__main__':
	main()