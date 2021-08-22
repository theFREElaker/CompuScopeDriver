#!/bin/sh

./configure
make
for MODULE_DIR in CsJohnDeere  CsRabbit  CsSpider  CsSplenda  CsDecade12  CsHexagon
do
    make -C Boards/KernelSpace/$MODULE_DIR/Linux
done
