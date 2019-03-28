/**
 * @file Collatz.cpp
 * @author Christopher Trevilla Figueroa
 * @brief 
 * @version 0.1
 * @date 2019-03-26
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include <iostream>
#include <cstring>
#include <chrono>

#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

#include "../Color.cpp"

using namespace std;

#define SIZE_SHAMEN 1024

/**
 * @brief 
 * 
 * @param vals 
 * @param YRange 
 * @param xScale 
 * @return cv::Mat 
 */
cv::Mat plotGraph(std::vector<double> &vals, int YRange[2], int xScale = 10)
{
    auto it = minmax_element(vals.begin(), vals.end());
    double scale = 1. / ceil(*it.second - *it.first);
    double bias = *it.first;
    int rows = (YRange[1] - YRange[0]) + 40;
    int cols = vals.size() * xScale + 40;

    cv::Mat image = cv::Mat::zeros(rows, cols, CV_8UC3);

    image.setTo(0);

    // draw axis
    cv::line(image, cv::Point(20, rows - 20), cv::Point(20, 20), cv::Scalar(166, 166, 166), 2);
    cv::line(image, cv::Point(20, rows - 20), cv::Point(cols - 20, rows - 20), cv::Scalar(166, 166, 166), 2);

    for (int i = 0; i < (int)vals.size(); i++)
    {
        if (i < (int)vals.size() - 1)
            cv::line(image, cv::Point(20 + i * xScale, rows - 20 - (vals[i] - bias) * scale * YRange[1]), cv::Point(20 + (i + 1) * xScale, rows - 20 - (vals[i + 1] - bias) * scale * YRange[1]), cv::Scalar(255, 0, 0), 1);

        cv::circle(image, cv::Point(20 + i * xScale, rows - 20 - (vals[i] - bias) * scale * YRange[1]), 5, cv::Scalar(0, 255, 120), /*CV_FILLED*/ 2, 8, 0);
    }

    return image;
}

/**
 * @brief Generate collatz 
 * 
 * @param num 
 * @return int 
 */
int collatz(int num)
{
    int steps = 0;
    while(num > 1){
        num = (num % 2) == 1 ? ((3 * num) + 1) : (num / 2);
        steps++;
    }

    return steps;
    
}

/**
 * @brief 
 * 
 * @param num_child 
 * @param lastNumber 
 * @return double 
 */
int start_work(int num_child, int lastNumber)
{
    /*****************/
    int pid, worker_pid[num_child], rc_pid, id_memory;
    int *sharedMemory = NULL;
    /******************/
    /******************/
    int from, to;
    int deltaSum = lastNumber / num_child;
    /******************/

    if ((id_memory = shmget(123, SIZE_SHAMEN, 0664 | IPC_CREAT)) == -1)
    {
        cerr << "Error: reserve shared memory" << endl;
    }

    if (num_child == 1)
    {
        return collatz(lastNumber);
    }

    for (int i = 0; i < num_child; i++)
    {
        pid = worker_pid[i] = fork();
        if (pid == 0)
        {
            printf("%sChild %d %s: %d || Parent: %d || Start work...\n", Color::blue, i, Color::def, getpid(), getppid());
            sharedMemory = (int *)shmat(id_memory, (void *)0, 0);
            from = i * deltaSum;
            to = (i == num_child - 1) ? lastNumber + 1 : (i + 1) * deltaSum;

            for(int j = from; j < to; j++)
            {
                //printf("\t\tNumber: %d\n",j);
                sharedMemory[i] = collatz(j);
            }
            //printf("Child %d says: finish!! steps %d\n", i, sharedMemory[i]);
            exit(100 + 1);
        }
    }

    if (pid != 0 and pid != -1)
    {

        int status;
        sharedMemory = (int *)shmat(id_memory, NULL, 0);

        double total = 0;
        for (int i = 0; i < num_child; i++)
        {
            rc_pid = waitpid(worker_pid[i], &status, 0);
            printf("\t%sChild %d %s: steps: %d\n", Color::blue, i, Color::def, sharedMemory[i]);
            total += sharedMemory[i];
        }

        return total;

        shmdt(&sharedMemory);
    }
}

int main()
{

    std::vector<double> timeProcess;
    int total;

    for (int i = 1; i < 5; i++)
    {
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        total = start_work(i, 13);
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
        std::cout << "\t" << Color::yellow << "Parent: total steps " << Color::def << total << Color::yellow << " It took me: " << Color::def << time_span.count() << " seconds\n\n"
                  << std::endl;
        timeProcess.push_back(time_span.count() * 1000);
    }

    /// plot the time results
    double maxValue = *std::max_element(timeProcess.begin(), timeProcess.end());
    int YRange[2] = {0, 640};

    cv::Mat graph = plotGraph(timeProcess, YRange, 50);
    cv::imshow("#Process vs Time(ms)", graph);

    cv::waitKey();

    return 0;
}