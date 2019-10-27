
#include <iostream>
#include <chrono>
#include <tasklet.hpp>

using namespace std::chrono_literals;

#define DEBUG() \
    std::cout << "[" << std::this_thread::get_id() << "] "<< __PRETTY_FUNCTION__ << std::endl;

#define LOG(n) \
    std::cout << "[" << std::this_thread::get_id() << "] "<< n << std::endl;

void task1(int a)
{
    DEBUG();
    //std::this_thread::sleep_for(500ms);
    std::cout << "task1: " << a << std::endl;
    LOG("task 1 end");
}

void task2(int a, int b)
{
    DEBUG();
    //std::this_thread::sleep_for(500ms);
    std::cout << "task2: " << a+b << std::endl;
    LOG("task 2 end");
}

void task3(int a)
{
    DEBUG();
    //std::this_thread::sleep_for(500ms);
    std::cout << "task3: " << a*10 << std::endl;
    LOG("task 3 end");
}

void task_test1()
{
    Task t1(std::bind(task1, 1));
    Task t2(std::bind(task2, 2, 2));
    Task t3(std::bind(task3, 3), std::bind([]{
        std::cout << "task 3 del task" << std::endl;
    }));

    t1.executeTask();

    t2.executeTask();
    t2.waitForTask();

    // t3 del task printed
}

void tasklet_test1()
{
    Tasklet tl;
    tl.start();

    LOG("started post task");
    for(int i=0; i<5; i++)
    {
        auto t1 = std::make_shared<Task>(std::bind(task1, i));
        auto t2 = std::make_shared<Task>(std::bind(task2, i, i));
        auto t3 = std::make_shared<Task>(std::bind(task3, i));

        tl.postTask(t1);
        tl.postTask(t2);
        tl.postUrgentTask(t3);
    }
    LOG("started post and wait");
    auto t4 = std::make_shared<Task>(std::bind(task1, -1));
    tl.postTaskAndWait(t4);
    tl.stop();

    auto end_task = std::make_shared<Task>(std::bind(task1, 55667788), [](){
        std::cout << "not going executed" << std::endl;
    });
    // should not executed
    tl.postTask(end_task);
    tl.postUrgentTask(end_task);
    // should not executed and not blocking
    tl.postTaskAndWait(end_task);
    tl.postUrgentTaskAndWait(end_task);
}

int main()
{
    //task_test1();
    tasklet_test1();
}
