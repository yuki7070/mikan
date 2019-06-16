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

try 0 0
try 42 42
try 21 '5+20-4'
try 41 " 12 + 34 - 5 "
try 47 "5+6*7"
try 15 "5*(9-6)"
try 4 "(3+5)/2"
try 10 "-10+20"
try 0 "10 == 5"
try 1 "10 == 10"
try 0 "10 != 10"
try 1 "10 != 5"
try 1 "10 < 20"
try 0 "20 < 20"
try 1 "20 <= 20"
try 0 "20 <= 10"
try 1 "10 > 5"
try 0 "5 > 5"
try 1 "10 >= 10"
try 0 "10 >= 20"

echo OK