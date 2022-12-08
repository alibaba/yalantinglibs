if [ $# == 0 ]
then
# for coverage, we should install
# 1. gcc
# 2. lcov 1.16
echo "use gcc && lcov"
echo "ignore"
#rm -rf .coverage
#rm -rf build
#mkdir build && cd build
#cmake -DCOVERAGE_TEST:STRING=ENABLE -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_C_COMPILER:FILEPATH=gcc -DCMAKE_CXX_COMPILER:FILEPATH=g++ ..
#make -j
#cd ..
#./build/tests/test_rpc
#rm -f .coverage/lcov.info
#rm -rf .coverage/report
#mkdir ./.coverage_lcov
#lcov --rc lcov_branch_coverage=1 --capture --directory . --output-file ./.coverage_lcov/lcov.info
#genhtml --rc lcov_branch_coverage=1 ./.coverage_lcov/lcov.info --output-directory ./.coverage_lcov/report
else
echo "use llvm-cov"
rm -rf .coverage_llvm_cov
rm -rf build
mkdir build && cd build || exit 1
export CC=clang
export CXX=clang++
cmake .. -DCOVERAGE_TEST=ON -DENABLE_SSL=ON
make -j test_rpc

# warning: test_rpc.profraw: malformed instrumentation profile data
# error: no profile can be merged
# add %m to fix the bug on ARM
# https://groups.google.com/g/llvm-dev/c/oaA58fbNMGg
# https://github.com/llvm/llvm-project/issues/50966
export LLVM_PROFILE_FILE="test_rpc-%m.profraw"
ls
cd tests
./test_rpc
llvm-profdata merge -sparse test_rpc-*.profraw -o test_rpc.profdata
llvm-cov show ./test_rpc -instr-profile=test_rpc.profdata -format=html -output-dir=../../.coverage_llvm_cov -ignore-filename-regex="async_simple|thirdparty|tests|asio|util|logging|struct_pack" -show-instantiations=false
echo 'Done!!!'
fi
