
// ========================================
// ADMIN - FOOD MANAGEMENT & ORDER MANAGEMENT
// ========================================

document.addEventListener("DOMContentLoaded", () => {
    const foodForm = document.getElementById("food-form");
    const foodList = document.getElementById("admin-food-list");
    const orderList = document.getElementById("admin-order-list");
    const foodMessage = document.getElementById("food-message");

    const foodIdInput = document.getElementById("food-id");
    const foodNameInput = document.getElementById("food-name");
    const foodDescriptionInput = document.getElementById("food-description");
    const foodPriceInput = document.getElementById("food-price");
    const foodCategoryInput = document.getElementById("food-category");
    const foodImageInput = document.getElementById("food-image");
    const foodAvailableInput = document.getElementById("food-available");

    const submitButton = document.getElementById("food-submit");
    const cancelButton = document.getElementById("food-cancel");

    const statuses = [
        "Pending",
        "Confirmed",
        "Preparing",
        "Delivering",
        "Completed",
        "Cancelled"
    ];

    // Escape HTML to prevent unsafe content injection.
    function escapeHTML(value) {
        return String(value ?? "").replace(/[&<>"']/g, char => ({
            "&": "&amp;",
            "<": "&lt;",
            ">": "&gt;",
            '"': "&quot;",
            "'": "&#39;"
        })[char]);
    }

    function money(value) {
        return Number(value || 0).toLocaleString("vi-VN") + " đ";
    }

    function showMessage(message, isError = false) {
        if (!foodMessage) return;

        foodMessage.textContent = message;
        foodMessage.style.color = isError ? "red" : "green";
    }

    function resetFoodForm() {
        if (foodForm) foodForm.reset();

        if (foodIdInput) foodIdInput.value = "";
        if (submitButton) submitButton.textContent = "Thêm món ăn";

        showMessage("");
    }

    // ========================================
    // LOAD FOOD LIST
    // ========================================

    async function loadFoods() {
        if (!foodList) return;

        try {
            const response = await apiRequest("/api/foods");
            const foods = response.foods || [];

            if (foods.length === 0) {
                foodList.innerHTML = "<p>Chưa có món ăn nào.</p>";
                return;
            }

            foodList.innerHTML = foods.map(food => `
                <div class="admin-item">
                    <div>
                        <h3>${escapeHTML(food.name)}</h3>
                        <p>${escapeHTML(food.description)}</p>
                        <p>Giá: ${money(food.price)}</p>
                        <p>Danh mục: ${escapeHTML(food.category)}</p>
                        <p>Trạng thái: ${food.available ? "Đang bán" : "Ngừng bán"}</p>
                    </div>

                    <div>
                        <button type="button" class="edit-food"
                            data-id="${Number(food.id)}">Sửa</button>

                        <button type="button" class="delete-food"
                            data-id="${Number(food.id)}">Xóa</button>
                    </div>
                </div>
            `).join("");

            foodList.querySelectorAll(".edit-food").forEach(button => {
                button.addEventListener("click", () => {
                    const food = foods.find(item =>
                        Number(item.id) === Number(button.dataset.id)
                    );

                    if (!food) return;

                    foodIdInput.value = food.id;
                    foodNameInput.value = food.name || "";
                    foodDescriptionInput.value = food.description || "";
                    foodPriceInput.value = food.price || 0;
                    foodCategoryInput.value = food.category || "";
                    foodImageInput.value = food.image || "";
                    foodAvailableInput.checked = Boolean(food.available);

                    submitButton.textContent = "Cập nhật món ăn";
                    showMessage("Đang chỉnh sửa món: " + food.name);
                    foodForm.scrollIntoView({ behavior: "smooth" });
                });
            });

            foodList.querySelectorAll(".delete-food").forEach(button => {
                button.addEventListener("click", async () => {
                    const id = button.dataset.id;

                    if (!confirm("Bạn có chắc chắn muốn xóa món ăn này?")) {
                        return;
                    }

                    try {
                        const result = await apiRequest(
                            `/api/admin/foods?id=${id}`,
                            { method: "DELETE" }
                        );

                        alert(result.message || "Đã xóa món ăn.");
                        await loadFoods();
                    } catch (error) {
                        alert(error.message || "Không thể xóa món ăn.");
                    }
                });
            });

        } catch (error) {
            foodList.innerHTML = "<p>Không thể tải danh sách món ăn.</p>";
            console.error("Load foods error:", error);
        }
    }

    // ========================================
    // ADD / UPDATE FOOD
    // ========================================

    if (foodForm) {
        foodForm.addEventListener("submit", async event => {
            event.preventDefault();

            const id = foodIdInput.value;

            const foodData = {
                name: foodNameInput.value.trim(),
                description: foodDescriptionInput.value.trim(),
                price: Number(foodPriceInput.value),
                image: foodImageInput.value.trim(),
                category: foodCategoryInput.value.trim(),
                available: foodAvailableInput.checked
            };

            if (!foodData.name) {
                showMessage("Vui lòng nhập tên món ăn.", true);
                return;
            }

            if (!Number.isFinite(foodData.price) || foodData.price < 0) {
                showMessage("Giá món ăn không hợp lệ.", true);
                return;
            }

            try {
                const result = await apiRequest(
                    id ? `/api/admin/foods?id=${id}` : "/api/admin/foods",
                    {
                        method: id ? "PUT" : "POST",
                        body: JSON.stringify(foodData)
                    }
                );

                showMessage(result.message || "Thao tác thành công.");
                resetFoodForm();
                await loadFoods();

            } catch (error) {
                showMessage(error.message || "Không thể lưu món ăn.", true);
            }
        });
    }

    // ========================================
    // CANCEL EDITING
    // ========================================

    if (cancelButton) {
        cancelButton.addEventListener("click", resetFoodForm);
    }

    // ========================================
    // LOAD ORDERS
    // ========================================

    async function loadOrders() {
        if (!orderList) return;

        try {
            const response = await apiRequest("/api/orders");
            const orders = response.orders || [];

            if (orders.length === 0) {
                orderList.innerHTML = "<p>Chưa có đơn hàng nào.</p>";
                return;
            }

            orderList.innerHTML = orders.map(order => {
                const items = (order.items || []).map(item => `
                    <li>
                        ${escapeHTML(item.food_name)}
                        x ${Number(item.quantity)}
                        - ${money(item.subtotal)}
                    </li>
                `).join("");

                const options = statuses.map(status => `
                    <option value="${status}"
                        ${order.status === status ? "selected" : ""}>
                        ${status}
                    </option>
                `).join("");

                return `
                    <div class="admin-order">
                        <h3>Đơn hàng #${Number(order.id)}</h3>
                        <p>Khách hàng: ${escapeHTML(order.username)}</p>
                        <p>Địa chỉ: ${escapeHTML(order.delivery_address)}</p>
                        <p>Ghi chú: ${escapeHTML(order.note)}</p>
                        <p>Ngày đặt: ${escapeHTML(order.created_at)}</p>
                        <p>Tổng tiền: ${money(order.total)}</p>

                        <p>Sản phẩm:</p>
                        <ul>${items}</ul>

                        <label>Trạng thái:</label>

                        <select class="order-status"
                            data-id="${Number(order.id)}">
                            ${options}
                        </select>

                        <button type="button" class="update-order"
                            data-id="${Number(order.id)}">
                            Cập nhật trạng thái
                        </button>
                    </div>
                `;
            }).join("");

            orderList.querySelectorAll(".update-order").forEach(button => {
                button.addEventListener("click", async () => {
                    const id = button.dataset.id;
                    const select = orderList.querySelector(
                        `.order-status[data-id="${id}"]`
                    );

                    try {
                        const result = await apiRequest(
                            "/api/orders/status",
                            {
                                method: "PUT",
                                body: JSON.stringify({
                                    order_id: Number(id),
                                    status: select.value
                                })
                            }
                        );

                        alert(result.message || "Đã cập nhật trạng thái.");
                        await loadOrders();

                    } catch (error) {
                        alert(error.message || "Không thể cập nhật đơn hàng.");
                    }
                });
            });

        } catch (error) {
            orderList.innerHTML = "<p>Không thể tải danh sách đơn hàng.</p>";
            console.error("Load orders error:", error);
        }
    }

    // ========================================
    // INITIALIZE ADMIN PAGE
    // ========================================

    loadFoods();
    loadOrders();
});
