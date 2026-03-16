const CREDENTIALS = [
    { rut: '111111111', password: 'admin123' }
];

function verificarCredenciales(rut, password) {
    return CREDENTIALS.some(c => c.rut === rut && c.password === password);
}
