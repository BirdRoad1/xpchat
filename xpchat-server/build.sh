#!/bin/bash

cmake .
make
strip --strip-unneeded bin/xpchatterserver