make: renderer.cpp
	g++ renderer.cpp -o renderer_test -Iincludes -lGL -lGLU -lglut -lGLEW -lX11 -lpthread

clean: renderer.cpp
	rm renderer_test
