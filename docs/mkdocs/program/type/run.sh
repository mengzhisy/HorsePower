#!/bin/bash
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <function>"
    exit 1
fi
image_convert=/usr/local/Cellar/imagemagick/7.0.9-27/bin/convert
# Example: ./run.sh lt
(gcc -g -o type main.c -std=c99 && ./type rules/$1.txt > genpdf/$1.tex && cd genpdf && pdflatex $1 &> /dev/null && ${image_convert} -density 300 $1.pdf -quality 100 -resize 810x600 $1.png)
# resolution: 810x600
# resolution: 1080x800
