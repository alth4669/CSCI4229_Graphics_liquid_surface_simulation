# Final Project
EXE=project

# Main target
all: $(EXE)

#  MinGW
ifeq "$(OS)" "Windows_NT"
CFLG=-O3 -Wall
LIBS=-lglut32cu -lglu32 -lopengl32
CLEAN=del *.exe *.o *.a
else
#  OSX
ifeq "$(shell uname)" "Darwin"
CFLG=-O3 -Wall -Wno-deprecated-declarations
LIBS=-framework GLUT -framework OpenGL
#  Linux/Unix/Solaris
else
CFLG=-O3 -Wall
LIBS=-lglut -lGLU -lGL -lm
endif
#  OSX/Linux/Unix/Solaris
CLEAN=rm -f $(EXE) *.o *.a
endif

# Dependencies
project.o: project.c texLoad.h
fatal.o: fatal.c texLoad.h
loadtexbmp.o: loadtexbmp.c texLoad.h
loadcubetexbmp.o: loadcubetexbmp.c texLoad.h
print.o: print.c texLoad.h
errcheck.o: errcheck.c texLoad.h

#  Create archive
texLoad.a:fatal.o loadtexbmp.o loadcubetexbmp.o print.o errcheck.o 
	ar -rcs $@ $^

# Compile rules
.c.o:
	gcc -c $(CFLG) $<
.cpp.o:
	g++ -c $(CFLG) $<

#  Link
project:project.o texLoad.a
	gcc -O3 -o $@ $^   $(LIBS)

#  Clean
clean:
	$(CLEAN)