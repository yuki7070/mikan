int set(int *arr) {
    int i;
    i = 0;
    for (; i < 10; i = i + 1) {
        *(arr + i) = i;
    }
    return 0;
}

int add(int *arr) {
    int x;
    x = 0;
    int i;
    i = 0;
    for (; i < 10; i = i + 1) {
        x = x + *(arr + i);
    }
    return x;
}

int main() {
    int arr[10];
    set(arr);
    return add(arr);
}