#include <iostream>
#include <string>
#include <vector>
#include <boost/intrusive/list.hpp>
#include "absl/container/flat_hash_map.h"
#include <bucket/Bucket.h>
#include <bitset/Bitset.h>
#include <store/OrderStore.h>
#include <OrderBook/OrderBook.h>

//using namespace boost::intrusive;
//
//
//typedef list_base_hook<link_mode<auto_unlink>> MyHook;
//
//struct Window : public MyHook {
//    uint32_t id;
//    std::string title;
//
//    Window(uint32_t id, std::string title) : id(id), title(std::move(title)) {}
//};
//
//// Тип нашего списка
//typedef list<Window, constant_time_size<false>> WindowStack;

int main() {

    
    lob::OrderStore<0, 10000, 10> store;

    // Контейнеры
    if (false) {
        //WindowStack active_windows; // Список (очередь отрисовки)
        //absl::flat_hash_map<uint32_t, Window*> window_registry; // Быстрый поиск

        //// 2. Создаем объекты (в реальности тут был бы твой PMR аллокатор)
        //Window* w1 = new Window(101, "Browser");
        //Window* w2 = new Window(102, "Terminal");
        //Window* w3 = new Window(103, "Editor");

        //// 3. ДОБАВЛЕНИЕ
        //auto add_to_system = [&](Window* w) {
        //    window_registry[w->id] = w;    // Кладем в мапу
        //    active_windows.push_back(*w);  // Кладем в список (БЕЗ АЛЛОКАЦИЙ!)
        //    };

        //add_to_system(w1);
        //add_to_system(w2);
        //add_to_system(w3);

        //// 4. ПРОХОД ПО СПИСКУ (Итерация)
        //std::cout << "Current windows stack:" << std::endl;
        //for (auto& win : active_windows) {
        //    std::cout << " - [" << win.id << "] " << win.title << std::endl;
        //}

        //// 5. УДАЛЕНИЕ ПО ID (Главная фишка)
        //auto close_window = [&](uint32_t id) {
        //    auto it = window_registry.find(id);
        //    if (it != window_registry.end()) {
        //        Window* w = it->second;

        //        // Мгновенно вырезаем из списка, не имея итератора этого списка
        //        w->unlink();

        //        // Убираем из мапы
        //        window_registry.erase(it);

        //        std::cout << "Window " << id << " closed." << std::endl;
        //        delete w;
        //    }
        //    };

        //close_window(102); // Закрываем Терминал

        //// 6. ПРОВЕРКА ПОСЛЕ УДАЛЕНИЯ
        //std::cout << "Windows after closing 102:" << std::endl;
        //for (auto& win : active_windows) {
        //    std::cout << " - " << win.title << std::endl;
        //}
    }
    return 0;
}