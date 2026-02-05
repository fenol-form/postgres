operation="$1"

if [ -d "build" ]; then
    ./build/bin/pg_ctl -D data stop
fi

rm logfile

if [ "$operation" != "dont_rebuild" ]; then
    rm -rf build
    mkdir build
fi

pushd build

export CFLAGS='-O0'
../configure --prefix=/postgres/build --with-tpde --enable-profiling
bear -- make world-bin -j32
make install-world-bin -j32

## Workaround until I get how Postgres install works
cp src/backend/jit/tpde/tpdejit.so src/backend/jit/tpde/llvmjit_types.bc lib/

# echo "jit_provider = 'tpdejit'" >> data/postgresql.conf
# echo "jit_above_cost = 0" >> data/postgresql.conf
# echo "jit = on" >> data/postgresql.conf
# echo "jit_optimize_above_cost = 0" >> data/postgresql.conf
# echo "jit_inline_above_cost = 0" >> data/postgresql.conf
# echo "jit_dump_bitcode = true" >> data/postgresql.conf

popd 
