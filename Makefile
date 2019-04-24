#Makefile

#bounce: bounce.cpp
#	

all: bounce.cpp
	g++ -o bouncer bounce.cpp `pkg-config --cflags --libs ../ffmpeg/lib/pkgconfig/*.pc`

movie:
	ffmpeg -r 30 -f image2 -i ../bouncer/frame%03d.cool movie.mp4

clean:
	rm *.o *.cool bouncer *mp4