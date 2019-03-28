/*
 * This program show the concurrency/parallelization process to make a sumatory
 *
 * The core program realize a sumatory from zero to N; The summatory is realizated
 * in chunks, the core promgam create child process to make a partial sumatories in
 * the range assigned, then the child send the partial results to the parent process.
 * The father admin and collect the partial results to get the total result.
 *
 * The main program repeat the summatory using K childs in each time
 * and plot the time consumed in the task.
 *
 */

#include <iostream>
#include <cstring>
#include <numeric>
#include <ctime>
#include <ratio>
#include <chrono>
#include <vector>
#include <cmath>

#include <sys/wait.h>
#include <unistd.h>

double computeNumericalIntegral(double (*ptr_func)(double), double a, double b, int n)
{
    double value = 0;
    double dx = (b - a) / n;
    double x = a;
    for (int iterator = 0; iterator < n; ++iterator, x += dx)
    {
        value += dx * ptr_func(x);
    }
    return (value);
}

double theFunction(double x)
{
    return pow(x, (1.0 / 5.0)) + abs(cos(x)) + (pow(x, 7) / 0.7);
}

/**
 * @brief The ResultTask struct
 * container results, time and computed value, for a task
 */
struct ResultTask
{
    double time;
    long value;
};

/**
 * @brief do_summatory the admin process to perfom the summatory
 *                     from Zero to limitSup, the summatory is made in
 *                     chunks each one assigned to child process.
 * @param numChilds    numChilds to create for make the task
 * @param limitSup     the limit number to add in the summatory
 * @return             the summatory value computed
 */
unsigned long do_summatory(int numChilds, double limitInf, double limitSup, int interval)
{
    int K = numChilds;
    unsigned long resultSummatory;

    if (K == 1) // Serial processing
    {
        return computeNumericalIntegral(theFunction, limitInf, limitSup, interval);
    }

    /// vars for admin childs
    pid_t pid;
    pid_t rc_pid;
    pid_t child_pids[K];

    /// vars for admin interprocess communication
    int fd[2 * K], nbytes;
    std::string message;
    char readbuffer[80];

    /// vars for the task to do
    int numSubDivisionsForChild = interval / K;
    int sizeSubDivision = (limitSup - limitInf) / interval;
    double sizeSubRange = sizeSubDivision * numSubDivisionsForChild;
    unsigned long partialIntegral = 0;
    unsigned long from, to;

    /// Creating child and assign work
    for (int ii = 0; ii < K; ii++)
    {
        /// creating the communication channel
        pipe(fd + ii * 2);

        /// try to create the child process
        switch (pid = child_pids[ii] = fork())
        {
        case -1:
            /// something went wrong
            /// parent exits
            std::cerr << "CRITICAL ERROR: went try create child: " << ii << std::endl;
            perror("fork");
            exit(1);

        case 0:
            /// Code for the children process
            std::cout << "Children " << ii << "-" << getpid() << " : start working now ... " << std::endl;
            // Child process closes up input side of pipe
            close(fd[0 + ii * 2]);

            /// assign the work to do
            from = ii * sizeSubRange;
            to = (ii == K - 1) ? limitSup + 1 : (ii + 1) * sizeSubRange;
            partialIntegral = computeNumericalIntegral(theFunction, from, to, numSubDivisionsForChild);

            /// send the result by the comunication channel
            /// through the output side of pipe
            message = std::to_string(partialIntegral);
            write(fd[1 + ii * 2], message.c_str(), message.length() + 1);

            /// terminate the child process and send
            /// a signal to the father
            exit(message.length() + 1);

            break;
            /*Missing code for parent process*/
        }
    }

    /*Parent process code*/
    if (pid != 0 && pid != -1)
    {
        /// Collect the partial results
        resultSummatory = 0;
        for (int ii = 0; ii < K; ii++)
        {
            int status;

            ///* Parent process closes up output side of pipe
            close(fd[1 + ii * 2]);

            rc_pid = waitpid(child_pids[ii], &status, 0);
            std::cout << "\tPARENT: Child " << ii << "-" << child_pids[ii] << " finished work" << std::endl;

            /// Read the result in a string from the pipe, channel communication
            nbytes = read(fd[0 + ii * 2], readbuffer, /*WEXITSTATUS(status)*/ sizeof(readbuffer));
            //std::cout<<" \tReceived string: "<< readbuffer<<std::endl;
            resultSummatory += std::atol(readbuffer);
        }

        /// return the final result
        return resultSummatory;
    }

    return 1; /// something went wrong
}

int main(void)
{
    long limitInf = 0;
    long limitSup = 2 * M_PI;
    int interval = 1E8;
    int maxNumberProcess = 10;

    /// vars for collect the time results and plot
    unsigned long valueComputed;

    std::vector<double> timeProcess;

    for (int kk = 1; kk < maxNumberProcess; ++kk)
    {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "\ttry using " << kk << " process: " << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

        valueComputed = do_summatory(kk, limitInf, limitSup, interval);

        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

        std::cout << "----------------------------------------" << std::endl;
        std::cout << "\tresult:" << valueComputed << " It took me: " << time_span.count() << " seconds\n\n"
                  << std::endl;

        timeProcess.push_back(time_span.count() * 1000);
    }
}