all:
	g++ -I src/include -L src/lib -o group3_race main.cpp -lmingw32 -lSDL3 -lSDL3_ttf