// App Logic
document.addEventListener('DOMContentLoaded', () => {
    updateDate();
    fetchData();

    // Auto refresh every 5 seconds
    setInterval(fetchData, 5000);
});

function updateDate() {
    const now = new Date();
    const options = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' };
    document.getElementById('current-date').textContent = now.toLocaleDateString('es-ES', options);
}

function showSection(sectionId) {
    // Hide all sections
    document.querySelectorAll('.section').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('nav a').forEach(el => el.classList.remove('active'));

    // Show target section
    document.getElementById(sectionId).classList.add('active');

    // Update nav
    const navItem = Array.from(document.querySelectorAll('nav a')).find(el => el.getAttribute('onclick').includes(sectionId));
    if (navItem) navItem.classList.add('active');

    // Update title
    const titles = {
        'dashboard': 'Dashboard',
        'students': 'Gestión de Estudiantes',
        'reports': 'Reportes',
        'settings': 'Configuración'
    };
    document.getElementById('page-title').textContent = titles[sectionId];
}

async function fetchData() {
    try {
        const response = await fetch('/api/recent');
        const data = await response.json();

        updateStats(data.records);
        renderTable(data.records);

    } catch (error) {
        console.error('Error fetching data:', error);
    }
}

function updateStats(records) {
    // Calculate stats from today's records
    // Note: In a real app, we might want a dedicated stats endpoint
    // For now, we count from the recent records

    const today = new Date().toISOString().split('T')[0];

    const todayRecords = records.filter(r => r.fecha === today);

    const breakfast = todayRecords.filter(r => r.tipo_racion === 1).length;
    const lunch = todayRecords.filter(r => r.tipo_racion === 2).length;

    document.getElementById('count-breakfast').textContent = breakfast;
    document.getElementById('count-lunch').textContent = lunch;
    document.getElementById('count-total').textContent = breakfast + lunch;
}

function renderTable(records) {
    const tbody = document.getElementById('records-body');

    if (records.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" class="text-center">No hay registros recientes</td></tr>';
        return;
    }

    let html = '';

    records.forEach(record => {
        const rationType = record.tipo_racion === 1 ?
            '<span class="badge badge-breakfast">Desayuno</span>' :
            '<span class="badge badge-lunch">Almuerzo</span>';

        const status = record.estado === 'SINCRONIZADO' ?
            '<span class="badge badge-synced">Sincronizado</span>' :
            '<span class="badge badge-pending">Pendiente</span>';

        // Format timestamp if available, otherwise use date
        const time = record.hora ? new Date(record.hora).toLocaleTimeString() : record.fecha;

        html += `
            <tr>
                <td>#${record.id}</td>
                <td>
                    <div style="font-weight: 600;">${record.nombre_completo}</div>
                    <div style="font-size: 0.75rem; color: var(--text-muted);">${record.run}</div>
                </td>
                <td>${record.curso}</td>
                <td>${rationType}</td>
                <td>${time}</td>
                <td>${record.terminal}</td>
                <td>${status}</td>
            </tr>
        `;
    });

    tbody.innerHTML = html;
}

function refreshData() {
    const btn = document.querySelector('.btn-refresh');
    btn.innerHTML = '<span class="icon">⌛</span> Cargando...';

    fetchData().then(() => {
        setTimeout(() => {
            btn.innerHTML = '<span class="icon">🔄</span> Actualizar';
        }, 500);
    });
}

function filterTable() {
    const input = document.getElementById('search-input');
    const filter = input.value.toLowerCase();
    const tbody = document.getElementById('records-body');
    const rows = tbody.getElementsByTagName('tr');

    for (let i = 0; i < rows.length; i++) {
        const nameCol = rows[i].getElementsByTagName('td')[1];
        if (nameCol) {
            const txtValue = nameCol.textContent || nameCol.innerText;
            if (txtValue.toLowerCase().indexOf(filter) > -1) {
                rows[i].style.display = "";
            } else {
                rows[i].style.display = "none";
            }
        }
    }
}
