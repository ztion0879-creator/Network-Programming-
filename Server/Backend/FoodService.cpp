#include "FoodService.h"
#include <sqlite3.h>
using nlohmann::json;
FoodService::FoodService(Database& db):database(db){}
json FoodService::getFoods(const std::string& search) {
    sqlite3_stmt* stmt=nullptr;
    const char* sql=search.empty()
        ? "SELECT id,name,description,price,image,category,available FROM Foods WHERE available=1 ORDER BY id DESC"
        : "SELECT id,name,description,price,image,category,available FROM Foods WHERE available=1 AND (name LIKE ? OR category LIKE ?) ORDER BY id DESC";
    if(sqlite3_prepare_v2(database.connection(),sql,-1,&stmt,nullptr)!=SQLITE_OK)
        return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
    if(!search.empty()) {
        std::string pattern="%"+search+"%";
        sqlite3_bind_text(stmt,1,pattern.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,2,pattern.c_str(),-1,SQLITE_TRANSIENT);
    }
    json foods=json::array();
    while(sqlite3_step(stmt)==SQLITE_ROW) foods.push_back({
        {"id",sqlite3_column_int(stmt,0)},
        {"name",(const char*)sqlite3_column_text(stmt,1)},
        {"description",(const char*)sqlite3_column_text(stmt,2)},
        {"price",sqlite3_column_double(stmt,3)},
        {"image",(const char*)sqlite3_column_text(stmt,4)},
        {"category",(const char*)sqlite3_column_text(stmt,5)},
        {"available",sqlite3_column_int(stmt,6)!=0}
    });
    sqlite3_finalize(stmt); return {{"success",true},{"foods",foods}};
}
json FoodService::addFood(const json& d) {
    std::string name=d.value("name",""), desc=d.value("description",""), image=d.value("image",""), category=d.value("category","");
    double price=d.value("price",0.0);
    if(name.empty() || price<0) return {{"success",false},{"message","Tên món bắt buộc và giá không được âm."}};
    sqlite3_stmt* s=nullptr;
    const char* sql="INSERT INTO Foods(name,description,price,image,category,available) VALUES(?,?,?,?,?,?)";
    if(sqlite3_prepare_v2(database.connection(),sql,-1,&s,nullptr)!=SQLITE_OK) return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
    sqlite3_bind_text(s,1,name.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(s,2,desc.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_double(s,3,price); sqlite3_bind_text(s,4,image.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(s,5,category.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_int(s,6,d.value("available",true)?1:0);
    int rc=sqlite3_step(s); sqlite3_finalize(s);
    if(rc!=SQLITE_DONE) return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
    return {{"success",true},{"message","Thêm món thành công."},{"food_id",sqlite3_last_insert_rowid(database.connection())}};
}
json FoodService::updateFood(int id,const json& d) {
    sqlite3_stmt* s=nullptr;
    const char* sql="UPDATE Foods SET name=?,description=?,price=?,image=?,category=?,available=? WHERE id=?";
    if(sqlite3_prepare_v2(database.connection(),sql,-1,&s,nullptr)!=SQLITE_OK) return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
    std::string name=d.value("name",""),desc=d.value("description",""),image=d.value("image",""),category=d.value("category","");
    double price=d.value("price",-1.0);
    if(name.empty()||price<0){sqlite3_finalize(s);return {{"success",false},{"message","Dữ liệu món ăn không hợp lệ."}};}
    sqlite3_bind_text(s,1,name.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_text(s,2,desc.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_double(s,3,price);sqlite3_bind_text(s,4,image.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(s,5,category.c_str(),-1,SQLITE_TRANSIENT);sqlite3_bind_int(s,6,d.value("available",true)?1:0);sqlite3_bind_int(s,7,id);
    int rc=sqlite3_step(s);int changed=sqlite3_changes(database.connection());sqlite3_finalize(s);
    if(rc!=SQLITE_DONE)return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
    if(!changed)return {{"success",false},{"message","Không tìm thấy món ăn."}};
    return {{"success",true},{"message","Cập nhật món thành công."}};
}
json FoodService::deleteFood(int id) {
    sqlite3_stmt* s=nullptr;
    if(sqlite3_prepare_v2(database.connection(),"DELETE FROM Foods WHERE id=?",-1,&s,nullptr)!=SQLITE_OK)return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
    sqlite3_bind_int(s,1,id);int rc=sqlite3_step(s);int changed=sqlite3_changes(database.connection());sqlite3_finalize(s);
    if(rc!=SQLITE_DONE)return {{"success",false},{"message","Không thể xóa món (có thể món đã nằm trong đơn hàng)."}};
    if(!changed)return {{"success",false},{"message","Không tìm thấy món ăn."}};
    return {{"success",true},{"message","Xóa món thành công."}};
}
