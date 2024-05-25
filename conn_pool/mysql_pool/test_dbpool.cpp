#include <iostream>
#include "DBPool.h"
#include "ZeroThreadpool.h"
#include "IMUser.h"

using namespace std;


// 真正测试连接池的代码起始
#define TASK_NUMBER 1000

#define DB_HOST_IP "127.0.0.1" // 数据库服务器ip
#define DB_HOST_PORT 3306
#define DB_DATABASE_NAME "mysql_pool_test" // 数据库对应的库名字, 这里需要自己提前用命令创建完毕
#define DB_USERNAME "root"                 // 数据库用户名
#define DB_PASSWORD "123456"               // 数据库密码
#define DB_POOL_NAME "mysql_pool"          // 连接池的名字，便于将多个连接池集中管理
#define DB_POOL_MAX_CON 4                  // 连接池支持的最大连接数量

// #define DB_HOST_IP          "114.215.169.66"             // 数据库服务器ip
// #define DB_HOST_PORT        3306
// #define DB_DATABASE_NAME    "mysql_pool_test"       // 数据库对应的库名字, 这里需要自己提前用命令创建完毕
// #define DB_USERNAME         "root"                  // 数据库用户名
// #define DB_PASSWORD         "01222t"                // 数据库密码
// #define DB_POOL_NAME        "mysql_pool"            // 连接池的名字，便于将多个连接池集中管理
// #define DB_POOL_MAX_CON     4                       // 连接池支持的最大连接数量

static uint64_t get_tick_count()
{
    struct timeval tval;
    uint64_t ret_tick;

    gettimeofday(&tval, NULL);

    ret_tick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
    return ret_tick;
}

//    任务耗时/(任务耗时+调度耗时) = cpu有效比值， 值越小，那开太多的线程会影响较大； 值越大，则多开线程影响也不大太大
// 用于测试cpu密集型
 void *workUsePool3(void *arg, int id)  
 {
    for(int i = 0; i < 10000; i++) {
        for(int j= 0; j < 1000; j++);
    }
    return NULL;
 }
// 用于测试cpu密集型 对比任务 耗时不同对线程数量的影响
 void *workUsePool4(void *arg, int id)  
 {
    for(int i = 0; i < 100; i++) {
        for(int j= 0; j < 100; j++);
    }
    return NULL;
 }

// 使用连接池的方式
void *workUsePool(void *arg, int id)    // 任务
{
    // printf("workUsePool id:%d\n", id);
    CDBPool *pDBPool = (CDBPool *)arg;
    CDBConn *pDBConn = pDBPool->GetDBConn(2000); // 获取连接，超时2000ms
    if (pDBConn)
    {
        bool ret = insertUser(pDBConn, id); // 插入用户信息
        if (!ret)
        {
            printf("insertUser failed\n");
        }
        pDBPool->RelDBConn(pDBConn);     // 然后释放连接
    }
    else
    {
        printf("GetDBConn failed\n");
    }
   
    // printf("exit id:%d\n", id);
    return NULL;
}
// 没有用连接池，每次任务的执行都重新初始化连接
void *workNoPool(void *arg, int id)
{
    // printf("workNoPool\n");
    arg = arg; // 避免警告
    const char *db_pool_name = DB_POOL_NAME;
    const char *db_host = DB_HOST_IP;
    int db_port = DB_HOST_PORT;
    const char *db_dbname = DB_DATABASE_NAME;
    const char *db_username = DB_USERNAME;
    const char *db_password = DB_PASSWORD;

    int db_maxconncnt = 1;      // 这里就只初始化一个连接

    CDBPool *pDBPool = new CDBPool(db_pool_name, db_host, db_port,
                                   db_username, db_password, db_dbname, db_maxconncnt);
    if (!pDBPool)
    {
        printf("workNoPool new CDBPool failed\n");
        return NULL;
    }
    if (pDBPool->Init())
    {
        printf("init db instance failed: %s\n", db_pool_name);
        return NULL;
    }

    CDBConn *pDBConn = pDBPool->GetDBConn();
    if (pDBConn)
    {
        bool ret = insertUser(pDBConn, id);
        if (!ret)
        {
            printf("insertUser failed\n");
        }
    }
    else
    {
        printf("GetDBConn failed\n");
    }
    pDBPool->RelDBConn(pDBConn);
    delete pDBPool;         // 销毁连接池，实际是销毁连接
    return NULL;
}

