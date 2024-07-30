import sys
import queue

import networkit as nk
import networkx as nx
import numpy as np

G_nx = nx.DiGraph(nx.drawing.nx_agraph.read_dot(str(sys.argv[1])))
G_nk = nk.Graph(G_nx.number_of_nodes(), directed=True)
G_nx_sparse = nx.DiGraph()

for u,v in G_nx.edges():
	G_nk.addEdge(int(u[1:]), int(v[1:]))

label_nx = nx.get_node_attributes(G_nx, 'label')
important_nk = [0] * G_nk.numberOfNodes()


for key,value in label_nx.items():
	important_nk[int(key[1:])] = 1

qs = list()
#q = queue.Queue()
label = [set() for i in range(G_nk.numberOfNodes())]

for n in G_nk.iterNodes():
	if G_nk.degreeIn(n) == 0:
		qs.append(queue.Queue())
		qs[-1].put(n)

# for n in G_nk.iterNodes():
# 	if G_nk.degreeIn(n) == 0:
# 		q.put(n)

# while not q.empty():
# 	current = q.get()
# 	for v in G_nk.iterNeighbors(current):
# 		q.put(v)
# 		if important_nk[current] > 0:
# 			label[v].add(current)
# 		else:
# 			label[v].update(label[current])

for q in qs:
	explored = [False] * G_nk.numberOfNodes()
	while not q.empty():
		current = q.get()
		for v in G_nk.iterNeighbors(current):
			if not explored[v]:
				q.put(v)
				explored[v] = True
			if important_nk[current] > 0:
				label[v].add(current)
			else:
				label[v].update(label[current])
numNodesSparse = sum(important_nk)
nodeMapping = dict()
i = 0

for key, value in enumerate(important_nk):
	if important_nk[key] > 0:
		nodeMapping[key] = i
		i = i + 1

for key,value in label_nx.items():
	G_nx_sparse.add_node(nodeMapping[int(key[1:])], label=value)

for key, v in enumerate(label):
	if important_nk[key] > 0:
		for u in v:
			G_nx_sparse.add_edge(nodeMapping[u], nodeMapping[key])

nx.drawing.nx_agraph.write_dot(G_nx_sparse, str(sys.argv[2]))

with open(sys.argv[2], "r") as f:
    lines = f.readlines()
with open(sys.argv[2], "w") as f:
    for line in lines:
        if "node [label" not in line:
            f.write(line)