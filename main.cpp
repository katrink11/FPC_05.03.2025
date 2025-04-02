#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <omp.h>
#include <fstream>
#include <string>
#include <thread>

using namespace std;

// Функция для очистки консоли (кросс-платформенная)
void clearScreen()
{
#ifdef _WIN32
	system("cls");
#else
	system("clear");
#endif
}

// Функция для подсчета живых соседей (без изменений)
int countNeighbors(const vector<vector<bool>> &grid, int i, int j, int rows, int cols)
{
	int count = 0;
	for (int di = -1; di <= 1; ++di)
	{
		for (int dj = -1; dj <= 1; ++dj)
		{
			if (di == 0 && dj == 0)
				continue;
			int ni = (i + di + rows) % rows;
			int nj = (j + dj + cols) % cols;
			if (grid[ni][nj])
				++count;
		}
	}
	return count;
}

// Визуализация сетки с анимацией
void printGrid(const vector<vector<bool>> &grid)
{
	clearScreen();
	for (const auto &row : grid)
	{
		for (bool cell : row)
		{
			cout << (cell ? '#' : ' ');
		}
		cout << endl;
	}
	cout << endl;
}

// Функция обновления сетки с использованием OpenMP
void updateGrid(const vector<vector<bool>> &current, vector<vector<bool>> &next, int rows, int cols)
{
#pragma omp parallel for
	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			int neighbors = countNeighbors(current, i, j, rows, cols);

			if (current[i][j])
			{
				next[i][j] = (neighbors == 2 || neighbors == 3);
			}
			else
			{
				next[i][j] = (neighbors == 3);
			}
		}
	}
}

// Инициализация случайного распределения
void initializeRandom(vector<vector<bool>> &grid, int rows, int cols)
{
	srand(time(nullptr));
	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			grid[i][j] = rand() % 2;
		}
	}
}

// Загрузка начальной конфигурации из файла
void initializeFromFile(vector<vector<bool>> &grid, const string &filename, int rows, int cols)
{
	ifstream file(filename);
	string line;
	for (int i = 0; i < rows && getline(file, line); ++i)
	{
		for (int j = 0; j < cols && j < line.size(); ++j)
		{
			grid[i][j] = (line[j] == 'X');
		}
	}
}

// Подсчет живых клеток с использованием OpenMP reduction
int countAlive(const vector<vector<bool>> &grid)
{
	int count = 0;
#pragma omp parallel for reduction(+ : count)
	for (size_t i = 0; i < grid.size(); ++i)
	{
		for (bool cell : grid[i])
		{
			if (cell)
				++count;
		}
	}
	return count;
}

int main(int argc, char *argv[])
{
	// Параметры по умолчанию
	int rows = 200, cols = 200;
	int iterations = 100;
	int threads = omp_get_max_threads();
	string init_type = "random";
	string filename;
	bool visualize = false;

	// Добавляем параметр для задержки анимации
	int animation_delay = 100; // миллисекунды

	// Парсинг аргументов командной строки
	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "--rows" && i + 1 < argc)
			rows = stoi(argv[++i]);
		else if (arg == "--cols" && i + 1 < argc)
			cols = stoi(argv[++i]);
		else if (arg == "--iter" && i + 1 < argc)
			iterations = stoi(argv[++i]);
		else if (arg == "--threads" && i + 1 < argc)
			threads = stoi(argv[++i]);
		else if (arg == "--init" && i + 1 < argc)
			init_type = argv[++i];
		else if (arg == "--file" && i + 1 < argc)
			filename = argv[++i];
		else if (arg == "--delay" && i + 1 < argc)
			animation_delay = stoi(argv[++i]);
		else if (arg == "--visualize")
			visualize = true;
	}

	omp_set_num_threads(threads);

	// Инициализация сетки
	vector<vector<bool>> current(rows, vector<bool>(cols));
	vector<vector<bool>> next(rows, vector<bool>(cols));

	if (init_type == "random")
	{
		initializeRandom(current, rows, cols);
	}
	else if (init_type == "file")
	{
		initializeFromFile(current, filename, rows, cols);
	}

	auto start = chrono::high_resolution_clock::now();

	// Основной цикл симуляции
	for (int iter = 0; iter < iterations; ++iter)
	{
		updateGrid(current, next, rows, cols);
		swap(current, next);

		// Вывод статистики
		if (visualize)
		{
			printGrid(current);
			this_thread::sleep_for(chrono::milliseconds(animation_delay));
		}
		cout << "Iteration " << iter + 1
			 << " Alive: " << countAlive(current)
			 << endl;
	}

	auto end = chrono::high_resolution_clock::now();
	chrono::duration<double> elapsed = end - start;

	cout << "\nSimulation completed in " << elapsed.count() << " seconds\n";
	cout << "Using " << threads << " threads\n";
	cout << "Field size: " << rows << "x" << cols << endl;

	return 0;
}
