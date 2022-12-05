#!/bin/sh
make -f Makefile-tests clean &&
make -f Makefile-tests -j 4 &&
make -f Makefile-tests coverage
