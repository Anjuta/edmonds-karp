import os
import time
import json

from graph_generator import generate_graph, dump_graph


if __name__ == "__main__":
	os.system("mpic++ edmons_karp_parallel.cpp -o ekp")

	proc_num = 16

	n = 5000

	reports = []
	for n in [2000, 4000, 6000, 8000]:
		run_times = []
		for try_num in [1, 2, 3]:
			print("running n: {}, np: {}, try: {}".format(n ,proc_num, try_num))

			start = time.time()
			os.system("mqrun ./ekp {} -np {} > /dev/null".format(graph_fname, proc_num))
			run_times.append(time.time() - start)

			print("time: {}".format(run_times[-1]))
		reports.append({"np": proc_num, "n": n, "run_times": run_times})

		print("n: {}, np: {}, min time: {}".format(n, proc_num, min(run_times)))

	print(reports)
	print(json.dumps(reports, indent=2))
