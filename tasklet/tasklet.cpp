
#include <iostream>
#include <cassert>

#include <tasklet.hpp>

#define DEBUG() \
    //std::cout << "[" << std::this_thread::get_id() << "] "<< __PRETTY_FUNCTION__ << std::endl;

#define LOG(n) \
    //std::cout << "[" << std::this_thread::get_id() << "] "<< n << std::endl;


Task::Task(std::function<void()> f)
{
    DEBUG();
    setTask(f);
}
Task::Task(std::function<void()> f, std::function<void()> del_f)
{
    DEBUG();
    setTask(f, del_f);
}
Task::~Task()
{
    DEBUG();
    std::lock_guard<std::mutex> lk(m);
    if(!done)
    {
        del_task();
    }
}
void Task::executeTask()
{
    DEBUG();
    std::lock_guard<std::mutex> lk(m);
    task();
    done = true;
    done_task.notify_all();
}
void Task::waitForTask()
{
    DEBUG();
    std::unique_lock<std::mutex> lk(m);
    if(!done)
        done_task.wait(lk, [this] {return done;});
}
void Task::setTask(std::function<void()> f)
{
    DEBUG();
    task = f;
    del_task = nullptr;
}

void Task::setTask(std::function<void()> f, std::function<void()> del_f)
{
    DEBUG();
    task = f;
    del_task = del_f;
}

Tasklet::Tasklet()
{
    DEBUG();
    auto _doStop = [this]()
    {
        DEBUG();
        clearTasks();
    };
    doStopTask = std::make_shared<Task>(std::bind(_doStop));
}
Tasklet::~Tasklet()
{
    DEBUG();
    doStopTask->waitForTask();
}
void Tasklet::postTask(std::shared_ptr<Task> t)
{
    DEBUG();
    std::lock_guard<std::mutex> lk(tasks_m);
    pending_normal_tasks.push_back(t);
    task_added.notify_all();
}
void Tasklet::postUrgentTask(std::shared_ptr<Task> t)
{
    DEBUG();
    std::lock_guard<std::mutex> lk(tasks_m);
    pending_priority_tasks.push_back(t);
    task_added.notify_all();
}
void Tasklet::postTaskAndWait(std::shared_ptr<Task> t)
{
    DEBUG();
    postTask(t);
    t->waitForTask();
}
void Tasklet::postUrgentTaskAndWait(std::shared_ptr<Task> t)
{
    DEBUG();
    postUrgentTask(t);
    t->waitForTask();
}

void Tasklet::clearTasks()
{
    DEBUG();
    std::lock_guard<std::mutex> lk(tasks_m);
    pending_priority_tasks.clear();
    pending_normal_tasks.clear();
    task_added.notify_all();
}

void Tasklet::start()
{
    DEBUG();
    worker_thread = std::thread(&Tasklet::consumeTask, this);
    LOG("started");
}
void Tasklet::stop(int deferred)
{
    DEBUG();
    if(deferred)
    {
        postTaskAndWait(doStopTask);
    }
    else
    {
        postUrgentTaskAndWait(doStopTask);
    }
    if(worker_thread.joinable()) worker_thread.join();
}

void Tasklet::consumeTask()
{
    DEBUG();
    do {
        std::shared_ptr<Task> current = dequeTask();
        current->executeTask();

        if(current == doStopTask)
        {
            LOG("doStop get!!");
            break;
        }
        else
        {
            LOG("continue");
        }
    } while(1);
}
std::shared_ptr<Task> Tasklet::dequeTask()
{
    DEBUG();
    std::unique_lock<std::mutex> lk(tasks_m);
    LOG("LOCK ACQUIRED");

    auto& prio_list = pending_priority_tasks;
    auto& norm_list = pending_normal_tasks;

    if(prio_list.empty()
    && norm_list.empty())
    {
        LOG("WAIT COND");
        task_added.wait(lk, [this]() {
            return !pending_priority_tasks.empty() || !pending_normal_tasks.empty();
        });
    }
    LOG("WAIT COND DONE");
    if(!prio_list.empty())
    {
        auto ret = prio_list.front();
        prio_list.erase(prio_list.begin());
        LOG("PRIO SIZE:");
        LOG(prio_list.size());
        return ret;
    }
    assert(!norm_list.empty());

    auto ret = norm_list.front();
    norm_list.erase(norm_list.begin());
    return ret;
}
