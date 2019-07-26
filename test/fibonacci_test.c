int a(int n) {
    if (n <= 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
    }
    return a(n-1) + a(n-2);
}

int main() {
    return a(10);
}