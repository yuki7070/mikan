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

try 10 "main() { abc = 10; return abc; }"
try 30 "main() { abc = 10; def = 20; return abc+def; }"
try 10 "main() { a = 10; if (5 > 1) { a = 10; } else { a = 5; } return a; }"
try 5 "main() { a = 10; if (1 > 5) { a = 10; } else { a = 5; } return a; }"
try 10 "main() { a = 5; if (5 > 1) { a = 10;} return a; }"
try 10 "test() { a = 7; b = 3; return a + b; } main() { return test(); }"
try 20 "main() { a = 10; if (a > 5) { b = 10; a = a + b; } else { b = 5; a = a + b; } return a; }"
try 6 "main() { a = 1; if (a > 5) { b = 10; a = a + b; } else { b = 5; a = a + b; } return a; }"
try 10 "test(a, b) { return a + b; } main() { a = 7; b = 3; return test(a, b); }"

echo OK