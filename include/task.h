// Copyright 2021 GHA Test Team
#pragma once
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>

typedef std::chrono::system_clock SystemClock;
typedef std::chrono::duration<float> FloatSeconds;
typedef std::lock_guard<std::mutex> Guard;

class Customer {
public:
    const int kItemsCount;
    const SystemClock::time_point kBornTime;

public:
    explicit Customer(int items_count);
};

struct Stats {
public:
    FloatSeconds work_time;
    int accepted_clients_count;
    int declined_clients_count;
    std::vector<FloatSeconds> checkouts_work_times;
    FloatSeconds clients_alive_time;
    int customers_queue_length;
    int customers_queue_tacts;

 public:
     Stats();
};

class Shop {
 private:

     class Checkout {
      private:
         std::thread thread_;
         bool is_alive_;
         int current_items_count_;
         std::mutex local_mutex_;
         std::mutex& shop_mutex_;
         Shop& shop_;

      public:
         const int kId;
         const float kIntensity;
         
      private:
         void Work();
         void Release();

      public:
         explicit Checkout(int id, float intensity, Shop& shop, std::mutex& shop_mutex);
         Checkout(const Checkout& other);
         void CustomerCome(int items_count);
         void Off();

         ~Checkout();
     };

     bool is_alive_;
     std::mutex mtx_;
     std::thread queue_thread_;
     std::queue<Checkout*> checkouts_queue_;
     std::queue<Customer> customers_queue_;
     std::vector<Checkout> checkouts_;
     Stats shop_stats_;
 public:
     const int kNumberOfCheckouts;
     const int kMaxCustomersQueueLength;
     const SystemClock::time_point kOpenTime;

 private:
     void CheckoutsControl();

 public:
     explicit Shop(int numbers_of_checkouts, float checkouts_intensity,
         int max_customers_queue_length);
     void CustomerCome(Customer customer);
     Stats GetStats();
     Checkout& GetCheckout(int id);
     bool IsOpen();
     void Off();

     ~Shop();
};

void SpawnCustomers(Shop& shop, int count = 10,
    float intensity = 1, int avg_items_count = 10);
