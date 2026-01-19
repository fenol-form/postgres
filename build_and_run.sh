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

# fi

export CFLAGS='-O0'
../configure --prefix=/postgres/build --with-tpde --enable-debug --enable-cassert --enable-profiling
bear -- make world-bin -j32
make install-world-bin -j32

## Workaround until I get how Postgres install works
cp src/backend/jit/tpde/tpdejit.so src/backend/jit/tpde/llvmjit_types.bc lib/

./bin/initdb -D data -c log_min_messages=debug5

./bin/pg_ctl stop -D data
./bin/pg_ctl -D data -l logfile start

# ./bin/psql -d template1 -f /postgres/pagila/pagila-schema.sql
# ./bin/psql -d template1 -f /postgres/pagila/pagila-data.sql

echo "jit_provider = 'tpdejit'" >> data/postgresql.conf
echo "jit_above_cost = 0" >> data/postgresql.conf
echo "jit = on" >> data/postgresql.conf
echo "jit_optimize_above_cost = 0" >> data/postgresql.conf
echo "jit_inline_above_cost = 0" >> data/postgresql.conf
echo "jit_dump_bitcode = true" >> data/postgresql.conf

./bin/pg_ctl restart -D data -l logfile

# possible commands to run after this point:
# ./bin/psql -d template1 -c "explain (analyze, verbose) select * from actor where first_name like '%P%';"

popd 
