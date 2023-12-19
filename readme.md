Тестировалось на Windows через Codeblocks, Для параллелизации использовался Parlaylib

Весь код в bfs.cpp

`void bfs_seq(int n, int s, int* dist, vector<vector<int>>& graph)` - послед.реализация

`void bfs_par(int n, int s, int* dist, vector<vector<int>>& graph)` - паралл.реализация

`int main()` - тестирование скорости

```
Average: Sequential bfs: 55.1298
Average: Parallel bfs:   15.687
Parallel variant is 3.51436 times faster
```
