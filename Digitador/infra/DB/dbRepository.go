// dbRepository Habla con la base de datos y le pide diferentes acciones
// de momento estas acciones son pedir datos de 1 usuario o de multiples usuarios
package DB

import (
	"database/sql"
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	db "Pydigitador/core/db" //archivo de python para transformar .xlsx a .sql

	_ "github.com/mattn/go-sqlite3"
)

// creamos la estructura del repositorio
type SQLiteUserRepository struct {
	db *sql.DB
}

// creamos esta funcion para instanciar el repositorio
func NewSQLiteUserRepository(dbPath string) (*SQLiteUserRepository, error) {
	//definimos usar los mismos parametros que en C
	db, err := sql.Open("sqlite3", fmt.Sprintf("file:%s?cache=shared&mode=rwc&_mutex=full", dbPath))

	//se abrio la base de datos correctamente?
	if err != nil {
		//no, reportar error
		return nil, fmt.Errorf("error abriendo DB: %w", err)
	}

	// esta conectada la base de datos?
	if err = db.Ping(); err != nil {
		//no, reportar error
		return nil, fmt.Errorf("error conectando a DB: %w", err)
	}

	// Habilitamos las foreign_keys (como en C++)
	_, err = db.Exec("PRAGMA foreign_keys = ON")
	//se habilitaron las foreign_keys?
	if err != nil {
		//no
		log.Printf("(-) [DB]: No se pudo habilitar foreign_keys: %v", err)
	}
	//si, devolvemos el repositorio
	return &SQLiteUserRepository{db: db}, nil
}

// creamos esta funcion para guardar los datos de un usuario
func (r *SQLiteUserRepository) SaveUser(user db.Usuario) error {
	query := `
        INSERT OR REPLACE INTO Usuarios 
        (run_id, dv, nombre_completo, id_rol, template_huella) 
        VALUES (?, ?, ?, ?, ?)
    `
	_, err := r.db.Exec(query,
		user.RunID,
		user.DV,
		user.NombreCompleto,
		user.IDRol,
		user.TemplateHuella,
	)
	if err != nil {
		return fmt.Errorf("(-) [GO]: error guardando usuario: %w", err)
	}
	return nil
}

// creamos esta funcion para guardar los datos de un usuario (Insert o Update)
func (r *SQLiteUserRepository) AddStudent(user db.Usuario) error {
	return r.SaveUser(user)
}

func (r *SQLiteUserRepository) UpdateStudent(user db.Usuario) error {
	// En SQLite INSERT OR REPLACE funciona como Update si el PRIMARY KEY (run_id) existe
	return r.SaveUser(user)
}

// Actualizar el curso y letra en la tabla DetailsEstudiante
func (r *SQLiteUserRepository) UpdateStudentCourse(runID string, curso string, letra string) error {
	query := `
        INSERT OR REPLACE INTO DetailsEstudiante (run_id, curso, letra) 
        VALUES (?, ?, ?)
    `
	_, err := r.db.Exec(query, runID, curso, letra)
	if err != nil {
		return fmt.Errorf("(-) [GO]: error guardando curso/letra: %w", err)
	}
	return nil
}

func (r *SQLiteUserRepository) DeleteStudentByRun(runID string) error {
	_, err := r.db.Exec("DELETE FROM DetailsEstudiante WHERE run_id = ?", runID)
	_, err = r.db.Exec("DELETE FROM Usuarios WHERE run_id = ?", runID)
	return err
}

func (r *SQLiteUserRepository) DeleteAllStudents() error {
	_, err := r.db.Exec("DELETE FROM DetailsEstudiante")
	if err != nil {
		return err
	}
	// Solo borramos estudiantes, si hay otros roles no se tocan
	_, err = r.db.Exec("DELETE FROM Usuarios WHERE id_rol = 3")
	return err
}

func (r *SQLiteUserRepository) DeleteStudentsByCourse(curso string, letra string) error {
	query := `SELECT run_id FROM DetailsEstudiante WHERE curso = ? AND letra = ?`
	rows, err := r.db.Query(query, curso, letra)
	if err != nil {
		return err
	}
	defer rows.Close()

	var runIDs []string
	for rows.Next() {
		var runID string
		if err := rows.Scan(&runID); err == nil {
			runIDs = append(runIDs, runID)
		}
	}

	for _, id := range runIDs {
		r.db.Exec("DELETE FROM DetailsEstudiante WHERE run_id = ?", id)
		r.db.Exec("DELETE FROM Usuarios WHERE run_id = ?", id)
	}

	return nil
}

