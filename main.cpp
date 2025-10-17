#include <vector>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <random>
#include <cassert>

struct Node {
    std::vector<uint8_t> key;
    std::vector<uint8_t> value;
    Node *left;
    Node *right;
    Node *parent;
    bool is_red;

    Node(const std::vector<uint8_t> &k, const std::vector<uint8_t> &v)
        : key(k), value(v), left(nullptr), right(nullptr), parent(nullptr), is_red(true) {
    }

    ~Node() = default;
};

int compare_keys(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b) {
    int min_len = (a.size() < b.size()) ? a.size() : b.size();
    for (int i = 0; i < min_len; i++) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
    }
    if (a.size() < b.size()) {
        return -1;
    }
    if (a.size() > b.size()) {
        return 1;
    }
    return 0;
}

class RedBlackTree {
private:
    Node *root;

    static Node *find_node(Node *node, const std::vector<uint8_t> &key) {
        while (node != nullptr) {
            int cmp = compare_keys(key, node->key);
            if (cmp == 0) {
                return node;
            }
            if (cmp < 0) {
                node = node->left;
            } else {
                node = node->right;
            }
        }
        return nullptr;
    }

    void rotate_left(Node *node) {
        Node *right_child = node->right;
        node->right = right_child->left;
        if (right_child->left != nullptr) {
            right_child->left->parent = node;
        }
        right_child->parent = node->parent;
        if (node->parent == nullptr) {
            root = right_child;
        } else if (node == node->parent->left) {
            node->parent->left = right_child;
        } else {
            node->parent->right = right_child;
        }
        right_child->left = node;
        node->parent = right_child;
    }

    void rotate_right(Node *node) {
        Node *left_child = node->left;
        node->left = left_child->right;
        if (left_child->right != nullptr) {
            left_child->right->parent = node;
        }
        left_child->parent = node->parent;
        if (node->parent == nullptr) {
            root = left_child;
        } else if (node == node->parent->right) {
            node->parent->right = left_child;
        } else {
            node->parent->left = left_child;
        }
        left_child->right = node;
        node->parent = left_child;
    }

    void fix_insert(Node *node) {
        while (node->parent != nullptr && node->parent->is_red) {
            if (node->parent == node->parent->parent->left) {
                Node *uncle = node->parent->parent->right;
                if (uncle != nullptr && uncle->is_red) {
                    node->parent->is_red = false;
                    uncle->is_red = false;
                    node->parent->parent->is_red = true;
                    node = node->parent->parent;
                } else {
                    if (node == node->parent->right) {
                        node = node->parent;
                        rotate_left(node);
                    }
                    node->parent->is_red = false;
                    node->parent->parent->is_red = true;
                    rotate_right(node->parent->parent);
                }
            } else {
                Node *uncle = node->parent->parent->left;
                if (uncle != nullptr && uncle->is_red) {
                    node->parent->is_red = false;
                    uncle->is_red = false;
                    node->parent->parent->is_red = true;
                    node = node->parent->parent;
                } else {
                    if (node == node->parent->left) {
                        node = node->parent;
                        rotate_right(node);
                    }
                    node->parent->is_red = false;
                    node->parent->parent->is_red = true;
                    rotate_left(node->parent->parent);
                }
            }
        }
        root->is_red = false;
    }

    static void delete_tree(const Node *node) {
        if (node == nullptr)
            return;
        delete_tree(node->left);
        delete_tree(node->right);
        delete node;
    }

public:
    RedBlackTree() : root(nullptr) {
    }

    ~RedBlackTree() {
        delete_tree(root);
    }

    void put(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value) {
        if (root == nullptr) {
            root = new Node(key, value);
            root->is_red = false;
            return;
        }

        Node *parent = nullptr;
        Node *current = root;
        int cmp = 0;

        while (current != nullptr) {
            parent = current;
            cmp = compare_keys(key, current->key);
            if (cmp == 0) {
                current->value = value;
                return;
            }
            if (cmp < 0) {
                current = current->left;
            } else {
                current = current->right;
            }
        }

        Node *new_node = new Node(key, value);
        new_node->parent = parent;

        if (parent != nullptr) {
            cmp = compare_keys(key, parent->key);
            if (cmp < 0) {
                parent->left = new_node;
            } else {
                parent->right = new_node;
            }
        }

        fix_insert(new_node);
    }

    bool get(const std::vector<uint8_t> &key, std::vector<uint8_t> &out_value) const {
        Node *node = find_node(root, key);
        if (node != nullptr) {
            out_value = node->value;
            return true;
        }
        return false;
    }
};

class ConcurrentRedBlackTree {
private:
    RedBlackTree tree;
    mutable std::shared_mutex mutex;

public:
    ConcurrentRedBlackTree() = default;

