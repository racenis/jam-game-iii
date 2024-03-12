src_main.o: ./src/main.cpp
	emcc -c -g -O0 -std=c++20 -I./src -Wno-undefined-var-template -I../tram-sdk/src -I../tram-sdk/libraries -I../tram-sdk/src -I../tram-sdk/libraries/bullet ./src/main.cpp -o src_main.o

src_quest.o: ./src/quest.cpp
	emcc -c -g -O0 -std=c++20 -I./src -Wno-undefined-var-template -I../tram-sdk/src -I../tram-sdk/libraries -I../tram-sdk/src -I../tram-sdk/libraries/bullet ./src/quest.cpp -o src_quest.o

clean:
	del src_main.o
	del src_quest.o

project: src_main.o src_quest.o 
	emcc -g src_main.o src_quest.o -sASSERTIONS=2 -sSAFE_HEAP=0 -sALLOW_MEMORY_GROWTH -sSTACK_OVERFLOW_CHECK=1 -sUSE_BULLET=1 -sUSE_GLFW=3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -L./ -L../tram-sdk/ -L../tram-sdk/libraries/binaries/web/ -ltramsdk -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -o jam-game-iii.html --preload-file ./