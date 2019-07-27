int main() {
    int x;
    int i;
    x = 0;
    i = 0;
    for (; i < 10; i = i + 1) {
        int j;
        j = 0;
        for (; j < 10; j = j + 1) {
            x = i+j;
        }
    }
    return x;
}