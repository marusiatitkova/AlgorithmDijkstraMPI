#include "pch.h"
#include <iostream>
#include <mpi.h>
#include <string>
#include <vector>
#include <iomanip>
#include <stdio.h>

#pragma warning(disable:4996)

const int INF = INT_MAX;

int procNum, procRank, n, m;
int *graphSizes;
std::vector< std::vector<std::pair<int, int > > > graph;
int *used;
int *distance;
int *usedProc;
int *distanceProc;
int *minDistance, *minIndex;
int nProc;
double startTime, endTime, duration;


void processInitializationNM() {
	freopen("out100000.txt", "r", stdin);

	std::cin >> n; // nodes count
	std::cin >> m; // edges count
}

void processInitializationGraph(int start) {

	for (int i = 0; i < m; i++) {
		int from, to, w;
		std::cin >> from >> to >> w;
		graph[from].push_back(std::make_pair(to, w));
		graph[to].push_back(std::make_pair(from, w));
	}
	for (int i = 0; i < n; i++) {
		graphSizes[i] = (int)(graph[i].size());
		used[i] = 0;
		distance[i] = INF;
	}

	distance[start] = 0;
}

void updateDistance(int node) {
	MPI_Gather(distanceProc, nProc, MPI_INT, distance, nProc, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Gather(usedProc, nProc, MPI_INT, used, nProc, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	if (procRank == 0) {
		used[node] = 1;
		for (int i = 0; i < graph[node].size(); i++) {
			int to = graph[node][i].first;
			int len = graph[node][i].second;
			if (!used[to] && distance[node] + len < distance[to]) {
				distance[to] = distance[node] + len;
			}
		}
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Scatter(distance, nProc, MPI_INT, distanceProc, nProc, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Scatter(used, nProc, MPI_INT, usedProc, nProc, MPI_INT, 0, MPI_COMM_WORLD);
}

void getMinDistance() {
	
	minDistance = new int[procNum];
	minIndex = new int[procNum];
	
	int *minProc, *indexProc;
	minProc = new int[1];
	indexProc = new int[1];

	minProc[0] = INF;
	indexProc[0] = -1;

	for (int i = 0; i < nProc; i++) {
		if (!usedProc[i] && distanceProc[i] <= minProc[0])
			minProc[0] = distanceProc[i], indexProc[0] = procRank * nProc + i;
	}
	
	//std::cout << "Rank = " << procRank << " min = " << minProc[0] << std::endl;
	
	MPI_Gather(minProc, 1, MPI_INT, minDistance, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Gather(indexProc, 1, MPI_INT, minIndex, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	
	int node;

	if (procRank == 0) {
		int minD = minDistance[0], indexD = minIndex[0];
		for (int i = 1; i < procNum; i++) {
			if (minDistance[i] < minD)
				minD = minDistance[i], indexD = minIndex[i];
		}
		node = indexD;
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(&node, 1, MPI_INT, 0, MPI_COMM_WORLD);

	updateDistance(node);
}

void dijkstra() {
	for (int i = 0; i < n; i++) {
		//std::cout << "min = " << i << std::endl;
		getMinDistance();
		//std::cout << "min = " << i << std::endl;
	}

	MPI_Gather(distanceProc, nProc, MPI_INT, distance, nProc, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	
	if (procRank == 0) {
		endTime = MPI_Wtime();
		duration = endTime - startTime;
		std::cout << "Time of execution = " << duration << "\n";
		//for (int i = 0; i < n; i++)
			//std::cout << "Node " << i << " - > " << distance[i] << "\n";
	}
}

int main(int argc, char* argv[]) {

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &procNum);
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);
	//std::cout << "Rank = " << procRank << "proc = " << procNum << std::endl;
	// printf("Rank = %d, proc = %d\n", procRank, procNum);

	if (procRank == 0) {
		startTime = MPI_Wtime();
		processInitializationNM();
	}
	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);

	graph.resize(n);
	used = new int[n];
	distance = new int[n];
	graphSizes = new int[n];
	nProc = n / procNum;
	/*if (n % procNum) {
		++nProc;
	}*/
	distanceProc = new int[nProc];
	usedProc = new int[nProc];

	if (procRank == 0) {
		processInitializationGraph(0);
	}
	MPI_Scatter(distance, nProc, MPI_INT, distanceProc, nProc, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Scatter(used, nProc, MPI_INT, usedProc, nProc, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Bcast(graphSizes, n, MPI_INT, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	if (procRank != 0) {
		for (int i = 0; i < n; i++) {
			graph[i].resize(graphSizes[i]);
		}
	}

	for (int i = 0; i < n; i++) {
		MPI_Bcast(graph[i].data(), (int)(graph[i].size() * 2), MPI_INT, 0, MPI_COMM_WORLD);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	dijkstra();

	MPI_Finalize();
}