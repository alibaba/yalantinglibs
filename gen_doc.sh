set -e
ROOT_DIR=$(pwd)
rm -rf docs
mkdir docs

cd website
rm -rf en/guide
mkdir en/guide
cd en/guide
cp ../../how-to-use-as-git-submodule.md how-to-use-as-git-submodule.md
cp ../../how-to-use-by-cmake-find_package.md how-to-use-by-cmake-find_package.md
cp ../../../README.md what-is-yalantinglibs.md
cp ../../../src/struct_pack/doc/Introduction_EN.md struct-pack-intro.md
cp ../../../src/struct_pack/doc/struct_pack_layout_EN.md struct-pack-layout.md
cp ../../../src/struct_pack/doc/struct_pack_type_system_EN.md struct-pack-type-system.md
cp ../../../src/struct_pb/doc/*.md .
cp ../../../src/coro_rpc/doc/coro_rpc_introduction_EN.md coro-rpc-intro.md

mkdir -p src/coro_rpc/doc/images
cp ../../../src/coro_rpc/doc/images/yalantinglibs_ding_talk_group.png src/coro_rpc/doc/images

rm -rf images
mkdir images
cd images
cp -r ../../../../src/struct_pack/doc/images/* .
cp -r ../../../../src/struct_pb/doc/images/* .
cp -r ../../../../src/coro_rpc/doc/images/* .
cd ../../..

rm -rf zh/guide
mkdir zh/guide
cd zh/guide
cp ../../../README.md what-is-yalantinglibs.md
cp ../../../src/struct_pack/doc/Introduction_CN.md struct-pack-intro.md
cp ../../../src/struct_pack/doc/struct_pack_type_system_CN.md struct-pack-type-system.md
cp ../../../src/struct_pack/doc/struct_pack_layout_CN.md struct-pack-layout.md
cp ../../../src/coro_rpc/doc/coro_rpc_introduction_cn.md coro-rpc-intro.md
cp -r ../../en/guide/images .
cp -r ../../en/guide/src .

# cd "$ROOT_DIR" || exit 1
cd "$ROOT_DIR"
yarn docs:build
doxygen Doxyfile
doxygen Doxyfile_cn
echo 'Generate Done!'

function handle_style(){
    echo "handle folder $1"
    for file in `ls $1`
    do
        # fix bad flag in substitute command: '/'
        fname="$1/$file"
        echo "handle file $fname"
        if [ -d $fname ]
        then
            handle_style "$1/$file"
        else
            if [[ $file = *.html ]];then
                sed -i.bak 's/class="VPContent" id="VPContent"/class="VPContent has-sidebar" id="VPContent"/g' "$fname"
            fi
        fi
    done
}
handle_style "$(pwd)/docs/zh"
echo "Fix Style Done!"