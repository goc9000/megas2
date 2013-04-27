#include <algorithm>

#define FIND(container, item) \
    (find((container).begin(), (container).end(), (item)))

#define CONTAINS(container, item) \
    (FIND(container, item) != (container).end())