    void put(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value) {
        std::unique_lock lock(mutex);
        tree.put(key, value);
    }

    bool get(const std::vector<uint8_t> &key, std::vector<uint8_t> &out_value) const {
        std::shared_lock lock(mutex);
        return tree.get(key, out_value);
    }
};

void test_concurrent_writes() {
    printf("Test 1: Concurrent Writes\n");
    ConcurrentRedBlackTree tree;
    const int num_threads = 8;
    const int ops_per_thread = 1000;

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&tree, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; i++) {
                std::vector<uint8_t> key = {
                    static_cast<uint8_t>(t),
                    static_cast<uint8_t>(i >> 8),
                    static_cast<uint8_t>(i & 0xFF)
                };
                std::vector<uint8_t> value = {
                    static_cast<uint8_t>(t * 100 + i)
                };
                tree.put(key, value);
            }
        });
    }

    for (auto &thread: threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Verify all keys are present
    int verified = 0;
    for (int t = 0; t < num_threads; t++) {
        for (int i = 0; i < ops_per_thread; i++) {
            std::vector<uint8_t> key = {
                static_cast<uint8_t>(t),
                static_cast<uint8_t>(i >> 8),
                static_cast<uint8_t>(i & 0xFF)
            };
            std::vector<uint8_t> result;
            assert(tree.get(key, result) == true);
            verified++;
        }
    }

    printf("%d concurrent writes completed in %lld ms\n", num_threads * ops_per_thread, duration.count());
    printf("All %d keys verified\n\n", verified);
}

void test_concurrent_reads() {
    printf("Test 2: Concurrent Reads\n");
    ConcurrentRedBlackTree tree;
    const int num_keys = 1000;
    const int num_reader_threads = 16;
    const int reads_per_thread = 10000;

    // Pre-populate tree
    for (int i = 0; i < num_keys; i++) {
        std::vector<uint8_t> key = {
            static_cast<uint8_t>(i >> 8),
            static_cast<uint8_t>(i & 0xFF)
        };
        std::vector<uint8_t> value = {
            static_cast<uint8_t>(i)
        };
        tree.put(key, value);
    }

    std::atomic<int> successful_reads{0};
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int t = 0; t < num_reader_threads; t++) {
        threads.emplace_back([&tree, &successful_reads, num_keys, reads_per_thread, t]() {
            for (int i = 0; i < reads_per_thread; i++) {
                int idx = (t * 7919 + i * 31) % num_keys;
                std::vector<uint8_t> key = {
                    static_cast<uint8_t>(idx >> 8),
                    static_cast<uint8_t>(idx & 0xFF)
                };
                std::vector<uint8_t> result;
                if (tree.get(key, result)) {
                    ++successful_reads;
                }
            }
        });
    }

    for (auto &thread: threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    printf("%d concurrent reads completed in %lld ms\n",
           num_reader_threads * reads_per_thread, duration.count());
    printf("%d successful reads\n\n", successful_reads.load());
}

void test_mixed_read_write() {
    printf("Test 3: Mixed Read/Write Operations\n");
    ConcurrentRedBlackTree tree;
    const int num_writer_threads = 4;
    const int num_reader_threads = 12;
    const int ops_per_thread = 5000;

    std::atomic<int> writes{0};
    std::atomic<int> reads{0};

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    // Writer threads
    for (int t = 0; t < num_writer_threads; t++) {
        threads.emplace_back([&, t]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);

            for (int i = 0; i < ops_per_thread; i++) {
                std::vector<uint8_t> key = {
                    static_cast<uint8_t>(dis(gen)),
                    static_cast<uint8_t>(dis(gen))
                };
                std::vector<uint8_t> value = {
                    static_cast<uint8_t>(t),
                    static_cast<uint8_t>(i & 0xFF)
                };
                tree.put(key, value);
                ++writes;
            }
        });
    }

    // Reader threads
    for (int t = 0; t < num_reader_threads; t++) {
        threads.emplace_back([&]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);

            for (int i = 0; i < ops_per_thread; i++) {
                std::vector<uint8_t> key = {
                    static_cast<uint8_t>(dis(gen)),
                    static_cast<uint8_t>(dis(gen))
                };
                std::vector<uint8_t> result;
                tree.get(key, result);
                ++reads;
            }
        });
    }

    for (auto &thread: threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    printf("Mixed operations completed in %lld ms\n", duration.count());
    printf("Writes: %d, Reads: %d, Ops/ms: %lld\n\n", writes.load(), reads.load(),
           (writes.load() + reads.load()) / duration.count());
}

int main() {
    printf("=== Concurrent Red-Black Tree Test ===\n\n");

    test_concurrent_writes();
    test_concurrent_reads();
    test_mixed_read_write();

    printf("=== All Tests Passed! ===\n");

    return 0;
}
