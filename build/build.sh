rm *.o
rm a.out
g++ -std=c++1y ../src/parser/*.cpp -c -g
g++ -std=c++1y ../src/event_loop/*.cpp -c -g
g++ -std=c++1y ../main.cpp -c -g
g++ *.o -o a.out -lev -g
