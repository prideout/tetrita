EXEC = tetrita
LIBS = -lm -lGL
IFLAGS = -I . -I source
CFLAGS = $(IFLAGS) -O3

OBJS = main.o os.x11.o game.o image.o constants.o draw.gl.o

tetrita: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

%.o: source/%.c
	$(CXX) -c $+ $(CFLAGS)

clean:
	-rm -f $(OBJS) core *~ source/*~ images/*~ *.o

clobber: clean
	-rm -f tetrita

run: tetrita
	./tetrita

release: clobber
	-rm -f ../tetrita.tar.gz
	-rm -f ../tetrita.tar
	tar -cvf ../tetrita.tar \
		../tetrita/GL/*.h \
		../tetrita/source/*.h \
		../tetrita/source/*.c \
		../tetrita/source/*.txt \
		../tetrita/images/basil1.dxt1.small.c \
		../tetrita/images/basil2.dxt1.small.c \
		../tetrita/images/basil3.dxt1.small.c \
		../tetrita/images/basil4.dxt1.small.c \
		../tetrita/images/numerals.c \
		../tetrita/images/philip.c \
		../tetrita/images/tile.small.c \
		../tetrita/images/title.dxt5.small.c \
		../tetrita/images/vera.c \
		../tetrita/images/tetrita.rc \
		../tetrita/images/tetrita.ico \
		../tetrita/Makefile \
		../tetrita/tetrita.vcproj
	gzip ../tetrita.tar
