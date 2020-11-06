#include <iostream>
#include <cstdlib>
#include <omp.h>
#include <time.h>
#include <chrono>
#include <thread>
using namespace std;

int n1 = 500;
int m1 = 500;
int n2 = 500;
int m2 = 500;
//n1 x m1
int** matrix1;
//n2 x m2
int** matrix2;
//n1 x m2
int** result;

void randomiseMatrix(int** matrix, int n, int m) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            matrix[i][j] = rand() % 11;
        }
    }
    return;
}
float check_efficiency(int thread_num)
{
    auto start = chrono::high_resolution_clock::now();
#pragma omp parallel for num_threads(thread_num) collapse(3) //shared(matrix1, matrix2, result) private(i, j, k)
    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < m2; j++) {
            result[i][j] = 0;
            for (int k = 0; k < m1; k++) {
                result[i][j] += (matrix1[i][k] * matrix2[k][j]);
            }
        }
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<float> duration = end - start;
    return duration.count();
}

int main(int argc, char** argv) {
    srand(time(NULL));
    
    matrix1 = (int**)malloc(sizeof(int*) * n1);
    for (int i = 0; i < n1; i++) {
        matrix1[i] = (int*)malloc(sizeof(int) * m1);
    }
    matrix2 = (int**)malloc(sizeof(int*) * n2);
    for (int i = 0; i < n2; i++) {
        matrix2[i] = (int*)malloc(sizeof(int) * m2);
    }
    result = (int**)malloc(sizeof(int*) * n1);;
    for (int i = 0; i < n1; i++) {
        result[i] = (int*)malloc(sizeof(int) * m2);
    }
    randomiseMatrix(matrix1, n1, m1);
    randomiseMatrix(matrix2, n2, m2);

    omp_set_num_threads(omp_get_max_threads());   //4

    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < m2; j++) {
            result[i][j] = 0;
            for (int k = 0; k < m1; k++) {
                result[i][j] += (matrix1[i][k] * matrix2[k][j]);
            }
        }
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<float> duration = end - start;

    cout << "Without Using OpenMP (1 Thread)=======" << endl;
    cout << "Time:\t" << duration.count() << endl;
    cout << "============Using OpenMP==============" << endl;
    for (int i = 1; i <= 64; i *= 2)
    {
        cout << i << " threads :\t" << check_efficiency(i) << '\t';
        //I have 2 cores but 4 logical processors
        if (i == omp_get_max_threads())
            cout << "(number of logical processors)";
        cout << endl;
    }

    return 0;
}