#include "Auth.h"
#include <sqlite3.h>
#include <sstream>
#include <iomanip>
#include <random>
#include <stdexcept>
#include <vector>
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
using nlohmann::json;
static std::string makeToken() {
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, (int)sizeof(chars)-2);
    std::string token;
    for (int i=0;i<48;++i) token += chars[dis(gen)];
    return token;
}
Auth::Auth(Database& db) : database(db) {}

std::string Auth::hashPassword(const std::string& password) const {
    unsigned char salt[16];
    if (BCryptGenRandom(nullptr, salt, sizeof(salt), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
        throw std::runtime_error("Cannot generate password salt");
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectSize=0, resultSize=0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
        throw std::runtime_error("Cannot open SHA256 provider");
    BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objectSize, sizeof(objectSize), &resultSize, 0);
    std::vector<unsigned char> object(objectSize);
    unsigned char digest[32];
    std::string input(reinterpret_cast<char*>(salt), sizeof(salt));
    input += password;
    NTSTATUS status = BCryptCreateHash(alg, &hash, object.data(), objectSize, nullptr, 0, 0);
    if (status == 0) status = BCryptHashData(hash, (PUCHAR)input.data(), (ULONG)input.size(), 0);
    if (status == 0) status = BCryptFinishHash(hash, digest, sizeof(digest), 0);
    if (hash) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg, 0);
    if (status != 0) throw std::runtime_error("SHA256 hashing failed");
    std::ostringstream out;
    out << "sha256$" << std::hex << std::setfill('0');
    for (auto c : salt) out << std::setw(2) << (int)c;
    out << "$";
    for (auto c : digest) out << std::setw(2) << (int)c;
    return out.str();
}

static bool verifyPassword(const std::string& password, const std::string& stored) {

    if (stored.rfind("sha256$",0) != 0) return false;
    auto sep = stored.find('$',7);
    if (sep == std::string::npos || sep-7 != 32 || stored.size()!=sep+1+64) return false;
    std::string saltHex=stored.substr(7,32), expected=stored.substr(sep+1);
    unsigned char salt[16];
    for (int i=0;i<16;++i) {
        try { salt[i]=(unsigned char)std::stoul(saltHex.substr(i*2,2),nullptr,16); }
        catch (...) { return false; }
    }
    std::string input(reinterpret_cast<char*>(salt),16); input += password;
    BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_HASH_HANDLE hash=nullptr;
    DWORD objectSize=0, resultSize=0;
    if (BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)!=0) return false;
    BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,(PUCHAR)&objectSize,sizeof(objectSize),&resultSize,0);
    std::vector<unsigned char> object(objectSize); unsigned char digest[32];
    NTSTATUS status=BCryptCreateHash(alg,&hash,object.data(),objectSize,nullptr,0,0);
    if(status==0) status=BCryptHashData(hash,(PUCHAR)input.data(),(ULONG)input.size(),0);
    if(status==0) status=BCryptFinishHash(hash,digest,sizeof(digest),0);
    if(hash) BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(alg,0);
    if(status!=0) return false;
    std::ostringstream out; out<<std::hex<<std::setfill('0');
    for(auto c:digest) out<<std::setw(2)<<(int)c;
    return out.str()==expected;
}

json Auth::registerUser(const json& data) {
    std::string username=data.value("username","");
    std::string password=data.value("password","");
    std::string fullName=data.value("full_name","");
    std::string phone=data.value("phone","");
    if(username.empty() || password.size()<6 || fullName.empty())
        return {{"success",false},{"message","Username, full_name bắt buộc; mật khẩu tối thiểu 6 ký tự."}};
    try {
        std::string hash=hashPassword(password);
        sqlite3_stmt* stmt=nullptr;
        const char* sql="INSERT INTO Users(username,password_hash,full_name,phone,role) VALUES(?,?,?,?, 'Customer')";
        if(sqlite3_prepare_v2(database.connection(),sql,-1,&stmt,nullptr)!=SQLITE_OK)
            return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
        sqlite3_bind_text(stmt,1,username.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,2,hash.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,3,fullName.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,4,phone.c_str(),-1,SQLITE_TRANSIENT);
        int rc=sqlite3_step(stmt); sqlite3_finalize(stmt);
        if(rc!=SQLITE_DONE) {
            if(rc==SQLITE_CONSTRAINT) return {{"success",false},{"message","Username đã tồn tại."}};
            return {{"success",false},{"message",sqlite3_errmsg(database.connection())}};
        }
        return {{"success",true},{"message","Đăng ký thành công."},{"user_id",sqlite3_last_insert_rowid(database.connection())}};
    } catch(const std::exception& e) { return {{"success",false},{"message",e.what()}}; }
}
json Auth::login(const json& data) {
    std::string username=data.value("username",""), password=data.value("password","");
    sqlite3_stmt* stmt=nullptr;
    const char* sql="SELECT id,password_hash,full_name,role FROM Users WHERE username=?";
    if(sqlite3_prepare_v2(database.connection(),sql,-1,&stmt,nullptr)!=SQLITE_OK)
        return {{"success",false},{"message","Lỗi truy vấn database."}};
    sqlite3_bind_text(stmt,1,username.c_str(),-1,SQLITE_TRANSIENT);
    int rc=sqlite3_step(stmt);
    if(rc!=SQLITE_ROW) { sqlite3_finalize(stmt); return {{"success",false},{"message","Sai username hoặc password."}}; }
    int id=sqlite3_column_int(stmt,0);
    std::string stored=(const char*)sqlite3_column_text(stmt,1);
    std::string name=(const char*)sqlite3_column_text(stmt,2);
    std::string role=(const char*)sqlite3_column_text(stmt,3);
    sqlite3_finalize(stmt);
    if(!verifyPassword(password,stored)) return {{"success",false},{"message","Sai username hoặc password."}};
    std::string token=makeToken(); sessions[token]=id;
    return {{"success",true},{"token",token},{"user",{{"id",id},{"username",username},{"full_name",name},{"role",role}}}};
}
json Auth::userFromToken(const std::string& token) {
    auto it=sessions.find(token);
    if(it==sessions.end()) return nullptr;
    sqlite3_stmt* stmt=nullptr;
    if(sqlite3_prepare_v2(database.connection(),"SELECT id,username,full_name,role FROM Users WHERE id=?",-1,&stmt,nullptr)!=SQLITE_OK) return nullptr;
    sqlite3_bind_int(stmt,1,it->second);
    json result=nullptr;
    if(sqlite3_step(stmt)==SQLITE_ROW)
        result={{"id",sqlite3_column_int(stmt,0)},
                {"username",(const char*)sqlite3_column_text(stmt,1)},
                {"full_name",(const char*)sqlite3_column_text(stmt,2)},
                {"role",(const char*)sqlite3_column_text(stmt,3)}};
    sqlite3_finalize(stmt); return result;
}
bool Auth::isAdmin(const std::string& token) {
    json u=userFromToken(token); return !u.is_null() && u.value("role","")=="Admin";
}
void Auth::logout(const std::string& token) { sessions.erase(token); }
