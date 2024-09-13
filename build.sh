#!/bin/bash

if ! command -v pkg-config &> /dev/null
then
    echo "pkg-config could not be found. Please install it."
    exit 1
fi

gcc -w -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL utils/Raylib/libraylib.a main.c -o main 
if [ $? -eq 0 ]; then
    echo "Compilation successful. Running the application..." 
    ./main
    rm main 
    clear
else
    echo "Compilation failed. Please check for errors."
fi
echo "Program successfully terminated."
