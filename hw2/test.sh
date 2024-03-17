#!/bin/bash
echo Running Make clean
make clean
echo
echo Running Make
make
echo
echo

echo Running tests from tests folder - rates are different for 3 tests 
echo
(( all = 0 ))
(( failed = 0 ))

for filename in tests/test*.command; do
    test_num=`echo $filename | cut -d'.' -f1`
    bash ${filename} > ${test_num}.YoursOut
    diff_result=$(diff ${test_num}.out ${test_num}.YoursOut)
    (( all++ ))
    if [ "$diff_result" != "" ]; then
        echo The test ${test_num} didnt pass
        echo ${diff_result}
        (( failed++ ))
    fi
done

echo failed ${failed} out of ${all}
echo
echo
echo Running tests from tests_t folder - times are different for 3 tests 
echo
(( all = 0 ))
(( failed = 0 ))
for filename in tests_t/test*.command; do
    test_num=`echo $filename | cut -d'.' -f1`
    bash ${filename} > ${test_num}.YoursOut
    diff_result=$(diff ${test_num}.out ${test_num}.YoursOut)
    (( all++ ))
    if [ "$diff_result" != "" ]; then
        echo The test ${test_num} didnt pass
        echo ${diff_result}
        (( failed++ ))
    fi
done
echo failed ${failed} out of ${all}
echo

echo
echo Running tests from examples folder - need to pass all 3
echo
(( all = 0 ))
(( failed = 0 ))
for filename in examples/example*.command; do
    test_num=`echo $filename | cut -d'.' -f1`
    bash ${filename} > ${test_num}.YoursOut
    diff_result=$(diff ${test_num}.out ${test_num}.YoursOut)
    (( all++ ))
    if [ "$diff_result" != "" ]; then
        echo The test ${test_num} didnt pass
        echo ${diff_result}
        (( failed++ ))
    fi
done
echo failed ${failed} out of ${all}
echo


echo
echo Ran all tests.
