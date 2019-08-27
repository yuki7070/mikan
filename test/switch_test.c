int main() {
    int x;
    x = 2;
    int i;
    i = 0;

    switch (x) {
    case 0:
        i = 10;
        break;
    case 1:
    case 2:
        for (; i<10; i=i+1) {
            if (i == 5) {
                break;
            }
        }
        x = x + i;
        break;
    }
    return x;
}