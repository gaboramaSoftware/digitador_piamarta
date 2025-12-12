// App.js

// Variables globales
let allStudents = [];
let currentStudent = null;
let clickTimeout = null;
let clickCount = 0;
let sensorAvailable = false;

// Inicialización
document.addEventListener('DOMContentLoaded', () => {
    updateDate();
    checkSensorStatus();
    fetchStats();
    fetchData();

    // Auto refresh cada 5 segundos
    setInterval(() => {
        fetchStats();
        fetchData();
    }, 5000);

    // Responsive search button behavior
    setupSearchBehavior('search-input');
    setupSearchBehavior('search-students-input');
});

function setupSearchBehavior(inputId) {
    const searchInput = document.getElementById(inputId);
    if (searchInput) {
        searchInput.addEventListener('click', (e) => {
            if (window.innerWidth <= 1050) {
                e.target.classList.add('expanded');
            }
        });

        searchInput.addEventListener('blur', (e) => {
            if (window.innerWidth <= 1050 && !e.target.value) {
                e.target.classList.remove('expanded');
            }
        });
    }
}

function updateDate() {
    const now = new Date();
    const options = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' };
    document.getElementById('current-date').textContent = now.toLocaleDateString('es-ES', options);
}

function showSection(sectionId) {
    document.querySelectorAll('.section').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('nav a').forEach(el => el.classList.remove('active'));
    document.getElementById(sectionId).classList.add('active');

    const navItem = Array.from(document.querySelectorAll('nav a')).find(el => el.getAttribute('onclick').includes(sectionId));
    if (navItem) navItem.classList.add('active');

    const titles = {
        'dashboard': 'Dashboard',
        'students': 'Gestión de Estudiantes',
        'reports': 'Reportes',
        'settings': 'Configuración'
    };
    document.getElementById('page-title').textContent = titles[sectionId];

    if (sectionId === 'students') {
        fetchStudents();
    }
}

// ========== VERIFICAR ESTADO DEL SENSOR ==========

async function checkSensorStatus() {
    try {
        const response = await fetch('/api/sensor/status');
        const data = await response.json();
        sensorAvailable = data.available;

        if (!sensorAvailable) {
            console.warn('⚠️ Sensor de huellas no disponible');
        } else {
            console.log('✓ Sensor de huellas conectado');
        }
    } catch (error) {
        console.error('Error verificando sensor:', error);
        sensorAvailable = false;
    }
}

// ========== API CALLS ==========

async function fetchStats() {
    try {
        const response = await fetch('/api/stats');
        const data = await response.json();

        document.getElementById('count-breakfast').textContent = data.desayunos;
        document.getElementById('count-lunch').textContent = data.almuerzos;
        document.getElementById('count-total').textContent = data.total;
    } catch (error) {
        console.error('Error fetching stats:', error);
    }
}

async function fetchData() {
    try {
        const response = await fetch('/api/recent');
        const data = await response.json();
        renderTable(data.records);
    } catch (error) {
        console.error('Error fetching recent data:', error);
    }
}

async function fetchStudents() {
    const tbody = document.getElementById('students-body');
    tbody.innerHTML = '<tr><td colspan="5" class="text-center">Cargando datos...</td></tr>';

    try {
        const response = await fetch('/api/students');
        allStudents = await response.json();
        renderStudentsTable(allStudents);
    } catch (error) {
        console.error('Error fetching students:', error);
        tbody.innerHTML = '<tr><td colspan="5" class="text-center">Error al cargar datos</td></tr>';
    }
}

async function fetchStudentStats(run) {
    try {
        const response = await fetch(`/api/students/${run}/stats`);
        const stats = await response.json();
        return stats;
    } catch (error) {
        console.error('Error fetching student stats:', error);
        return {
            desayunos: 0,
            almuerzos: 0,
            total: 0,
            porcentaje_asistencia: 0
        };
    }
}

// ========== RENDER FUNCTIONS ==========

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

function renderStudentsTable(students) {
    const tbody = document.getElementById('students-body');

    if (students.length === 0) {
        tbody.innerHTML = '<tr><td colspan="4" class="text-center">No hay estudiantes registrados</td></tr>';
        return;
    }

    let html = '';

    students.forEach(student => {
        html += `
            <tr ondblclick="openStudentStats('${student.run}')" style="cursor: pointer;">
                <td>${student.run}</td>
                <td>${student.nombre}</td>
                <td>${student.curso}</td>
                <td>
                    <button class="btn-action" onclick="event.stopPropagation(); openStudentStats('${student.run}')" title="Ver estadísticas">📊</button>
                    <button class="btn-action btn-action-danger" onclick="event.stopPropagation(); openDeleteModalDirect('${student.run}')" title="Eliminar">🗑️</button>
                </td>
            </tr>
        `;
    });

    tbody.innerHTML = html;
}

