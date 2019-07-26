#!/bin/bash
try() {
    expected="$1"
    input="$2"

    ./mikan "$input" > tmp.s
    gcc -static -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$expected expected, but got $actual"
        exit 1
    fi
}

try 10 "int main() { int abc; abc = 10; return abc; }"
try 10 "int main() { int a; a = 5; int b; b = 2; return a*b; }"
try 30 "int main() { int abc; abc = 10; int def; def = 20; return abc+def; }"
try 10 "int main() { int a; a = 10; if (5 > 1) { a = 10; } else { a = 5; } return a; }"
try 5 "int main() { int a; a = 10; if (1 > 5) { a = 10; } else { a = 5; } return a; }"
try 10 "int main() { int a; a = 5; if (5 > 1) { a = 10;} return a; }"
try 10 "int test() { int a; a = 7; int b; b = 3; return a + b; } int main() { return test(); }"
try 20 "int main() { int a; a = 10; if (a > 5) { int b; b = 10; a = a + b; } return a; }"
try 6 "int main() { int a; a = 1; if (a > 5) { int b; b = 10; a = a + b; } else { int b; b = 5; a = a + b; } return a; }"
try 10 "int test(int a, int b) { return a + b; } int main() { int a; a = 7; int b; b = 3; return test(a, b); }"
try 10 "int main() { int x; x = 10; int *y; y = &x; return *y;}"
try 4 "int main() { int x; x = sizeof(x); return x; }"
try 8 "int main() { int *x; return sizeof(x); }"
try 10 "int main() { int x[10]; int y; y = 10; return y; }"
try 2 "int main() { int a[2]; *a = 1; *(a+1) = 2; return *(a+1); }"
try 2 "int main() { int a[2]; *a = 1; *(a+1) = 2; int *p; p = a; return *(p+1); }"
try 3 "int main() { int a[2]; *a = 1; *(a+1) = 2; int *p; p = a; return *p+*(p+1); }"
try 3 "int main() { int a[2]; *a = 1; a[1] = 2; int *p; p = a; return *p+p[1]; }"
try 5 "int x; int main() { int y; x = 3; y = 2; return x+y; }"
try 3 "int x; int test() { x = 3; return x; } int main() { int x; x = 5; return test(); }"
try 3 "int main() { char x[3]; x[0] = -1; x[1] = 2; int y; y = 4; return x[0] + y; }"
try 55 "int a(int n) { int b; b = 0; if (n == 0) { b = 0; } if (n == 1) { b = 1; } if (n == 2) { b = 1;} if (n > 2) { b = a(n-1) + a(n-2); } return b; } int main() { return a(10); }"
try 55 "test/test.c"
try 45 "test/for1_test.c"
try 45 "test/while1_test.c"
try 55 "test/fibonacci_test.c"

echo OK