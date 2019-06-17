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
try 5 "ab = 0; while (ab < 5) ab = ab + 1; if (ab == 5) return ab;"

echo OK