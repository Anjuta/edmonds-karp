import sys
import random


def get_non_zero_cost(max_cost):
	return random.randint(1, max_cost)


def generate_graph(n, max_cost, zero_probability=0.3):
	def get_cost():
		if random.random() >= zero_probability:
			return get_non_zero_cost(max_cost)
		return 0

	graph = [[get_cost() for _ in range(n)]
			 for _ in range(n)]
	
	fix_graph_errors(graph, max_cost)
	return graph

def fix_graph_errors(graph, max_cost):
	n = len(graph)
	for i in range(n):
		graph[i][0] = 0
		graph[n-1][i] = 0
		graph[i][i] = 0

	for i in range(n):
		for j in range(n):
			if graph[i][j] and graph[j][i]:
				if random.random() >= 0.5:
					graph[i][j] = 0
				else:
					graph[j][i] = 0

	for i in range(n - 1):
		candidates = list(range(1, n))
		if all(graph[i][j] == 0 for j in candidates):
			random.shuffle(candidates)
			j = next(j for j in candidates if graph[j][i] == 0 and i != j)
			graph[i][j] = get_non_zero_cost(max_cost)

	for j in range(1, n):
		candidates = list(range(n - 1))
		if all(graph[i][j] == 0 for i in candidates):
			random.shuffle(candidates)
			i = next(i for i in candidates if graph[j][i] == 0 and i != j)
			graph[i][j] = get_non_zero_cost(max_cost)


def dump_graph(graph, file_name):
	with open(file_name, 'w') as f:
		f.write('{}\n'.format(len(graph)))
		for r in graph:
			f.write('{}\n'.format(' '.join(map(str, r))))


if __name__ == "__main__":
	n = int(sys.argv[1])
	g = generate_graph(n, 100)
	dump_graph(g, 'graph.txt'.format(n))
