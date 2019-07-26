int main() {
    int x;
    x = 0;
    int i;
    i = 0;
    for (; i < 10; i = i + 1) {
        x = i + x;
    }
    return x;
}