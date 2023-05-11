// Copyright 2021 GHA Test Team
#include "task.h"

Customer::Customer(int items_count) : 
    kItemsCount{ items_count }, kBornTime{std::chrono::system_clock::now()}
{
}


void Shop::CheckoutsControl()
{
    while (is_alive_) {
        Guard lock(mtx_);
        shop_stats_.customers_queue_tacts++;
        shop_stats_.customers_queue_length += customers_queue_.size();
        if (checkouts_queue_.empty() || customers_queue_.empty()) {
            std::this_thread::yield();
            continue;
        }
        auto checkout = checkouts_queue_.front();
        checkouts_queue_.pop();
        auto customer = customers_queue_.front();
        customers_queue_.pop();
        auto end = SystemClock::now();
        FloatSeconds time = end - customer.kBornTime;
        shop_stats_.clients_alive_time += time;
        checkout->CustomerCome(customer.kItemsCount);
    }
}

Shop::Shop(int numbers_of_checkouts, float checkouts_intensity,
    int max_customers_queue_length) :
    kMaxCustomersQueueLength{max_customers_queue_length},
    kNumberOfCheckouts{numbers_of_checkouts},
    is_alive_{ true }, checkouts_queue_{std::queue<Checkout*>()},
    shop_stats_{Stats()},
    customers_queue_{std::queue<Customer>()},
    checkouts_{std::vector<Checkout>()},
    kOpenTime{SystemClock::now()}
{
    for (int i = 0; i < numbers_of_checkouts; i++) {
        Checkout checkout(i, checkouts_intensity, *this, mtx_);
        checkouts_.push_back(checkout);

        std::lock_guard<std::mutex> lock(mtx_);
        shop_stats_.checkouts_work_times.push_back(FloatSeconds(0));
    }
    for (int i = 0; i < numbers_of_checkouts; i++) {
        checkouts_queue_.push(&checkouts_[i]);
    }
    queue_thread_ = std::thread(&Shop::CheckoutsControl, this);
}

void Shop::CustomerCome(Customer customer)
{
    Guard lock(mtx_);
    if (customers_queue_.size() > kMaxCustomersQueueLength) {
        shop_stats_.declined_clients_count++;
    }
    else {
        shop_stats_.accepted_clients_count++;
        customers_queue_.push(customer);
    }
}

Stats Shop::GetStats()
{
    Guard lock(mtx_);
    return shop_stats_;
}

Shop::Checkout& Shop::GetCheckout(int id)
{
    return checkouts_[id];
}

bool Shop::IsOpen()
{
    return is_alive_;
}

void Shop::Off()
{
    if (!is_alive_) return;
    bool condition = true;
    do {
        Guard lock(mtx_);
        condition = !customers_queue_.empty();
        condition = condition || checkouts_queue_.size() != kNumberOfCheckouts;
    } while (condition);
    is_alive_ = false;
    queue_thread_.join();
    for (auto& checkout : checkouts_) {
        checkout.Off();
    }
    auto end = SystemClock::now();
    shop_stats_.work_time = end - kOpenTime;
}

Shop::~Shop()
{
    Off();
}

void Shop::Checkout::Work()
{
    auto start = SystemClock::now();
    while (is_alive_) {
        auto current = 0;
        {
            Guard lock(local_mutex_);
            current = current_items_count_;
        }
        if (current <= 0) {
            start = SystemClock::now();
            std::this_thread::yield();
            continue;
        }
        auto time = std::chrono::duration<float>(1 / kIntensity);
        std::this_thread::sleep_for(time);
        
        Guard lock(local_mutex_);
        current_items_count_--;

        if (current_items_count_ == 0) {
            auto end = SystemClock::now();
            FloatSeconds time = end - start;
            {
                Guard lock(shop_mutex_);
                shop_.shop_stats_.checkouts_work_times[kId] += time;
                shop_.shop_stats_.clients_alive_time += time;
            }
            Release();
        }
        
    }
}

void Shop::Checkout::Release()
{
    Guard lock(shop_.mtx_);
    shop_.checkouts_queue_.push(this);
}



Shop::Checkout::Checkout(int id, float intensity,
    Shop& shop, std::mutex& shop_mutex) :
    kId{ id }, kIntensity{ intensity },
    shop_{ shop }, shop_mutex_{shop_mutex},
    is_alive_{ true }, current_items_count_{ 0 },
    thread_{ std::thread(&Checkout::Work, this) }
{
}

Shop::Checkout::Checkout(const Checkout& other) :
    Checkout(other.kId, other.kIntensity, other.shop_, other.shop_mutex_)
{
    Guard lock(local_mutex_);
    current_items_count_ = other.current_items_count_;
}

void Shop::Checkout::CustomerCome(int items_count)
{
    Guard lock(local_mutex_);
    current_items_count_ += items_count;
}

void Shop::Checkout::Off()
{
    if (!is_alive_) return;
    bool condition = true;
    do {
        Guard lock(local_mutex_);
        condition = current_items_count_ > 0;
        std::this_thread::yield();
    } while (condition);
    is_alive_ = false;
    thread_.join();
}

Shop::Checkout::~Checkout()
{
    Off();
}

Stats::Stats() :
    accepted_clients_count{ 0 },
    declined_clients_count{ 0 },
    clients_alive_time{ FloatSeconds(0) },
    checkouts_work_times{ std::vector<FloatSeconds>() },
    work_time { FloatSeconds(0) },
    customers_queue_length { 0 },
    customers_queue_tacts{ 0 }
{
}

void SpawnCustomers(Shop& shop, int count,
    float intensity, int avg_items_count)
{
    int temp = 0;
    for (int i = 0; i < count; i++) {
        int count = 1 + std::rand() % (2 * avg_items_count - 1);
        temp += count;
        Customer customer(count);
        shop.CustomerCome(customer);
        std::this_thread::sleep_for(FloatSeconds(1 / intensity));
    }
    std::cout << "Avg items: " << temp / count << std::endl;
}
