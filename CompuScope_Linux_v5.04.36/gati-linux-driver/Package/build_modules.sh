#!/bin/sh

for MODULE_DIR in CsJohnDeere  CsRabbit  CsCobraMax  CsSpider  CsSplenda  CsDecade12  CsHexagon
do
    make -C Boards/KernelSpace/$MODULE_DIR/Linux $@
done
