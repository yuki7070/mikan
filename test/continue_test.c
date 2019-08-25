int main() {
    int i;
    i = 0;
    int x;
    x = 0;
    for (; i < 10; i = i + 1) {
        if (i == 5) {
            continue;
        }
        x = x + i;
    }
    return x;
}