#!/bin/zsh
function exit_if_error() {
	EXIT_CODE=$1
	if [[ $EXIT_CODE != 0 ]]; then 
		echo "---- Exiting build script due to error above ----"
		exit $EXIT_CODE
	fi
}

if [[ -n "`ls build`" ]]; then
	echo "---- Cleaning up build... ----"
	rm "build/"*
fi

echo "---- Compiling modules... ----"
g++ src/compiler.cpp -o build/compiler.o -c -ggdb 
exit_if_error $?
g++ src/main.cpp -o build/main.o -c -ggdb
exit_if_error $?
echo "---- Building riddical... ----"
g++ build/compiler.o build/main.o -o build/riddical -I src/
exit_if_error $?
