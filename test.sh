#!/bin/bash
try() {
    expected="$1"
    input="$2"

    ./mikan "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$expected expected, but got $actual"
        exit 1
    fi
}

try 10 "int main() { int abc = 10; return abc; }"
try 30 "int main() { int abc = 10; int def = 20; return abc+def; }"
try 10 "int main() { int a = 10; if (5 > 1) { a = 10; } else { a = 5; } return a; }"
try 5 "int main() { int a = 10; if (1 > 5) { a = 10; } else { a = 5; } return a; }"
try 10 "int main() { int a = 5; if (5 > 1) { a = 10;} return a; }"
try 10 "int test() { int a = 7; int b = 3; return a + b; } int main() { return test(); }"
try 20 "int main() { int a = 10; if (a > 5) { int b = 10; a = a + b; } else { int b = 5; a = a + b; } return a; }"
try 6 "int main() { int a = 1; if (a > 5) { int b = 10; a = a + b; } else { int b = 5; a = a + b; } return a; }"
try 10 "int test(int a, int b) { return a + b; } int main() { int a = 7; int b = 3; return test(a, b); }"
try 10 "int main() { int *a; *a = 10; return *a; }"

echo OK