// 使用连接池的测试
int testWorkUsePool(int thread_num, int db_maxconncnt, int task_num)
{
    const char *db_pool_name = DB_POOL_NAME;
    const char *db_host = DB_HOST_IP;
    int db_port = DB_HOST_PORT;
    const char *db_dbname = DB_DATABASE_NAME;
    const char *db_username = DB_USERNAME;
    const char *db_password = DB_PASSWORD;

    // 每个连接池都对应一个对象
    CDBPool *pDBPool = new CDBPool(db_pool_name, db_host, db_port,
                                   db_username, db_password, db_dbname, db_maxconncnt);
    if (pDBPool->Init())
    {
        printf("init db instance failed: %s", db_pool_name);
        return -1;
    }

    CDBConn *pDBConn = pDBPool->GetDBConn(); // 获取连接
    if (pDBConn)
    {
        bool ret = pDBConn->ExecuteDrop(DROP_IMUSER_TABLE); // 删除表
        if (ret)
        {
            // printf("DROP_IMUSER_TABLE ok\n");
        }
        // 1. 创建表
        ret = pDBConn->ExecuteCreate(CREATE_IMUSER_TABLE); // 创建表
        if (ret)
        {
            // printf("CREATE_IMUSER_TABLE ok\n");
        }
        else
        {
            printf("ExecuteCreate failed\n");
            return -1;
        }
    }
    pDBPool->RelDBConn(pDBConn); // 一定要归还

    printf("task_num = %d, thread_num = %d, connection_num:%d, use_pool:1\n",
           task_num, thread_num, db_maxconncnt);

    ZERO_ThreadPool threadpool;
    threadpool.init(thread_num); // 设置线程数量
    threadpool.start();          // 启动线程池
    uint64_t start_time =  get_tick_count();
    for (int i = 0; i < task_num; i++)
    {
        threadpool.exec(workUsePool, (void *)pDBPool, i); //把所有的任务都给线程池
    }
    cout << "need time0: " <<  get_tick_count()  - start_time << "ms\n";
    threadpool.waitForAllDone(); // 等待所有执行万再退出
    cout << "need time1: " <<  get_tick_count()  - start_time << "ms\n";
    threadpool.stop();
    cout << "need time2: " <<  get_tick_count()  - start_time << "ms\n\n";
    delete pDBPool;
    return 0;
}
// 初始化和使用连接池是一样的
int testWorkNoPool(int thread_num, int db_maxconncnt, int task_num)
{
    const char *db_pool_name = DB_POOL_NAME;
    const char *db_host = DB_HOST_IP;
    int db_port = DB_HOST_PORT;
    const char *db_dbname = DB_DATABASE_NAME;
    const char *db_username = DB_USERNAME;
    const char *db_password = DB_PASSWORD;

    // 每个连接池都对应一个对象
    CDBPool *pDBPool = new CDBPool(db_pool_name, db_host, db_port,
                                   db_username, db_password, db_dbname, db_maxconncnt);

    if (pDBPool->Init())
    {
        printf("init db instance failed: %s", db_pool_name);
        return -1;
    }

    CDBConn *pDBConn = pDBPool->GetDBConn(); // 获取连接
    if (pDBConn)
    {
        bool ret = pDBConn->ExecuteDrop(DROP_IMUSER_TABLE); // 删除表
        if (ret)
        {
            printf("DROP_IMUSER_TABLE ok\n");
        }
        // 1. 创建表
        ret = pDBConn->ExecuteCreate(CREATE_IMUSER_TABLE); // 创建表
        if (ret)
        {
            printf("CREATE_IMUSER_TABLE ok\n");
        }
        else
        {
            printf("ExecuteCreate failed\n");
            return -1;
        }
    }
    pDBPool->RelDBConn(pDBConn);

    printf("task_num = %d, thread_num = %d, connection_num:%d, use_pool:0\n",
           task_num, thread_num, db_maxconncnt);
    ZERO_ThreadPool threadpool;
    threadpool.init(thread_num); // 设置线程数量
    threadpool.start();          // 启动线程池
    uint64_t start_time =  get_tick_count();
    for (int i = 0; i < task_num; i++)
    {
        threadpool.exec(workNoPool, (void *)pDBPool, i);  // 主要在于执行函数的区别。
    }
    cout << "need time0: " <<  get_tick_count()  - start_time << "ms\n";
    threadpool.waitForAllDone(); // 等待所有执行万再退出
    cout << "need time1: " <<  get_tick_count()  - start_time << "ms\n";
    threadpool.stop();
    cout << "need time2: " <<  get_tick_count()  - start_time << "ms\n\n";
    return 0;
}
 

// select name,phone  from IMUser;
int main(int argc, char **argv)
{
    int thread_num = 1;                  // 线程池线程数量初始化
    int db_maxconncnt = DB_POOL_MAX_CON; // 经验公式(不能硬套)-连接池最大连接数量(核数*2 + 磁盘数量)
    int task_num = TASK_NUMBER;

    int thread_num_tbl[] = {1, 2, 4, 8, 16, 32, 64, 128, 256}; // , 512
    // int thread_num_tbl[] = {1}; // , 512
    // int thread_num_tbl[] = { 4, 8, 16, 32, 64, 128, 256};
    task_num = (argc > 1) ? atoi(argv[1]) : TASK_NUMBER;
    printf("testWorkUsePool\n");
    for(int i = 0; i < int(sizeof(thread_num_tbl)/sizeof(thread_num_tbl[0])); i++) {
        thread_num = thread_num_tbl[i];
        db_maxconncnt = thread_num; // 目前连接和线程数量一样
        // if(thread_num > 3)
        //     db_maxconncnt = thread_num - 3; // 目前连接和线程数量不一样， 用于测试连接池 wait
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