// Creamos la funcion para obtener los datos de un usuario
func (r *SQLiteUserRepository) GetUser(runID string) (*db.Usuario, error) {
	//buscamos datos del usuario a traves de sql
	query := `SELECT run_id, 
              dv, 
              nombre_completo, 
              id_rol, 
              template_huella 
              FROM Usuarios 
              WHERE run_id = ?`

	//Recopilasmos los datos del usuario y los almacenamos dentro de la variable user
	var user db.Usuario
	err := r.db.QueryRow(query, runID).Scan(
		&user.RunID,
		&user.DV,
		&user.NombreCompleto,
		&user.IDRol,
		&user.TemplateHuella,
	)

	//se encontraron filas en la consulta sql???
	if err == sql.ErrNoRows {
		//no, retronar error
		return nil, fmt.Errorf("(-) [GO]: no se encontraron datos del usuario: %w", err)
	}
	//se encontraron filas pero el problema persiste?
	if err != nil {
		//si, retornar error
		return nil, fmt.Errorf("(-) [GO]: error obteniendo usuario: %w", err)
	}

	return &user, nil
}

// creamos la funcion para obtener todos los usuarios (por si el cliente la pide)
func (r *SQLiteUserRepository) GetAllUsers() ([]db.Usuario, error) {
	// Consultamos a la base de datos que nos mande todos los datos de la tabla Usuarios
	query := `SELECT run_id, dv, nombre_completo, id_rol, template_huella FROM Usuarios`

	// generamos una variable que capture los errores de consulta (si existen XD)
	rows, err := r.db.Query(query)

	// hubo un error consultando usuarios?
	if err != nil {
		// si, retornamos error
		return nil, fmt.Errorf("error consultando usuarios: %w", err)
	}
	// destruimos la consulta para evitar conflictos con otros procesos
	defer rows.Close()

	// preparamos una lista (slice) vacía para ir guardando a los usuarios
	var users []db.Usuario

	// iteramos por cada fila que nos devolvió la base de datos
	for rows.Next() {
		var user db.Usuario
		// extraemos los valores de la fila actual y los "mapeamos" a los campos del struct
		err := rows.Scan(&user.RunID, &user.DV, &user.NombreCompleto, &user.IDRol, &user.TemplateHuella)

		// si falla el escaneo de una fila, avisamos qué pasó
		if err != nil {
			return nil, fmt.Errorf("error escaneando usuario: %w", err)
		}

		// si todo salió bien, metemos al usuario a nuestra lista
		users = append(users, user)
	}

	// revisamos si el ciclo for se rompió por algún error raro durante la iteración
	if err = rows.Err(); err != nil {
		return nil, fmt.Errorf("error iterando sobre las filas: %w", err)
	}

	// devolvemos la lista llena de usuarios y un nil (porque aquí no hubo errores, ¡ganamos!)
	return users, nil
}

// creamos la funcion para obtener todos los perfiles, incluyendo el curso
func (r *SQLiteUserRepository) GetAllProfiles() ([]db.PerfilEstudiante, error) {
	query := `
		SELECT u.run_id, u.dv, u.nombre_completo, u.id_rol, u.template_huella, IFNULL(d.curso, 'N/A') as curso, IFNULL(d.letra, '') as letra
		FROM Usuarios u
		LEFT JOIN DetailsEstudiante d ON u.run_id = d.run_id
	`
	rows, err := r.db.Query(query)
	if err != nil {
		return nil, fmt.Errorf("error consultando perfiles: %w", err)
	}
	defer rows.Close()

	var profiles []db.PerfilEstudiante
	for rows.Next() {
		var p db.PerfilEstudiante
		err := rows.Scan(&p.RunID, &p.DV, &p.NombreCompleto, &p.IDRol, &p.TemplateHuella, &p.Curso, &p.Letra)
		if err != nil {
			return nil, fmt.Errorf("error escaneando perfil: %w", err)
		}
		profiles = append(profiles, p)
	}

	if err = rows.Err(); err != nil {
		return nil, fmt.Errorf("error iterando sobre las filas: %w", err)
	}

	return profiles, nil
}

