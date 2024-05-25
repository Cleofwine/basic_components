<<<<<<< HEAD
先生成redis的库
cd hiredis
make

编译同步、异步连接redis

gcc sync.c -o sync -I./ -L./hiredis -lhiredis
gcc async.c -o async -I./ -L./hiredis -lhiredis
=======
# 先生成redis的库
```
cd hiredis
make
```
>>>>>>> 7354773674ddb9798c618bb20a6f63931267fc65

# 编译同步、异步连接redis
```
gcc sync.c -o sync    -I./ -L./hiredis -l:libhiredis.a  
gcc async.c -o async    -I./ -L./hiredis -l:libhiredis.a 
``` 

# 运行
```
异步
./async
同步
./sync
```