#!/bin/bash

[[ -z "$1" ]] && echo "usage: ./build.sh all|clean|project_name" && exit 1
find . -mindepth 1 -maxdepth 1 -type d -print0 | while IFS= read -r -d $'\0' dir; do
	cd "$dir"
	stripped_dir=$(echo "$dir" | cut -c 3-)

	if [ "$1" = "all" ]; then
		cp ../makefile makefile
		sed -i "s/a.out/$stripped_dir/g" makefile
		make
	elif [ "$1" = "clean" ]; then
		[ -f makefile ] && make clean && rm makefile
	elif [ "$stripped_dir" = "$1" ]; then
		cp ../makefile makefile
		make
	fi
	cd ../
done	