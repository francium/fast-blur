blur_fast: fast_blur.c ppmFile.c
	gcc fast_blur.c ppmFile.c \
		-o fast_blur \
		-std=c99 \
		-Wall \
		-flto \
		-Ofast \
		-march=native \
		-funroll-loops \
		-fwhole-program \
		-fno-signed-zeros \
		-fno-trapping-math \
		-fopenmp
