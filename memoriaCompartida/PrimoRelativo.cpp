/*Falta terminar*/

#include <iostream>
#include <cstring>
#include <chrono>

#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>

using namespace std;

#define SIZE_SHAMEN 1024

int isRelativePrimo(int a, int b){
    int m = a<b?a:b;
    for(int i=2; i<=m; i++)
    {
        if(a%i==0 && b%i==0)
        return 0;
    }
    return 1;
}

int main()
{
    int pid, ID_memory;
    int *ptrSharedMemory = NULL;
    /*key_t clave;
    
    clave =ftok("/bins/ls", NUM);*/
    
    //int matriz[1000][1000];

    std::cout<< " === SHARED MEMORY === \n"<<std::endl;

    /*if((ID_memory = shmget(clave, SIZE_SHAMEN, 0664|IPC_CREAT))==-1)
    {
        std::cerr<<"ERROR: reverse shared memory"<<std::endl;
    }*/
    
    if((ID_memory = shmget(1315511, SIZE_SHAMEN, 0664|IPC_CREAT))==-1)
    {
        std::cerr<<"ERROR: reverse shared memory"<<std::endl;
    }

    pid=fork();

    switch(pid)
    {
        case -1:
        std::cerr<<"ERROR: creating childs, using fork"<<std::endl;
        break;

        case 0: //process childs
        ptrSharedMemory = (int *)shmat(ID_memory,(void *)0,0);

        for(int row=0; row<1000; ++row)
        {
            for(int colum=0; colum<1000; ++colum)
            {
                if(isRelativePrimo(row,colum))
                {
                    ptrSharedMemory[row][colum]=255;
                }
                else
                {
                    ptrSharedMemory[row][colum]=0;
                }
            }
        }
        break;

        default: //process father
        //sleep(5);

        ptrSharedMemory = (int *)shmat(ID_memory,NULL,0);
        
        for(int i=0; i<1000; ++i)
        {
            for(int j=0; j<1000; ++j)
            {
                std::cout<<"Values in shared memory " <<ptrSharedMemory[i][j]<<std::endl;
            }
        }
        shmdt(&ptrSharedMemory);
        if(shmctl(ID_memory,IPC_RMID,NULL)==-1)
        {
            std::cerr<<"Something went wrong "<<std::endl;
        }
        break;
    }
    return 0;
}