// ========== MODAL FUNCTIONS ==========

function openEnrollModal() {
    if (!sensorAvailable) {
        if (!confirm('⚠️ ADVERTENCIA: El sensor de huellas no está disponible.\n\n' +
            'No se podrá capturar la huella dactilar del estudiante.\n' +
            'Verifique que el lector USB esté conectado.\n\n' +
            '¿Desea continuar de todas formas?')) {
            return;
        }
    }

    document.getElementById('enrollModal').style.display = 'flex';
    document.getElementById('enrollForm').reset();
}

function closeEnrollModal() {
    document.getElementById('enrollModal').style.display = 'none';
}

async function openStudentStats(run) {
    const student = allStudents.find(s => s.run === run);
    if (!student) return;

    currentStudent = student;

    const firstLetter = student.nombre.charAt(0).toUpperCase();
    document.getElementById('stats-avatar-letter').textContent = firstLetter;
    document.getElementById('stats-student-name').textContent = student.nombre;
    document.getElementById('stats-student-run').textContent = `RUN: ${student.run}`;
    document.getElementById('stats-student-curso').textContent = `Curso: ${student.curso}`;

    const [nombre, ...apellidos] = student.nombre.split(' ');
    document.getElementById('edit-nombre').value = nombre;
    document.getElementById('edit-apellido').value = apellidos.join(' ');
    document.getElementById('edit-run').value = student.run;

    const stats = await fetchStudentStats(run);
    document.getElementById('stats-breakfast').textContent = stats.desayunos || 0;
    document.getElementById('stats-lunch').textContent = stats.almuerzos || 0;
    document.getElementById('stats-total').textContent = stats.total || 0;
    document.getElementById('stats-percentage').textContent = `${stats.porcentaje_asistencia || 0}%`;

    document.getElementById('statsModal').style.display = 'flex';
}

function closeStatsModal() {
    document.getElementById('statsModal').style.display = 'none';
    currentStudent = null;
}

function openDeleteModalDirect(run) {
    const student = allStudents.find(s => s.run === run);
    if (!student) return;

    currentStudent = student;
    document.getElementById('delete-student-name').textContent = student.nombre;
    document.getElementById('deleteModal').style.display = 'flex';
}

// ========== FUNCIONES DEL MODAL DE EDICIÓN ==========

function openEditModal() {
    if (!currentStudent) {
        console.error("No hay estudiante seleccionado para editar");
        return;
    }

    document.getElementById('statsModal').style.display = 'none';

    const nombreCompleto = currentStudent.nombre || "";
    const partesNombre = nombreCompleto.split(" ");
    
    const nombre = partesNombre[0] || ""; 
    const apellidos = partesNombre.slice(1).join(" ") || "";

    document.getElementById('edit-run').value = currentStudent.run;
    document.getElementById('edit-nombre').value = nombre;
    document.getElementById('edit-apellido').value = apellidos;

    document.getElementById('editarModal').style.display = 'flex';
}

function closeEditModal() {
    document.getElementById('editarModal').style.display = 'none';    
}

function openDeleteModal() {
    confirmDeleteStudent(); 
}

function confirmDeleteStudent() {
    if (!currentStudent) return;

    document.getElementById('delete-student-name').textContent = currentStudent.nombre;
    closeStatsModal();
    document.getElementById('deleteModal').style.display = 'flex';
}

function closeDeleteModal() {
    document.getElementById('deleteModal').style.display = 'none';
}

// ========== Modal Detalle Estudiante ==========

async function openDetalleModal() {  
    if(!currentStudent) return;

    // Rellenar información del estudiante
    document.getElementById('detalle-student-name').textContent = currentStudent.nombre;
    document.getElementById('detalle-student-run').textContent = `RUN: ${currentStudent.run}`;

    // Abrir el modal
    document.getElementById('detalleModal').style.display = 'flex';
    
    // Llenar la tabla del historial
    const tbody = document.getElementById('detalle-body');
    tbody.innerHTML = '<tr><td colspan="4" class="text-center">Cargando datos...</td></tr>';

    try {
        // Llamada a la API real
        const response = await fetch(`/api/students/${currentStudent.run}/history`);
        const historial = await response.json();

        // Renderizar la tabla
        if (historial.length === 0) {
            tbody.innerHTML = '<tr><td colspan="4" class="text-center">No hay registros previos</td></tr>';
        } else {
            let html = '';
            historial.forEach(reg => {
                const badgeClass = reg.tipo === 'Almuerzo' ? 'badge-lunch' : 'badge-breakfast';
                html += `
                    <tr>
                        <td>${reg.fecha}</td>
                        <td>${reg.hora}</td>
                        <td><span class="badge ${badgeClass}">${reg.tipo}</span></td>
                        <td>${reg.estado}</td>
                    </tr>
                `;
            });
            tbody.innerHTML = html;
        }

    } catch (error) {
        console.error("Error cargando historial", error);
        tbody.innerHTML = '<tr><td colspan="4" class="text-center">Error al cargar datos</td></tr>';
    }
}

