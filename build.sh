#!/bin/bash

[[ -z "$1" ]] && echo "usage: ./build.sh all|clean|project_path" && exit 1
base=$(pwd)
find . -mindepth 3 -maxdepth 3 -type d -not -path '*/\.*' -print0 | while IFS= read -r -d $'\0' dir; do
	cd "$dir"
	project_name=$(basename `pwd`)
	stripped_dir=$(echo "$dir" | cut -c 3-)

	if [ "$1" = "all" ] || [ "$stripped_dir" = "${1}" ]; then
		cp "$base/makefile" makefile
		sed -i "s@a.out@$project_name@g" makefile
		sed -i "1iPROJ_DIR ?= $base" makefile
		make
	elif [ "$1" = "clean" ]; then
		[ -f makefile ] && make clean && rm makefile
	fi
	cd "$base"
done	