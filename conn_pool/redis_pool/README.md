# 项目简介

redis同步连接池



# 编译

```
mkdir build
cd build
cmake ..
make
```



# 测试连接池

注意要启动redis-server，并且要注意的是默认会清空redis db 7，请注意自己程序的数据，比如把db设置为其他值



简单测试

```
./test_cache
```



和线程池一起测试

```
./test_cachepool
```