// creamos la funcion para obtener todos los templates
func (r *SQLiteUserRepository) ObtenerTodosTemplates() (map[string][]byte, error) {
	//pedimos todos los usuarios a la base de datos
	users, err := r.GetAllUsers()
	//hubo un error obteniendo usuarios?
	if err != nil {
		//si, retornar error
		return nil, fmt.Errorf("error obteniendo usuarios para templates: %w", err)
	}

	//creamos un mapa para guardar los templates
	templates := make(map[string][]byte)
	//iteramos por cada usuario
	for _, u := range users {
		//si el template no esta vacio
		if len(u.TemplateHuella) > 0 {
			//lo guardamos en el mapa
			templates[u.RunID] = u.TemplateHuella
		}
	}
	//devolvemos el mapa de templates
	return templates, nil
}

// ADVERTENCIA ESTO FUNCIONA EN LIFO (ultimo en registrar primero en borrar)
// creamos la funcion para borrar ultimo registro (lifo)
func (r *SQLiteUserRepository) BorrarUsuario() bool {
	// borramos el ultimo registro de la tabla
	query := `DELETE FROM Usuarios WHERE rowid = (SELECT MAX(rowid) FROM Usuarios)`

	// ejecutamos la consulta
	resultado, err := r.db.Exec(query)

	// ¿Ocurrió un error grave en la base de datos?
	if err != nil {
		fmt.Printf("(-) [GO]: Error en la base de datos al borrar: %v\n", err)
		return false
	}

	// Comprobamos cuántas filas se borraron
	filasBorradas, err := resultado.RowsAffected()

	//hubo un error al verificar las filas borradas?
	if err != nil {
		fmt.Printf("(-) [GO]: Error al verificar filas borradas: %v\n", err)
		return false
	}

	// Si no se borró ninguna fila, es porque la tabla ya estaba vacía
	if filasBorradas == 0 {
		fmt.Printf("(-) [GO]: No se encontraron registros para borrar\n")
		return false
	}

	// Si llegamos aquí, se borró correctamente
	fmt.Printf("(+) [GO]: Último registro borrado exitosamente\n")
	return true
}

// GuardarRegistroRacion inserta el ticket generado en la base de datos
func (r *SQLiteUserRepository) GuardarRegistroRacion(registro db.RegistroRacion) error {
	query := `
        INSERT INTO RegistrosRaciones 
        (id_estudiante, fecha_servicio, tipo_racion, id_terminal, hora_evento, estado_registro) 
        VALUES (?, ?, ?, ?, ?, ?)
    `
	_, err := r.db.Exec(query,
		registro.IDEstudiante,
		registro.FechaServicio,
		registro.TipoRacion,
		registro.IDTerminal,
		registro.HoraEvento,
		registro.EstadoRegistro,
	)
	if err != nil {
		return fmt.Errorf("(-) [GO]: error guardando registro de racion: %w", err)
	}
	return nil
}

// ADVERTENCIA ESTO FUNCIONA EN LIFO (ultimo en registrar primero en borrar)
// creamos la funcion para borrar ultimo registro (lifo)
func (r *SQLiteUserRepository) BorrarRegistro() bool {
	// borramos el ultimo registro de la tabla
	query := `DELETE FROM RegistrosRaciones WHERE id_registro = (SELECT MAX(id_registro) FROM RegistrosRaciones)`

	// ejecutamos la consulta
	resultado, err := r.db.Exec(query)

	// ¿Ocurrió un error grave en la base de datos?
	if err != nil {
		fmt.Printf("(-) [GO]: Error en la base de datos al borrar: %v\n", err)
		return false
	}

	// Comprobamos cuántas filas se borraron
	filasBorradas, err := resultado.RowsAffected()

	//hubo un error al verificar las filas borradas?
	if err != nil {
		fmt.Printf("(-) [GO]: Error al verificar filas borradas: %v\n", err)
		return false
	}

	// Si no se borró ninguna fila, es porque la tabla ya estaba vacía
	if filasBorradas == 0 {
		fmt.Printf("(-) [GO]: No se encontraron registros para borrar\n")
		return false
	}

	// Si llegamos aquí, se borró correctamente
	fmt.Printf("(+) [GO]: Último registro borrado exitosamente\n")
	return true
}

