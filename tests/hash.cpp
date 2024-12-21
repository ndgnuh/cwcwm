#include <boost/unordered/unordered_flat_map.hpp>
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
using namespace std;

static int TABLE_SIZE;

// boost::unordered::unordered_flat_map<string, char*> umap;
unordered_map<string, char*> umap;

void setup_data()
{
    srand(69);

    for (int i = 0; i < TABLE_SIZE; i++) {
        string j = to_string(i);
        umap[j] = nullptr;
    }
}

void destroy_data()
{
    for (int i = 0; i < TABLE_SIZE; ++i) {
        string key = to_string(i);
        umap.erase(key);
    }

    for (int i = 0; i < TABLE_SIZE; ++i) {
        string key = to_string(i);
        assert(umap[key] == NULL);
    }
}

void basic_perf()
{
    // string cache[MAX_LOOP];
    for (int i = 0; i < TABLE_SIZE; ++i) {
        // cache[i] = to_string(i);
        // umap[cache[i]] = nullptr;

        string j = to_string(i);
        umap[j] = (char *)(long)i;
    }

    for (int i = 0; i < TABLE_SIZE; ++i) {
        // string elem = cache[i];
        // char* value = umap[cache[i]];
        // assert(value == nullptr);

        string elem = to_string(i);
        char* value = umap[elem];
        assert(value == (char *)i);
    }

    for (int i = 0; i < TABLE_SIZE; ++i) {
        // umap.erase(cache[i]);

        string elem = to_string(i);
        umap.erase(elem);
    }

    for (int i = 0; i < TABLE_SIZE; ++i) {
        // string elem = cache[i];
        // char* value = umap[cache[i]];
        // assert(value == nullptr);

        string elem = to_string(i);
        char* value = umap[elem];
        assert(value == nullptr);
    }
}

void repeated_read()
{

    // seq
    for (int i = 0; i < TABLE_SIZE; i++) {
        string j = to_string(i);
        umap[j] = nullptr;
    }

    // rand
    for (int i = 0; i < TABLE_SIZE; i++) {
        string j = to_string(rand() % TABLE_SIZE);
        char* val = umap[j];
    }
}

int main()
{
    TABLE_SIZE = 1e7;
    setup_data();
    for (int i = 0; i < 5; i++) {
        // basic_perf();
        repeated_read();
    }

    destroy_data();
    return 0;
}
