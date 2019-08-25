int main() {
    int i;
    i = 0;
    int x;
    x = 0;
    for (; i < 10; i = i + 1) {
        int j;
        j = 0;
        for (; j < 10; j = j + 1) {
            x = x + j + i;
            if (j == 5) {
                break;
            }
        }
        if (i == 2) {
            break;
        }
    }
    return x;
}