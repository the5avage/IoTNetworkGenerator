#pragma once

struct TaskBuffer
{
    SemaphoreHandle_t semaphore;
    std::vector<std::function<void(void)>> tasks;

    void addTask(std::function<void(void)> task);
    void executeTasks();

    TaskBuffer()
    {
        semaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(semaphore);
    }
};

extern TaskBuffer taskBuffer;

