#!/bin/bash
lcov --capture --directory . --output-file coverage-gcov.info --no-external
lcov --output-file coverage-gcov.info --remove coverage-gcov.info 'moc_*.cpp' '*.moc*' '.*rcc*' '*3rdparty*'
genhtml coverage-gcov.info --output-directory doc/coverage
