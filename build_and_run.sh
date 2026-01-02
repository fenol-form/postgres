operation="$1"

if [ -d "build" ]; then
    ./build/bin/pg_ctl -D /data stop
fi
if [ "$operation" = "from_scratch" ]; then
    rm -rf build
    if [ -d "/data" ]; then
        rm -rf /data
    fi
    mkdir build
    pushd build
    echo "Build from scratch"
else
    pushd build
fi
rm /logfile

../configure --prefix=/postgres/build --with-tpde --enable-debug --enable-cassert --enable-profiling
bear -- make world-bin -j32
make uninstall -j32
make install-world-bin -j32

## Workaround until I get how Postgres install works
cp src/backend/jit/tpde/tpdejit.so lib
cp src/backend/jit/tpde/llvmjit_types.bc lib

rm -rf /data
./bin/initdb -D /data -c log_min_messages=debug5

echo "jit_provider = 'tpdejit'" >> /data/postgresql.conf
echo "jit_above_cost = 0" >> /data/postgresql.conf
echo "jit = on" >> /data/postgresql.conf
echo "jit_optimize_above_cost = 0" >> /data/postgresql.conf
echo "jit_inline_above_cost = 0" >> /data/postgresql.conf

./bin/pg_ctl stop -D /data

./bin/pg_ctl -D /data -l /logfile start
./bin/psql -d template1 -f /postgres/pagila/pagila-schema.sql
./bin/psql -d template1 -f /postgres/pagila/pagila-data.sql
./bin/psql -d template1 -c "explain (analyze, verbose) select * from actor where first_name like '%P%';"

popd 
