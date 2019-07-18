/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "saiga/core/Core.h"

using namespace Saiga;

void report(int i)
{
#pragma omp critical
    std::cout << "Thread/Group: " << omp_get_thread_num() << "/" << omp_get_num_threads() << " Element: " << i
              << std::endl;
}

void simpleForLoop()
{
    std::cout << "Starting simple OpenMP loop..." << std::endl;
#pragma omp parallel for num_threads(4)
    for (int i = 0; i < 20; ++i)
    {
        int tid = omp_get_thread_num();
        report(i);
    }
    std::cout << std::endl;
}

void simpleGroup()
{
    std::cout << "Starting simple OpenMP loop..." << std::endl;
#pragma omp parallel num_threads(4)
    {
        int tid = omp_get_thread_num();

        report(-1);
        // Let's wait on the previous cout to finish
#pragma omp barrier
        // Only a single thread exetuces this statement. (it can be any id)
        // + it has an implicit barrier at the end
#pragma omp single
        report(-1);
#pragma omp single
        report(-1);
        // Let this loop be executed by the current thread group.
        // Note: we don't need the 'parallel' keyword on 'omp for'
#pragma omp for
        for (int i = 0; i < 20; ++i)
        {
            int tid2 = omp_get_thread_num();
            report(i);
        }

#pragma omp parallel
        {
            int tid2 = omp_get_thread_num();
            report(tid);
        }
    }
    std::cout << std::endl;
}


void reduction()
{
    int sum = 0;

    int N = 100000;
    std::vector<int> data(N, 1);
#pragma omp parallel for reduction(+ : sum)
    for (int i = 0; i < N; ++i)
    {
        sum += data[i];
    }
    SAIGA_ASSERT(sum == N);
    std::cout << "Reduction: " << sum << std::endl;



    // Use an eigen vector
    std::vector<Vec4> dataV(N, Vec4(1, 1, 1, 1));
    Vec4 sumV(0, 0, 0, 0);
#pragma omp parallel for reduction(+ : sumV)
    for (int i = 0; i < N; ++i)
    {
        sumV += dataV[i];
    }
    //    SAIGA_ASSERT(sumV == Vec4(N, N, N, N));
    std::cout << "Reduction Vector: " << sumV.transpose() << std::endl;
}
int main(int argc, char* argv[])
{
    simpleForLoop();
    simpleGroup();
    reduction();
    std::cout << "Done." << std::endl;

    return 0;
}
