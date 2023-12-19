#include<vector>
#include<algorithm>
#include<queue>
#include<ctime>
#include<chrono>
#include<iostream>
#include"external/parlib/include/parlay/parallel.h"
#include"external/parlib/include/parlay/primitives.h"
#include"external/parlib/include/parlay/sequence.h"
using namespace std;

const int side = 500;
const bool do_validation = false;
const int INFINITE_DISTANCE = 1e9;
void bfs_seq(int n, int s, int* dist, vector<vector<int>>& graph) {
	for (int i = 0; i < n; ++i) {
		dist[i] = INFINITE_DISTANCE;
	}
	dist[s] = 0;
	vector<bool>used(n);
	used[s] = 1;
	queue<int>q;
	q.push(s);
	while (q.size()) {
		int t = q.front();
		q.pop();
		for (int x : graph[t]) {
			if (!used[x]) {
				dist[x] = dist[t] + 1;
				used[x] = 1;
				q.push(x);
			}
		}
	}
}

void bfs_par(int n, int s, int* dist, vector<vector<int>>& graph) {
	parlay::parallel_for(0, n, [&](size_t i) {dist[i] = INFINITE_DISTANCE; });
	parlay::sequence<int>frontier = { s };
	vector<atomic_bool>used(n);
	dist[s] = 0;
	used[s] = true;
	parlay::sequence<int>sizes;
	parlay::sequence<int>frontier2;
	while (frontier.size()) {
        int d = dist[frontier[0]];
		sizes.resize(frontier.size());
		parlay::parallel_for(0, frontier.size(), [&](size_t i) {sizes[i] = graph[frontier[i]].size(); });
		int total = parlay::scan_inplace(sizes);
		frontier2.resize(total);
		parlay::parallel_for(0, frontier.size(), [&](size_t i) {
			int from = sizes[i];
			int x = frontier[i];
			for (int q : graph[x]) {
				bool falseval = false;
				if (used[q].compare_exchange_strong(falseval, true)) {
					frontier2[from++] = q;
					dist[q] = d + 1;
				} else {
					frontier2[from++] = -1;
				}
			}
        });
        frontier = parlay::filter(frontier2, [&](int x) {return x != -1; });
	}
}

int main() {
	int n = side * side * side;
	vector<vector<int>>graph(n);
	for (int i = 0; i < n; ++i) {
		if (i % side) {
			graph[i].push_back(i - 1);
			graph[i - 1].push_back(i);
		}
		if ((i / side) % side) {
			graph[i].push_back(i - side);
			graph[i - side].push_back(i);
		}
		if ((i / side / side) % side) {
			graph[i].push_back(i - side * side);
			graph[i - side * side].push_back(i);
		}
	}
	long long total_t_seq = 0;
	long long total_t_par = 0;
	int* dist = new int[n];
	for (int i = 0; i < 5; ++i) {
		auto start_t = chrono::high_resolution_clock::now();
		bfs_seq(n, 0, dist, graph);
		auto t2 = std::chrono::duration_cast<std::chrono::milliseconds>(chrono::high_resolution_clock::now() - start_t).count();
		total_t_seq += t2;
        cout << "This run: sequential bfs: " << t2 / (double) CLOCKS_PER_SEC << '\n';
        if (do_validation) {
			for (int i = 0; i < n; ++i) {
				if (dist[i] != i % side + i / side % side + i / side / side % side) {
					cout << "Invalid seq bfs result";
					return -1;
				}
			}
		}
		start_t = chrono::high_resolution_clock::now();
		bfs_par(n, 0, dist, graph);
		t2 = std::chrono::duration_cast<std::chrono::milliseconds>(chrono::high_resolution_clock::now() - start_t).count();
		total_t_par += t2;
        cout << "This run: Parallel bfs:   " << t2 / (double) CLOCKS_PER_SEC << '\n';
		if (do_validation) {
			for (int i = 0; i < n; ++i) {
				if (dist[i] != i % side + i / side % side + i / side / side % side) {
					cout << "Invalid par bfs result";
					return -1;
				}
			}
		}
	}
	delete[] dist;
	cout << "Average: Sequential bfs: " << total_t_seq / 5.0 / CLOCKS_PER_SEC << '\n';
	cout << "Average: Parallel bfs:   " << total_t_par / 5.0 / CLOCKS_PER_SEC << '\n';
	cout << "Parallel variant is " << total_t_seq / (double) total_t_par << " times faster";
	return 0;
}
