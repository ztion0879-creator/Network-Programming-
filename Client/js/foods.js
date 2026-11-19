
let allFoods = [];

function safe(s) {
    return String(s ?? '').replace(/[&<>"']/g, c => ({
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#39;'
    }[c]));
}

function card(food) {
    const image = food.image
        ? `<img src="${safe(food.image)}" alt="${safe(food.name)}" onerror="this.style.display='none';this.nextElementSibling.style.display='flex'">`
        : '';

    return `
        <article class="food-card">
            <div class="food-visual">
                ${image}
                <div class="food-placeholder" style="display:${image ? 'none' : 'flex'}">
                    🍽️
                </div>
            </div>
            <div class="food-info">
                <h3>${safe(food.name)}</h3>
                <p>${safe(food.description || food.category || 'Món ngon mỗi ngày')}</p>
                <div class="food-bottom">
                    <span class="price">${money(food.price)}</span>
                    <button class="btn small" data-id="${food.id}">
                        Thêm vào giỏ
                    </button>
                </div>
            </div>
        </article>
    `;
}

async function load() {
    const target = document.getElementById('food-list');

    try {
        const d = await apiRequest('/api/foods');
        allFoods = d.foods || d.data || [];
        render(allFoods, target);
    } catch (e) {
        target.innerHTML = `<p class="empty">${safe(e.message)}</p>`;
    }
}

function render(items, target) {
    target.innerHTML = items.length
        ? items.map(card).join('')
        : '<p class="empty">Chưa có món ăn phù hợp.</p>';

    target.querySelectorAll('[data-id]').forEach(b => {
        b.onclick = () => {
            const f = allFoods.find(
                x => Number(x.id) === Number(b.dataset.id)
            );

            if (!f) return;

            addToCart(f);

            const c = document.getElementById('cart-count');
            if (c) {
                c.textContent = getCart().reduce(
                    (s, x) => s + x.quantity, 0
                );
            }

            b.textContent = 'Đã thêm ✓';
        };
    });
}

document.addEventListener('DOMContentLoaded', () => {
    load();

    document.getElementById('food-search')?.addEventListener('input', e => {
        render(
            allFoods.filter(x =>
                String(x.name).toLowerCase().includes(
                    e.target.value.toLowerCase()
                )
            ),
            document.getElementById('food-list')
        );
    });
});
