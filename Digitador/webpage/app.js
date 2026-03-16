// App.js

// Variables globales
let allStudents = [];
let allRecords = [];
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
        allRecords = data.records;
    } catch (error) {
        console.error('Error fetching recent data:', error);
    }
}

async function fetchStudents() {
    const tbody = document.getElementById('students-body');
    if (tbody) {
        tbody.innerHTML = '<tr><td colspan="5" class="text-center">Cargando datos...</td></tr>';
    }

    try {
        const response = await fetch('/api/students');
        allStudents = await response.json();
        if (tbody) {
            renderStudentsTable(allStudents);
        }
        loadCourses(); // Cargar cursos dinámicamente usando los datos de allStudents
    } catch (error) {
        console.error('Error fetching students:', error);
        if (tbody) {
            tbody.innerHTML = '<tr><td colspan="5" class="text-center">Error al cargar datos</td></tr>';
        }
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

// Funciones para eliminar Estudiantes
function deleteCourses() {
    const grid = document.getElementById('delete-courses-grid');
    grid.innerHTML = '';

    // Extraer todos los cursos únicos actuales
    const cursosSet = new Set();
    allStudents.forEach(student => {
        const c = student.curso || "";
        const l = student.letra || "";
        if (c !== "" && c !== "N/A") {
            cursosSet.add(`${c}-${l}`);
        }
    });

    const cursosMap = Array.from(cursosSet).sort((a, b) => a.localeCompare(b));

    if (cursosMap.length === 0) {
        grid.innerHTML = '<div class="no-courses-message"><p>No hay cursos registrados</p></div>';
    } else {
        cursosMap.forEach(courseKey => {
            const parts = courseKey.split('-');
            const curso = parts[0];
            const letra = parts[1] || "";
            const formatedName = formatCourseLabel(curso, letra);

            const button = document.createElement('button');
            button.className = 'btn-curso';
            button.style.backgroundColor = '#FEE2E2';
            button.style.color = '#991B1B';
            button.style.borderColor = '#FCA5A5';

            button.textContent = `❌ ${formatedName}`;
            button.onclick = () => handleDeleteSingleCourse(curso, letra, formatedName);

            grid.appendChild(button);
        });
    }

    document.getElementById('deleteCoursesModal').style.display = 'flex';
}

function closeDeleteCoursesModal() {
    document.getElementById('deleteCoursesModal').style.display = 'none';
}

async function handleDeleteSingleCourse(curso, letra, formattedName) {
    if (!confirm(`⚠️ ATENCIÓN: ¿Seguro que desea eliminar todos los alumnos pertenecientes al curso '${formattedName}'?\n\nEsta acción NO se puede deshacer.`)) {
        return;
    }

    try {
        const response = await fetch(`/api/courses?curso=${encodeURIComponent(curso)}&letra=${encodeURIComponent(letra)}`, { method: 'DELETE' });
        const result = await response.json();
        if (result.success) {
            alert(`✅ Estudiantes de ${formattedName} eliminados correctamente.`);
            await fetchStudents(); // recargar panel principal
            fetchStats();
            deleteCourses(); // recargar la cuadricula de borrado
        } else {
            alert('Error al intentar eliminar el curso: ' + (result.error || 'Desconocido'));
        }
    } catch (e) {
        alert('Error de conexión con el servidor.');
    }
}

async function handleDeleteCourses() {
    if (!confirm(`🚨 PELIGRO 🚨\n\n¿Realmente desea borrar a TODOS los estudiantes del sistema?\nEsto limpiará la base de datos completa de usuarios.\n\nESTA ACCION NO SE PUEDE DESHACER.`)) {
        return;
    }
    const btn = document.getElementById('delete-all-students-confirm-btn');
    const originalText = btn.innerHTML;
    btn.innerHTML = 'Eliminando...';
    btn.disabled = true;

    try {
        const response = await fetch('/api/students', { method: 'DELETE' });
        const result = await response.json();
        if (result.success) {
            alert('Todos los estudiantes han sido eliminados correctamente.');
            closeDeleteCoursesModal();
            fetchStudents(); // recargar
            fetchStats();
        } else {
            alert('Error eliminando estudiantes.');
        }
    } catch (e) {
        alert('Error de conexión.');
    }
    btn.innerHTML = originalText;
    btn.disabled = false;
}

// Funciones para eliminar Registros
function deleteAllRecords() {
    document.getElementById('deleteRecordsModal').style.display = 'flex';
}
function closeDeleteRecordsModal() {
    document.getElementById('deleteRecordsModal').style.display = 'none';
}
async function handleDeleteRecords() {
    const btn = document.getElementById('delete-all-records-confirm-btn');
    const originalText = btn.innerHTML;
    btn.innerHTML = 'Eliminando...';
    btn.disabled = true;

    try {
        const response = await fetch('/api/records', { method: 'DELETE' });
        const result = await response.json();
        if (result.success) {
            alert('Todos los registros han sido limpiados correctamente.');
            closeDeleteRecordsModal();
            fetchRecentRecords(); // recargar tabla
            fetchStats();
        } else {
            alert('Error limpiando registros.');
        }
    } catch (e) {
        alert('Error de conexión.');
    }
    btn.innerHTML = originalText;
    btn.disabled = false;
}

// ========== DYNAMIC COURSES ==========
function formatCourseLabel(curso, letra) {
    if (!curso || curso === "N/A") return 'Sin Curso';
    let base = curso.toString().trim();
    if (["1", "2", "3", "4"].includes(base)) {
        base = `${base}° Medio`;
    } else if (["5", "6", "7", "8"].includes(base)) {
        base = `${base}° Básico`;
    }
    return letra ? `${base} ${letra.trim()}` : base;
}

function loadCourses() {
    const grid = document.getElementById('courses-grid');
    const noCoursesMsg = document.querySelector('.no-courses-message');

    // Extract unique courses from the loaded students
    const cursosSet = new Set();
    allStudents.forEach(student => {
        if (student.curso && student.curso.trim() !== "" && student.curso !== "N/A") {
            const letraFormat = student.letra ? `-${student.letra.trim()}` : "";
            cursosSet.add(student.curso.trim() + letraFormat);
        }
    });

    // Sort them alphabetically (or optionally apply custom sorting logic if needed)
    const cursosMap = Array.from(cursosSet).sort((a, b) => a.localeCompare(b));

    populateCourseSelects(cursosMap);

    if (cursosMap.length === 0) {
        // Show empty message
        grid.style.display = 'block';
        if (noCoursesMsg) noCoursesMsg.style.display = 'block';

        // Remove old buttons except the empty message container
        Array.from(grid.children).forEach(child => {
            if (!child.classList.contains('no-courses-message')) {
                grid.removeChild(child);
            }
        });
    } else {
        grid.style.display = 'grid';
        if (noCoursesMsg) noCoursesMsg.style.display = 'none';
        renderCourses(cursosMap);
    }
}

function populateCourseSelects(cursosMap) {
    const selects = ['enroll-curso', 'edit-curso'];
    selects.forEach(id => {
        const select = document.getElementById(id);
        if (!select) return;

        // Reset to placeholder
        select.innerHTML = '<option value="">Seleccione un curso</option>';

        cursosMap.forEach(courseKey => {
            const parts = courseKey.split('-');
            const curso = parts[0];
            const letra = parts[1] || "";

            const option = document.createElement('option');
            option.value = courseKey;
            option.textContent = formatCourseLabel(curso, letra);
            select.appendChild(option);
        });
    });
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
        const cursoDisplay = formatCourseLabel(student.curso, student.letra);
        const hasHuella = student.hasHuella ? '✅ Sí' : '❌ No';
        html += `
            <tr ondblclick="openStudentStats('${student.run}')" style="cursor: pointer;">
                <td>${student.run}</td>
                <td>${student.nombre}</td>
                <td>${cursoDisplay}</td>
                <td>${hasHuella}</td>
                <td>
                    <button class="btn-action" onclick="event.stopPropagation(); openStudentStats('${student.run}')" title="Ver estadísticas">📊Editar</button>
                    <button class="btn-action btn-action-danger" onclick="event.stopPropagation(); openDeleteModalDirect('${student.run}')" title="Eliminar">🗑️Eliminar</button>
                </td>
            </tr>
        `;
    });

    tbody.innerHTML = html;
}

