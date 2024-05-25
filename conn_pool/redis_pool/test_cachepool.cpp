#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <setjmp.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>
#include <sstream>

#include "ZeroThreadpool.h"
#include "CachePool.h"


using namespace std;

#define TASK_NUMBER 10000

#define DB_HOST_IP          "127.0.0.1"             // 数据库服务器ip
#define DB_HOST_PORT        6379
#define DB_INDEX            7                       // redis默认支持16个db
#define DB_PASSWORD         ""                      // 数据库密码，不设置AUTH时该参数为空
#define DB_POOL_NAME        "redis_pool"            // 连接池的名字，便于将多个连接池集中管理
#define DB_POOL_MAX_CON     4       
 

static uint64_t get_tick_count()
{
    struct timeval tval;
    uint64_t ret_tick;

    gettimeofday(&tval, NULL);

    ret_tick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
    return ret_tick;
}
 


// #define random(x) (rand()%x)

static string int2string(uint32_t user_id)
{
    stringstream ss;
    ss << user_id;
    return ss.str();
}

void *workUsePool(void *arg, int id)
{
    CachePool *pCachePool = (CachePool *)arg;
    CacheConn *pCacheConn = pCachePool->GetCacheConn();
    if (pCacheConn)
    {
        string key = "user:" + int2string(id);
        string name = "liaoqingfu-" + int2string(id);
        string ret = pCacheConn->set(key, name);
        if (ret.empty())
        {
            printf("insertUser failed\n");
        }
        pCachePool->RelCacheConn(pCacheConn);
    }
    else
    {
        printf("GetDBConn failed\n");
    }
    
    return NULL;
}

void *workNoPool(void *arg, int id)
{
    CacheConn *pCacheConn = new CacheConn(DB_HOST_IP, DB_HOST_PORT, DB_INDEX, DB_PASSWORD);
    if (pCacheConn)
    {
        string key = "user:" + int2string(id);
        string name = "liaoqingfu-" + int2string(id);
        string ret = pCacheConn->set(key, name);
        if (ret.empty())
        {
            printf("insertUser failed\n");
        }
        delete pCacheConn;
    }
    else
    {
        printf("new CacheConn failed\n");
    }

    return NULL;
}
 

// 使用连接池的测试
int testWorkUsePool(int thread_num, int db_maxconncnt, int task_num)
{
    const char *db_pool_name = DB_POOL_NAME;
    const char *db_host    =  DB_HOST_IP;
    uint16_t db_port       =  DB_HOST_PORT;
    const char *password = DB_PASSWORD;
    int db_index = DB_INDEX;

    // 每个连接池都对应一个对象
    CachePool *pCachePool = new CachePool(db_pool_name, db_host, db_port, db_index, password, db_maxconncnt);
    if (pCachePool->Init())
    {
        printf("init redis cache pool failed: %s", db_pool_name);
        return -1;
    }
    CacheConn *pCacheConn = pCachePool->GetCacheConn();
    pCacheConn->flushdb();
    pCachePool->RelCacheConn(pCacheConn);

    printf("task_num = %d, thread_num = %d, connection_num:%d, use_pool:1\n",
           task_num, thread_num, db_maxconncnt);

    ZERO_ThreadPool threadpool;
    threadpool.init(thread_num); // 设置线程数量
    threadpool.start();          // 启动线程池
    uint64_t start_time = get_tick_count();
    for (int i = 0; i < task_num; i++)
    {
        threadpool.exec(workUsePool, (void *)pCachePool, i);
    }
    cout << "need time0: " <<  get_tick_count()  - start_time << "ms\n";
    threadpool.waitForAllDone(); // 等待所有执行万再退出
    cout << "need time1: " <<  get_tick_count()  - start_time << "ms\n";
    threadpool.stop();
    cout << "need time2: " <<  get_tick_count()  - start_time << "ms\n\n";
    delete pCachePool;
    return 0;
}
// 初始化和使用连接池是一样的
int testWorkNoPool(int thread_num, int db_maxconncnt, int task_num)
{
    const char *db_pool_name = DB_POOL_NAME;
    const char *db_host    =  DB_HOST_IP;
    uint16_t db_port       =  DB_HOST_PORT;
    const char *password = DB_PASSWORD;
    int db_index = DB_INDEX;

    // 每个连接池都对应一个对象
    CachePool *pCachePool = new CachePool(db_pool_name, db_host, db_port, db_index, password, db_maxconncnt);

    if (pCachePool->Init())
    {
        printf("init redis cache pool failed: %s", db_pool_name);
        return -1;
    }

    CacheConn *pCacheConn = pCachePool->GetCacheConn();
    pCacheConn->flushdb();
    pCachePool->RelCacheConn(pCacheConn);

    printf("task_num = %d, thread_num = %d, connection_num:%d, use_pool:0\n",
           task_num, thread_num, db_maxconncnt);
    ZERO_ThreadPool threadpool;
    threadpool.init(thread_num); // 设置线程数量
    threadpool.start();          // 启动线程池
    uint64_t start_time = get_tick_count();
    for (int i = 0; i < task_num; i++)
    {
        threadpool.exec(workNoPool, (void *)pCachePool, i);  // 主要在于执行函数的区别。
    }
    cout << "need time0: " <<  get_tick_count()  - start_time << "ms\n";
    threadpool.waitForAllDone(); // 等待所有执行万再退出
    cout << "need time1: " <<  get_tick_count()  - start_time << "ms\n";
    threadpool.stop();
    cout << "need time2: " <<  get_tick_count()  - start_time << "ms\n\n";
    return 0;
}

int main(int argc, char **argv)
{
    int thread_num = 1;                  // 线程池线程数量初始化
    int db_maxconncnt = DB_POOL_MAX_CON; // 经验公式(不能硬套)-连接池最大连接数量(核数*2 + 磁盘数量)
    int task_num = TASK_NUMBER;

    int thread_num_tbl[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
    task_num = (argc > 1) ? atoi(argv[1]) : TASK_NUMBER;
    printf("testWorkUsePool\n");
    for(int i = 0; i < int(sizeof(thread_num_tbl)/sizeof(thread_num_tbl[0])); i++) {
        thread_num = thread_num_tbl[i];
        db_maxconncnt = thread_num;
        testWorkUsePool(thread_num, db_maxconncnt, task_num);
    }

    printf("\n\ntestWorkNoPool\n");
    for(int i = 0; i < int(sizeof(thread_num_tbl)/sizeof(thread_num_tbl[0])); i++) {
        thread_num = thread_num_tbl[i];
        db_maxconncnt = thread_num;
        testWorkNoPool(thread_num, db_maxconncnt, task_num);
    }  
    cout << "main finish!" << endl;
    return 0;
}