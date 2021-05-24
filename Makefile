build:
	gcc raylib_functions_parser.c -o raylib_functions_parser

build-json: build
	./raylib_functions_parser > raylib_functions_parser.json

clean:
	rm raylib_functions_parser
	rm raylib_functions_parser.json