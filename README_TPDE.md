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
    --cap-add=SYS_PTRACE \
    --cap-add=SYS_ADMIN \
    --cap-add=PERFMON \
    --security-opt seccomp=unconfined \
    --name postgres-tpde postgres-tpde:latest \
    sleep infinity
```

TPC-H Benchmarking

```
docker run \
    --mount type=bind,src=.,dst=/postgres \
    --mount type=bind,src=$(pwd)/../pg-tpch,dst=/pg-tpch \
    -d --network host \
    --cap-add=CAP_SYS_PTRACE \
    --cap-add=CAP_SYS_ADMIN \
    --cap-add=CAP_PERFMON \
    --privileged \
    --security-opt seccomp=unconfined \
    --name postgres-tpch-bench postgres-tpch-bench:latest \
    sleep infinity
```
