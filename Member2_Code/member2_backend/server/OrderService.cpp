#include "OrderService.h"
#include <sqlite3.h>
#include <set>
#include <vector>
using nlohmann::json;
OrderService::OrderService(Database& db):database(db){}
json OrderService::createOrder(int userId,const json& data) {
    std::string address=data.value("delivery_address","");
    std::string note=data.value("note","");
    if(address.empty()||!data.contains("items")||!data["items"].is_array()||data["items"].empty())
        return {{"success",false},{"message","Cần địa chỉ giao hàng và danh sách món không được rỗng."}};
    sqlite3* db=database.connection();
    if(sqlite3_exec(db,"BEGIN IMMEDIATE TRANSACTION",nullptr,nullptr,nullptr)!=SQLITE_OK)
        return {{"success",false},{"message",sqlite3_errmsg(db)}};
    auto rollback=[&](){sqlite3_exec(db,"ROLLBACK",nullptr,nullptr,nullptr);};
    double total=0;
    struct Item{int id,qty;double price;std::string name;};
    std::vector<Item> items;
    std::set<int> seen;
    for(const auto& item:data["items"]) {
        int foodId=item.value("food_id",0), qty=item.value("quantity",0);
        if(foodId<=0||qty<=0||!seen.insert(foodId).second){rollback();return {{"success",false},{"message","Món ăn hoặc số lượng không hợp lệ (không gửi trùng food_id)."}};}
        sqlite3_stmt* q=nullptr;
        if(sqlite3_prepare_v2(db,"SELECT name,price FROM Foods WHERE id=? AND available=1",-1,&q,nullptr)!=SQLITE_OK){rollback();return {{"success",false},{"message",sqlite3_errmsg(db)}};}
        sqlite3_bind_int(q,1,foodId);
        if(sqlite3_step(q)!=SQLITE_ROW){sqlite3_finalize(q);rollback();return {{"success",false},{"message","Có món không tồn tại hoặc đã hết bán."}};}
        std::string name=(const char*)sqlite3_column_text(q,0);double price=sqlite3_column_double(q,1);sqlite3_finalize(q);
        items.push_back({foodId,qty,price,name});total+=price*qty;
    }
    sqlite3_stmt* order=nullptr;
    const char* os="INSERT INTO Orders(user_id,total,delivery_address,note,status) VALUES(?,?,?,?, 'Pending')";
    if(sqlite3_prepare_v2(db,os,-1,&order,nullptr)!=SQLITE_OK){rollback();return {{"success",false},{"message",sqlite3_errmsg(db)}};}
    sqlite3_bind_int(order,1,userId);sqlite3_bind_double(order,2,total);
    sqlite3_bind_text(order,3,address.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_text(order,4,note.c_str(),-1,SQLITE_TRANSIENT);
    if(sqlite3_step(order)!=SQLITE_DONE){sqlite3_finalize(order);rollback();return {{"success",false},{"message",sqlite3_errmsg(db)}};}
    sqlite3_finalize(order);sqlite3_int64 orderId=sqlite3_last_insert_rowid(db);
    sqlite3_stmt* detail=nullptr;
    const char* ds="INSERT INTO OrderDetails(order_id,food_id,food_name,unit_price,quantity,subtotal) VALUES(?,?,?,?,?,?)";
    if(sqlite3_prepare_v2(db,ds,-1,&detail,nullptr)!=SQLITE_OK){rollback();return {{"success",false},{"message",sqlite3_errmsg(db)}};}
    for(const auto& it:items){
        sqlite3_reset(detail);sqlite3_clear_bindings(detail);
        sqlite3_bind_int64(detail,1,orderId);sqlite3_bind_int(detail,2,it.id);
        sqlite3_bind_text(detail,3,it.name.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_double(detail,4,it.price);
        sqlite3_bind_int(detail,5,it.qty);sqlite3_bind_double(detail,6,it.price*it.qty);
        if(sqlite3_step(detail)!=SQLITE_DONE){sqlite3_finalize(detail);rollback();return {{"success",false},{"message",sqlite3_errmsg(db)}};}
    }
    sqlite3_finalize(detail);
    if(sqlite3_exec(db,"COMMIT",nullptr,nullptr,nullptr)!=SQLITE_OK){rollback();return {{"success",false},{"message",sqlite3_errmsg(db)}};}
    return {{"success",true},{"message","Đặt hàng thành công."},{"order_id",orderId},{"total",total},{"status","Pending"}};
}
json OrderService::getOrders(int userId,bool admin) {
    sqlite3_stmt* s=nullptr;
    const char* sql=admin
      ?"SELECT o.id,o.user_id,u.username,o.total,o.delivery_address,o.note,o.status,o.created_at FROM Orders o JOIN Users u ON u.id=o.user_id ORDER BY o.id DESC"
      :"SELECT o.id,o.user_id,u.username,o.total,o.delivery_address,o.note,o.status,o.created_at FROM Orders o JOIN Users u ON u.id=o.user_id WHERE o.user_id=? ORDER BY o.id DESC";
    if(sqlite3_prepare_v2(database.connection(),sql,-1,&s,nullptr)!=SQLITE_OK)return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
    if(!admin)sqlite3_bind_int(s,1,userId);
    json orders=json::array();
    while(sqlite3_step(s)==SQLITE_ROW){
        int id=sqlite3_column_int(s,0);
        json o={{"id",id},{"user_id",sqlite3_column_int(s,1)},{"username",(const char*)sqlite3_column_text(s,2)},
          {"total",sqlite3_column_double(s,3)},{"delivery_address",(const char*)sqlite3_column_text(s,4)},
          {"note",(const char*)sqlite3_column_text(s,5)},{"status",(const char*)sqlite3_column_text(s,6)},
          {"created_at",(const char*)sqlite3_column_text(s,7)},{"items",json::array()}};
        sqlite3_stmt* d=nullptr;
        if(sqlite3_prepare_v2(database.connection(),"SELECT food_id,food_name,unit_price,quantity,subtotal FROM OrderDetails WHERE order_id=?",-1,&d,nullptr)==SQLITE_OK){
            sqlite3_bind_int(d,1,id);
            while(sqlite3_step(d)==SQLITE_ROW)o["items"].push_back({
              {"food_id",sqlite3_column_int(d,0)},{"food_name",(const char*)sqlite3_column_text(d,1)},
              {"unit_price",sqlite3_column_double(d,2)},{"quantity",sqlite3_column_int(d,3)},
              {"subtotal",sqlite3_column_double(d,4)}});
        }
        if(d)sqlite3_finalize(d);orders.push_back(o);
    }
    sqlite3_finalize(s);return {{"success",true},{"orders",orders}};
}
json OrderService::updateStatus(int orderId,const std::string& status) {
    static const std::set<std::string> allowed={"Pending","Confirmed","Preparing","Delivering","Completed","Cancelled"};
    if(!allowed.count(status))return {{"success",false},{"message","Trạng thái đơn hàng không hợp lệ."}};
    sqlite3_stmt* s=nullptr;
    if(sqlite3_prepare_v2(database.connection(),"UPDATE Orders SET status=? WHERE id=?",-1,&s,nullptr)!=SQLITE_OK)return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
    sqlite3_bind_text(s,1,status.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_int(s,2,orderId);
    int rc=sqlite3_step(s),changed=sqlite3_changes(database.connection());sqlite3_finalize(s);
    if(rc!=SQLITE_DONE)return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
    if(!changed)return {{"success",false},{"message","Không tìm thấy đơn hàng."}};
    return {{"success",true},{"message","Cập nhật trạng thái thành công."},{"status",status}};
}
