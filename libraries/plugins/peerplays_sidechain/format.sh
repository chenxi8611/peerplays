#!/bin/bash 
find . -regex ".*[c|h]pp" | xargs clang-format -i
