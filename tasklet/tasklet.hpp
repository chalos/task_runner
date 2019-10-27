#pragma once

#include <future>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

class Task {
public:
    Task(std::function<void()> f);
    Task(std::function<void()> f, std::function<void()> del_f);
    ~Task();
    void executeTask();
    void waitForTask();
private:
    std::function<void()> task;
    std::function<void()> del_task;
    std::mutex m;
    std::condition_variable done_task;
    bool done;

    void setTask(std::function<void()> f);
    void setTask(std::function<void()> f, std::function<void()> del_f);

    friend class Tasklet;
};

class Tasklet {
public:
    Tasklet();
    ~Tasklet();
    void postTask(std::shared_ptr<Task> t);
    void postUrgentTask(std::shared_ptr<Task> t);
    void postTaskAndWait(std::shared_ptr<Task> t);
    void postUrgentTaskAndWait(std::shared_ptr<Task> t);

    void start();
    void stop(int deferred = false);
private:
    std::thread worker_thread;
    std::vector<std::shared_ptr<Task>> pending_normal_tasks;
    std::vector<std::shared_ptr<Task>> pending_priority_tasks;
    std::mutex tasks_m;
    std::condition_variable task_added;

    void consumeTask();
    std::shared_ptr<Task> dequeTask();

    void clearTasks();
    std::shared_ptr<Task> doStopTask;
};