// creamos la funcion para revisar los ultimos 10 registros de la base de datos (Legacy)
func (r *SQLiteUserRepository) ObtenerUltimosRegistros() ([]db.RegistroRacion, error) {
	query := `SELECT id_registro, id_estudiante, fecha_servicio, tipo_racion FROM RegistrosRaciones ORDER BY id_registro DESC LIMIT 10`

	rows, err := r.db.Query(query)
	if err != nil {
		return nil, fmt.Errorf("error consultando registros: %w", err)
	}
	defer rows.Close()

	var registros []db.RegistroRacion
	for rows.Next() {
		var registro db.RegistroRacion
		err := rows.Scan(&registro.IDRegistro, &registro.IDEstudiante, &registro.FechaServicio, &registro.TipoRacion)
		if err != nil {
			return nil, fmt.Errorf("error escaneando registro: %w", err)
		}
		registros = append(registros, registro)
	}
	return registros, nil
}

// creamos la funcion para revisar los ultimos X registros con datos del alumno
func (r *SQLiteUserRepository) GetRecentRecords(limit int) ([]db.RegistroRecienteDTO, error) {
	query := `
		SELECT 
			r.id_registro, 
			u.nombre_completo, 
			u.run_id, 
			COALESCE(de.curso, 'N/A') as curso,
			COALESCE(de.letra, '') as letra,
			r.fecha_servicio, 
			r.hora_evento, 
			r.tipo_racion, 
			r.id_terminal, 
			r.estado_registro 
		FROM RegistrosRaciones r
		JOIN Usuarios u ON r.id_estudiante = u.run_id
		LEFT JOIN DetailsEstudiante de ON u.run_id = de.run_id
		ORDER BY r.id_registro DESC 
		LIMIT ?`

	rows, err := r.db.Query(query, limit)
	if err != nil {
		return nil, fmt.Errorf("error consultando registros recientes: %w", err)
	}
	defer rows.Close()

	var registros []db.RegistroRecienteDTO
	for rows.Next() {
		var reg db.RegistroRecienteDTO
		err := rows.Scan(
			&reg.ID,
			&reg.NombreCompleto,
			&reg.Run,
			&reg.Curso,
			&reg.Letra,
			&reg.FechaServicio,
			&reg.HoraEvento,
			&reg.TipoRacion,
			&reg.NUC,
			&reg.EstadoRegistro,
		)
		if err != nil {
			return nil, err
		}
		registros = append(registros, reg)
	}
	return registros, nil
}

// Nueva funcion para extraer registros masivos con filtros de excel
func (r *SQLiteUserRepository) GetExportRecords(startDate, endDate string, courses []string, raciones []string) ([]db.RegistroRecienteDTO, error) {
	query := `
		SELECT 
			r.id_registro, 
			u.nombre_completo, 
			u.run_id, 
			COALESCE(de.curso, 'N/A') as curso,
			COALESCE(de.letra, '') as letra,
			r.fecha_servicio, 
			r.hora_evento, 
			r.tipo_racion, 
			r.id_terminal, 
			r.estado_registro 
		FROM RegistrosRaciones r
		JOIN Usuarios u ON r.id_estudiante = u.run_id
		LEFT JOIN DetailsEstudiante de ON u.run_id = de.run_id
		WHERE 1=1`

	args := []interface{}{}

	if startDate != "" && endDate != "" {
		query += " AND r.fecha_servicio BETWEEN ? AND ?"
		args = append(args, startDate, endDate)
	}

	if len(raciones) > 0 {
		placeholders := []string{}
		for _, rac := range raciones {
			placeholders = append(placeholders, "?")
			args = append(args, rac)
		}
		query += " AND r.tipo_racion IN (" + strings.Join(placeholders, ",") + ")"
	}

	if len(courses) > 0 && courses[0] != "all" {
		courseConditions := []string{}
		for _, curso := range courses {
			parts := strings.Split(curso, "-")
			c := parts[0]
			l := ""
			if len(parts) > 1 {
				l = parts[1]
			}
			if c == "Sin Curso" {
				courseConditions = append(courseConditions, "(de.curso IS NULL OR de.curso = 'N/A')")
			} else {
				courseConditions = append(courseConditions, "(de.curso = ? AND de.letra = ?)")
				args = append(args, c, l)
			}
		}
		query += " AND (" + strings.Join(courseConditions, " OR ") + ")"
	}

	query += " ORDER BY r.id_registro DESC"

	rows, err := r.db.Query(query, args...)
	if err != nil {
		return nil, fmt.Errorf("error consultando registros de exportación: %w", err)
	}
	defer rows.Close()

	var registros []db.RegistroRecienteDTO
	for rows.Next() {
		var reg db.RegistroRecienteDTO
		err := rows.Scan(
			&reg.ID,
			&reg.NombreCompleto,
			&reg.Run,
			&reg.Curso,
			&reg.Letra,
			&reg.FechaServicio,
			&reg.HoraEvento,
			&reg.TipoRacion,
			&reg.NUC,
			&reg.EstadoRegistro,
		)
		if err != nil {
			return nil, err
		}
		registros = append(registros, reg)
	}
	return registros, nil
}

