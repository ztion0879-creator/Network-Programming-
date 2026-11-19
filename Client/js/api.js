// API base URL: đổi nếu Server chạy ở địa chỉ/port khác.
const API_BASE = 'http://localhost:8080';
async function apiRequest(path, options={}) {
  const headers = {'Content-Type':'application/json', ...(options.headers||{})};
  const token = localStorage.getItem('foodie_token');
  if (token) headers.Authorization = `Bearer ${token}`;
  let response;
  try { response = await fetch(API_BASE + path, {...options, headers}); }
  catch (e) { throw new Error('Không kết nối được Server. Hãy kiểm tra server.exe đang chạy và cấu hình CORS.'); }
  const raw = await response.text();
  let data = {}; try { data = raw ? JSON.parse(raw) : {}; } catch { data = {message:raw}; }
  if (!response.ok || data.success === false) throw new Error(data.message || `HTTP ${response.status}`);
  return data;
}
function money(value){return Number(value||0).toLocaleString('vi-VN')+' đ';}
function getCart(){try{return JSON.parse(localStorage.getItem('foodie_cart')||'[]')}catch{return []}}
function saveCart(cart){localStorage.setItem('foodie_cart',JSON.stringify(cart));}
function addToCart(food){const cart=getCart();const found=cart.find(x=>Number(x.id)===Number(food.id));if(found)found.quantity++;else cart.push({id:Number(food.id),name:food.name,price:Number(food.price),quantity:1});saveCart(cart);}
