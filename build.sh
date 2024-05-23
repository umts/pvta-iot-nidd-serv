#!/bin/sh

dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)

# Reset in case getopts has been used previously in the shell
OPTIND=1
while getopts "dprm" opt; do
  case "$opt" in
    d)
      debug=1
      ;;
    p)
      prune=1
      ;;
    r)
      docker_run=1
      ;;
    m)
      if [ $debug ]; then
        echo "Minsize image doesn't support debugging; ignoring debug flag"
        echo
      fi
      minsize=1
      ;;
  esac
done
shift $((OPTIND-1))

cd $dir

(cd $dir/modules/vzw-nidd-front-end && bundle exec middleman build --clean)
rm -rf ./assets
mv $dir/modules/vzw-nidd-front-end/build $dir/assets

if [ $minsize ]; then
  docker build -t iots:dev --secret id=vzw_secrets.h,src=$dir/config/vzw_secrets.h -f $dir/Dockerfile.minsize $dir
elif [ $debug ]; then
  docker build -t iots:dev --secret id=vzw_secrets.h,src=$dir/config/vzw_secrets.h --build-arg debug=true $dir
else
  docker build -t iots:dev --secret id=vzw_secrets.h,src=$dir/config/vzw_secrets.h $dir
fi

if [ $prune ]; then
  docker container prune -f && docker image prune -f && docker buildx prune -f
fi

if [ $docker_run ]; then
  docker run --rm -p 80:80 --name iots_dev iots:dev
fi

cd -