// Estadisticas globales del dia (o historicas)
func (r *SQLiteUserRepository) GetStats() db.Stats {
	var stats db.Stats
	// Contar desayunos (TipoRacion 1)
	r.db.QueryRow("SELECT COUNT(*) FROM RegistrosRaciones WHERE tipo_racion = 1").Scan(&stats.Desayunos)
	// Contar almuerzos (TipoRacion 2)
	r.db.QueryRow("SELECT COUNT(*) FROM RegistrosRaciones WHERE tipo_racion = 2").Scan(&stats.Almuerzos)
	stats.Total = stats.Desayunos + stats.Almuerzos
	return stats
}

// Historial de un estudiante especifico
func (r *SQLiteUserRepository) GetStudentHistory(runID string) ([]db.HistorialRacion, error) {
	query := `SELECT fecha_servicio, tipo_racion, estado_registro 
	          FROM RegistrosRaciones WHERE id_estudiante = ? 
	          ORDER BY id_registro DESC`

	rows, err := r.db.Query(query, runID)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var historial []db.HistorialRacion
	for rows.Next() {
		var h db.HistorialRacion
		var tipo int
		var estado int
		err := rows.Scan(&h.Fecha, &tipo, &estado)
		if err != nil {
			return nil, err
		}

		// Adaptamos formatos para el Dashboard
		if tipo == 1 {
			h.Tipo = "Desayuno"
		} else {
			h.Tipo = "Almuerzo"
		}

		if estado == 1 {
			h.Estado = "Sincronizado"
		} else {
			h.Estado = "Pendiente"
		}

		historial = append(historial, h)
	}
	return historial, nil
}

// Estadisticas de un estudiante especifico
func (r *SQLiteUserRepository) GetStudentStats(runID string) db.Stats {
	var stats db.Stats
	r.db.QueryRow("SELECT COUNT(*) FROM RegistrosRaciones WHERE id_estudiante = ? AND tipo_racion = 1", runID).Scan(&stats.Desayunos)
	r.db.QueryRow("SELECT COUNT(*) FROM RegistrosRaciones WHERE id_estudiante = ? AND tipo_racion = 2", runID).Scan(&stats.Almuerzos)
	stats.Total = stats.Desayunos + stats.Almuerzos
	return stats
}

func (r *SQLiteUserRepository) SincronizarRegistros() error {
	// Placeholder por ahora, simulamos exito
	return nil
}

func (r *SQLiteUserRepository) DeleteAllRecords() error {
	_, err := r.db.Exec("DELETE FROM RegistrosRaciones")
	return err
}

func (r *SQLiteUserRepository) AddRecord(record db.RegistroRacion) error {
	return r.GuardarRegistroRacion(record)
}

// ObtenerPerfilPorRunID devuelve el PerfilEstudiante dado un RunID, incluyendo su curso y letra
func (r *SQLiteUserRepository) ObtenerPerfilPorRunID(runID string) (*db.PerfilEstudiante, error) {
	query := `SELECT u.run_id, u.dv, u.nombre_completo, u.id_rol, u.template_huella, 
	                 IFNULL(d.curso, 'N/A') as curso, IFNULL(d.letra, '') as letra
	          FROM Usuarios u
	          LEFT JOIN DetailsEstudiante d ON u.run_id = d.run_id
	          WHERE u.run_id = ?`

	var p db.PerfilEstudiante
	err := r.db.QueryRow(query, runID).Scan(
		&p.RunID,
		&p.DV,
		&p.NombreCompleto,
		&p.IDRol,
		&p.TemplateHuella,
		&p.Curso,
		&p.Letra,
	)

	if err == sql.ErrNoRows {
		return nil, fmt.Errorf("(-) [GO]: no se encontró el usuario con RunID %s: %w", runID, err)
	}
	if err != nil {
		return nil, fmt.Errorf("(-) [GO]: error obteniendo perfil: %w", err)
	}

	return &p, nil
}