function renderCourses(cursosArray) {
    const grid = document.getElementById('courses-grid');

    // Clear only buttons to preserve standard elements if needed
    Array.from(grid.children).forEach(child => {
        if (!child.classList.contains('no-courses-message')) {
            grid.removeChild(child);
        }
    });

    cursosArray.forEach(courseKey => {
        const parts = courseKey.split('-');
        const curso = parts[0];
        const letra = parts[1] || "";

        const button = document.createElement('button');
        button.className = 'btn-curso';
        button.textContent = formatCourseLabel(curso, letra);
        button.setAttribute('onclick', `openCourseModal('${courseKey}')`);

        // Add before the empty state container
        const noCoursesMsg = document.querySelector('.no-courses-message');
        if (noCoursesMsg) {
            grid.insertBefore(button, noCoursesMsg);
        } else {
            grid.appendChild(button);
        }
    });
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
    document.getElementById('stats-student-curso').textContent = `Curso: ${formatCourseLabel(student.curso, student.letra)}`;

    // Check if the student has a fingerprint
    try {
        const response = await fetch(`/api/students/${student.run}`);
        const data = await response.json();
        const fpStatusDisplay = document.getElementById('stats-student-fingerprint');
        if (data.success && data.student && data.student.hasHuella) {
            fpStatusDisplay.innerHTML = '✅ <span style="color: #065F46; font-weight: bold;">Huella Registrada</span>';
        } else {
            fpStatusDisplay.innerHTML = '❌ <span style="color: #991B1B; font-weight: bold;">Sin Huella Biometrica</span>';
        }
    } catch (err) {
        document.getElementById('stats-student-fingerprint').innerHTML = '⚠️ <span style="color: #B45309;">Estado Desconocido</span>';
    }

    const [nombre, ...apellidos] = student.nombre.split(' ');
    document.getElementById('edit-nombre').value = nombre;
    document.getElementById('edit-apellido').value = apellidos.join(' ');
    document.getElementById('edit-run').value = student.run;

    const sKey = student.curso.trim() + (student.letra ? "-" + student.letra.trim() : "");
    document.getElementById('edit-curso').value = sKey;

    const stats = await fetchStudentStats(run);
    document.getElementById('stats-breakfast').textContent = stats.desayunos || 0;
    document.getElementById('stats-lunch').textContent = stats.almuerzos || 0;
    document.getElementById('stats-total').textContent = stats.total || 0;
    document.getElementById('stats-percentage').textContent = `${stats.porcentaje_asistencia || 0}%`;

    document.getElementById('statsModal').style.display = 'flex';
}

