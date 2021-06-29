#include "ThreadPool.hpp"
#include <iostream>
#include <Windows.h>

void Func()
{
   Sleep(2000);
}

int main()
{
    MySpace::ThreadPool pool(2);
    pool.ShutDown();
    try
    {
        auto func_future = pool.CommitTask(Func);
        func_future.get();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }   
    
    std::system("pause");
    return 0;
}