function closeDetalleModal() {
    document.getElementById('detalleModal').style.display = 'none';
}

// ========== FORM HANDLERS - ENROLAMIENTO CON HUELLA ==========

async function handleEnrollStudent(event) {
    event.preventDefault();

    const formData = {
        run: document.getElementById('enroll-run').value.toUpperCase(),
        nombre: document.getElementById('enroll-nombre').value.trim(),
        apellido: document.getElementById('enroll-apellido').value.trim(),
        curso: document.getElementById('enroll-curso').value
    };

    const nombreCompleto = `${formData.nombre} ${formData.apellido}`;

    const submitBtn = event.target.querySelector('button[type="submit"]');
    const originalBtnText = submitBtn.innerHTML;
    submitBtn.innerHTML = '<span class="icon">👆</span> Coloque su dedo en el sensor...';
    submitBtn.disabled = true;

    const modalBody = document.querySelector('#enrollModal .modal-body');
    let statusDiv = document.getElementById('fingerprint-status');

    if (!statusDiv) {
        statusDiv = document.createElement('div');
        statusDiv.id = 'fingerprint-status';
        statusDiv.style.cssText = `
            margin: 1rem 0;
            padding: 1rem;
            background-color: #FEF3C7;
            border: 2px solid #F59E0B;
            border-radius: 0.5rem;
            text-align: center;
            font-weight: 600;
            color: #92400E;
        `;
        modalBody.insertBefore(statusDiv, modalBody.firstChild);
    }

    statusDiv.innerHTML = '⏳ Esperando huella... Por favor, coloque su dedo en el sensor.';
    statusDiv.style.display = 'block';

    try {
        const response = await fetch('/api/students', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                run: formData.run,
                nombre: nombreCompleto,
                curso: formData.curso
            })
        });

        const result = await response.json();

        if (response.ok && result.success) {
            statusDiv.innerHTML = '✅ ¡Huella capturada exitosamente!';
            statusDiv.style.backgroundColor = '#D1FAE5';
            statusDiv.style.borderColor = '#10B981';
            statusDiv.style.color = '#065F46';

            setTimeout(() => {
                alert(`✓ Estudiante enrolado exitosamente\n\n` +
                    `Nombre: ${nombreCompleto}\n` +
                    `RUN: ${formData.run}\n` +
                    `Huella: ${result.fingerprint_size} bytes capturados`);
                closeEnrollModal();
                fetchStudents();
                checkSensorStatus();
            }, 1000);
        } else {
            let errorMsg = result.message || 'Error desconocido';
            let errorIcon = '❌';

            if (result.error_code === 'SENSOR_INIT_FAILED') {
                errorIcon = '🔌';
                errorMsg = 'Sensor desconectado. Verifique el cable USB y reintente.';
            } else if (result.error_code === 'FINGERPRINT_TIMEOUT') {
                errorIcon = '⏱️';
                errorMsg = 'Tiempo agotado. No se detectó el dedo en el sensor.';
            } else if (result.error_code === 'DATABASE_ERROR') {
                errorIcon = '🗄️';
                errorMsg = 'Error de base de datos. El RUN podría estar duplicado.';
            }

            statusDiv.innerHTML = `${errorIcon} ${errorMsg}`;
            statusDiv.style.backgroundColor = '#FEE2E2';
            statusDiv.style.borderColor = '#EF4444';
            statusDiv.style.color = '#991B1B';

            alert(`Error al enrolar estudiante:\n\n${errorMsg}`);
        }
    } catch (error) {
        console.error('Error enrolling student:', error);
        statusDiv.innerHTML = '⚠️ Error de conexión con el servidor';
        statusDiv.style.backgroundColor = '#FEE2E2';
        statusDiv.style.borderColor = '#EF4444';
        statusDiv.style.color = '#991B1B';
        alert('Error al conectar con el servidor. Verifique su conexión.');
    } finally {
        submitBtn.innerHTML = originalBtnText;
        submitBtn.disabled = false;

        if (statusDiv.innerHTML.includes('❌') || statusDiv.innerHTML.includes('⚠️')) {
            setTimeout(() => {
                statusDiv.style.display = 'none';
            }, 3000);
        }
    }
}

