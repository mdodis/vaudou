#!/bin/bash
if [ -z "$1" ]; then 
	MODE="build"
else
	MODE="$1"
fi
BASEDIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )
pushd $BASEDIR

case $MODE in
	build)
		echo "Building..."
		cmake --build build
	;;

	configure)
		echo "Configuring..."
		mkdir -p ./build
		cmake -G Ninja -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=1 .
	;;
esac

popd