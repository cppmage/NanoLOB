#pragma once
#include <array>



static constexpr size_t MAX_LEVEL = 25;

template <typename Derived>
struct skip_hook {
	std::array<Derived*, MAX_LEVEL> prev;
	std::array<Derived*, MAX_LEVEL> next;
	skip_hook() {
		next.fill(nullptr);
		prev.fill(nullptr);
	}
	void unlink() {
		for (int i = 0; i < MAX_LEVEL and prev[i]!=nullptr; i++) {
			prev[i]->next[i] = next[i];
			next[i]->prev[i] = prev[i];
		}
		next.fill(nullptr);
		prev.fill(nullptr);
	}
};

struct test_node : public skip_hook<test_node> {
	int key=0;
	test_node() = default;
	friend bool operator<(const test_node& a, const test_node& b) {
		return a.key < b.key;
	}
	friend bool operator>(const test_node& a, const test_node& b) {
		return a.key > b.key;
	}
};

using node_t = test_node;
template<typename Compare = std::greater<node_t>>
class IntrusiveSkiplist {
private:
	Compare cmp;
	node_t head;
	node_t tail;
	uint32_t fast_rand() {
		static thread_local uint32_t x = 123456789;
		x ^= x << 13;
		x ^= x >> 17;
		x ^= x << 5;
		return x;
	}
	uint8_t generate_height() {
		uint8_t h = 1;
		while ((fast_rand() & 1) && h < MAX_LEVEL) {
			h++;
		}
		return h;
	}

public:
	IntrusiveSkiplist(Compare c = Compare()) : cmp(c) {
		for (size_t i = 0; i < MAX_LEVEL; i++) {
			head.next[i] = &tail;
			tail.prev[i] = &head;
		}
	}

	void insert(node_t* node) {
		std::array<node_t*, MAX_LEVEL> update;
		int level = MAX_LEVEL - 1;
		node_t* cur_node = &head;
		while (level>=0) {

			if (!cmp(*(node), *(cur_node->next[level])) or cur_node->next[level] == &tail) {
				update[level] = cur_node;
				level--;
			}
			else {
				cur_node = cur_node->next[level];
			}
			
		}

		uint8_t height = generate_height();
		for (uint8_t i = 0; i < height; ++i) {
			node->next[i] = update[i]->next[i];
			node->prev[i] = update[i];

			update[i]->next[i]->prev[i] = node;
			update[i]->next[i] = node;
		}

	}

	node_t* front() {
		return head.next[0];
	}
	node_t* back() {
		return tail.prev[0];
	}


};