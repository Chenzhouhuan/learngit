#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>

#include <jdbc/cppconn/connection.h>
#include <jdbc/cppconn/statement.h>

#include <iostream>

sql::mysql::MySQL_Driver *driver = 0;
sql::Connection *conn = 0;

int main(){
    //建立连接
    try{
        driver = sql::mysql::get_mysql_driver_instance();
        conn = driver->connect("tcp://localhost:3306/test", "root", "137583992");

        std::cout << "连接成功" << std::endl;
    }catch(...){
        std::cout << "连接失败" << std::endl;
    }
    //获取Statement对象：向MySQL服务器发送SQL语句
    sql::Statement *stmt;
    stmt = conn->createStatement();
    sql::ResultSet *res;
    res = stmt->executeQuery("SELECT * FROM City");
    while(res->next()) std::cout << res->getString("CityName") << std::endl;
    //插入数据
    int updateCount = stmt->executeUpdate("insert into City (CityName) values ('Napier, New Zealand')");

    if(conn != 0) delete conn;
    delete stmt;
    delete res;
    return 0;
}