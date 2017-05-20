#!/bin/bash

VASIM=../vasim

# generate mnrl

echo "------------------------"
echo "|  Generating mnrl...  |"
echo "------------------------"

for f in *.anml; do
    base=$(basename $f ".anml")
    printf "%-40s " "$base"
    $VASIM -q -m $f
    mv automata_0.mnrl $base.mnrl
    echo "        done"
done

echo ""
echo "---------------------------"
echo "|  Testing simulation...  |"
echo "---------------------------"
# run vasim with anml and mnrl
for f in *.anml; do
    base=$(basename $f ".anml")
    
    printf "%-40s " "$base"
    
    $VASIM -q -r $f $base.input
    mv reports_0tid_0packet.txt $f.output
    
    $VASIM -q -r $base.mnrl $base.input
    mv reports_0tid_0packet.txt $base.mnrl.output
    
    if ./diff_reports.py "$f.output" "$base.mnrl.output" >& /dev/null ; then
        echo "        ok  "
    else
        echo "      FAILED"
    fi
done

echo "Testing done." 