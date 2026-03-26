// esquema para la base de datos
package db

import (
	"database/sql"
	"fmt"
)

// InitDatabase construye la base de datos (tablas) y puebla los catálogos
// solo si no existen los datos en las tablas Curso y Letra.
func InitDatabase(db *sql.DB) error {
	// 1. Crear Tablas usando sintaxis compatible con SQLite
	queries := []string{
		`CREATE TABLE IF NOT EXISTS Curso (
			id_curso INTEGER PRIMARY KEY AUTOINCREMENT,
			nombre TEXT NOT NULL UNIQUE
		);`,

		`CREATE TABLE IF NOT EXISTS Letra (
			id_letra INTEGER PRIMARY KEY AUTOINCREMENT,
			caracter TEXT NOT NULL UNIQUE
		);`,

		`CREATE TABLE IF NOT EXISTS Usuarios (
			run_id TEXT PRIMARY KEY NOT NULL,
			dv TEXT NOT NULL,
			nombre_completo TEXT NOT NULL,
			id_rol INTEGER NOT NULL DEFAULT 3,
			password_hash TEXT,
			template_huella BLOB,
			activo INTEGER NOT NULL DEFAULT 1 -- 1=TRUE, 0=FALSE
		);`,

		`CREATE TABLE IF NOT EXISTS DetailsEstudiante (
			run_id TEXT PRIMARY KEY NOT NULL,
			id_curso INTEGER NOT NULL,
			id_letra INTEGER NOT NULL,
			FOREIGN KEY (run_id) REFERENCES Usuarios (run_id) ON DELETE CASCADE,
			FOREIGN KEY (id_curso) REFERENCES Curso (id_curso),
			FOREIGN KEY (id_letra) REFERENCES Letra (id_letra)
		);`,

		`CREATE TABLE IF NOT EXISTS RegistrosRaciones (
			id_registro INTEGER PRIMARY KEY AUTOINCREMENT,
			id_estudiante TEXT NOT NULL,
			fecha_servicio TEXT NOT NULL, -- Formato "YYYY-MM-DD"
			tipo_racion INTEGER NOT NULL, -- 0=N/A, 1=Desayuno, 2=Almuerzo
			id_terminal TEXT NOT NULL,
			hora_evento INTEGER NOT NULL, -- Unix Epoch ms
			estado_registro INTEGER NOT NULL DEFAULT 0, -- 0=Peadiente, 1=Sincronizado
			FOREIGN KEY (id_estudiante) REFERENCES Usuarios (run_id),
			UNIQUE (id_estudiante, fecha_servicio, tipo_racion) -- Regla RF2 (una ración de cada tipo por día)
		);`,

		`CREATE TABLE IF NOT EXISTS ConfiguracionGlobal (
			id_unica INTEGER PRIMARY KEY CHECK (id_unica = 1),
			id_terminal TEXT NOT NULL DEFAULT 'TOTEM-1',
			tipo_racion INTEGER NOT NULL DEFAULT 0,
			puerto_impresora TEXT NOT NULL DEFAULT 'COM3'
		);`,
	}

	for _, q := range queries {
		if _, err := db.Exec(q); err != nil {
			return fmt.Errorf("error creando tablas: %w", err)
		}
	}

	// 2. Poblar datos por defecto (solo si no existen)
	// Usamos INSERT OR IGNORE para que no falle si el registro ya existe (por el constraint UNIQUE)
	poblarSQL := []string{
		// Cursos (ID numérico autogenerado, evitamos duplicados por nombre)
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('1° Básico');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('2° Básico');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('3° Básico');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('4° Básico');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('5° Básico');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('6° Básico');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('7° Básico');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('8° Básico');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('1° Medio');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('2° Medio');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('3° Medio');",
		"INSERT OR IGNORE INTO Curso (nombre) VALUES ('4° Medio');",

		// Letras (A-D de ejemplo, pero puedes agregar más aquí)
		"INSERT OR IGNORE INTO Letra (caracter) VALUES ('A');",
		"INSERT OR IGNORE INTO Letra (caracter) VALUES ('B');",
		"INSERT OR IGNORE INTO Letra (caracter) VALUES ('C');",
		"INSERT OR IGNORE INTO Letra (caracter) VALUES ('D');",
	}

	for _, q := range poblarSQL {
		if _, err := db.Exec(q); err != nil {
			return fmt.Errorf("error poblando datos de catálogo: %w", err)
		}
	}

	// 3. Inicializar configuración global si no existe (id_unica = 1 siempre)
	db.Exec("INSERT OR IGNORE INTO ConfiguracionGlobal (id_unica, id_terminal, tipo_racion, puerto_impresora) VALUES (1, 'TOTEM-1', 0, 'COM3')")

	return nil
}
