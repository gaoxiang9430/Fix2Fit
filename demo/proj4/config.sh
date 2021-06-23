rm -f *.gcda
rm -f src/*.gcda
rm -rf $OUT/*
./autogen.sh &> /dev/null
CFLAGS="$CFLAGS  -fsanitize-undefined-trap-on-error -fno-sanitize=vptr -lrt -fsanitize=float-divide-by-zero" CXXFLAGS="$CXXFLAGS  -fsanitize-undefined-trap-on-error -fno-sanitize=vptr -lrt -stdlib=libstdc++ -fsanitize=float-divide-by-zero" ./configure &> config.log
make clean -s &> /dev/null
