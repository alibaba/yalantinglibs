git clone https://github.com/alibaba/async_simple.git
cd async_simple
for tmp in `ls -a`
do
  if [ "$tmp" != async_simple ]
  then
    if [ "$tmp" != "." ]
    then
      if [ "$tmp" != ".." ]
          then
            rm -rf $tmp;
          fi
    fi
  fi;
done;