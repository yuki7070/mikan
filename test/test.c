int a(int n) {
  int b;
  b = 0;
  if (n == 0) {
    b = 0;
  }
  if (n == 1) {
    b = 1;
  }
  if (n == 2) {
    b = 1;
  }
  if (n > 2) {
    b = a(n-1) + a(n-2);
  }
  return b;
}

int main() {
  return a(10);
}