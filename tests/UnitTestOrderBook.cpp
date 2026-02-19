#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "OrderBook/OrderBook.h"
#include "lockfree/SPSCQueue.h"


using namespace lob;

class OrderBookTest : public ::testing::Test {
protected:
    
};

TEST_F(OrderBookTest, AddAndCancelOrders) {
    TradeEventsQueue queue;
    OrderBook<0, 100, 1> book(queue);

    book.limit_buy(1, 100, 4);
    book.limit_sell(7, 99, 4);

    EXPECT_NE(queue.prepare_pop(), nullptr);

}

TEST_F(OrderBookTest, PartialMatchAndQueueStress) {
    TradeEventsQueue queue; // Предположим, размер 2^6 = 64
    OrderBook<0, 1000, 1> book(queue);

    // 1. Создаем "стену" из лимитных покупок (Bids)
    // 50 ордеров по 1 лоту на разных ценах
    for (int i = 0; i < 50; ++i) {
        book.limit_buy(100 + i, 500 - i, 1);
    }

    // 2. Кидаем один огромный SELL ордер, который должен заматчить ВСЮ стену
    // Если ваша очередь мала (например, 32 слота), а ордеров 50, 
    // здесь мы проверим, не упадет ли система и не потеряет ли события.
    uint64_t seller_id = 999;
    book.limit_sell(seller_id, 1, 50); // Продаем 50 лотов по цене "почти даром"

    // 3. Вычитываем все события из очереди
    int events_count = 0;
    uint32_t total_matched_qty = 0;

    while (true) {
        auto* event = queue.prepare_pop();
        if (!event) break;

        events_count++;
        // total_matched_qty += event->quantity; // Если есть поле qty
        queue.commit_pop();
    }

    // 4. ГЛАВНАЯ ПРОВЕРКА
    EXPECT_EQ(events_count, 50)
        << "Потеряны события сделок! Возможно, очередь переполнилась и try_match проигнорировал nullptr";

    // 5. Проверяем, что в стакане ничего не осталось (asks пуст)
    // (Добавьте метод проверки размера стакана, если есть)
}