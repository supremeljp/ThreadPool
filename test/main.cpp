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
    auto func_future = pool.CommitTask(Func);
    func_future.get();
    std::system("pause");
    return 0;
}