import os
import time
import json

from graph_generator import generate_graph, dump_graph


if __name__ == "__main__":
	os.system("mpic++ edmons_karp_parallel.cpp -o ekp")

	n = 5000

	dump_graph(generate_graph(n, 100), 'graph1.txt')
	dump_graph(generate_graph(n, 100), 'graph2.txt')
	dump_graph(generate_graph(n, 100), 'graph3.txt')

	reports = []
	for proc_num in [2, 4, 8, 16]:
		run_times = []
		for graph_fname in ['graph1.txt', 'graph2.txt', 'graph3.txt']:
			print("running n: {}, np: {}, graph: {}".format(n ,proc_num, graph_fname))

			start = time.time()
			os.system("mqrun ./ekp {} -np {} > /dev/null".format(graph_fname, proc_num))
			run_times.append(time.time() - start)

			print("time: {}".format(run_times[-1]))
		reports.append({"np": proc_num, "n": n, "run_times": run_times})

		print("n: {}, np: {}, min time: {}".format(n, proc_num, min(run_times)))

	print(reports)
	print(json.dumps(reports, indent=2))
	with open('report.json', 'w') as f:
		json.dump(f, reports, indent=2)
