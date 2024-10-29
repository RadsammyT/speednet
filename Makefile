#Remember to retab!

ifndef VERB
    .SILENT:
endif

linuxLibs:
	echo "---rlImGui---"
	cd libs/out/linux && \
		c++ \
		-c ../../rayImGui/*.cpp &&\
		ar rcs librlimgui.a *.o \
	&& cd ../../..
	echo "---frustum---"
	cd libs/out/linux && \
		cc \
		-c ../../frustum/*.c &&\
		ar rcs libfrustum.a *.o \
	&& cd ../../..

linux: # This target assumes you have raylib installed onto /usr/local/(bin/include).
	echo "---linux---"
	c++ \
		src/*.cpp \
	-Llibs/out/linux \
	-lrlimgui \
	-lraylib \
	-lfrustum \
	-Ilibs \
	-o bin/linux/game \
	-DPLATFORM_DESKTOP \
	$(ADDITIONAL_FLAGS)

webLibs:
	echo "---rlImGui---"
	cd libs/out/web && \
		emcc -c ../../rayImGui/*.cpp \
		-I. -I../../../raylib/src -Ilibs \
		&& emar rcs librlimgui.a *.o \
	&& cd ../../.. 
	echo "---frustum---"
	cd libs/out/web && \
		emcc -c ../../frustum/*.c\
		-I. -I../../../raylib/src -Ilibs \
		&& emar rcs libfrustum.a *.o \
	&& cd ../../.. 

web:
	echo "---web---"
	emcc src/*.cpp \
	-o bin/web/game.html \
	-O3 -Wall -std=c++20 \
	./raylib/src/libraylib.a \
	./libs/out/web/librlimgui.a \
	./libs/out/web/libfrustum.a \
	-I. -I./raylib/src -Ilibs \
	-L. -L./raylib/src -Llibs/out/web \
	-s USE_GLFW=3 -s ASYNCIFY \
	-sGL_ENABLE_GET_PROC_ADDRESS \
	-s TOTAL_MEMORY=256MB \
	--preload-file res/web/ \
	-s EXPORTED_RUNTIME_METHODS=ccall \
	--shell-file=./raylib/src/minshell.html \
	-DPLATFORM_WEB $(ADDITIONAL_FLAGS)

compileRaylibWeb:
	echo "---raylib-web---"
	cd raylib/src && \
	make PLATFORM=PLATFORM_WEB && \
	cd ../../..

