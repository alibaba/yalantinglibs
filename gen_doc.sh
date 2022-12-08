ROOT_DIR=$(pwd)
rm -rf docs
mkdir docs

cd website
rm -rf guide
mkdir guide
cd guide
cp ../../README.md what-is-yalantinglibs.md
cp ../../src/struct_pack/doc/Introduction_en.md struct-pack-intro.md
cp ../../src/coro_rpc/doc/coro_rpc_introduction_en.md coro-rpc-intro.md

mkdir -p src/coro_rpc/doc/images
cp ../../src/coro_rpc/doc/images/yalantinglibs_ding_talk_group.png src/coro_rpc/doc/images

rm -rf images
mkdir images
cd images
cp -r ../../../src/struct_pack/doc/images/* .
cp -r ../../../src/coro_rpc/doc/images/* .
cd ../..

rm -rf zh
mkdir zh
cd zh
mkdir guide
cd guide
cp ../../../README.md what-is-yalantinglibs.md
cp ../../../src/struct_pack/doc/Introduction_CN.md struct-pack-intro.md
cp ../../../src/struct_pack/doc/struct_pack_type_system_CN.md struct-pack-type-system.md
cp ../../../src/struct_pack/doc/struct_pack_layout_CN.md struct-pack-layout.md
cp ../../../src/coro_rpc/doc/coro_rpc_introduction_cn.md coro-rpc-intro.md
cp -r ../../guide/images .
cp -r ../../guide/src .

cd "$ROOT_DIR" || exit 1
yarn docs:build
doxygen Doxyfile
doxygen Doxyfile_cn
echo 'Done!'
