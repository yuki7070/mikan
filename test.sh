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

try 2 "ab = 2; return ab;"
try 14 "a = 3; b = 5 * 6 - 8; return a + b /2;"
try 10 "ab = 2; ab = 10; return ab;"
try 5 "return 5;"
try 5 "ab = 10; if (ab > 5) ab = 5; return ab;"
try 5 "ab = 3; if (4 > 5) return ab; else return 5;"
try 4 "ab = 2; if (5 == 5) return 4; else return ab;"
try 5 "a = 0; while (a < 5) a = a + 1; if (a == 5) return a;"
try 5 "a = 2; a = a + 3; return a;"
try 5 "a = 0; for (a = 0; a < 5; a = a + 1) {} return a;"
try 5 "a = 3; if (10 > 5) { a = 5; } return a;"
try 60 "a = 0; for (i = 0; i < 5; i = i + 1) { a = a + i; a = a + 10;} return a;"
try "OK\n" "return foo();"

echo OK