//colocar el registro de huella en "Huella registrada"
function huellaRegistrada(hasHuella) {
    if (hasHuella) {
        return '<span class="badge badge-success" style="background-color: #D1FAE5; color: #065F46; padding: 4px 8px; border-radius: 4px; font-weight: bold;">Sí</span>';
    } else {
        return '<span class="badge badge-danger" style="background-color: #FEE2E2; color: #991B1B; padding: 4px 8px; border-radius: 4px; font-weight: bold;">No</span>';
    }
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
    if (!currentStudent) return;

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

    const cursoValue = document.getElementById('enroll-curso').value;
    const parts = cursoValue.split('-');

    const formData = {
        run: document.getElementById('enroll-run').value.toUpperCase(),
        nombre: document.getElementById('enroll-nombre').value.trim(),
        apellido: document.getElementById('enroll-apellido').value.trim(),
        curso: parts[0] || "",
        letra: parts[1] || ""
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
                curso: formData.curso,
                letra: formData.letra
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
                if (statusDiv) statusDiv.style.display = 'none';
            }, 3000);
        }
    }
}

// ==== VERIFICACION EN TIEMPO REAL ====
// Cuando el usuario termina de escribir el RUN (pierde el foco), verificamos si existe.
document.getElementById('enroll-run').addEventListener('blur', async function (e) {
    const run = e.target.value.toUpperCase();
    if (run.length < 8) return;

    try {
        const response = await fetch(`/api/students/${run}`);
        const data = await response.json();
        if (data.success && data.student) {
            // El estudiante ya está en la Base de Datos
            if (data.student.hasHuella) {
                alert('⚠️ Atención: Este estudiante ya tiene una huella registrada en el sistema.');
            }

            // Separar el nombre completo en Nombre y Apellido
            const partesNombre = data.student.nombre.split(' ');
            document.getElementById('enroll-nombre').value = partesNombre[0] || '';
            document.getElementById('enroll-apellido').value = partesNombre.slice(1).join(' ') || '';

            // Autoseleccionar curso si lo tiene
            if (data.student.curso) {
                const sKey = data.student.curso.trim() + (data.student.letra ? "-" + data.student.letra.trim() : "");
                document.getElementById('enroll-curso').value = sKey;
            }
        }
    } catch (err) {
        console.error("Error validando alumno:", err);
    }
});

