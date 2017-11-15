//
//  philosopher1.cpp
//  
//
//  Created by 张雲淞 on 2017/5/27.
//
//

#include <cstdlib>
#include <iostream>
#include <cerrno>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <list>

using namespace std;

void philosopher(int);
void table(int,int);

#include "mpi.h"

#define FORK_REQUEST 1
#define FORK_RESPONSE 2
#define FORK_RELEASE 3
#define DEBUG 1

int main (int argc, char *argv[])
{
    int id; //my MPI ID
    int p; //total MPI processes
    
    //int tag =1;
    
    //Init MPI
    MPI::Init(argc,argv);
    
    //Get the num of processes
    p = MPI::COMM_WORLD.Get_size();
    
    //Determine the rank of this process
    id = MPI::COMM_WORLD.Get_rank();
    
    //Safety check
    if(p < 3)
    {
        MPI::Finalize();
        std::cerr << "Need at least 2 philosophers! Try again" << std::endl;
        return 1;
    }
    
    //Depending on rank, philosopher or table
    if(id == 0)
        table(id,p);
    else
        philosopher(id);
    
    MPI::Finalize();
    return 0;
}

/* Philosopher function - only philosopher processes run this*/
void philosopher(int id)
{
    if(DEBUG)
        std::cout<< "Hello from philosopher " << id << std::endl;
    
    int in_buffer[1];
    int out_buffer[1];
    MPI::Status status;
    srand(time(NULL)+id);
    
    while (true) {
        if(DEBUG)
            std::cout<<"Philosopher "<<id<<" is sleeping "<<std::endl;
        sleep(rand()%10);//Sleep
        
        if(DEBUG)
            std::cout<<"Philosopher "<<id<<" is waiting to eat "<<std::endl;
        MPI::COMM_WORLD.Send(out_buffer,1,MPI_INT,0,FORK_REQUEST);//Request forks
        MPI::COMM_WORLD.Recv(in_buffer,1,MPI::INT,0,FORK_RESPONSE,status); // waiting for response
        if (DEBUG)
            std::cout<<"Philosopher "<<id<<" is eating "<<std::endl;
        sleep(rand()%10);
        if(DEBUG)
            std::cout<<"Philosopher "<<id<<" has done eating "<<std::endl;
        MPI::COMM_WORLD.Send(out_buffer,1,MPI_INT,0,FORK_RELEASE);//Release forks
    }
}

/* Table function - only table process run this */
void table(int id,int p)
{
    std::cout<<"Hello from table "<<id<<std::endl;
    int in_buffer[1];
    int out_buffer[1];
    int philosopher;
    MPI::Status status;
    
    std::list<int> queue;
    
    bool fork[p-1];
    for(int i = 0;i<p-1;i++)
        fork[i] = true; //init all forks as free
    
    while (true) {
        MPI::COMM_WORLD.Recv(in_buffer,1,MPI::INT,MPI::ANY_SOURCE, MPI::ANY_TAG,status);//Receive message
        philosopher = status.Get_source();//Record the source of the message
        
        if (status.Get_tag() == FORK_REQUEST) {//If request for forks
            if(DEBUG)
                std::cout<<"Table "<<id<<" got philosopher "<<philosopher<<" fork request "<<std::endl;
            if(fork[philosopher%(p-1)] == true && fork[philosopher-1]==true)//If both forks are free
            {
                fork[philosopher%(p-1)]= false;
                fork[philosopher-1] = false;//Set the forks taken
                MPI::COMM_WORLD.Send(out_buffer,1,MPI_INT,philosopher,FORK_RESPONSE);//Send Fork to the philosopher
                if (DEBUG)
                    std::cout<<"Table "<<id<<" sent philosopher "<<philosopher<<" the forks "<<std::endl;
            }
            else //If both forks are not free
                queue.push_back(philosopher);
        }
        if (status.Get_tag() == FORK_RELEASE) { // If request to release forks
            fork[philosopher%(p-1)]= true;
            fork[philosopher-1] = true;
            if(DEBUG)
                std::cout<<"Table "<<id<<" got philosopher "<<philosopher<<"'s forks free"<<std::endl;
            
            if(!queue.empty())
            {
                for(std::list<int>::iterator it = queue.begin();it != queue.end();it++) //iterator the whole list
                {
                    philosopher = *it;
                    if(fork[philosopher%(p-1)] == true && fork[philosopher-1] == true){ //if one of them can get both forks
                        fork[philosopher%(p-1)] = false;
                        fork[philosopher-1] = false;
                        MPI::COMM_WORLD.Send(out_buffer,1,MPI_INT,philosopher,FORK_RESPONSE);
                        if (DEBUG)
                            std::cout<<"Table "<<id<<" sent philosopher "<<philosopher<<" the forks "<<std::endl;
                        it = queue.erase(it); // Remove from wait list
                    }
                }
            }
        }
    }
}








































