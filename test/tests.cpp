// Copyright 2021 GHA Test Team
#include <gtest/gtest.h>
#include "task.h"

TEST(Customer_tests, create) {
    auto start = SystemClock::now();
    Customer customer(5);
    auto end = SystemClock::now();
    FloatSeconds true_value = end - start;
    FloatSeconds customer_value = end - customer.kBornTime;
    EXPECT_EQ(5, customer.kItemsCount);
    float value = true_value.count() - customer_value.count();
    EXPECT_TRUE(std::abs(value) <= 0.001);
}

TEST(Shop_tests, create) {
    int checkouts = 5;
    float intensity = 3;
    int max_length = 2;
    Shop shop(checkouts, intensity, max_length);
    shop.Off();

    EXPECT_EQ(shop.kMaxCustomersQueueLength, max_length);
    EXPECT_EQ(shop.kNumberOfCheckouts, checkouts);
    EXPECT_EQ(shop.GetCheckout(0).kIntensity, intensity);
}

TEST(Shop_tests, off) {
    Shop shop(1, 1, 1);
    shop.Off();
    EXPECT_FALSE(shop.IsOpen());
}

TEST(Shop_tests, stats_create) {
    Stats st = Stats();

    EXPECT_EQ(st.accepted_clients_count, 0);
    EXPECT_EQ(st.declined_clients_count, 0);
    EXPECT_EQ(st.customers_queue_length, 0);
    EXPECT_EQ(st.customers_queue_tacts, 0);
    EXPECT_EQ(st.work_time, FloatSeconds(0));
    EXPECT_EQ(st.checkouts_work_times, std::vector<FloatSeconds>());
    EXPECT_EQ(st.clients_alive_time, FloatSeconds(0));
}

TEST(Shop_tests, shop_stats) {
    auto start = SystemClock::now();
    Shop shop(1, 0.1, 1);
    Customer customer(1);
    shop.CustomerCome(customer);
    shop.CustomerCome(customer);
    shop.CustomerCome(customer);
    shop.Off();
    auto end = SystemClock::now();
    auto stats = shop.GetStats();
    FloatSeconds true_time = end - start;
    float work_time = stats.work_time.count();
    float checkout_time = stats.checkouts_work_times[0].count();
    float difference = true_time.count() - work_time;
    float checkout_difference = checkout_time - work_time;
    float avg_queue_length = (float)stats.customers_queue_length;
    avg_queue_length /= stats.customers_queue_tacts;
    
    EXPECT_TRUE(std::abs(difference) <= 0.001);
    EXPECT_EQ(stats.accepted_clients_count, 2);
    EXPECT_EQ(stats.declined_clients_count, 1);
    EXPECT_TRUE(std::abs(stats.clients_alive_time.count() - 30) <= 1);
    EXPECT_TRUE(std::abs(checkout_difference) <= 0.1);
    EXPECT_TRUE(std::abs(avg_queue_length - 0.5) <= 1);
}

TEST(Shop_tests, checkouts) {
    Shop shop(1, 1, 1);
    auto& checkout = shop.GetCheckout(0);
    checkout.CustomerCome(5);
    shop.Off();
    auto stats = shop.GetStats();
    auto time = stats.checkouts_work_times[0];
    std::cout << time.count();
    EXPECT_EQ(checkout.kIntensity, 1);
    EXPECT_EQ(checkout.kId, 0);
    EXPECT_TRUE(std::abs(time.count() - 5) <= 1);
}
TEST(Customer_tests, spawn) {
    Shop shop(1, 1, 1);
    auto start = SystemClock::now();
    SpawnCustomers(shop, 10, 5, 2);
    auto end = SystemClock::now();
    FloatSeconds time = end - start;
    shop.Off();
    auto stats = shop.GetStats();
    auto accepted = stats.accepted_clients_count;
    auto decliend = stats.declined_clients_count;
    auto checkout_time = stats.checkouts_work_times[0].count();
    auto max_time = accepted * 3 + 1;
    auto min_time = accepted - 1;
    EXPECT_TRUE(std::abs(time.count() - 2) <= 1);
    EXPECT_EQ(accepted + decliend, 10);
    EXPECT_TRUE(min_time <= checkout_time <= max_time);
}
