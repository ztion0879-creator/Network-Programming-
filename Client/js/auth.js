
document.getElementById('login-form')?.addEventListener('submit', async e => {
    e.preventDefault();

    const f = new FormData(e.target);
    const msg = document.getElementById('form-message');

    msg.textContent = 'Đang đăng nhập...';

    try {
        const d = await apiRequest('/api/login', {
            method: 'POST',
            body: JSON.stringify({
                username: f.get('username'),
                password: f.get('password')
            })
        });

        // Save login information
        localStorage.setItem('foodie_token', d.token);
        localStorage.setItem('foodie_user', d.user.username);
        localStorage.setItem('foodie_role', d.user.role);

        msg.textContent = 'Đăng nhập thành công!';

        // Redirect based on account role
        if (d.user.role === 'Admin') {
            location.href = 'admin.html';
        } else {
            location.href = 'foods.html';
        }

    } catch (err) {
        msg.textContent = err.message;
    }
});


// REGISTER
document.getElementById('register-form')?.addEventListener('submit', async e => {
    e.preventDefault();

    const f = new FormData(e.target);
    const msg = document.getElementById('form-message');

    msg.textContent = 'Đang tạo tài khoản...';

    try {
        const d = await apiRequest('/api/register', {
            method: 'POST',
            body: JSON.stringify({
                username: f.get('username'),
                password: f.get('password'),
                full_name: f.get('full_name'),
                phone: f.get('phone'),
                address: f.get('address')
            })
        });

        msg.textContent = d.message || 'Đăng ký thành công!';

        setTimeout(() => {
            location.href = 'login.html';
        }, 900);

    } catch (err) {
        msg.textContent = err.message;
    }
});