async function handleEditStudent(event) {
    event.preventDefault();

    if (!currentStudent) return;

    const nombre = document.getElementById('edit-nombre').value.trim();
    const apellido = document.getElementById('edit-apellido').value.trim();
    const nombreCompleto = `${nombre} ${apellido}`;

    try {
        const response = await fetch(`/api/students/${currentStudent.run}`, {
            method: 'PUT',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                nombre: nombreCompleto
            })
        });

        if (response.ok) {
            alert('Estudiante actualizado exitosamente');
            closeEditModal();
            closeStatsModal();
            fetchStudents();
        } else {
            const error = await response.json();
            alert('Error al actualizar estudiante: ' + (error.message || 'Error desconocido'));
        }
    } catch (error) {
        console.error('Error updating student:', error);
        alert('Error al conectar con el servidor');
    }
}

async function handleDeleteStudent() {
    if (!currentStudent) return;

    try {
        const response = await fetch(`/api/students/${currentStudent.run}`, {
            method: 'DELETE'
        });

        if (response.ok) {
            alert('Estudiante eliminado exitosamente');
            closeDeleteModal();
            fetchStudents();
            currentStudent = null;
        } else {
            const error = await response.json();
            alert('Error al eliminar estudiante: ' + (error.message || 'Error desconocido'));
        }
    } catch (error) {
        console.error('Error deleting student:', error);
        alert('Error al conectar con el servidor');
    }
}

// ========== FILTER FUNCTIONS ==========

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

function filterStudentsTable() {
    const input = document.getElementById('search-students-input');
    const filter = input.value.toLowerCase();
    const tbody = document.getElementById('students-body');
    const rows = tbody.getElementsByTagName('tr');

    for (let i = 0; i < rows.length; i++) {
        const runCol = rows[i].getElementsByTagName('td')[0];
        const nameCol = rows[i].getElementsByTagName('td')[1];
        if (runCol && nameCol) {
            const runText = runCol.textContent || runCol.innerText;
            const nameText = nameCol.textContent || nameCol.innerText;
            if (runText.toLowerCase().indexOf(filter) > -1 || nameText.toLowerCase().indexOf(filter) > -1) {
                rows[i].style.display = "";
            } else {
                rows[i].style.display = "none";
            }
        }
    }
}

function filterStudentsByCourse() {
    const courseFilter = document.getElementById('course-filter').value;
    const tbody = document.getElementById('students-body');
    const rows = tbody.getElementsByTagName('tr');

    for (let i = 0; i < rows.length; i++) {
        const courseCol = rows[i].getElementsByTagName('td')[2];
        if (courseCol) {
            const courseText = courseCol.textContent || courseCol.innerText;
            if (courseFilter === "" || courseText === courseFilter) {
                rows[i].style.display = "";
            } else {
                rows[i].style.display = "none";
            }
        }
    }
}

// ========== EXPORT FUNCTIONS ==========

async function exportToExcel() {
    const btn = document.querySelector('.btn-exportar');
    const originalText = btn.innerHTML;
    btn.innerHTML = '<span class="icon">⌛</span> Exportando...';
    btn.disabled = true;

    try {
        const response = await fetch('/api/export/records');
        if (response.ok) {
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `registros_${new Date().toISOString().split('T')[0]}.csv`;
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
        } else {
            alert('Error al exportar los datos');
        }
    } catch (error) {
        console.error('Error exporting to Excel:', error);
        alert('Error al exportar los datos');
    } finally {
        btn.innerHTML = originalText;
        btn.disabled = false;
    }
}

async function exportStudentsToExcel() {
    const btn = event.target;
    const originalText = btn.innerHTML;
    btn.innerHTML = '<span class="icon">⌛</span> Exportando...';
    btn.disabled = true;

    try {
        const response = await fetch('/api/export/students');
        if (response.ok) {
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `estudiantes_${new Date().toISOString().split('T')[0]}.csv`;
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
        } else {
            alert('Error al exportar los datos');
        }
    } catch (error) {
        console.error('Error exporting students to Excel:', error);
        alert('Error al exportar los datos');
    } finally {
        btn.innerHTML = originalText;
        btn.disabled = false;
    }
}

// ========== REFRESH DATA ==========

function refreshData() {
    const btn = document.querySelector('.btn-refresh');
    btn.innerHTML = '<span class="icon">⌛</span> Cargando...';

    Promise.all([fetchStats(), fetchData(), checkSensorStatus()]).then(() => {
        setTimeout(() => {
            btn.innerHTML = '<span class="icon">🔄</span> Actualizar';
        }, 500);
    });
}

// Cerrar modales al hacer clic fuera de ellos
window.onclick = function (event) {
    if (event.target.classList.contains('modal')) {
        event.target.style.display = 'none';
    }
}