
compiler = gcc -c -std=c++20 -I../tram-sdk/src/ -I../tram-sdk/libraries/ -Ilibraries
compilerlibsloc = -L../tram-sdk/ -L../tram-sdk/libraries/binaries/win64/ -Llibraries
compilerlibs = -ltramsdk -lglfw3 -lgdi32 -lopengl32 -lOpenAl32 -lBulletDynamics -lBulletCollision -lLinearMath -llua

default: main.o
	g++ -std=c++20 -o poo main.o $(compilerlibsloc) $(compilerlibs)

	
	
clean:
	del main.o

main.o: src/main.cpp
	$(compiler) src/main.cpp -o main.o
	