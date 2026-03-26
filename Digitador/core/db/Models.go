package db

type RolUsuario int

const (
	RolInvalido      RolUsuario = 0
	RolAdministrador RolUsuario = 1
	RolOperador      RolUsuario = 2
	RolEstudiante    RolUsuario = 3
)

type TipoRacion int

const (
	RacionNoAplica TipoRacion = 0
	Desayuno       TipoRacion = 1
	Almuerzo       TipoRacion = 2
)

type EstadoSemaforo int

const (
	AprobadoPAE       EstadoSemaforo = 1
	AprobadoNormal    EstadoSemaforo = 2
	RechazadoDoble    EstadoSemaforo = 3
	RechazadoNoPAE    EstadoSemaforo = 4
	RechazadoNoExiste EstadoSemaforo = 5
)

type EstadoRegistro int

const (
	Pendiente    EstadoRegistro = 0
	Sincronizado EstadoRegistro = 1
)

// -- Nuevas Estructuras Maestras (Normalización) --

type Curso struct {
	IDCurso int    `json:"id_curso"`
	Nombre  string `json:"nombre"` // ej: "1° Básico"
}

type Letra struct {
	IDLetra  int    `json:"id_letra"`
	Caracter string `json:"caracter"` // ej: "A"
}

// -- Estructuras de tablas de dominio --

type Usuario struct {
	RunID          string     `json:"run_id"`
	DV             string     `json:"dv"`
	NombreCompleto string     `json:"nombre_completo"`
	IDRol          RolUsuario `json:"id_rol"`
	PasswordHash   string     `json:"-"`
	TemplateHuella []byte     `json:"-"`
	Activo         bool       `json:"activo"` // <-- Maneja el estado de la huella/alumno sin borrarlo
}

type RegistroRacion struct {
	IDRegistro     int64          `json:"id_registro"`
	IDEstudiante   string         `json:"id_estudiante"`
	FechaServicio  string         `json:"fecha_servicio"`
	TipoRacion     TipoRacion     `json:"tipo_racion"`
	IDTerminal     string         `json:"id_terminal"` // Se mantiene para saber si fue en el NUC u otro terminal
	HoraEvento     int64          `json:"hora_evento"`
	EstadoRegistro EstadoRegistro `json:"estado_registro"`
}

type DetalleEstudiante struct {
	RunID   string `json:"run_id"`
	IDCurso int    `json:"id_curso"` // FK a tabla Curso
	IDLetra int    `json:"id_letra"` // FK a tabla Letra
}

type Configuracion struct {
	IDTerminal      string     `json:"id_terminal"`
	TipoRacion      TipoRacion `json:"tipo_racion"`
	PuertoImpresora string     `json:"puerto_impresora"`
}

// -- Data Transfer Objects (DTO) --

type PerfilEstudiante struct {
	RunID          string     `json:"run_id"`
	DV             string     `json:"dv"`
	NombreCompleto string     `json:"nombre_completo"`
	IDRol          RolUsuario `json:"id_rol"`
	TemplateHuella []byte     `json:"-"`
	Activo         bool       `json:"activo"`
	// Aquí en el DTO sí devolvemos el string para que el frontend lo lea fácil
	Curso string `json:"curso"`
	Letra string `json:"letra"`
}

type RequestEnrolarUsuario struct {
	RunNuevo       string `json:"run_nuevo"`
	DVNuevo        string `json:"dv_nuevo"`
	NombreNuevo    string `json:"nombre_nuevo"`
	IDCursoNuevo   int    `json:"id_curso_nuevo"` // Ahora recibimos IDs para evitar errores de tipeo
	IDLetraNueva   int    `json:"id_letra_nueva"`
	TemplateHuella []byte `json:"-"`
}

// DTO para hacer un join entre las tablas (Optimizado para el dashboard)
type RegistroRecienteDTO struct {
	ID             int64          `json:"id_registro"`
	NombreCompleto string         `json:"nombre_completo"`
	Run            string         `json:"run"`
	Curso          string         `json:"curso"` // Resultado del JOIN con la tabla Curso
	Letra          string         `json:"letra"` // Resultado del JOIN con la tabla Letra
	FechaServicio  string         `json:"fecha_servicio"`
	HoraEvento     int64          `json:"hora_evento"`
	TipoRacion     TipoRacion     `json:"tipo_racion"`
	NUC            string         `json:"id_terminal"`
	EstadoRegistro EstadoRegistro `json:"estado_registro"`
}

type HistorialRacion struct {
	Fecha  string `json:"fecha"`
	Hora   string `json:"hora"`
	Tipo   string `json:"tipo"`
	Estado string `json:"estado"`
}

type Stats struct {
	Desayunos int `json:"desayunos"`
	Almuerzos int `json:"almuerzos"`
	Total     int `json:"total"`
}

// -- mapping y adaptadores--
type DB interface {
	ObtenerTodosTemplates() (map[string][]byte, error)
	GetUser(runID string) (*Usuario, error)
	ObtenerPerfilPorRunID(runID string) (*PerfilEstudiante, error)
}

// MatchThreshold es el umbral de coincidencia para comparar huellas (corregí el typo en inglés de paso)
const MatchThreshold = 300