// funcion para borrar todos los registros (DANGER)
func (r *SQLiteUserRepository) BorrarTodo() bool {
	// Borramos tabla de registros de ración
	_, err := r.db.Exec(`DELETE FROM RegistrosRaciones`)
	if err != nil {
		fmt.Printf("(-) [GO]: Error borrando RegistroRacion: %v\n", err)
		return false
	}

	// Borramos tabla de usuarios
	_, err = r.db.Exec(`DELETE FROM Usuarios`)
	if err != nil {
		fmt.Printf("(-) [GO]: Error borrando Usuarios: %v\n", err)
		return false
	}

	fmt.Printf("(+) [GO]: Base de datos limpiada por completo (DANGER ZONE)\n")
	return true
}

// subir excel de datos arrojados por la funcion SubirExcel
// c:\Proyectos\Pydigitador\infra\DB\dbRepository.go

func (r *SQLiteUserRepository) SubirPlantillaExcel(path string, force bool) error {
	var existe int 
	err := r.db.QueryRow("SELECT COUNT(*) FROM Usuarios WHERE id_rol = 3").Scan(&existe)

	if err == nil && existe > 0 && !force {
		return fmt.Errorf("CONFLICT_STUDENTS")
	}

	if path == "" {
		return fmt.Errorf("(-) [GO]: No se proporcionó la ruta del archivo Excel")
	}

	// --- CORRECCIÓN DE RUTAS INTELIGENTE ---
	pythonScript := "app/excel/SubirExcel.py"
	if _, err := os.Stat(pythonScript); os.IsNotExist(err) {
		// Si no está en ./app, probamos en ../app (por si estamos en /cmd)
		pythonScript = "../app/excel/SubirExcel.py"
		if _, err := os.Stat(pythonScript); os.IsNotExist(err) {
			return fmt.Errorf("(-) [GO]: No se encontró el script Python en 'app/excel/' ni en '../app/excel/'")
		}
	}

	// Usamos una ruta absoluta para el SQL temporal para evitar líos de carpetas
	sqlOutput := filepath.Join(os.TempDir(), "alumnos_temp.sql")

	// Intentamos usar 'py' (lanzador universal de Windows) para asegurar que use el Python con las librerías instaladas
	cmd := exec.Command("py", pythonScript, path, sqlOutput)

	// Capturamos el error de salida para dar más info
	output, err := cmd.CombinedOutput()
	if err != nil {
		return fmt.Errorf("(-) [PYTHON ERROR]: %v\nDetalle: %s", err, string(output))
	}

	// 3. VALIDACIÓN: Verificar que Python realmente creó el archivo .sql
	if _, err := os.Stat(sqlOutput); os.IsNotExist(err) {
		return fmt.Errorf("(-) [GO]: El proceso terminó pero no se generó el archivo SQL intermedio")
	}

	// Leemos el sql y lo mandamos a la BD
	sqlBytes, err := os.ReadFile(sqlOutput)
	if err != nil {
		return fmt.Errorf("(-) [GO]: Error al leer el archivo SQL generado: %v", err)
	}

	// Limpieza inmediata del archivo temporal
	defer os.Remove(sqlOutput)

	sqlQuery := string(sqlBytes)
	if strings.TrimSpace(sqlQuery) == "" {
		return fmt.Errorf("(-) [GO]: El archivo SQL generado está vacío")
	}

	_, err = r.db.Exec(sqlQuery)
	if err != nil {
		return fmt.Errorf("(-) [DB ERROR]: Error al inyectar los datos: %v", err)
	}

	fmt.Println("(+) [GO]: Base de datos poblada con éxito desde el Excel.")
	return nil
}

// cerramos la conexion a la base de datos
func (r *SQLiteUserRepository) Close() error {
	return r.db.Close()
}
