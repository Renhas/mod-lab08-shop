// Copyright 2021 GHA Test Team
#include <iostream>
#include "../ShopLib/task.h"
#include <random>

struct Result {
    float lambda;
    float mu;
    float ro;
    float free_probability;
    float queue_probability;
    float decline_probability;
    float relative_capacity;
    float absolute_capacity;
    float avg_queue_length;
    float avg_served;
    float avg_alive_time;
    float avg_wait_time;
};

void PrintResult(Result result);
Result CalculateAll(float shop_intensity, float customer_intensity,
    int checkouts, int queue_length);
Result CalculateActual(Stats stats, int queue_length);
int main()
{
    float shop_intensity = 10;
    float customer_intensity = 30;
    int checkouts = 2;
    int avg_items_count = 3;
    int queue_length = 10;
    Shop shop(checkouts, shop_intensity, queue_length);
    std::cout << "Shop created" << std::endl;
    SpawnCustomers(shop, 200, customer_intensity, avg_items_count);
    std::cout << "Customers created" << std::endl;
    shop.Off();
    auto stats = shop.GetStats();
    float work_time = stats.work_time.count();
    float avg_clients_alive_time = stats.clients_alive_time.count();
    avg_clients_alive_time /= stats.accepted_clients_count;
    std::cout << "Accepted: " << stats.accepted_clients_count << std::endl;
    std::cout << "Declined: " << stats.declined_clients_count << std::endl;
    std::cout << "Total work time: " << work_time << std::endl;
    int id = 0;
    float total_work_time = 0;
    for (auto& time : stats.checkouts_work_times) {
        std::cout << "Checkout #" << id + 1 << ": " << time.count() << std::endl;
        total_work_time += time.count();
        id++;
    }
    float avg_work_time = total_work_time / checkouts;
    std::cout << "Average work time: " << avg_work_time << std::endl;
    std::cout << "Average downtime: " << work_time - avg_work_time << std::endl;

    Result theory = CalculateAll(shop_intensity,
        customer_intensity * avg_items_count, checkouts, queue_length);
    Result actual = CalculateActual(stats, queue_length);
    std::cout << "\nTheory\n" << std::endl;
    PrintResult(theory);
    std::cout << "\nActual\n" << std::endl;
    PrintResult(actual);
    char temp;
    std::cin >> temp;
}

void PrintResult(Result result)
{
    //std::cout << "Shop intensity: " << result.mu << std::endl;
    //std::cout << "Customer intensity: " << result.lambda << std::endl;
    //std::cout << "Channel load: " << result.ro << std::endl;
    //std::cout << "Free proba: " << result.free_probability << std::endl;
    //std::cout << "Queue proba: " << result.queue_probability << std::endl;
    std::cout << "Decline proba: " << result.decline_probability << std::endl;
    std::cout << "Relative: " << result.relative_capacity << std::endl;
    std::cout << "Absolute: " << result.absolute_capacity << std::endl;
    std::cout << "Avg queue length: " << result.avg_queue_length << std::endl;
    //std::cout << "Avg served: " << result.avg_served << std::endl;
    std::cout << "Avg alive time: " << result.avg_alive_time << std::endl;
    //std::cout << "Avg wait time: " << result.avg_wait_time << std::endl;
}

Result CalculateAll(float shop_intensity, float customer_intensity,
    int checkouts, int queue_length)
{
    Result res = Result();
    res.lambda = customer_intensity;
    res.mu = shop_intensity;
    res.ro = res.lambda / res.mu;
    res.free_probability = 0;
    float numerator = 1, denominator = 1;
    for (int i = 0; i <= checkouts; i++) {
        res.free_probability += numerator / denominator;
        numerator *= res.ro;
        denominator *= i + 1;
    }
    denominator /= checkouts + 1;
    float ro_n_1 = numerator / (denominator * checkouts);
    numerator /= res.ro;
    float ro_n = numerator / denominator;
    res.queue_probability = ro_n;
    for (int i = checkouts + 1; i <= checkouts + queue_length; i++) 
    {
        numerator *= res.ro;
        denominator *= checkouts;
        res.free_probability += numerator / denominator;
    }
    res.free_probability = 1 / res.free_probability;
    float ro_n_m = numerator / denominator;
    float ro_m = ro_n_m / ro_n;
    float ro_checkouts = 1 - res.ro / checkouts;
    res.queue_probability *= 1 - ro_m;
    res.queue_probability /= ro_checkouts;
    res.queue_probability *= res.free_probability;
    res.decline_probability = ro_n_m * res.free_probability;

    res.relative_capacity = 1 - res.decline_probability;
    res.absolute_capacity = res.lambda * res.relative_capacity;
    res.avg_queue_length = res.free_probability * ro_n_1;
    res.avg_queue_length *= 1 - ro_m * (1 + queue_length * ro_checkouts);
    res.avg_queue_length /= ro_checkouts * ro_checkouts;
    res.avg_served = res.absolute_capacity / res.mu;
    res.avg_wait_time = res.avg_queue_length / res.lambda;
    res.avg_alive_time = res.avg_wait_time + res.relative_capacity / res.mu;
    return res;
}

Result CalculateActual(Stats stats, int queue_length)
{
    Result res = Result();
    int accepted = stats.accepted_clients_count;
    int declined = stats.declined_clients_count;
    int all_count = accepted + declined;
    int checkouts_count = stats.checkouts_work_times.size();
    float work_time = stats.work_time.count();
    res.avg_alive_time = stats.clients_alive_time.count();
    res.avg_alive_time /= accepted;
    res.avg_queue_length = (float)stats.customers_queue_length;
    res.avg_queue_length /= stats.customers_queue_tacts;
    res.decline_probability = (float)declined / all_count;
    res.relative_capacity = 1 - res.decline_probability;
    res.absolute_capacity = accepted / work_time;

    res.lambda = all_count / work_time;
    //res.lambda = all_count * res.avg_alive_time;
    res.avg_wait_time = res.avg_queue_length / res.lambda;
    res.mu = res.relative_capacity / (res.avg_alive_time - res.avg_wait_time);
    //res.mu = accepted * res.avg_alive_time / checkouts_count;
    res.avg_served = res.absolute_capacity / res.mu;

    float total_work_time = 0;
    for (auto& time : stats.checkouts_work_times) {
        total_work_time += time.count();
    }
    float avg_work_time = total_work_time / checkouts_count;
    float avg_free_time = work_time - avg_work_time;
    float one_proba = avg_free_time / work_time;
    res.free_probability = std::powf(one_proba, checkouts_count);
    res.queue_probability = res.avg_queue_length / queue_length;
    res.ro = res.lambda / res.mu;
    return res;
}
