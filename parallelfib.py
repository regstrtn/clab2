from multiprocessing import Pool

def fib(x):
	if(x==1): return 1
	if(x==0): return 0
	return fib(x-1) + fib(x-2)

if __name__ == '__main__':
    p = Pool(5)
    print(p.map(fib, [60, 60, 60, 60, 60]))

