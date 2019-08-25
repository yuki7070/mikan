void test(int *a) {
    *a = 10;
    return;
}

int main() {
    int *b;
    int a;
    b = &a;
    test(b);
    return a;
}