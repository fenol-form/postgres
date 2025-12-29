Build and run docker container to run Postgres with JIT TPDE engine:

```
docker build . -t postgres-tpde:latest -f DockerfileTpdeDev
```

```
docker run \
    --mount type=bind,src=.,dst=/postgres \
    -d --network host \
    --name postgres-tpde postgres-tpde:latest \
    sleep infinity
```