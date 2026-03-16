//servidor de apirest, el trabajo que hacia antes crowserver
//la idea es mover los datos a base de APIs generadas en este archivo

package web

import (
	"encoding/csv"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	"Pydigitador/app/ticket"
	Sensor "Pydigitador/core/Hardware/Sensor"
	Database "Pydigitador/core/db"
	Repo "Pydigitador/infra/DB"
)

// definimos la estructura que go convertira a Json
type SensorStatus struct {
	Availeble bool `json:"available"`
}

type Stats struct {
	Desayunos int `json:"desayunos"`
	Almuerzos int `json:"almuerzos"`
	Total     int `json:"total"`
}

func StartApiServer(port int, s *Sensor.SensorAdapter, r *Repo.SQLiteUserRepository) {

	//funcion mux para manejar las peticiones de api
	mux := http.NewServeMux()

	//endpoint para verificar estado del sensor
	mux.HandleFunc("/api/sensor/status", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		status := SensorStatus{
			Availeble: s != nil,
		}
		json.NewEncoder(w).Encode(status)
	})

	//endpoint para obtener estadisticas
	mux.HandleFunc("/api/stats", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		stats := r.GetStats()
		json.NewEncoder(w).Encode(stats)
	})

	//endpoint para obtener registros recientes
	mux.HandleFunc("/api/recent", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		records, _ := r.GetRecentRecords(20)

		var mappedRecords []map[string]interface{}
		for _, rec := range records {
			mappedRecords = append(mappedRecords, map[string]interface{}{
				"id":              rec.ID,
				"nombre_completo": rec.NombreCompleto,
				"run":             rec.Run,
				"curso":           "N/A", // El modelo actual no tiene el curso asociado directamente
				"tipo_racion":     rec.TipoRacion,
				"hora":            rec.HoraEvento,
				"fecha":           rec.FechaServicio,
				"terminal":        rec.NUC,
				"estado":          "SINCRONIZADO", // El frontend evalúa SINCRONIZADO
			})
		}
		json.NewEncoder(w).Encode(map[string]interface{}{"records": mappedRecords})
	})

	mux.HandleFunc("GET /api/students", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		students, _ := r.GetAllProfiles()

		var res []map[string]interface{}
		for _, s := range students {
			res = append(res, map[string]interface{}{
				"run":       s.RunID + "-" + s.DV,
				"nombre":    s.NombreCompleto,
				"curso":     s.Curso,
				"letra":     s.Letra,
				"hasHuella": len(s.TemplateHuella) > 0,
			})
		}
		json.NewEncoder(w).Encode(res)
	})

	mux.HandleFunc("GET /api/students/{run}", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		fullRun := r_req.PathValue("run")
		parts := strings.Split(fullRun, "-")
		runID := parts[0]

		perfil, err := r.ObtenerPerfilPorRunID(runID)
		if err != nil {
			json.NewEncoder(w).Encode(map[string]interface{}{"success": false, "message": "Estudiante no encontrado"})
			return
		}

		// Check if it really has a fingerprint 
		hasHuella := len(perfil.TemplateHuella) > 0

		json.NewEncoder(w).Encode(map[string]interface{}{
			"success": true,
			"student": map[string]interface{}{
				"run":     perfil.RunID,
				"nombre":  perfil.NombreCompleto,
				"curso":   perfil.Curso,
				"letra":   perfil.Letra,
				"hasHuella": hasHuella,
			},
		})
	})

	mux.HandleFunc("POST /api/students", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		var reqData struct {
			Run    string `json:"run"` // ej: 12345678K (sin guion)
			Nombre string `json:"nombre"`
			Curso  string `json:"curso"`
			Letra  string `json:"letra"`
		}
		if err := json.NewDecoder(r_req.Body).Decode(&reqData); err != nil {
			json.NewEncoder(w).Encode(map[string]interface{}{"success": false, "message": "Datos invalidos"})
			return
		}

		if s == nil {
			w.WriteHeader(http.StatusServiceUnavailable)
			json.NewEncoder(w).Encode(map[string]interface{}{
				"success":    false,
				"error_code": "SENSOR_INIT_FAILED",
				"message":    "Sensor desconectado",
			})
			return
		}

		plantilla, err := s.CapturarHuella()
		if err != nil {
			json.NewEncoder(w).Encode(map[string]interface{}{
				"success":    false,
				"error_code": "FINGERPRINT_TIMEOUT",
				"message":    "Error al leer la huella o timeout",
			})
			return
		}

		// Parse RUN y DV
		runLen := len(reqData.Run)
		var runID, dv string
		if runLen > 1 {
			runID = reqData.Run[:runLen-1]
			dv = strings.ToUpper(reqData.Run[runLen-1:])
		} else {
			runID = reqData.Run
		}

		user := Database.Usuario{
			RunID:          runID,
			DV:             dv,
			NombreCompleto: reqData.Nombre,
			IDRol:          Database.RolEstudiante,
			TemplateHuella: plantilla,
		}

		err = r.AddStudent(user)
		if err != nil {
			json.NewEncoder(w).Encode(map[string]interface{}{
				"success":    false,
				"error_code": "DATABASE_ERROR",
				"message":    "Error al guardar en base de datos",
			})
			return
		}

		// Save the course info to the DetailsEstudiante table
		r.UpdateStudentCourse(runID, reqData.Curso, reqData.Letra)

		json.NewEncoder(w).Encode(map[string]interface{}{
			"success":          true,
			"fingerprint_size": len(plantilla),
		})
	})

	// Estadísticas del alumno (hace match perfecto con fetch(`/api/students/${run}/stats`))
	mux.HandleFunc("GET /api/students/{run}/stats", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		fullRun := r_req.PathValue("run")
		parts := strings.Split(fullRun, "-")

		stats := r.GetStudentStats(parts[0])
		json.NewEncoder(w).Encode(stats)
	})

	// Historial del alumno (que lo usas en openDetalleModal de tu JS)
	mux.HandleFunc("GET /api/students/{run}/history", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		fullRun := r_req.PathValue("run")
		parts := strings.Split(fullRun, "-")

		history, _ := r.GetStudentHistory(parts[0])
		json.NewEncoder(w).Encode(history)
	})

	// Editar alumno
	mux.HandleFunc("PUT /api/students/{run}", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		fullRun := r_req.PathValue("run")
		parts := strings.Split(fullRun, "-")
		runID := parts[0]

		var reqData struct {
			Nombre string `json:"nombre"`
			Curso  string `json:"curso"`
			Letra  string `json:"letra"`
		}
		json.NewDecoder(r_req.Body).Decode(&reqData)

		user, err := r.GetUser(runID)
		if err != nil {
			json.NewEncoder(w).Encode(map[string]string{"error": "Usuario no encontrado"})
			return
		}

		user.NombreCompleto = reqData.Nombre
		r.UpdateStudent(*user)

		if reqData.Curso != "" || reqData.Letra != "" {
			r.UpdateStudentCourse(runID, reqData.Curso, reqData.Letra)
		}

		json.NewEncoder(w).Encode(map[string]string{"status": "success"})
	})

	// Eliminar alumno (No necesita leer el Body, solo la URL)
	mux.HandleFunc("DELETE /api/students/{run}", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		fullRun := r_req.PathValue("run")
		parts := strings.Split(fullRun, "-")
		runID := parts[0]

		r.DeleteStudentByRun(runID)
		json.NewEncoder(w).Encode(map[string]string{"status": "success"})
	})

	// Eliminar todos los registros de alimentos
	mux.HandleFunc("DELETE /api/records", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		err := r.DeleteAllRecords()
		if err != nil {
			json.NewEncoder(w).Encode(map[string]bool{"success": false})
			return
		}
		json.NewEncoder(w).Encode(map[string]bool{"success": true})
	})

	// Eliminar todos los alumnos/cursos
	mux.HandleFunc("DELETE /api/students", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		err := r.DeleteAllStudents()
		if err != nil {
			json.NewEncoder(w).Encode(map[string]bool{"success": false})
			return
		}
		json.NewEncoder(w).Encode(map[string]bool{"success": true})
	})

	// Eliminar un curso especifico
	mux.HandleFunc("DELETE /api/courses", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		curso := r_req.URL.Query().Get("curso")
		letra := r_req.URL.Query().Get("letra")

		err := r.DeleteStudentsByCourse(curso, letra)
		if err != nil {
			json.NewEncoder(w).Encode(map[string]interface{}{"success": false, "error": err.Error()})
			return
		}
		json.NewEncoder(w).Encode(map[string]bool{"success": true})
	})

	// endpoint de exportar registros
	mux.HandleFunc("/api/export/records", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "text/csv")
		w.Header().Set("Content-Disposition", "attachment;filename=registros.csv")

		startDate := r_req.URL.Query().Get("start")
		endDate := r_req.URL.Query().Get("end")
		courseParam := r_req.URL.Query().Get("courses")
		racionParam := r_req.URL.Query().Get("raciones")
		
		var courses []string
		if courseParam != "" {
			courses = strings.Split(courseParam, ",")
		}

		var raciones []string
		if racionParam != "" {
			raciones = strings.Split(racionParam, ",")
		}

		records, err := r.GetExportRecords(startDate, endDate, courses, raciones)
		if err != nil {
			// Si hay error en la consulta masiva o está vacío, de respaldo usar los ultimos
			records, _ = r.GetRecentRecords(1500)
		}

		csvWriter := csv.NewWriter(w)
		// Incluir la columna "Curso" en el archivo CSV
		csvWriter.Write([]string{"ID", "Nombre", "RUN", "Curso", "Fecha", "Hora", "Tipo Racion", "Terminal", "Estado"})
		
		for _, rec := range records {
			racion := "Desayuno"
			if rec.TipoRacion == 2 {
				racion = "Almuerzo"
			}
			estadoStr := "Pendiente"
			if rec.EstadoRegistro == 1 {
				estadoStr = "Sincronizado"
			}

			cursoCompleto := rec.Curso
			if rec.Letra != "" {
				cursoCompleto += "-" + rec.Letra
			}

			csvWriter.Write([]string{
				fmt.Sprintf("%d", rec.ID),
				rec.NombreCompleto,
				rec.Run,
				cursoCompleto,
				rec.FechaServicio,
				time.UnixMilli(rec.HoraEvento).Format("15:04:05"),
				racion,
				rec.NUC,
				estadoStr,
			})
		}
		csvWriter.Flush()
	})

	// endpoint de exportar estudiantes
	mux.HandleFunc("/api/export/students", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "text/csv")
		w.Header().Set("Content-Disposition", "attachment;filename=estudiantes.csv")

		students, _ := r.GetAllUsers()
		csvWriter := csv.NewWriter(w)
		csvWriter.Write([]string{"RUN", "Nombre", "Rol"})
		for _, std := range students {
			csvWriter.Write([]string{
				std.RunID + "-" + std.DV,
				std.NombreCompleto,
				fmt.Sprintf("%d", std.IDRol),
			})
		}
		csvWriter.Flush()
	})

	// endpoint para subir excel
	mux.HandleFunc("POST /api/upload-excel", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		err := r_req.ParseMultipartForm(10 << 20) // limite 10mb
		if err != nil {
			w.WriteHeader(http.StatusBadRequest)
			json.NewEncoder(w).Encode(map[string]interface{}{"success": false, "message": "Error al procesar el archivo"})
			return
		}
        
		force := r_req.FormValue("force") == "true"

		file, header, err := r_req.FormFile("excel_file")
		if err != nil {
			w.WriteHeader(http.StatusBadRequest)
			json.NewEncoder(w).Encode(map[string]interface{}{"success": false, "message": "No se encontró el archivo en la peticion"})
			return
		}
		defer file.Close()

		ext := filepath.Ext(header.Filename)
		if ext != ".xls" && ext != ".xlsx" {
			w.WriteHeader(http.StatusBadRequest)
			json.NewEncoder(w).Encode(map[string]interface{}{"success": false, "message": "El archivo debe tener extensión .xls o .xlsx"})
			return
		}

		tempPath := filepath.Join(os.TempDir(), "temp_excel"+ext)
		outFile, err := os.Create(tempPath)
		if err != nil {
			json.NewEncoder(w).Encode(map[string]interface{}{"success": false, "message": "Error creando archivo temporal"})
			return
		}

		_, err = io.Copy(outFile, file)
		outFile.Close() // cerramos inmediatamente despues de copiar
		if err != nil {
			json.NewEncoder(w).Encode(map[string]interface{}{"success": false, "message": "Error guardando archivo temporal"})
			return
		}
		defer os.Remove(tempPath)

		err = r.SubirPlantillaExcel(tempPath, force)
		if err != nil {
			if err.Error() == "CONFLICT_STUDENTS" {
				json.NewEncoder(w).Encode(map[string]interface{}{
					"success":              false,
					"require_confirmation": true,
					"message":              "Advertencia: Ya hay estudiantes registrados en el sistema. ¿Desea sobrescribir la información de los usuarios existentes (manteniendo sus huellas) y agregar a los nuevos?",
				})
				return
			}
			// Hubo un error procesando el Excel o inyectando los datos
			w.WriteHeader(http.StatusInternalServerError)
			json.NewEncoder(w).Encode(map[string]interface{}{"success": false, "message": err.Error()})
			return
		}

		json.NewEncoder(w).Encode(map[string]interface{}{"success": true, "message": "Estudiantes insertados exitosamente"})
	})

	//endpoint para registrar asistencia
	mux.HandleFunc("/api/register", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		var record Database.RegistroRacion
		json.NewDecoder(r_req.Body).Decode(&record)
		r.AddRecord(record)
		json.NewEncoder(w).Encode(map[string]string{"status": "success"})
	})

	//endpoint para sincronizar registros
	mux.HandleFunc("/api/sync", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		r.SincronizarRegistros()
		json.NewEncoder(w).Encode(map[string]string{"status": "success"})
	})

	// endpoint para verificar huella (Usado por el Totem)
	mux.HandleFunc("/api/verify_finger", func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Content-Type", "application/json")

		if s == nil {
			json.NewEncoder(w).Encode(map[string]string{"status": "sensor_unavailable"})
			return
		}

		plantilla, err := s.CapturarHuella()
		if err != nil {
			json.NewEncoder(w).Encode(map[string]string{"type": "no_match", "status": "waiting"})
			return
		}

		allTemplates, _ := r.ObtenerTodosTemplates()
		var runID string
		for run, tpl := range allTemplates {
			score, _ := s.CompararHuellas(plantilla, tpl)
			if score >= Database.MatchTreshold {
				runID = run
				break
			}
		}

		if runID == "" {
			json.NewEncoder(w).Encode(map[string]string{"type": "no_match", "status": "rejected"})
			return
		}

		perfil, _ := r.ObtenerPerfilPorRunID(runID)
		fechaDB := time.Now().Format("2006-01-02")
		fechaTXT := time.Now().Format("02/01/2006 15:04")
		var racionStr string
		var racionEnum Database.TipoRacion

		if time.Now().Hour() < 11 {
			racionStr = "Desayuno"
			racionEnum = Database.Desayuno
		} else {
			racionStr = "Almuerzo"
			racionEnum = Database.Almuerzo
		}

		nuevoRegistro := Database.RegistroRacion{
			IDEstudiante:   runID,
			FechaServicio:  fechaDB,
			TipoRacion:     racionEnum,
			IDTerminal:     "TOTEM-1",
			HoraEvento:     time.Now().UnixMilli(),
			EstadoRegistro: Database.Pendiente,
		}

		err = r.AddRecord(nuevoRegistro)
		if err != nil {
			json.NewEncoder(w).Encode(map[string]interface{}{
				"type": "ticket", "status": "rejected_double",
				"data": map[string]string{"nombre": perfil.NombreCompleto, "racion": racionStr},
			})
			return
		}

		ticket.EmitirTicket(*perfil, fechaTXT, racionStr)

		json.NewEncoder(w).Encode(map[string]interface{}{
			"type": "ticket", "status": "approved",
			"data": map[string]string{
				"nombre": perfil.NombreCompleto, "run": perfil.RunID + "-" + perfil.DV,
				"curso": perfil.Curso, "letra": perfil.Letra, "racion": racionStr,
			},
		})
	})

	//servir archivos estaticos
	exePath, err := os.Executable()
	var webpageDir string
	if err == nil {
		webpageDir = filepath.Join(filepath.Dir(exePath), "webpage")
	} else {
		webpageDir = "webpage"
	}

	if _, err := os.Stat(webpageDir); os.IsNotExist(err) {
		webpageDir = "./webpage"
		if _, err := os.Stat(webpageDir); os.IsNotExist(err) {
			webpageDir = "../webpage"
		}
	}

	mux.Handle("/", http.FileServer(http.Dir(webpageDir)))

	// CORS Middleware
	handler := http.HandlerFunc(func(w http.ResponseWriter, r_req *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Accept")
		if r_req.Method == "OPTIONS" {
			w.WriteHeader(http.StatusOK)
			return
		}
		mux.ServeHTTP(w, r_req)
	})

	fmt.Printf("(+) [WEB]: Servidor API y Dashboard iniciado en http://localhost:%d\n", port)
	http.ListenAndServe(":"+strconv.Itoa(port), handler)
}