async function handleEditStudent(event) {
    event.preventDefault();

    if (!currentStudent) return;

    const nombre = document.getElementById('edit-nombre').value.trim();
    const apellido = document.getElementById('edit-apellido').value.trim();

    const cursoValue = document.getElementById('edit-curso').value;
    const parts = cursoValue.split('-');
    const curso = parts[0] || "";
    const letra = parts[1] || "";

    const nombreCompleto = `${nombre} ${apellido}`;

    try {
        const response = await fetch(`/api/students/${currentStudent.run}`, {
            method: 'PUT',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                nombre: nombreCompleto,
                curso: curso,
                letra: letra
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

async function openExportModal() {

    try {
        const response = await fetch('/api/recent');
        const data = await response.json();
        allRecords = data.records || [];
    } catch (err) {
        console.error("Error validando registros antes de exportar", err);
        allRecords = [];
    }

    //cancelar operacion si no existen registros dentro de la base de datos
    if (!allRecords || allRecords.length === 0) {
        alert('No hay registros guardados aun, no es posible exportar informacion');
        return;
    }

    const today = new Date().toISOString().split('T')[0];
    document.getElementById('export-start-date').value = today;
    document.getElementById('export-end-date').value = today;

    const cursosGrid = document.getElementById('export-courses-grid');
    cursosGrid.innerHTML = '';

    let btnTodos = document.createElement('button');
    btnTodos.className = 'btn-curso active';
    btnTodos.textContent = 'Todos los Cursos';
    btnTodos.dataset.value = 'all';
    cursosGrid.appendChild(btnTodos);

    const cursosSet = new Set();
    allStudents.forEach(student => {
        const c = student.curso || "";
        const l = student.letra || "";
        if (c !== "" && c !== "N/A") {
            cursosSet.add(`${c}-${l}`);
        }
    });

    Array.from(cursosSet).sort((a, b) => a.localeCompare(b)).forEach(courseKey => {
        let btn = document.createElement('button');
        btn.className = 'btn-curso';
        const parts = courseKey.split('-');
        btn.textContent = formatCourseLabel(parts[0], parts[1]);
        btn.dataset.value = courseKey;
        btn.onclick = () => {
            btn.classList.toggle('active');
            btnTodos.classList.remove('active');
        };
        cursosGrid.appendChild(btn);
    });

    btnTodos.onclick = () => {
        btnTodos.classList.toggle('active');
        if (btnTodos.classList.contains('active')) {
            Array.from(cursosGrid.children).forEach(c => {
                if (c !== btnTodos) c.classList.remove('active');
            });
        }
    };

    const racionGrid = document.getElementById('export-raciones-grid');
    racionGrid.innerHTML = '';
    [{ name: 'Desayuno', val: '1' }, { name: 'Almuerzo', val: '2' }].forEach(r => {
        let btn = document.createElement('button');
        btn.className = 'btn-curso active';
        btn.textContent = r.name;
        btn.dataset.value = r.val;
        btn.onclick = () => btn.classList.toggle('active');
        racionGrid.appendChild(btn);
    });

    document.getElementById('exportModal').style.display = 'flex';
}

function closeExportModal() {
    document.getElementById('exportModal').style.display = 'none';
}

async function handleExport() {
    const startDate = document.getElementById('export-start-date').value;
    const endDate = document.getElementById('export-end-date').value;

    const exportBtn = event.target;
    const originalText = exportBtn.innerHTML;
    exportBtn.innerHTML = '⌛ Generando...';
    exportBtn.disabled = true;

    try {
        const selectedCourses = Array.from(document.getElementById('export-courses-grid').children)
            .filter(btn => btn.classList.contains('active'))
            .map(btn => btn.dataset.value);

        const selectedRaciones = Array.from(document.getElementById('export-raciones-grid').children)
            .filter(btn => btn.classList.contains('active'))
            .map(btn => btn.dataset.value);

        if (selectedRaciones.length === 0 || selectedCourses.length === 0) {
            alert("Debe seleccionar al menos un curso y un tipo de ración.");
            return;
        }

        const queryParams = new URLSearchParams({
            start: startDate,
            end: endDate,
            courses: selectedCourses.join(','),
            raciones: selectedRaciones.join(',')
        });

        const response = await fetch(`/api/export/records?${queryParams.toString()}`);
        if (response.ok) {
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `reporte_raciones_${startDate}.csv`;
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
            closeExportModal();
        } else {
            alert('Error al exportar los datos desde el servidor.');
        }
    } catch (error) {
        console.error('Error exporting to Excel:', error);
        alert('Error de red al intentar exportar los datos.');
    } finally {
        exportBtn.innerHTML = originalText;
        exportBtn.disabled = false;
    }
}

//=== exel functions ===
document.getElementById('hidden-file-input').addEventListener('change', function (event) {
    const file = event.target.files[0];
    if (!file) return;

    // Validación básica en el frontend
    if (!file.name.endsWith('.xls') && !file.name.endsWith('.xlsx')) {
        alert('Por favor, selecciona solo archivos Excel (.xls o .xlsx)');
        return;
    }

    const btn = document.getElementById('file-upload-btn');
    const originalText = btn.innerHTML;

    async function uploadExcelToApi(force) {
        const formData = new FormData();
        formData.append('excel_file', file);
        if (force) formData.append('force', 'true');

        btn.innerHTML = '<span class="icon">⌛</span> Subiendo...';
        btn.disabled = true;

        try {
            const response = await fetch('/api/upload-excel', {
                method: 'POST',
                body: formData
            });

            const result = await response.json();

            if (response.ok && (result.success || result.message === "Estudiantes insertados exitosamente")) {
                alert('Excel procesado correctamente. Base de datos actualizada.');
                fetchStudents(); // Recargar tabla
                fetchStats();
            } else if (result.require_confirmation) {
                if (confirm(result.message)) {
                    await uploadExcelToApi(true);
                    return;
                }
            } else {
                alert('Error al procesar el Excel: ' + result.message);
            }
        } catch (error) {
            console.error('Error subiendo Excel:', error);
            alert('Error conectando con el servidor');
        }

        btn.innerHTML = originalText;
        btn.disabled = false;
        event.target.value = ''; // Limpiamos input
    }

    uploadExcelToApi(false);
});

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
function openCourseModal(courseKey) {
    if (typeof courseKey !== 'string' || courseKey === 'course-modal') {
        document.getElementById('course-modal').style.display = 'flex';
        return;
    }

    const parts = courseKey.split('-');
    const cursoNombre = parts[0];
    const letraNombre = parts[1] || "";
    const displayTitle = formatCourseLabel(cursoNombre, letraNombre);

    const modalTitle = document.getElementById('course-modal-title');
    if (modalTitle) modalTitle.textContent = `Lista Curso (${displayTitle})`;

    document.getElementById('course-modal').style.display = 'flex';

    const tbody = document.getElementById('course-students-body');
    if (!tbody) return;

    // Filter students
    const list = allStudents.filter(s => {
        const sKey = s.curso.trim() + (s.letra ? "-" + s.letra.trim() : "");
        return sKey === courseKey;
    });

    if (list.length === 0) {
        tbody.innerHTML = '<tr><td colspan="3" class="text-center">No hay alumnos</td></tr>';
        return;
    }

    let html = '';
    list.forEach(student => {
        html += `
            <tr ondblclick="closeEditCursoModal(); openStudentStats('${student.run}')" style="cursor: pointer;">
                <td>${student.run}</td>
                <td>${student.nombre}</td>
                <td>
                    <button class="btn-action" onclick="event.stopPropagation(); closeEditCursoModal(); openStudentStats('${student.run}')" title="Ver estadísticas">📊Editar</button>
                    <button class="btn-action btn-action-danger" onclick="event.stopPropagation(); closeEditCursoModal(); openDeleteModalDirect('${student.run}')" title="Eliminar">🗑️Eliminar</button>
                </td>
                <td>${huellaRegistrada(student.hasHuella)}</td>
            </tr>
        `;
    });
    tbody.innerHTML = html;
}

function filterCourseStudents() {
    const input = document.getElementById('search-course-students');
    if (!input) return;
    const filter = input.value.toLowerCase();
    const tbody = document.getElementById('course-students-body');
    if (!tbody) return;
    const rows = tbody.getElementsByTagName('tr');

    for (let i = 0; i < rows.length; i++) {
        const runCol = rows[i].getElementsByTagName('td')[0];
        const nameCol = rows[i].getElementsByTagName('td')[1];
        if (runCol && nameCol) {
            const txtValueRun = runCol.textContent || runCol.innerText;
            const txtValueName = nameCol.textContent || nameCol.innerText;
            if (txtValueRun.toLowerCase().indexOf(filter) > -1 || txtValueName.toLowerCase().indexOf(filter) > -1) {
                rows[i].style.display = "";
            } else {
                rows[i].style.display = "none";
            }
        }
    }
}

function closeEditCursoModal() {
    document.getElementById('course-modal').style.display = 'none';
}

function handleEditCurso(event) {
    event.preventDefault();
    // Lógica para actualizar curso si es necesario
    closeEditCursoModal();
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