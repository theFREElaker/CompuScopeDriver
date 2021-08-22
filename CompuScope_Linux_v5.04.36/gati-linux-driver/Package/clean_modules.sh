#!/bin/sh
fullpath=$(dirname $0)
cd $fullpath >/dev/null

for MODULE_DIR in CsJohnDeere CsRabbit CsCobraMax CsSpider CsSplenda CsDecade12 CsHexagon ; do
    make -C Boards/KernelSpace/$MODULE_DIR/Linux clean >/dev/null
done

cd - >/dev/null
