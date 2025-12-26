// Funcionalidad básica
        document.getElementById('togglePassword').addEventListener('click', function() {
            const passwordInput = document.getElementById('password');
            const type = passwordInput.getAttribute('type') === 'password' ? 'text' : 'password';
            passwordInput.setAttribute('type', type);
            this.textContent = type === 'password' ? '👁️' : '🙈';
        });

        document.getElementById('loginForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const username = document.getElementById('username').value;
            const password = document.getElementById('password').value;
            const submitBtn = document.getElementById('submitBtn');
            
            // Simulación de carga
            submitBtn.innerHTML = 'Autenticando...';
            submitBtn.disabled = true;
            
            setTimeout(() => {
                alert(`Autenticación simulada\nUsuario: ${username}\nContraseña: ${password}`);
                submitBtn.innerHTML = 'Iniciar Sesión';
                submitBtn.disabled = false;
            }, 1500);
        });

        // Botones alternativos
        document.querySelectorAll('.alt-button').forEach(button => {
            button.addEventListener('click', function() {
                const provider = this.textContent.trim();
                alert(`Autenticación con ${provider} (simulada)`);
            });
        });