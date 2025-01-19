#include <iostream>
#include <sqlite3.h>
#include <sys/socket.h>
using namespace std;

//SQLite 的基本使用
//（1）零配置，无须安装和管理配置；
//（2）储存在单一磁盘文件中的一个完整的数据库；
//（3）数据库文件可以在不同字节顺序的机器间自 由共享；
//（4）支持数据库大小至 2TB；
//（5）对数据的操作比目前流行的大多数数据库要快。

//SQLite 系统命令
//.help         查看帮助信息
//.quit         退出数据库
//.exit         退出数据库
//.databases    查看数据库
//.schema       查看表的结构
//.tables       显示数据库中的所有表的名字

//SQLite API

//打开由 filename 参数指定的 SQLite 数据库文件
//int sqlite3_open(const char *filename, sqlite3 **ppDb);

//返回描述错误的提示
//const char *sqlite3_errmsg(sqlite3*);

//关闭 sqlite 数据库
//int sqlite3_close(sqlite3*);

//执行数据库 SQLite 的操作
//int sqlite3_exec(sqlite3*,const char *sql, int (*callback)(void*,int,char**,char**), void *arg; char **errmsg);

//查询数据库的函数
//int sqlite3_get_table()

//释放 sqlite3_get_table()函数返回的结果表
//void sqlite3_free_table(char **result);

//https://www.sqlite.org/download.html 下载 C source code as an amalgamation
//解压，复制 sqlite3.c  sqlite3.h 文件到项目目录
//CMakeLists.txt 中添加  add_compile_options(-l sqlite3)

int do_insert(sqlite3 *db) {
    int id;
    char name[32] = {0};
    char sex[8] = {0};
    int score;
    char sql[1024] = {0};
    char *errmsg;

    printf("input id:\n");
    scanf("%d", &id);
    printf("input name:\n");
    scanf("%s", name);
    getchar();
    printf("input sex:\n");
    scanf("%s", &sex);
    getchar();
    printf("input score:\n");
    scanf("%d", &score);

    sprintf(sql, "insert into stu values(%d, '%s', '%s', %d)", id, name, sex, score);

    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        printf("%s\n", errmsg);
    } else {
        printf("插入成功\n");
    }

    return 0;
}

int do_delete(sqlite3 *db) {
    int id;
    char sql[1024] = {0};
    char *errmsg;

    printf("input id:\n");
    scanf("%d", &id);

    sprintf(sql, "delete from stu where id = %d", id);

    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        printf("%s\n", errmsg);
    } else {
        printf("删除成功\n");
    }

    return 0;
}

int do_update(sqlite3 *db) {
    int id;
    char sql[1024] = {0};
    char name[32] = {0};
    char *errmsg;

    printf("input id:\n");
    scanf("%d", &id);
    printf("input name:\n");
    scanf("%s", name);
    getchar();

    sprintf(sql, "update stu set name='%s' where id = %d", name, id);

    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        printf("%s\n", errmsg);
    } else {
        printf("更新成功\n");
    }

    return 0;
}

int flags = 0;

int callback(void *arg, int col, char **value, char **name) {
    //打印列表中一条记录的所有信息，col 为记录中的列数
    if (flags == 0) {
        for (int i = 0; i < col; i++) {
            printf("%-8s", name[i]);
        }
        printf("\n");
        flags = 1;
    }
    //打印列的内容值
    for (int i = 0; i < col; i++) {
        printf("%-8s", value[i]);
    }
    printf("\n");
    return 0;
}

int do_query(sqlite3 *db) {
    char *errmsg;
    char sql[1024] = "select * from stu";
    flags = 0;
    if (sqlite3_exec(db, sql, callback, NULL, &errmsg) != SQLITE_OK) {
        printf("%s\n", errmsg);
    } else {
        printf("查询成功\n");
    }

    return 0;
}

int do_query2(sqlite3 *db) {
    char *errmsg;
    char **ret;
    int row;
    int col;

    if (sqlite3_get_table(db, "select * from stu", &ret, &row, &col, &errmsg) != SQLITE_OK) {
        printf("%s\n", errmsg);
        return -1;
    } else {
        printf("查询成功\n");
    }

    for (int i = 0; i < col; i++) {
        printf("%-8s", ret[i]);
    }

    printf("\n");

    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            printf("%-8s", ret[col + i * col + j]);
        }
        printf("\n");
    }

    // 清理资源
    sqlite3_free_table(ret);

    return 0;
}

int main() {
    sqlite3 *db;
    char *errmsg;

    //打开数据库
    if (sqlite3_open("test.db", &db) != SQLITE_OK) {
        printf("%s\n", sqlite3_errmsg(db));
        return -1;
    }

    //创建数据库表
    if (sqlite3_exec(db, "create table if not exists stu(id int, name char, sex char, score int);", NULL, NULL,
                     &errmsg) != SQLITE_OK) {
        printf("%s\n", sqlite3_errmsg(db));
        return -1;
    }

    int n;
    while (1) {
        printf("1:insert 2:query 3:query2 4:delete 5:update 6:quit\n");
        printf("please select:\n");
        scanf("%d", &n);

        switch (n) {
            case 1:
                do_insert(db);
                break;
            case 2:
                do_query(db);
                break;
            case 3:
                do_query2(db);
                break;
            case 4:
                do_delete(db);
                break;
            case 5:
                do_update(db);
                break;
            case 6:
                printf("exit\n");
                sqlite3_close(db);
                exit(0);
                break;
            default:
                printf("输入有误，请重新输入\n");
                break;
        }
    }

    return 0;
}
