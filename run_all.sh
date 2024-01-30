for i in {1..10}
do
    if [ $i -lt 10 ]; then
        test=case0$i
    else
        test=case$i
    fi
    echo -n "case0$i   |   "

    sh run.sh $test

done