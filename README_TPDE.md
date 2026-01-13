Build and run docker container to run Postgres with JIT TPDE engine:

```
docker build \
    --build-arg USER_ID=$(id -u) \
    --build-arg GROUP_ID=$(id -g) \
    . -t postgres-tpde:latest -f DockerfileTpdeDev
```

```
docker run \
    --mount type=bind,src=.,dst=/postgres \
    -d --network host \
    --cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
    --name postgres-tpde postgres-tpde:latest \
    sleep infinity
```
